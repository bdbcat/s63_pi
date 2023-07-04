/**********************************************************************
 *
 * Name:     cpl_error.cpp
 * Project:  CPL - Common Portability Library
 * Purpose:  Error handling functions.
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
 * $Log: cpl_error.cpp,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:29  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.27  2003/05/27 20:44:16  warmerda
 * use VSI time services
 *
 * Revision 1.26  2003/05/08 21:51:14  warmerda
 * added CPL{G,S}etConfigOption() usage
 *
 * Revision 1.25  2003/04/04 14:57:38  dron
 * _vsnprintf() hack moved to the cpl_config.h.vc.
 *
 * Revision 1.24  2003/04/04 14:16:07  dron
 * Use _vsnprintf() in Windows environment.
 *
 * Revision 1.23  2002/10/23 20:19:37  warmerda
 * Modify log file naming convention as per patch from Dale.
 *
 * Revision 1.22  2002/08/01 20:02:54  warmerda
 * added CPL_LOG_ERRORS support
 *
 * Revision 1.21  2001/12/14 19:45:17  warmerda
 * Avoid use of errno in prototype.
 *
 * Revision 1.20  2001/11/27 17:01:06  warmerda
 * added timestamp to debug messages
 *
 * Revision 1.19  2001/11/15 16:11:08  warmerda
 * use vsnprintf() for debug calls if it is available
 *
 * Revision 1.18  2001/11/02 22:07:58  warmerda
 * added logging error handler
 *
 * Revision 1.17  2001/07/18 04:00:49  warmerda
 *
 * Revision 1.16  2001/02/15 16:30:57  warmerda
 * fixed initialization of fpLog
 *
 * Revision 1.15  2001/01/19 21:16:41  warmerda
 * expanded tabs
 *
 * Revision 1.14  2000/11/30 17:30:10  warmerda
 * added CPLGetLastErrorType
 *
 * Revision 1.13  2000/03/31 14:37:48  warmerda
 * only use vsnprintf where available
 *
 * Revision 1.12  2000/03/31 14:11:55  warmerda
 * added CPLErrorV
 *
 * Revision 1.11  2000/01/10 17:35:45  warmerda
 * added push down stack of error handlers
 *
 * Revision 1.10  1999/11/23 04:16:56  danmo
 * Fixed var. initialization that failed to compile as C
 *
 * Revision 1.9  1999/09/03 17:03:45  warmerda
 * Completed partial help line.
 *
 * Revision 1.8  1999/07/23 14:27:47  warmerda
 * CPLSetErrorHandler returns old handler
 *
 * Revision 1.7  1999/06/27 16:50:52  warmerda
 * added support for CPL_DEBUG and CPL_LOG variables
 *
 * Revision 1.6  1999/06/26 02:46:11  warmerda
 * Fixed initialization of debug messages.
 *
 * Revision 1.5  1999/05/20 14:59:05  warmerda
 * added CPLDebug()
 *
 * Revision 1.4  1999/05/20 02:54:38  warmerda
 * Added API documentation
 *
 * Revision 1.3  1998/12/15 19:02:27  warmerda
 * Avoid use of errno as a variable
 *
 * Revision 1.2  1998/12/06 02:52:52  warmerda
 * Implement assert support
 *
 * Revision 1.1  1998/12/03 18:26:02  warmerda
 * New
 *
 **********************************************************************/

#include "cpl_error.h"
#include "cpl_vsi.h"
#include "cpl_conv.h"

#define TIMESTAMP_DEBUG

/* static buffer to store the last error message.  We'll assume that error
 * messages cannot be longer than 2000 chars... which is quite reasonable
 * (that's 25 lines of 80 chars!!!)
 */
static char gszCPLLastErrMsg[2000] = "";
static int  gnCPLLastErrNo = 0;
static CPLErr geCPLLastErrType = CE_None;

static CPLErrorHandler gpfnCPLErrorHandler = CPLDefaultErrorHandler;

