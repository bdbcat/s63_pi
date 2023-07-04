/******************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * cpl_vsisimple.cpp
 *
 * This is a simple implementation (direct to Posix) of the Virtual System
 * Interface (VSI).  See cpl_vsi.h.
 *
 * TODO:
 *  - add some assertions to ensure that arguments are widely legal.  For
 *    instance validation of access strings to fopen().
 *
 * $Log: cpl_vsisimple.cpp,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:29  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.17  2003/09/10 19:44:36  warmerda
 * added VSIStrerrno()
 *
 * Revision 1.16  2003/09/08 08:11:40  dron
 * Added VSIGMTime() and VSILocalTime().
 *
 * Revision 1.15  2003/05/27 20:46:18  warmerda
 * added VSI IO debugging stuff
 *
 * Revision 1.14  2003/05/21 04:20:30  warmerda
 * avoid warnings
 *
 * Revision 1.13  2002/06/17 14:00:16  warmerda
 * segregate VSIStatL() and VSIStatBufL.
 *
 * Revision 1.12  2002/06/15 03:10:22  aubin
 * remove debug test for 64bit compile
 *
 * Revision 1.11  2002/06/15 00:07:23  aubin
 * mods to enable 64bit file i/o
 *
 * Revision 1.10  2001/07/18 04:00:49  warmerda
 *
 * Revision 1.9  2001/04/30 18:19:06  warmerda
 * avoid stat on macos_pre10
 *
 * Revision 1.8  2001/01/19 21:16:41  warmerda
 * expanded tabs
 *
 * Revision 1.7  2001/01/03 05:33:17  warmerda
 * added VSIFlush
 *
 * Revision 1.6  2000/12/14 18:29:48  warmerda
 * added VSIMkdir
 *
 * Revision 1.5  2000/01/26 19:06:29  warmerda
 * fix up mkdir/unlink for windows
 *
 * Revision 1.4  2000/01/25 03:11:03  warmerda
 * added unlink and mkdir
 *
 * Revision 1.3  1998/12/14 04:50:33  warmerda
 * Avoid C++ comments so it will be C compilable as well.
 *
 * Revision 1.2  1998/12/04 21:42:57  danmo
 * Added #ifndef WIN32 arounf #include <unistd.h>
 *
 * Revision 1.1  1998/12/03 18:26:03  warmerda
 * New
 *
 */

#include "cpl_vsi.h"
#include "cpl_error.h"

/* for stat() */

#ifndef WIN32
#  include <unistd.h>
#else
#  include <io.h>
#  include <fcntl.h>
#  include <direct.h>
#endif
#include <sys/stat.h>
#include <time.h>

/************************************************************************/
/*                              VSIFOpen()                              */
/************************************************************************/

FILE *VSIFOpen( const char * pszFilename, const char * pszAccess )

{
    FILE * fp;

    fp = fopen( (char *) pszFilename, (char *) pszAccess );

    VSIDebug3( "VSIFOpen(%s,%s) = %p", pszFilename, pszAccess, fp );

    return( fp );
}

/************************************************************************/
/*                             VSIFClose()                              */
/************************************************************************/

int VSIFClose( FILE * fp )

{
    VSIDebug1( "VSIClose(%p)", fp );

    return( fclose(fp) );
}

/************************************************************************/
/*                              VSIFSeek()                              */
/************************************************************************/

int VSIFSeek( FILE * fp, long nOffset, int nWhence )

{
#ifdef VSI_DEBUG
    if( nWhence == SEEK_SET )
    {
        VSIDebug2( "VSIFSeek(%p,%d,SEEK_SET)", fp, nOffset );
    }
    else if( nWhence == SEEK_END )
    {
        VSIDebug2( "VSIFSeek(%p,%d,SEEK_END)", fp, nOffset );
    }
    else if( nWhence == SEEK_CUR )
    {
        VSIDebug2( "VSIFSeek(%p,%d,SEEK_CUR)", fp, nOffset );
    }
    else
    {
        VSIDebug3( "VSIFSeek(%p,%d,%d-Unknown)", fp, nOffset, nWhence );
    }
#endif

    return( fseek( fp, nOffset, nWhence ) );
}

/************************************************************************/
/*                              VSIFTell()                              */
/************************************************************************/

long VSIFTell( FILE * fp )

{
    VSIDebug2( "VSIFTell(%p) = %ld", fp, ftell(fp) );

    return( ftell( fp ) );
}

/************************************************************************/
/*                             VSIRewind()                              */
/************************************************************************/

void VSIRewind( FILE * fp )

{
    VSIDebug1("VSIRewind(%p)", fp );
    rewind( fp );
}

/************************************************************************/
/*                              VSIFRead()                              */
/************************************************************************/

size_t VSIFRead( void * pBuffer, size_t nSize, size_t nCount, FILE * fp )

{
    size_t nResult = fread( pBuffer, nSize, nCount, fp );

    VSIDebug3( "VSIFRead(%p,%ld) = %ld",
               fp, (long) nSize * nCount, (long) nResult * nSize );

    return nResult;
}

/************************************************************************/
/*                             VSIFWrite()                              */
/************************************************************************/

size_t VSIFWrite( void * pBuffer, size_t nSize, size_t nCount, FILE * fp )

{
    size_t nResult = fwrite( pBuffer, nSize, nCount, fp );

    VSIDebug3( "VSIFWrite(%p,%ld) = %ld",
               fp, (long) nSize * nCount, (long) nResult );

    return nResult;
}

