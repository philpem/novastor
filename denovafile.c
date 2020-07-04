#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/stat.h>
#include <errno.h>

#include "utils.h"

#pragma pack(push,1)

typedef struct {		/* Header written to tape for each file */
  uint32_t       fsize;			        /* File data size */
  uint16_t fdate;         /* File date */
  uint16_t ftime;         /* File time */
  uint16_t bkdate;        /* Backup date in binary format */
  uint16_t bktime;        /* Backup time in binary format */
  unsigned char eattrib;        /* Extended file attribute byte */
  unsigned char fattrib;        /* File attribute */
  unsigned char fname[82];      /* Complete file name ([0] == 0xff if name is in ename) */
  uint32_t ealength;                /* Length of extended attributes (OS/2) */
  uint16_t crdate;        /* File creation date */
  uint16_t crtime;        /* File creation time */
  uint16_t ladate;        /* File last access date */
  uint16_t latime;        /* File last access time */
  uint16_t arcdate;       /* Last archive date */
  uint16_t arctime;       /* Last archive time */
  uint32_t ownerid;                 /* File's owner id */
  char fid[12];                 /* File header id string '<<NoVaStOr>>'*/
//  char ename[256];              /* Extended name if longer than 81 characters */
  } TAPEHEAD;

#pragma pack(pop)

#define FHDR_MAGIC "<<NoVaStOr>>"

#define BLKSZ 1024


// DOS file attribute bits -- from https://dos4gw.org/File_Attribute
#define ATTR_READONLY      1
#define ATTR_HIDDEN        2
#define ATTR_SYSTEM        4
#define ATTR_VOLUMELABEL   8
#define ATTR_SUBDIRECTORY  16
#define ATTR_ARCHIVE       32

/**
 * Convert DOS path (C:\X\Y\Z.FOO) to UNIX, in-place
 */
void path_dos2unix(char *str)
{
	char *p;
	do {
		p = strchr(str, '\\');
		if (p != NULL) {
			*p = '/';
		}
	} while (p != NULL);
}

/**
 * Make all directories leading up to the file
 *
 * from https://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix
 */
int mkpath(char* file_path, mode_t mode) {
    assert(file_path && *file_path);
    for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}

int main(int argc, char **argv)
{
	TAPEHEAD	hdr;
	char		filename[256];
	size_t		hsz;

	FILE *fin = fopen("decomp.bin", "rb");

	size_t bufsz = 1048576;
	uint8_t *buf = malloc(bufsz);

	while (true) {
		size_t n = fread(&hdr, sizeof(hdr), 1, fin);
		hsz = sizeof(hdr);

		if (n != 1) {
			fprintf(stderr, "denovafile: EOF\n");
			break;
		}

		// check magic
		if (memcmp(hdr.fid, FHDR_MAGIC, sizeof(hdr.fid)) != 0) {
			fprintf(stderr, "denovafile: FID not correct -- ofs = %lX -- trying to resync...\n", ftell(fin) - sizeof(hdr));
			fseek(fin, -(sizeof(hdr)-1), SEEK_CUR);
			continue;
		}

		// get filename -- may be a short or long one
		if (hdr.fname[0] == 0xFF) {
			assert(fread(&filename, sizeof(filename), 1, fin) == 1);
			hsz += 256;
		} else {
			strncpy(filename, hdr.fname, sizeof(hdr.fname));
		}

		// convert path to UNIX form
		path_dos2unix(filename);

		// create directorie
		mkpath(filename, S_IRWXU);

		fprintf(stderr, "%s -- %u bytes -- created ", filename, hdr.fsize);
		printdosdate(hdr.crdate, hdr.crtime);
		fprintf(stderr, " -- modified ");
		printdosdate(hdr.fdate, hdr.ftime);
		fprintf(stderr, " -- attributes %02X\n", hdr.fattrib);
		fprintf(stderr, "\n");

		if (hdr.fattrib & ATTR_SUBDIRECTORY) {
			//fprintf(stderr, "   Subdirectory\n");
			//mkdir(filename, 0777);

			// Sometimes NovaBack sets the filesize to 0xFFFFFFFF for subdirs. Bleh.
			hdr.fsize = 0;
		} else {
			//fprintf(stderr, "   File\n");
			//fprintf(stderr, "   File size: %u\n", hdr.fsize);
			//fprintf(stderr, "   EA   size: %u\n", hdr.ealength);

			// allocate memory if needed
			if (hdr.fsize > bufsz) {
				buf = realloc(buf, hdr.fsize);
				assert(buf);
				bufsz = hdr.fsize;
			}

			// read file data and save to file
			FILE *fo = fopen(filename, "wb");
			if (hdr.fsize > 0) {
				n = fread(buf, hdr.fsize, 1, fin);
				assert(n == 1);
				fwrite(buf, hdr.fsize, 1, fo);
			}
			fclose(fo);
		}

		// Skip padding to get to next header
		uint32_t paddingBytes = (BLKSZ - ((hdr.fsize + hsz) % BLKSZ));
		if (paddingBytes == BLKSZ) {
			paddingBytes = 0;
		}

		//fprintf(stderr, "  padding: %u\n", paddingBytes);
		if (paddingBytes > 0) {
			n = fread(buf, paddingBytes, 1, fin);
			assert(n == 1);
		}
	}
}
