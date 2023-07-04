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
 * cpl_vsi.h
 *
 * Include file defining the Virtual System Interface (VSI) functions.  This
 * should normally be included by all translators using VSI functions for
 * accessing system services.  It is also used by the GDAL core, and can be
 * used by higher level applications which adhere to VSI use.
 *
 * Most VSI functions are direct analogs of Posix C library functions.
 * VSI exists to allow ``hooking'' these functions to provide application
 * specific checking, io redirection and so on. 
 * 
 * $Log: cpl_vsi.h,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.18  2003/09/10 19:44:36  warmerda
 * added VSIStrerrno()
 *
 * Revision 1.17  2003/09/08 08:11:40  dron
 * Added VSIGMTime() and VSILocalTime().
 *
 * Revision 1.16  2003/05/27 20:44:40  warmerda
 * added VSI io debugging macros
 *
 * Revision 1.15  2002/06/17 14:10:14  warmerda
 * no stat64 on Win32
 *
 * Revision 1.14  2002/06/17 14:00:16  warmerda
 * segregate VSIStatL() and VSIStatBufL.
 *
 * Revision 1.13  2002/06/15 02:13:13  aubin
 * remove debug test for 64bit compile
 *
 * Revision 1.12  2002/06/15 00:07:23  aubin
 * mods to enable 64bit file i/o
 *
 * Revision 1.11  2001/04/30 18:19:06  warmerda
 * avoid stat on macos_pre10
 *
 * Revision 1.10  2001/01/19 21:16:41  warmerda
 * expanded tabs
 *
 * Revision 1.9  2001/01/03 17:41:44  warmerda
 * added #define for VSIFFlushL
 *
 * Revision 1.8  2001/01/03 16:17:50  warmerda
 * added large file API
 *
 * Revision 1.7  2000/12/14 18:29:48  warmerda
 * added VSIMkdir
 *
 * Revision 1.6  2000/01/25 03:11:03  warmerda
 * added unlink and mkdir
 *
 * Revision 1.5  1999/05/23 02:43:57  warmerda
 * Added documentation block.
 *
 * Revision 1.4  1999/02/25 04:48:11  danmo
 * Added VSIStat() macros specific to _WIN32 (for MSVC++)
 *
 * Revision 1.3  1999/01/28 18:31:25  warmerda
 * Test on _WIN32 rather than WIN32.  It seems to be more reliably defined.
 *
 * Revision 1.2  1998/12/04 21:42:57  danmo
 * Added #ifndef WIN32 arounf #include <unistd.h>
 *
 * Revision 1.1  1998/12/03 18:26:02  warmerda
 * New
 *
 */

#ifndef CPL_VSI_H_INCLUDED
#define CPL_VSI_H_INCLUDED

#include "cpl_port.h"
/**
 * \file cpl_vsi.h
 *
 * Standard C Covers
 *
 * The VSI functions are intended to be hookable aliases for Standard C
 * I/O, memory allocation and other system functions. They are intended
 * to allow virtualization of disk I/O so that non file data sources
 * can be made to appear as files, and so that additional error trapping
 * and reporting can be interested.  The memory access API is aliased
 * so that special application memory management services can be used.
 *
 * Is is intended that each of these functions retains exactly the same
 * calling pattern as the original Standard C functions they relate to.
 * This means we don't have to provide custom documentation, and also means
 * that the default implementation is very simple.
 */


/* -------------------------------------------------------------------- */
/*      We need access to ``struct stat''.                              */
/* -------------------------------------------------------------------- */
#ifndef _WIN32
#  include <unistd.h>
#endif
#if !defined(macos_pre10)
#  include <sys/stat.h>
#endif

CPL_C_START

/* ==================================================================== */
/*      stdio file access functions.                                    */
/* ==================================================================== */

FILE CPL_DLL *  VSIFOpen( const char *, const char * );
int CPL_DLL     VSIFClose( FILE * );
int CPL_DLL     VSIFSeek( FILE *, long, int );
long CPL_DLL    VSIFTell( FILE * );
void CPL_DLL    VSIRewind( FILE * );
void CPL_DLL    VSIFFlush( FILE * );

size_t CPL_DLL  VSIFRead( void *, size_t, size_t, FILE * );
size_t CPL_DLL  VSIFWrite( void *, size_t, size_t, FILE * );
char CPL_DLL   *VSIFGets( char *, int, FILE * );
int CPL_DLL     VSIFPuts( const char *, FILE * );
int CPL_DLL     VSIFPrintf( FILE *, const char *, ... );

int CPL_DLL     VSIFGetc( FILE * );
int CPL_DLL     VSIFPutc( int, FILE * );
int CPL_DLL     VSIUngetc( int, FILE * );
int CPL_DLL     VSIFEof( FILE * );

/* ==================================================================== */
/*      VSIStat() related.                                              */
/* ==================================================================== */

typedef struct stat VSIStatBuf;
int CPL_DLL VSIStat( const char *, VSIStatBuf * );

