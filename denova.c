#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lzs.h"

/* do not change these once set, they are going out to tape */
#define	EOVFLAGS_XXXXXX 			0x01			/* compare to IX_QIC in ixfunc.h */
#define	EOVFLAGS_UTC_TIMES			0x02			/* coordinated universal time */
#define	EOVFLAGS_ANSI_FIELDS		0x04			/* tdesc and fdesc are in ANSI */
#define	EOVFLAGS_PASSWORD_PRESENT	0x08			/* password is present */
#define	EOVFLAGS_ENCRYPTED			0x10			/* tape is encrypted */
#define	ANSITOOEMSIG				"AnsiToOem"		/* indicates filenames are in ANSI */
#define	ENCRTEST					"Encryption"	/* text for encrpytion_test field */
#define	NOVLIT						"<<NoVaStOr>>"
#define	NOVLIT_LEN					12

// NovaStor block size
#define	BLKSZ						1024


// Tape blocks are packed little endian.
#pragma pack(push,1)
/**
 * Tape header/EOV record
 */
typedef struct {
	uint16_t		index_present;		/* True if index is in partition 1 */
	uint8_t			compress_type;		/* Compression type, 0 = none */
	uint8_t			eovflags;			/* :EOVFLAGS_* ... (pla) stole a byte from compression for flags */
	uint16_t		fdate;				/* File date */
	uint16_t		ftime;				/* File time */
	uint16_t		bkdate;				/* Backup date in binary format */
	uint16_t		bktime;				/* Backup time in binary format */
	uint8_t			volseq;				/* Volume sequence number */
	uint8_t			password;			/* Hashed password */
	uint16_t		program_version;	/* Version backup was created with */
	char			eovlit[4];			/* HDR1/EOV1 */
	char			fdesc[36];			/* Backup set description */
	char			novlit[2][12];		/* <<NoVaStOr>> literal */
	char			tdesc[36];			/* Tape volume identification */
	char			AnsiSig[10];		/* :ANSITOOEMSIG ... Verifies table present */
	unsigned char	AnsiToOem[256];		/* Ansi to Oem translation table */
	char			encryption_test[8];	/* "Encrypt\0" to test for correct key */
	char			fixpad[634];		/* Allow 1024 bytes for fixed block devices */
} BACKUPSETHEADER;

/**
 * Compressed buffer header
 */
typedef struct {
	uint64_t		current_offset;		/* Offset of this block in uncompressed bytes */
	uint32_t		data_len;			/* Length of compressed data */
} BUFHEADER;
#pragma pack(pop)