/************************************************************************/
/*                             VSIFFlush()                              */
/************************************************************************/

void VSIFFlush( FILE * fp )

{
    VSIDebug1( "VSIFFlush(%p)", fp );
    fflush( fp );
}

/************************************************************************/
/*                              VSIFGets()                              */
/************************************************************************/

char *VSIFGets( char *pszBuffer, int nBufferSize, FILE * fp )

{
    return( fgets( pszBuffer, nBufferSize, fp ) );
}

/************************************************************************/
/*                              VSIFGetc()                              */
/************************************************************************/

int VSIFGetc( FILE * fp )

{
    return( fgetc( fp ) );
}

/************************************************************************/
/*                             VSIUngetc()                              */
/************************************************************************/

int VSIUngetc( int c, FILE * fp )

{
    return( ungetc( c, fp ) );
}

/************************************************************************/
/*                             VSIFPrintf()                             */
/*                                                                      */
/*      This is a little more complicated than just calling             */
/*      fprintf() because of the variable arguments.  Instead we        */
/*      have to use vfprintf().                                         */
/************************************************************************/

int     VSIFPrintf( FILE * fp, const char * pszFormat, ... )

{
    va_list     args;
    int         nReturn;

    va_start( args, pszFormat );
    nReturn = vfprintf( fp, pszFormat, args );
    va_end( args );

    return( nReturn );
}

/************************************************************************/
/*                              VSIFEof()                               */
/************************************************************************/

int VSIFEof( FILE * fp )

{
    return( feof( fp ) );
}

/************************************************************************/
/*                              VSIFPuts()                              */
/************************************************************************/

int VSIFPuts( const char * pszString, FILE * fp )

{
    return fputs( pszString, fp );
}

/************************************************************************/
/*                              VSIFPutc()                              */
/************************************************************************/

int VSIFPutc( int nChar, FILE * fp )

{
    return( fputc( nChar, fp ) );
}

/************************************************************************/
/*                             VSICalloc()                              */
/************************************************************************/

void *VSICalloc( size_t nCount, size_t nSize )

{
    return( calloc( nCount, nSize ) );
}

/************************************************************************/
/*                             VSIMalloc()                              */
/************************************************************************/

void *VSIMalloc( size_t nSize )

{
    return( malloc( nSize ) );
}

/************************************************************************/
/*                             VSIRealloc()                             */
/************************************************************************/

void * VSIRealloc( void * pData, size_t nNewSize )

{
    return( realloc( pData, nNewSize ) );
}

/************************************************************************/
/*                              VSIFree()                               */
/************************************************************************/

void VSIFree( void * pData )

{
    if( pData != NULL )
        free( pData );
}

/************************************************************************/
/*                             VSIStrdup()                              */
/************************************************************************/

char *VSIStrdup( const char * pszString )

{
    return( strdup( pszString ) );
}

/************************************************************************/
/*                              VSIStat()                               */
/************************************************************************/

int VSIStat( const char * pszFilename, VSIStatBuf * pStatBuf )

{
#if defined(macos_pre10)
    return -1;
#else
    return( stat( pszFilename, pStatBuf ) );
#endif
}

/************************************************************************/
/*                              VSIMkdir()                              */
/************************************************************************/

int VSIMkdir( const char *pszPathname, long mode )

{
#ifdef WIN32
    (void) mode;
    return mkdir( pszPathname );
#elif defined(macos_pre10)
    return -1;
#else
    return mkdir( pszPathname, mode );
#endif
}

/************************************************************************/
/*                             VSIUnlink()                              */
/*************************a***********************************************/

int VSIUnlink( const char * pszFilename )

{
    return unlink( pszFilename );
}

/************************************************************************/
/*                              VSIRmdir()                              */
/************************************************************************/

int VSIRmdir( const char * pszFilename )

{
    return rmdir( pszFilename );
}

/************************************************************************/
/*                              VSITime()                               */
/************************************************************************/

unsigned long VSITime( unsigned long * pnTimeToSet )

{
    time_t tTime;

    tTime = time( NULL );

    if( pnTimeToSet != NULL )
        *pnTimeToSet = (unsigned long) tTime;

    return (unsigned long) tTime;
}

/************************************************************************/
/*                              VSICTime()                              */
/************************************************************************/

const char *VSICTime( unsigned long nTime )

{
    time_t tTime = (time_t) nTime;

    return (const char *) ctime( &tTime );
}

/************************************************************************/
/*                             VSIGMTime()                              */
/************************************************************************/

struct tm *VSIGMTime( const time_t *pnTime, struct tm *poBrokenTime )
{

#if HAVE_GMTIME_R
    gmtime_r( pnTime, poBrokenTime );
#else
    struct tm   *poTime;
    poTime = gmtime( pnTime );
    memcpy( poBrokenTime, poTime, sizeof(tm) );
#endif

    return poBrokenTime;
}

/************************************************************************/
/*                             VSILocalTime()                           */
/************************************************************************/

struct tm *VSILocalTime( const time_t *pnTime, struct tm *poBrokenTime )
{

#if HAVE_LOCALTIME_R
    localtime_r( pnTime, poBrokenTime );
#else
    struct tm   *poTime;
    poTime = localtime( pnTime );
    memcpy( poBrokenTime, poTime, sizeof(tm) );
#endif

    return poBrokenTime;
}

/************************************************************************/
/*                            VSIStrerror()                             */
/************************************************************************/

char *VSIStrerror( int nErrno )

{
    return strerror( nErrno );
}
