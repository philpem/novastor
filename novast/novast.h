/* ======== IGeoS ===== Distribition full ===== IGeoS =========
**                                                                         
** novast.h     : Created by 'AMSS-developer' on Wed Sep 30 19:29:02 2015
**                                                                         
** Product      : IGeoS - Integrated Geoscience Software
**                                                                         
** Description  : System for seismic, well log, and potential-field data analysis
**                                                                         
** =================== Limited License: ===================================
** 
** This software is provided free under the following terms and conditions:
** 
** 1. Permission to use, copy, and modify this software 
** for non-commercial purposes without fee is hereby granted, provided  
** that this copyright notice, the warranty disclaimer, and this
** permission notice appear in all copies.
** 
** 2. Distribution of this software or any part of it "bundled" in with
** any product is considered to be a 'commercial purpose'.
** 
** 3. Any new or adapted code developed to operate as a part of this
** software shall be contributed to the authors and distributed under
** the same license.
** 
** ================== Warranty Disclaimer: ================================
** 
** This software is provided "as is", with no support and without
** obligation on the part of the author to assist in its use, correction,
** modification, or enhancement. No guarantees or warranties,
** either express or implied, and regarding the accuracy, safety, or
** fitness for any particular purpose are provided by any contributor
** to this software package.
** 
** ======== IGeoS ===== Distribition full ===== IGeoS ========= */


/*===== THIS DESCRIPTION apparently refers to the NEW NovaBack format.
	I removed soeme fields and HOPE thatr these changes transform
	the format into the old format used at GEON.
	To go back to the origunal (new) format, uncomment the following:

#define NEW_FORMAT

	- I. Morozov, Dec. 9, 1998
=========================================================================*/

/*
Tape format produced by Novaback -

1.      Backup set header format:
*/

typedef struct {		 /* Tape HDR/EOV record */
  short int index_present;		/* True if index is in partition 1 */
  unsigned char compress_type;	/* Compression type, 0 = none */
  unsigned char eovflags;	/* :EOVFLAGS_* ... (pla) stole a byte from compression for flags */
  unsigned short fdate; 	/* File date */
  unsigned short ftime; 	/* File time */
  unsigned short bkdate;	/* Backup date in binary format */
  unsigned short bktime;	/* Backup time in binary format */
  unsigned char volseq; 	/* Volume sequence number */
  unsigned char password;	/* Hashed password */
  short int program_version;	/* Version backup was created with */
  char eovlit[4];		    /* HDR1/EOV1 */

#ifdef NEW_FORMAT
#define FDESC_LENGTH	36
#define TDESC_LENGTH	36

  char fdesc[FDESC_LENGTH];	/* Backup set description */
  char novlit[2][12];		/* <<NoVaStOr>> literal */
  char tdesc[TDESC_LENGTH];	 /* Tape volume identification */

  char AnsiSig[10]; 	    /* :ANSITOOEMSIG ... Verifies table present */
  unsigned char AnsiToOem[256];     /* Ansi to Oem translation table */
  char encryption_test[8];  /* "Encrypt\0" to test for correct key */
  char fixpad[634];         /* Allow 1024 bytes for fixed block devices */

#else
#define FDESC_LENGTH	36
#define TDESC_LENGTH	36

  char fdesc[FDESC_LENGTH];	/* Backup set description */
  char novlit[2][12];		/* <<NoVaStOr>> literal */
  char tdesc[TDESC_LENGTH];	/* Tape volume identification */
  char fixpad[396];         /* Allow 512 bytes for fixed block devices */

#endif

  } EOVSTR;

/* do not change these once set, they are going out to tape */
#define EOVFLAGS_XXXXXX 			0x01	/* compare to IX_QIC in ixfunc.h */
#define EOVFLAGS_UTC_TIMES			0x02	/* coordinated universal time */
#define EOVFLAGS_ANSI_FIELDS		0x04	/* tdesc and fdesc are in ANSI */
#define EOVFLAGS_PASSWORD_PRESENT	0x08	/* password is present */
#define EOVFLAGS_ENCRYPTED			0x10	/* tape is encrypted */
#define ANSITOOEMSIG	"AnsiToOem"         /* indicates filenames are in ANSI */
#define ENCRTEST "Encryption"               /* text for encrpytion_test field */


/*
        This structure is always written by its self in one block.

        This block is written at the beginning of every backup set.  Also if
        a backup set overflows from one physical volume to another, it is
        written at the end of the first tape (eovlit = 'EOV1') and at the
        beginning of the second tape (eovlit = 'HDR1' and vol seq incremented).
        At read time (restore, verify) the correct continuation tape is
        verified by requiring the 32 bit binary date and time on the
        continuation tape HDR1 record to match the date and time on the
        HDR1 record at the beginning of the backup set.  The volume sequence
        number must also be one greater that the previous tape.
*/

/*
2.      Header at the beginning of each file.

All dates and times are DOS binary format.
*/

typedef struct _TAPEHEAD {		/* Header written to tape for each file */
  int fsize;			        /* File data size */
  unsigned short int fdate;         /* File date */
  unsigned short int ftime;         /* File time */
  unsigned short int bkdate;        /* Backup date in binary format */
  unsigned short int bktime;        /* Backup time in binary format */
  unsigned char eattrib;        /* Extended file attribute byte */
  unsigned char fattrib;        /* File attribute */
  /* unsigned */ char fname[82];      /* Complete file name ([0] == 0xff if name is in ename) */
  int ealength;                /* Length of extended attributes (OS/2) */
  unsigned short int crdate;        /* File creation date */
  unsigned short int crtime;        /* File creation time */
  unsigned short int ladate;        /* File last access date */
  unsigned short int latime;        /* File last access time */
  unsigned short int arcdate;       /* Last archive date */
  unsigned short int arctime;       /* Last archive time */
  int ownerid;                 /* File's owner id */
  char fid[12];                 /* File header id string '<<NoVaStOr>>'*/

#ifdef NEW_FORMAT
#define ENAME_LENGTH	256

  char ename[ENAME_LENGTH];     /* Extended name if longer than 81 characters */

#endif

  } TAPEHEAD;

