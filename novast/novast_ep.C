/* ======== IGeoS ===== Distribition full ===== IGeoS =========
**                                                                         
** novast_ep.C  : Created by 'AMSS-developer' on Wed Sep 30 19:29:02 2015
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


#include "sia_module.C.h"
#include "../define/define.h"
#include "novast.h"

/*=============================================================================
	Binary format Conversions
==============================================================================*/

short int NOVAST_short ( byte *w )
{
  byte c [2];

  *c = w [ 1 ]; c [ 1 ] = *w;
  return *( (short int*) c );
}

int NOVAST_long ( byte *w )
{
  byte c [4];

  *c = w [ 3 ]; c [ 1 ] = w [ 2 ]; c [ 2 ] = w [ 1 ]; c [ 3 ] = *w;
  return *( (int*) c );
}

/*==========================================================================
	String terminated with 0
==========================================================================*/

char *NOVAST_str ( char *str, int length )
{
  memcpy ( MSG_str, str, length );
  MSG_str [ length ] = '\0';
  return MSG_str;
}

/*==========================================================================
	Swap bytes in backup set header
==========================================================================*/

void NOVAST_decode_bset_header ( EOVSTR *bsh )
{
  bsh->index_present = NOVAST_short ( (byte*) &bsh->index_present );
  bsh->program_version = NOVAST_short ( (byte*) &bsh->program_version );
}

/*==========================================================================
	Print the contents of backup set header
==========================================================================*/

void NOVAST_show_bset_header ( EOVSTR *bsh )
{
  int	n, i;

  printf ( "\n==== Backup set header (%d bytes): ======", n = sizeof ( *bsh ) );

  printf ( "\nIndex       : %d", bsh->index_present );
  printf ( "\nCompression : %d", bsh->compress_type );
  printf ( "\nEOVFlags    : %d", bsh->eovflags );
  printf ( "\nFile date   : %d; time: %d", bsh->fdate, bsh->ftime );
  printf ( "\nD/T Formats : %d; %d", bsh->bkdate, bsh->bktime );
  printf ( "\nVolume      : %d", bsh->volseq );
  printf ( "\nPassword    : %d", bsh->password );
  printf ( "\nVersion     : %d", bsh->program_version );
  printf ( "\nHDR1/EOV1   : %s", NOVAST_str ( bsh->eovlit, 4 ) );
  printf ( "\nDescription : %s", NOVAST_str ( bsh->fdesc, FDESC_LENGTH ) );
  printf ( "\nLiteral     : '%s' '%s'", NOVAST_str ( bsh->novlit[0], 12 ),
				NOVAST_str ( bsh->novlit[1], 12 ) );

  printf ( "\nTape vol.ID : %s", NOVAST_str ( bsh->tdesc, TDESC_LENGTH ) );

#ifdef NEW_FORMAT


  printf ( "\nAnsiSig     : %s", NOVAST_str ( bsh->AnsiSig, 10 ) );

		/* :ANSITOOEMSIG ... Verifies table present */

  printf ( "\nEncript     : %s", NOVAST_str ( bsh->encryption_test, 8 ) );

#endif

  printf ( "\n=== End of backup set header =======\n\n" );
}

/*==========================================================================
	Swap bytes in file header
==========================================================================*/

void NOVAST_decode_file_header ( TAPEHEAD *f )
{
  f->fsize = NOVAST_long ( (byte*) &f->fsize );
  f->ownerid = NOVAST_long ( (byte*) &f->ownerid );
}

/*=====================================================================
	Offset to "KR"
======================================================================*/

int NOVAST_l ( char *c, int length )
{
  int i;

  for ( i = 0; i < length; c++, i++ )
    if ( *c == 'K' && c[1] == 'R' )
      return i;

  return -1;
}

/*==========================================================================
	Print the contents of file header
==========================================================================*/

void NOVAST_show_file_header ( TAPEHEAD *f )
{
  int	k, n, i;

  n = sizeof ( *f );

  printf ( "\n==== File header (%d bytes; KR at %d): ======", n,
		 		k = NOVAST_l ( (char*) f, n ) );

  printf ( "\nSize        : %d", f->fsize );

#ifdef NEW_FORMAT

  if ( NOVAST_short ( (byte*) f->fname ) == 0xff )
    printf ( "\nExt. Name : %s", NOVAST_str ( f->ename, 256 ) );
  else
  {
    printf ( "\nName      : '%s'", NOVAST_str ( (char*) f->fname, 82 ) );
    printf ( "\nExt. Name : %s", NOVAST_str ( f->ename, 256 ) );
  }

#else

  printf ( "\nName      : '%s'", NOVAST_str ( (char*) f->fname, 82 ) );

#endif

  printf ( "\nFile date   : %d; time: %d", f->fdate, f->ftime );
  printf ( "\nCreat. date : %d; time: %d", f->crdate, f->crtime );
  printf ( "\nAccess date : %d; time: %d", f->ladate, f->latime );
  printf ( "\nArc date    : %d; time: %d", f->arcdate, f->arctime );

  printf ( "\nD/T Formats : %d; %d", f->bkdate, f->bktime );
  printf ( "\nOwner ID    : %d", f->ownerid );
  printf ( "\nFile ID     : %s", NOVAST_str ( f->fid, 12 ) );
}