typedef struct errHandler
{
    struct errHandler   *psNext;
    CPLErrorHandler     pfnHandler;
} CPLErrorHandlerNode;

static CPLErrorHandlerNode * psHandlerStack = NULL;

/**********************************************************************
 *                          CPLError()
 **********************************************************************/

/**
 * Report an error.
 *
 * This function reports an error in a manner that can be hooked
 * and reported appropriate by different applications.
 *
 * The effect of this function can be altered by applications by installing
 * a custom error handling using CPLSetErrorHandler().
 *
 * The eErrClass argument can have the value CE_Warning indicating that the
 * message is an informational warning, CE_Failure indicating that the
 * action failed, but that normal recover mechanisms will be used or
 * CE_Fatal meaning that a fatal error has occured, and that CPLError()
 * should not return.
 *
 * The default behaviour of CPLError() is to report errors to stderr,
 * and to abort() after reporting a CE_Fatal error.  It is expected that
 * some applications will want to supress error reporting, and will want to
 * install a C++ exception, or longjmp() approach to no local fatal error
 * recovery.
 *
 * Regardless of how application error handlers or the default error
 * handler choose to handle an error, the error number, and message will
 * be stored for recovery with CPLGetLastErrorNo() and CPLGetLastErrorMsg().
 *
 * @param eErrClass one of CE_Warning, CE_Failure or CE_Fatal.
 * @param err_no the error number (CPLE_*) from cpl_error.h.
 * @param fmt a printf() style format string.  Any additional arguments
 * will be treated as arguments to fill in this format in a manner
 * similar to printf().
 */

void    CPLError(CPLErr eErrClass, int err_no, const char *fmt, ...)
{
    va_list args;

    /* Expand the error message
     */
    va_start(args, fmt);
    CPLErrorV( eErrClass, err_no, fmt, args );
    va_end(args);
}

/************************************************************************/
/*                             CPLErrorV()                              */
/************************************************************************/

void    CPLErrorV(CPLErr eErrClass, int err_no, const char *fmt, va_list args )
{
    /* Expand the error message
     */
#if defined(HAVE_VSNPRINTF)
    vsnprintf( gszCPLLastErrMsg, sizeof(gszCPLLastErrMsg), fmt, args );
#else
    vsprintf(gszCPLLastErrMsg, fmt, args);
#endif

    /* If the user provided his own error handling function, then call
     * it, otherwise print the error to stderr and return.
     */
    gnCPLLastErrNo = err_no;
    geCPLLastErrType = eErrClass;

    if( CPLGetConfigOption("CPL_LOG_ERRORS",NULL) != NULL )
        CPLDebug( "CPLError", "%s", gszCPLLastErrMsg );

    if( gpfnCPLErrorHandler )
        gpfnCPLErrorHandler(eErrClass, err_no, gszCPLLastErrMsg);

    if( eErrClass == CE_Fatal )
        abort();
}

/************************************************************************/
/*                              CPLDebug()                              */
/************************************************************************/

/**
 * Display a debugging message.
 *
 * The category argument is used in conjunction with the CPL_DEBUG
 * environment variable to establish if the message should be displayed.
 * If the CPL_DEBUG environment variable is not set, no debug messages
 * are emitted (use CPLError(CE_Warning,...) to ensure messages are displayed).
 * If CPL_DEBUG is set, but is an empty string or the word "ON" then all
 * debug messages are shown.  Otherwise only messages whose category appears
 * somewhere within the CPL_DEBUG value are displayed (as determinted by
 * strstr()).
 *
 * Categories are usually an identifier for the subsystem producing the
 * error.  For instance "GDAL" might be used for the GDAL core, and "TIFF"
 * for messages from the TIFF translator.
 *
 * @param pszCategory name of the debugging message category.
 * @param pszFormat printf() style format string for message to display.
 *        Remaining arguments are assumed to be for format.
 */

void CPLDebug( const char * pszCategory, const char * pszFormat, ... )

