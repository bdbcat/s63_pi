/**********************************************************************
 *
 * Name:     cpl_error.h
 * Project:  CPL - Common Portability Library
 * Purpose:  CPL Error handling
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 1998, Daniel Morissette
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
 * $Log: cpl_error.h,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.16  2001/11/02 22:07:58  warmerda
 * added logging error handler
 *
 * Revision 1.15  2001/01/19 21:16:41  warmerda
 * expanded tabs
 *
 * Revision 1.14  2000/11/30 17:30:10  warmerda
 * added CPLGetLastErrorType
 *
 * Revision 1.13  2000/08/24 18:08:17  warmerda
 * made default and quiet error handlers public on windows
 *
 * Revision 1.12  2000/06/26 21:44:07  warmerda
 * added CPLE_UserInterrupt for progress terminations
 *
 * Revision 1.11  2000/03/31 14:11:55  warmerda
 * added CPLErrorV
 *
 * Revision 1.10  2000/01/10 17:35:45  warmerda
 * added push down stack of error handlers
 *
 * Revision 1.9  1999/07/23 14:27:47  warmerda
 * CPLSetErrorHandler returns old handler
 *
 * Revision 1.8  1999/05/20 14:59:05  warmerda
 * added CPLDebug()
 *
 * Revision 1.7  1999/05/20 02:54:38  warmerda
 * Added API documentation
 *
 * Revision 1.6  1999/02/17 05:40:47  danmo
 * Fixed CPLAssert() macro to work with EGCS.
 *
 * Revision 1.5  1999/01/11 15:34:29  warmerda
 * added reserved range comment
 *
 * Revision 1.4  1998/12/15 19:02:27  warmerda
 * Avoid use of errno as a variable
 *
 * Revision 1.3  1998/12/06 22:20:42  warmerda
 * Added error code.
 *
 * Revision 1.2  1998/12/06 02:52:52  warmerda
 * Implement assert support
 *
 * Revision 1.1  1998/12/03 18:26:02  warmerda
 * New
 *
 **********************************************************************/

#ifndef _CPL_ERROR_H_INCLUDED_
#define _CPL_ERROR_H_INCLUDED_

#include "cpl_port.h"

/*=====================================================================
                   Error handling functions (cpl_error.c)
 =====================================================================*/

/**
 * \file cpl_error.h
 *
 * CPL error handling services.
 */

CPL_C_START

typedef enum
{
    CE_None = 0,
    CE_Debug = 1,
    CE_Warning = 2,
    CE_Failure = 3,
    CE_Fatal = 4

} CPLErr;

void CPL_DLL CPLError(CPLErr eErrClass, int err_no, const char *fmt, ...);
void CPL_DLL CPLErrorV(CPLErr, int, const char *, va_list );
void CPL_DLL CPLErrorReset();
int CPL_DLL CPLGetLastErrorNo();
CPLErr CPL_DLL CPLGetLastErrorType();
const char CPL_DLL * CPLGetLastErrorMsg();

typedef void (*CPLErrorHandler)(CPLErr, int, const char*);
CPLErrorHandler CPL_DLL CPLSetErrorHandler(CPLErrorHandler);
void CPL_DLL CPLPushErrorHandler( CPLErrorHandler );
void CPL_DLL CPLPopErrorHandler();
void CPL_DLL CPLDefaultErrorHandler( CPLErr, int, const char * );
void CPL_DLL CPLQuietErrorHandler( CPLErr, int, const char * );
void CPL_DLL CPLLoggingErrorHandler( CPLErr, int, const char * );

void CPL_DLL CPLDebug( const char *, const char *, ... );
void CPL_DLL _CPLAssert( const char *, const char *, int );

#ifdef DEBUG
#  define CPLAssert(expr)  ((expr) ? (void)(0) : _CPLAssert(#expr,__FILE__,__LINE__))
#else
#  define CPLAssert(expr)
#endif

CPL_C_END

/* ==================================================================== */
/*      Well known error codes.                                         */
/* ==================================================================== */

#define CPLE_None                       0
#define CPLE_AppDefined                 1
#define CPLE_OutOfMemory                2
#define CPLE_FileIO                     3
#define CPLE_OpenFailed                 4
#define CPLE_IllegalArg                 5
#define CPLE_NotSupported               6
#define CPLE_AssertionFailed            7
#define CPLE_NoWriteAccess              8
#define CPLE_UserInterrupt              9

/* 100 - 299 reserved for GDAL */

#endif /* _CPL_ERROR_H_INCLUDED_ */