/*==========================================================================
	Read the backup file
==========================================================================*/

boolean NOVAST_read ( NOVAST *N )
{
  unsigned long offset;
  int		i, n, l;
  byte		*buf = NULL;			/* file copy buffer */
  SIA_FILE	f;				/* output file */
  char		*fn, fname [ MAX_STR_LENGTH ];	/* its name */

  if ( SIA_fread ( &N->bset_header, sizeof ( N->bset_header ), &N->file ) )
  {
    NOVAST_decode_bset_header ( &N->bset_header );
    NOVAST_show_bset_header ( &N->bset_header );
  }

  offset = ftell ( N->file.stream.f );

  for ( i = 0; SIA_fread ( &N->file_header, sizeof ( N->file_header ), &N->file ); i++ )
  {
    NOVAST_decode_file_header ( &N->file_header );
/*    NOVAST_show_file_header ( &N->file_header ); */

    /*--- replace '\' with '/' ------------------*/

    for ( fn = (char*) N->file_header.fname, l = 0; l < 82; fn++, l++ )
      if ( *fn == '\\' )
	*fn = '/';

    /*--- substitutions in the output file name curently disanbled !!! ------------------*/

 //   if ( ( fn = SIA.flow()->substitute ( N->file_header.fname, N->reassign, &l ) ) != N->file_header.fname )
 //     fn = strcat ( strcpy ( fname, fn ), (char*) N->file_header.fname + l );
 //   else
      fn = strcpy ( fname, fn );

    SIA_str_edit ( fn );		/* process the name */
 
    /*--- create output file ---------------------*/

    if ( SIA_fopen ( fn, SIA_WRITE, SIA_ignore, &f ) )
    {
      /*--- allocate buffer and read the data ---------------------*/

      buf = (byte*) SIA_allocate ( buf, N->file_header.fsize );

      if ( !SIA_fread ( buf, N->file_header.fsize, &N->file ) )
      {
	SIA_err ( SIA_ignore, "Error reading file '%s'", N->file_header.fname );
	goto Next;
      }

      /*--- write new file ---------------------*/

      if ( SIA_fwrite ( buf, N->file_header.fsize, &f ) )
	f.close();
      else
      {
	SIA_err ( SIA_ignore, "Error writing file '%s'", f.name.data  );
	goto Next;
      }

      SIA_message ( MSG_str,"Extracted file '%s'", f.name.data );
    }

    /*--- move to the next file ---------------------*/

  Next:

    n = (int) ceil ( (double) ( N->file_header.fsize + sizeof ( N->file_header ) ) / 512.0 );
    offset += 512 * n;
    fseek ( N->file.stream.f, offset, SEEK_SET );
  }

  sprintf ( MSG_str, "READ %d files", i );
  SIA_message ( MSG_str );

  return i > 0;
}

/*==========================================================================
	Creates test log of the file by looking at L:\ ... .DSS pattern
==========================================================================*/

void NOVAST_test ( NOVAST *N )
{
  int		i, n, got, stop, pattlen = 3, stoplen = 4;
  char	c;
  const char *pattern = "L:\\", *pstop = ".DSS";

  got = 0;
  while ( SIA_fread ( &c, 1, &N->file ) )
  {
    if ( c == pattern [ got ] )
      ++got;
    else
      got = 0;

    if ( got == pattlen )
    {
      printf ( "at %ld: ", ftell ( N->file.stream.f ) - 17 );
      got = 0;

      stop = 0;
      while ( SIA_fread ( &c, 1, &N->file ) )
      {
	printf ( "%c", c );

        if ( c == pstop [ stop ] )
          ++stop;
	else
	  stop = 0;

        if ( stop == stoplen )
	{
          printf ( "\n" );
	  break;
	}
      }
    }
  }
}

/*==========================================================================
	Extracts all files from the backup volume
==========================================================================*/

extern "C"
{
int novast_editp_ ( NOVAST **N );
}
int novast_editp_ ( NOVAST **N )
{
  CHARSTR str;

  *N = new NOVAST;

  switch ( str.param ( "FNAME" ) )
  {
    case PARAM_ERROR :
    case PARAM_DEFAULT :	SIA_error ( "No input file name", SIA_abort );
				return EDIT_PHASE_MODULE;
  }

  str.edit();		/* process the name */

  if ( !SIA_fopen ( str, SIA_READ | SIA_BINARY, SIA_ignore, &(*N)->file ) )
    return EDIT_PHASE_MODULE;

  (*N)->reassign = NULL;	/* init reassignment table */

  while ( PDF_list ( "assign" ) )
    DEFINE_add ( *(*N)->reassign, "NAME", "SUBST", NULL );

  NOVAST_read ( *N );
  SIA_close ( &(*N)->file );

  return EDIT_PHASE_MODULE;
}
