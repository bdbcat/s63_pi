/**********************************************************************
 *
 * Project:  CPL - Common Portability Library
 * Purpose:  CPL Multi-Threading, and process handling portability functions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 **********************************************************************
 * Copyright (c) 2002, Frank Warmerdam
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log: cpl_multiproc.h,v $
 * Revision 1.1.1.1  2006/08/21 05:52:19  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.3  2003/04/23 04:36:55  warmerda
 * pthreads based implementation
 *
 * Revision 1.2  2002/05/24 04:09:24  warmerda
 * fixed CPL_DLL declarations
 *
 * Revision 1.1  2002/05/24 04:01:01  warmerda
 * New
 *
 **********************************************************************/

#ifndef _CPL_MULTIPROC_H_INCLUDED_
#define _CPL_MULTIPROC_H_INCLUDED_

#include "cpl_port.h"

// There are three primary implementations of the multi-process support
// controlled by one of CPL_MULTIPROC_WIN32, CPL_MULTIPROC_PTHREAD or
// CPL_MULTIPROC_STUB being defined.  If none are defined, the stub
// implementation will be used.

#if defined(WIN32) && !defined(CPL_MULTIPROC_STUB)
#  define CPL_MULTIPROC_WIN32
#endif

#if !defined(CPL_MULTIPROC_WIN32) && !defined(CPL_MULTIPROC_PTHREAD) \
 && !defined(CPL_MULTIPROC_STUB)
#  define CPL_MULTIPROC_STUB
#endif

CPL_C_START

void CPL_DLL *CPLLockFile( const char *pszPath, double dfWaitInSeconds );
void  CPL_DLL CPLUnlockFile( void *hLock );

void CPL_DLL *CPLCreateMutex();
int   CPL_DLL CPLAcquireMutex( void *hMutex, double dfWaitInSeconds );
void  CPL_DLL CPLReleaseMutex( void *hMutex );
void  CPL_DLL CPLDestroyMutex( void *hMutex );

int   CPL_DLL CPLGetPID();
int   CPL_DLL CPLCreateThread( void (*pfnMain)(void *), void *pArg );
void  CPL_DLL CPLSleep( double dfWaitInSeconds );

const char CPL_DLL *CPLGetThreadingModel();

CPL_C_END

#endif /* _CPL_MULTIPROC_H_INCLUDED_ */