{
    char        *pszMessage;
    va_list     args;
    const char  *pszDebug = CPLGetConfigOption("CPL_DEBUG",NULL);

#define ERROR_MAX 25000

/* -------------------------------------------------------------------- */
/*      Does this message pass our current criteria?                    */
/* -------------------------------------------------------------------- */
    if( pszDebug == NULL )
        return;

    if( !EQUAL(pszDebug,"ON") && !EQUAL(pszDebug,"") )
    {
        int            i, nLen = strlen(pszCategory);

        for( i = 0; pszDebug[i] != '\0'; i++ )
        {
            if( EQUALN(pszCategory,pszDebug+i,nLen) )
                break;
        }

        if( pszDebug[i] == '\0' )
            return;
    }

/* -------------------------------------------------------------------- */
/*    Allocate a block for the error.                                   */
/* -------------------------------------------------------------------- */
    pszMessage = (char *) VSIMalloc( ERROR_MAX );
    if( pszMessage == NULL )
        return;

/* -------------------------------------------------------------------- */
/*      Dal -- always log a timestamp as the first part of the line     */
/*      to ensure one is looking at what one should be looking at!      */
/* -------------------------------------------------------------------- */

    pszMessage[0] = '\0';
#ifdef TIMESTAMP_DEBUG
    if( CPLGetConfigOption( "CPL_TIMESTAMP", NULL ) != NULL )
    {
        strcpy( pszMessage, VSICTime( VSITime(NULL) ) );

        // On windows anyway, ctime puts a \n at the end, but I'm not
        // convinced this is standard behaviour, so we'll get rid of it
        // carefully

        if (pszMessage[strlen(pszMessage) -1 ] == '\n')
        {
            pszMessage[strlen(pszMessage) - 1] = 0; // blow it out
        }
        strcat( pszMessage, ": " );
    }
#endif

/* -------------------------------------------------------------------- */
/*      Add the category.                                               */
/* -------------------------------------------------------------------- */
    strcat( pszMessage, pszCategory );
    strcat( pszMessage, ": " );

/* -------------------------------------------------------------------- */
/*      Format the application provided portion of the debug message.   */
/* -------------------------------------------------------------------- */
    va_start(args, pszFormat);
#if defined(HAVE_VSNPRINTF)
    vsnprintf(pszMessage+strlen(pszMessage), ERROR_MAX - strlen(pszMessage),
              pszFormat, args);
#else
    vsprintf(pszMessage+strlen(pszMessage), pszFormat, args);
#endif
    va_end(args);

/* -------------------------------------------------------------------- */
/*      If the user provided his own error handling function, then call */
/*      it, otherwise print the error to stderr and return.             */
/* -------------------------------------------------------------------- */
    if( gpfnCPLErrorHandler )
        gpfnCPLErrorHandler(CE_Debug, CPLE_None, pszMessage);

    VSIFree( pszMessage );
}

/**********************************************************************
 *                          CPLErrorReset()
 **********************************************************************/

/**
 * Erase any traces of previous errors.
 *
 * This is normally used to ensure that an error which has been recovered
 * from does not appear to be still in play with high level functions.
 */

void    CPLErrorReset()
{
    gnCPLLastErrNo = CPLE_None;
    gszCPLLastErrMsg[0] = '\0';
    geCPLLastErrType = CE_None;
}


/**********************************************************************
 *                          CPLGetLastErrorNo()
 **********************************************************************/

/**
 * Fetch the last error number.
 *
 * This is the error number, not the error class.
 *
 * @return the error number of the last error to occur, or CPLE_None (0)
 * if there are no posted errors.
 */

int     CPLGetLastErrorNo()
{
    return gnCPLLastErrNo;
}

/**********************************************************************
 *                          CPLGetLastErrorType()
 **********************************************************************/

/**
 * Fetch the last error type.
 *
 * This is the error class, not the error number.
 *
 * @return the error number of the last error to occur, or CE_None (0)
 * if there are no posted errors.
 */