#ifdef _WIN32
#  define VSI_ISLNK(x)  ( 0 )            /* N/A on Windows */
#  define VSI_ISREG(x)  ((x) & S_IFREG)
#  define VSI_ISDIR(x)  ((x) & S_IFDIR)
#  define VSI_ISCHR(x)  ((x) & S_IFCHR)
#  define VSI_ISBLK(x)  ( 0 )            /* N/A on Windows */
#else
#  define VSI_ISLNK(x)  S_ISLNK(x)
#  define VSI_ISREG(x)  S_ISREG(x)
#  define VSI_ISDIR(x)  S_ISDIR(x)
#  define VSI_ISCHR(x)  S_ISCHR(x)
#  define VSI_ISBLK(x)  S_ISBLK(x)
#endif

/* ==================================================================== */
/*      64bit stdio file access functions.  If we have a big size       */
/*      defined, then provide protypes for the large file API,          */
/*      otherwise redefine to use the regular api.                      */
/* ==================================================================== */
#ifdef VSI_LARGE_API_SUPPORTED

typedef GUIntBig vsi_l_offset;

FILE CPL_DLL *  VSIFOpenL( const char *, const char * );
int CPL_DLL     VSIFCloseL( FILE * );
int CPL_DLL     VSIFSeekL( FILE *, vsi_l_offset, int );
vsi_l_offset CPL_DLL VSIFTellL( FILE * );
void CPL_DLL    VSIRewindL( FILE * );
size_t CPL_DLL  VSIFReadL( void *, size_t, size_t, FILE * );
size_t CPL_DLL  VSIFWriteL( void *, size_t, size_t, FILE * );
int CPL_DLL     VSIFEofL( FILE * );
void CPL_DLL    VSIFFlushL( FILE * );

#ifndef WIN32
typedef struct stat64 VSIStatBufL;
int CPL_DLL     VSIStatL( const char *, VSIStatBufL * );
#else
#define VSIStatBufL    VSIStatBuf
#define VSIStatL       VSIStat
#endif

#else

typedef long vsi_l_offset;

#define vsi_l_offset long

#define VSIFOpenL      VSIFOpen
#define VSIFCloseL     VSIFClose
#define VSIFSeekL      VSIFSeek
#define VSIFTellL      VSIFTell
#define VSIFRewindL    VSIFRewind
#define VSIFReadL      VSIFRead
#define VSIFWriteL     VSIFWrite
#define VSIFEofL       VSIFEof
#define VSIFFlushL     VSIFFlush
#define VSIStatBufL    VSIStatBuf
#define VSIStatL       VSIStat

#endif

/* ==================================================================== */
/*      Memory allocation                                               */
/* ==================================================================== */

void CPL_DLL   *VSICalloc( size_t, size_t );
void CPL_DLL   *VSIMalloc( size_t );
void CPL_DLL    VSIFree( void * );
void CPL_DLL   *VSIRealloc( void *, size_t );
char CPL_DLL   *VSIStrdup( const char * );

/* ==================================================================== */
/*      Other...                                                        */
/* ==================================================================== */

int CPL_DLL VSIMkdir( const char * pathname, long mode );
int CPL_DLL VSIRmdir( const char * pathname );
int CPL_DLL VSIUnlink( const char * pathname );
char CPL_DLL *VSIStrerror( int );

/* ==================================================================== */
/*      Time quering.                                                   */
/* ==================================================================== */

unsigned long CPL_DLL VSITime( unsigned long * );
const char CPL_DLL *VSICTime( unsigned long );
struct tm CPL_DLL *VSIGMTime( const time_t *pnTime,
                              struct tm *poBrokenTime );
struct tm CPL_DLL *VSILocalTime( const time_t *pnTime,
                                 struct tm *poBrokenTime );

/* -------------------------------------------------------------------- */
/*      the following can be turned on for detailed logging of          */
/*      almost all IO calls.                                            */
/* -------------------------------------------------------------------- */
#ifdef VSI_DEBUG

#ifndef DEBUG
#  define DEBUG
#endif

#include "cpl_error.h"

#define VSIDebug4(f,a1,a2,a3,a4)   CPLDebug( "VSI", f, a1, a2, a3, a4 );
#define VSIDebug3( f, a1, a2, a3 ) CPLDebug( "VSI", f, a1, a2, a3 );
#define VSIDebug2( f, a1, a2 )     CPLDebug( "VSI", f, a1, a2 );
#define VSIDebug1( f, a1 )         CPLDebug( "VSI", f, a1 );
#else
#define VSIDebug4( f, a1, a2, a3, a4 ) {}
#define VSIDebug3( f, a1, a2, a3 ) {}
#define VSIDebug2( f, a1, a2 )     {}
#define VSIDebug1( f, a1 )         {}
#endif

CPL_C_END

#endif /* ndef CPL_VSI_H_INCLUDED */