/*
If the complete path and file name exceeds 81 characters, fname[0] == 0xff and
the name is placed in ename.  If ename is not used the header is 128 bytes long.
If ename is used the header is 384 bytes long.


3.      Trailer written at the end of each backup set.

*/

typedef struct {
  char tid[20];                 /* 'NOVASTOR TRAILER BLK' */
  unsigned int indexbytes;     /* Not used */
  unsigned int totalbytes;     /* Not used */
  short int	totalfiles;         /* Not used */
  short int	dsn;                /* Backup set sequence number within this */
  } TLRLBL;                     /* set of backups.  This sequence may span */
                                /* more than one physical volume. */

/*
4.      A tape has the following format.

        ----- First physical tape -----

        Backup set header 1 (HDR1)  Vol seq number = 0
           File header
            [Extended attribute data, if present (ealength > 0)]
           File data
           File header
            [Extended attribute data, if present (ealength > 0)]
           File data
           .
           .
        Tape Mark
        Backup set trailer   Backup set number = 1
        Tape Mark

        Backup set header 2 (HDR1)
           File header
            [Extended attribute data, if present (ealength > 0)]
           File data
           File header
            [Extended attribute data, if present (ealength > 0)]
           File data
           .
           .    <---------- Early warning
        Tape Mark
        Backup set end of volume (EOV1)
        Tape Mark

        ----- Second physical tape -----

        Backup set header 2 (HDR1)  Matches backup set header 2 from tape 1
           File data (continued)    except Vol seq number = 1
           File header
            [Extended attribute data, if present (ealength > 0)]
           File data
           .
           .
        Tape Mark
        Backup set trailer   Backup set number = 2
        Tape Mark

Some notes ...

1.      Header and trailer records are always in a block by themselves, but
        file headers are immediately followed by the beginning data for
        that file.

2.      File headers always start at the beginning of a block.  If extended
        attributes are present, that data is right after the header and the
        file data begins in the next byte after the EA's. At the end of the
        file data, the remainder of a fixed block is not used.  For variable
        block devices, it is written as a short block.  Normal blocks on
        variable block devices are 16384 bytes long.

3.      Empty subdirectories have a header with zero data length and
        the file attribute byte has the subdirectory bit set to distinguish
        it from a zero length file.


Partitionable drives.

        For devices which can be partitioned with an index (P1) partition,
        at the conclusion of a backup, the following format is written to
        the P1 partition:

        1.      The aix index file from disk
        2.      Tape mark
        3.      The bix index file from disk
        4.      Tape mark

    The P1 data is in no way required to restore from partitionable drives.
    It is only used to quickly rebuild Novastor indexes on disk.

Compression -

If the data is compressed (see flag in backup set header), Stac compression
routines are used.  Backup set header blocks, trailer blocks, and EOV blocks
are not compressed.  The remainder of the data blocks are compressed.  On
variable length blocks, each physical block is a compression unit.  On
fixed block devices, a compression unit may be up to 64k long.  Each
compression unit starts with the following structure:

*/

typedef unsigned char NV_QWORD[64];	/* I.M. */

typedef struct {
	NV_QWORD current_offset;    /* 64 bit binary field */
	int  data_len;             /* Length of compression unit */
	} BUF_HDR;

/*

'current_offset' is the total offset in uncompressed bytes into this backup set
for the beginning byte of this compression unit.

data_len is the number of compressed bytes in this compression unit.

When reading a compressed tape with fixed blocks, 'data_len' must be used to
determine the number of bytes to decompress.  In general, the uncompressed
byte offset into the backup set (current_offset) of the beginning of a
compression unit will NOT be at any particular 512/1024 byte boundary in
the uncompressed stream.

The compression logic is to allocate a buffer, say 32k, then compress 512 byte
data blocks into it until the compress routine returns 'output buffer full'.
The compression routine is then flushed and the BUF_HDR structure at the
beginning of the buffer updated, and that data is written to tape.  Then
the remainder of the current incomming data block is compressed to the next
buffer.

Encryption -

Encryption is done using the 64 bit blowfish algorithm.  This is done on each
block before it is written to tape.  The backup set headers are not encrypted
except for the one field noted above which is used to validate the encryption
key for restoring.  This algorithm has an 8 byte granularity, so for the short
blocks which may be present at the end of a file on variable length block tapes,
they are extended to an 8 byte boundary.  The extra bytes are decrypted, but
are ignored by the restore routine since the restore length is taken from the
'fsize' field in the file header.

If compression and encryption are both in force, compression is done first,
then encryption.

File names -

File names are recorded on tape as they are returned by the file access
routines on the system used to backup the data.  On systems where this
is the ANSI character set, the backup stores the ansi-to-oem translation
table.  If this tape is then restored on a system that uses OEM rather
than ansi file names, the table recorded in the tape header at backup
time is used to convert file names to OEM before restoring.

*/

struct NOVAST
{
  SIA_FILE	file;
  EOVSTR	bset_header;	/* backup set header */
  TAPEHEAD	file_header;	/* file header */

  /*----- drive and dir name reaasignment ------*/

  SIA_SUBSTITUTION	*reassign;
};