CPLErr CPLGetLastErrorType()
{
    return geCPLLastErrType;
}

/**********************************************************************
 *                          CPLGetLastErrorMsg()
 **********************************************************************/

/**
 * Get the last error message.
 *
 * Fetches the last error message posted with CPLError(), that hasn't
 * been cleared by CPLErrorReset().  The returned pointer is to an internal
 * string that should not be altered or freed.
 *
 * @return the last error message, or NULL if there is no posted error
 * message.
 */

const char* CPLGetLastErrorMsg()
{
    return gszCPLLastErrMsg;
}

/************************************************************************/
/*                       CPLDefaultErrorHandler()                       */
/************************************************************************/

void CPLDefaultErrorHandler( CPLErr eErrClass, int nError,
                             const char * pszErrorMsg )

{
    static int       bLogInit = FALSE;
    static FILE *    fpLog = stderr;

    if( !bLogInit )
    {
        bLogInit = TRUE;

        fpLog = stderr;
        if( CPLGetConfigOption( "CPL_LOG", NULL ) != NULL )
        {
            fpLog = fopen( CPLGetConfigOption("CPL_LOG",""), "wt" );
            if( fpLog == NULL )
                fpLog = stderr;
        }
    }

    if( eErrClass == CE_Debug )
        fprintf( fpLog, "%s\n", pszErrorMsg );
    else if( eErrClass == CE_Warning )
        fprintf( fpLog, "Warning %d: %s\n", nError, pszErrorMsg );
    else
        fprintf( fpLog, "ERROR %d: %s\n", nError, pszErrorMsg );

    fflush( fpLog );
}

/************************************************************************/
/*                        CPLQuietErrorHandler()                        */
/************************************************************************/

void CPLQuietErrorHandler( CPLErr eErrClass , int nError,
                           const char * pszErrorMsg )

{
    if( eErrClass == CE_Debug )
        CPLDefaultErrorHandler( eErrClass, nError, pszErrorMsg );
}

/************************************************************************/
/*                       CPLLoggingErrorHandler()                       */
/************************************************************************/

void CPLLoggingErrorHandler( CPLErr eErrClass, int nError,
                             const char * pszErrorMsg )

{
    static int       bLogInit = FALSE;
    static FILE *    fpLog = stderr;

    if( !bLogInit )
    {
        const char *cpl_log = NULL;

        CPLSetConfigOption( "CPL_TIMESTAMP", "ON" );

        bLogInit = TRUE;

        cpl_log = CPLGetConfigOption("CPL_LOG", NULL );

        fpLog = stderr;
        if( cpl_log != NULL && EQUAL(cpl_log,"OFF") )
        {
            fpLog = NULL;
        }
        else if( cpl_log != NULL )
        {
            char      path[5000];
            int       i = 0;

            strcpy( path, cpl_log );

            while( (fpLog = fopen( path, "rt" )) != NULL )
            {
                fclose( fpLog );

                /* generate sequenced log file names, inserting # before ext.*/
                if (strrchr(cpl_log, '.') == NULL)
                {
                    sprintf( path, "%s_%d%s", cpl_log, i++,
                             ".log" );
                }
                else
                {
                    int pos = 0;
                    char *cpl_log_base = strdup(cpl_log);
                    pos = strcspn(cpl_log_base, ".");
                    if (pos > 0)
                    {
                        cpl_log_base[pos] = '\0';
                    }
                    sprintf( path, "%s_%d%s", cpl_log_base,
                             i++, ".log" );
                    free(cpl_log_base);
                }
            }

            fpLog = fopen( path, "wt" );
        }
    }

    if( fpLog == NULL )
        return;

    if( eErrClass == CE_Debug )
        fprintf( fpLog, "%s\n", pszErrorMsg );
    else if( eErrClass == CE_Warning )
        fprintf( fpLog, "Warning %d: %s\n", nError, pszErrorMsg );
    else
        fprintf( fpLog, "ERROR %d: %s\n", nError, pszErrorMsg );

    fflush( fpLog );
}