void printdosdate(const uint16_t date, const uint16_t time)
{
	uint32_t ds = (date << 16) | time;

	uint16_t y = ((ds >> 25) & 127) + 1980;
	uint8_t m = (ds >> 21) & 15;
	uint8_t d = (ds >> 16) & 31;
	uint8_t hh = (ds >> 11) & 31;
	uint8_t mm = (ds >> 5) & 63;
	uint8_t ss = ((ds) & 31) * 2;

	printf("%04d-%02d-%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
}

int main(void)
{
	BACKUPSETHEADER bHdr;
	BUFHEADER bufHdr;
	char buf[40];

	// open backup file
	FILE *fp = fopen("../tape1.dmp", "rb");
	assert(fp != NULL);

	// read backup set header
	fread(&bHdr, sizeof(bHdr), 1, fp);

	// check magic number
	if (memcmp(bHdr.novlit[0], NOVLIT, NOVLIT_LEN) || memcmp(bHdr.novlit[1], NOVLIT, NOVLIT_LEN)) {
		fprintf(stderr, "Not a NovaStor backup file.\n");
		return -1;
	} else {
		printf("NovaStor backup file detected, version 0x%04X\n", bHdr.program_version);
	}

	// print backup set header
	printf("---- BACKUP SET HEADER ----\n");
	printf("Index present:           %s\n", bHdr.index_present ? "Yes" : "No");
	printf("Compression:             %d\n", bHdr.compress_type);
	printf("EOV flags:               %s%s%s%s%s\n",
		bHdr.eovflags & EOVFLAGS_XXXXXX           ? "IX_QIC " : "",
		bHdr.eovflags & EOVFLAGS_UTC_TIMES        ? "UTC_TIMES " : "",
		bHdr.eovflags & EOVFLAGS_ANSI_FIELDS      ? "ANSI_FIELDS " : "",
		bHdr.eovflags & EOVFLAGS_PASSWORD_PRESENT ? "PASSWORD " : "",
		bHdr.eovflags & EOVFLAGS_ENCRYPTED        ? "ENCRYPTED " : ""
		);
	
	printf("File   date/time:        ");
	printdosdate(bHdr.fdate, bHdr.ftime);
	printf("\n");

	printf("Backup date/time:        ");
	printdosdate(bHdr.bkdate, bHdr.bktime);
	printf("\n");

	printf("Volume sequence number:  %d\n", bHdr.volseq);
	printf("Password hash:           0x%02X\n", bHdr.password);
	printf("Program version:         %d\n", bHdr.program_version);

	memcpy(buf, bHdr.eovlit, sizeof(bHdr.eovlit));
	buf[sizeof(bHdr.eovlit)] = '\0';
	printf("Header signature:        %s\n", buf);

	memcpy(buf, bHdr.fdesc, sizeof(bHdr.fdesc));
	buf[sizeof(bHdr.fdesc)] = '\0';
	printf("Backup set description:  [%s]\n", buf);

	memcpy(buf, bHdr.tdesc, sizeof(bHdr.tdesc));
	buf[sizeof(bHdr.tdesc)] = '\0';
	printf("Tape volume description: [%s]\n", buf);

	printf("\n\n");

	int nblks = 0;

#define CBUFSZ 32768
#define DBUFSZ 1048576
	uint8_t *rbuf = malloc(CBUFSZ);
	uint8_t *dbuf = malloc(DBUFSZ);
	assert(buf != NULL);

	FILE *fo = fopen("decompressed.bin", "wb");
	assert(fo != NULL);

	LzsDecompressParameters_t decompParms;
	lzs_decompress_init(&decompParms);

	while (!feof(fp)) {
		if (fread(&bufHdr, sizeof(bufHdr), 1, fp) < 1) {
			printf("Reached EOF\n%d blocks processed.\n", nblks);
			break;
		}

		nblks++;
		//printf("BUFHDR  o=%-5lu  len=%u\n", bufHdr.current_offset, bufHdr.data_len);

		// read compressed data
		if (fread(rbuf, 1, bufHdr.data_len, fp) != bufHdr.data_len) {
			printf("Read too few bytes while reading compressed data\n");
			break;
		}

		// LZS decompress
		decompParms.inPtr = rbuf;
		decompParms.inLength = bufHdr.data_len;
		decompParms.outPtr = dbuf;
		decompParms.outLength = DBUFSZ;
		size_t decompSz = lzs_decompress_incremental(&decompParms);
		//printf("  LZS: %u bytes written, status %d\n", decompSz, decompParms.status);

		// Normally we'd have to deal with leftover data in the input buffer. Because of how NovaStor works, that's not
		// the case. NS compresses data into a 32KiB write buffer. When this buffer fills, it deletes any incomplete
		// blocks and writes an EOF marker.
		// This primarily means that if there's a tape dropout, NS can recover by seeking ahead to the next 32K block.
		// What it means for us is, we don't have to deal with blocks straddling multiple reads.
		assert(decompParms.inLength == 0);

		// dump the decompressed data in the output file
		fwrite(dbuf, 1, decompSz, fo);

		// Tape drive can only write a multiple of the block size, which means
		// there will be padding we need to skip. Skip them.
		uint32_t paddingBytes = (BLKSZ - ((bufHdr.data_len + sizeof(bufHdr)) % BLKSZ));
		fseek(fp, paddingBytes, SEEK_CUR);
	}

	free(rbuf);
	free(dbuf);

	fclose(fo);
	fclose(fp);

	return 0;
}
