/**********************************************************************
 *
 * Name:     cpl_string.h
 * Project:  CPL - Common Portability Library
 * Purpose:  String and StringList functions.
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
 * $Log: cpl_string.h,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.15  2003/07/17 10:15:40  dron
 * CSLTestBoolean() added.
 *
 * Revision 1.14  2003/03/11 21:33:03  warmerda
 * added URL encode/decode support, untested
 *
 * Revision 1.13  2003/01/30 19:15:55  warmerda
 * added some docs
 *
 * Revision 1.12  2002/07/12 22:37:05  warmerda
 * added CSLFetchBoolean
 *
 * Revision 1.11  2002/05/28 18:53:43  warmerda
 * added XML escaping support
 *
 * Revision 1.10  2002/04/26 14:55:26  warmerda
 * Added CPLEscapeString() and CPLUnescapeString() (unescape untested)
 *
 * Revision 1.9  2002/03/05 14:26:57  warmerda
 * expanded tabs
 *
 * Revision 1.8  2002/01/16 03:59:28  warmerda
 * added CPLTokenizeString2
 *
 * Revision 1.7  2000/10/06 15:19:03  warmerda
 * added CPLSetNameValueSeparator
 *
 * Revision 1.6  2000/04/26 18:25:10  warmerda
 * implement CPL_DLL
 *
 * Revision 1.5  2000/03/30 05:38:48  warmerda
 * added CPLParseNameValue
 *
 * Revision 1.4  1999/06/26 14:05:19  warmerda
 * Added CSLFindString().
 *
 * Revision 1.3  1999/02/17 01:41:58  warmerda
 * Added CSLGetField
 *
 * Revision 1.2  1998/12/04 21:40:42  danmo
 * Added more Name=Value manipulation fuctions
 *
 * Revision 1.1  1998/12/03 18:26:02  warmerda
 * New
 *
 **********************************************************************/

#ifndef _CPL_STRING_H_INCLUDED
#define _CPL_STRING_H_INCLUDED

#include "cpl_vsi.h"
#include "cpl_error.h"
#include "cpl_conv.h"

/**
 * \file cpl_string.h
 *
 * Various convenience functions for working with strings and string lists.
 *
 * A StringList is just an array of strings with the last pointer being
 * NULL.  An empty StringList may be either a NULL pointer, or a pointer to
 * a pointer memory location with a NULL value.
 *
 * A common convention for StringLists is to use them to store name/value
 * lists.  In this case the contents are treated like a dictionary of
 * name/value pairs.  The actual data is formatted with each string having
 * the format "<name>:<value>" (though "=" is also an acceptable separator).
 * A number of the functions in the file operate on name/value style
 * string lists (such as CSLSetNameValue(), and CSLFetchNameValue()).
 *
 */

CPL_C_START

char CPL_DLL **CSLAddString(char **papszStrList, const char *pszNewString);
int CPL_DLL CSLCount(char **papszStrList);
const char CPL_DLL *CSLGetField( char **, int );
void CPL_DLL CSLDestroy(char **papszStrList);
char CPL_DLL **CSLDuplicate(char **papszStrList);

char CPL_DLL **CSLTokenizeString(const char *pszString );
char CPL_DLL **CSLTokenizeStringComplex(const char *pszString,
                                   const char *pszDelimiter,
                                   int bHonourStrings, int bAllowEmptyTokens );
char CPL_DLL **CSLTokenizeString2( const char *pszString,
                                   const char *pszDelimeter,
                                   int nCSLTFlags );

#define CSLT_HONOURSTRINGS      0x0001
#define CSLT_ALLOWEMPTYTOKENS   0x0002
#define CSLT_PRESERVEQUOTES     0x0004
#define CSLT_PRESERVEESCAPES    0x0008

int CPL_DLL CSLPrint(char **papszStrList, FILE *fpOut);
char CPL_DLL **CSLLoad(const char *pszFname);
int CPL_DLL CSLSave(char **papszStrList, const char *pszFname);

char CPL_DLL **CSLInsertStrings(char **papszStrList, int nInsertAtLineNo,
                         char **papszNewLines);
char CPL_DLL **CSLInsertString(char **papszStrList, int nInsertAtLineNo,
                        char *pszNewLine);
char CPL_DLL **CSLRemoveStrings(char **papszStrList, int nFirstLineToDelete,
                         int nNumToRemove, char ***ppapszRetStrings);
int CPL_DLL CSLFindString( char **, const char * );
int CPL_DLL CSLTestBoolean( const char *pszValue );
int CPL_DLL CSLFetchBoolean( char **papszStrList, const char *pszKey,
                             int bDefault );

const char CPL_DLL *CPLSPrintf(char *fmt, ...);
char CPL_DLL **CSLAppendPrintf(char **papszStrList, char *fmt, ...);

const char CPL_DLL *
      CPLParseNameValue(const char *pszNameValue, char **ppszKey );
const char CPL_DLL *
      CSLFetchNameValue(char **papszStrList, const char *pszName);
char CPL_DLL **
      CSLFetchNameValueMultiple(char **papszStrList, const char *pszName);
char CPL_DLL **
      CSLAddNameValue(char **papszStrList,
                      const char *pszName, const char *pszValue);
char CPL_DLL **
      CSLSetNameValue(char **papszStrList,
                      const char *pszName, const char *pszValue);
void CPL_DLL CSLSetNameValueSeparator( char ** papszStrList,
                                       const char *pszSeparator );

#define CPLES_BackslashQuotable 0
#define CPLES_XML               1
#define CPLES_URL               2   /* unescape only for now */

char CPL_DLL *CPLEscapeString( const char *pszString, int nLength,
                               int nScheme );
char CPL_DLL *CPLUnescapeString( const char *pszString, int *pnLength,
                                 int nScheme );

CPL_C_END

#endif /* _CPL_STRING_H_INCLUDED */