/**********************************************************************
 *                          CPLSetErrorHandler()
 **********************************************************************/

/**
 * Install custom error handler.
 *
 * Allow the library's user to specify his own error handler function.
 * A valid error handler is a C function with the following prototype:
 *
 * <pre>
 *     void MyErrorHandler(CPLErr eErrClass, int err_no, const char *msg)
 * </pre>
 *
 * Pass NULL to come back to the default behavior.  The default behaviour
 * (CPLDefaultErrorHandler()) is to write the message to stderr.
 *
 * The msg will be a partially formatted error message not containing the
 * "ERROR %d:" portion emitted by the default handler.  Message formatting
 * is handled by CPLError() before calling the handler.  If the error
 * handler function is passed a CE_Fatal class error and returns, then
 * CPLError() will call abort(). Applications wanting to interrupt this
 * fatal behaviour will have to use longjmp(), or a C++ exception to
 * indirectly exit the function.
 *
 * Another standard error handler is CPLQuietErrorHandler() which doesn't
 * make any attempt to report the passed error or warning messages but
 * will process debug messages via CPLDefaultErrorHandler.
 *
 * @param pfnErrorHandler new error handler function.
 * @return returns the previously installed error handler.
 */

CPLErrorHandler CPLSetErrorHandler( CPLErrorHandler pfnErrorHandler )
{
    CPLErrorHandler     pfnOldHandler = gpfnCPLErrorHandler;

    gpfnCPLErrorHandler = pfnErrorHandler;

    return pfnOldHandler;
}



/************************************************************************/
/*                        CPLPushErrorHandler()                         */
/************************************************************************/

/**
 * Assign new CPLError handler.
 *
 * The old handler is "pushed down" onto a stack and can be easily
 * restored with CPLPopErrorHandler().  Otherwise this works similarly
 * to CPLSetErrorHandler() which contains more details on how error
 * handlers work.
 *
 * @param pfnErrorHandler new error handler function.
 */

void CPLPushErrorHandler( CPLErrorHandler pfnErrorHandler )

{
    CPLErrorHandlerNode         *psNode;

    psNode = (CPLErrorHandlerNode *) VSIMalloc(sizeof(CPLErrorHandlerNode));
    psNode->psNext = psHandlerStack;
    psNode->pfnHandler = gpfnCPLErrorHandler;

    psHandlerStack = psNode;

    CPLSetErrorHandler( pfnErrorHandler );
}

/************************************************************************/
/*                         CPLPopErrorHandler()                         */
/************************************************************************/

/**
 * Restore old CPLError handler.
 *
 * Discards the current error handler, and restore the one in use before
 * the last CPLPushErrorHandler() call.
 */

void CPLPopErrorHandler()

{
    if( psHandlerStack != NULL )
    {
        CPLErrorHandlerNode     *psNode = psHandlerStack;

        psHandlerStack = psNode->psNext;
        CPLSetErrorHandler( psNode->pfnHandler );
        VSIFree( psNode );
    }
}

/************************************************************************/
/*                             _CPLAssert()                             */
/*                                                                      */
/*      This function is called only when an assertion fails.           */
/************************************************************************/

/**
 * Report failure of a logical assertion.
 *
 * Applications would normally use the CPLAssert() macro which expands
 * into code calling _CPLAssert() only if the condition fails.  _CPLAssert()
 * will generate a CE_Fatal error call to CPLError(), indicating the file
 * name, and line number of the failed assertion, as well as containing
 * the assertion itself.
 *
 * There is no reason for application code to call _CPLAssert() directly.
 */

void _CPLAssert( const char * pszExpression, const char * pszFile,
                 int iLine )

{
    CPLError( CE_Fatal, CPLE_AssertionFailed,
              "Assertion `%s' failed\n"
              "in file `%s', line %d\n",
              pszExpression, pszFile, iLine );
}

