/******************************************************************************
 *
 * Project:  OGR ODBC Driver
 * Purpose:  Declarations for ODBC Access Cover API.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam
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
 * $Log: cpl_odbc.h,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.8  2003/11/24 20:45:00  warmerda
 * make CollectResultsInfo() public
 *
 * Revision 1.7  2003/10/29 17:56:57  warmerda
 * Added PrimaryKeys() support
 *
 * Revision 1.6  2003/10/06 20:04:08  warmerda
 * added escaping support
 *
 * Revision 1.5  2003/10/06 17:16:18  warmerda
 * added windows.h for windows, and fixed m_panColSize type
 *
 * Revision 1.4  2003/09/26 20:02:41  warmerda
 * update GetColData()
 *
 * Revision 1.3  2003/09/26 13:51:02  warmerda
 * Add documentation
 *
 * Revision 1.2  2003/09/25 17:09:49  warmerda
 * added some more methods
 *
 * Revision 1.1  2003/09/24 15:39:14  warmerda
 * New
 *
 */

#ifndef CPL_ODBC_H_INCLUDED
#define CPL_ODBC_H_INCLUDED

#include "cpl_port.h"

#ifdef WIN32
#  include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

/**
 * \file cpl_odbc.h
 *
 * ODBC Abstraction Layer (C++).
 */

class CPLODBCStatement;

/**
 * A class representing an ODBC database session.
 *
 * Includes error collection services.
 */

class CPL_DLL CPLODBCSession {
    char      m_szLastError[SQL_MAX_MESSAGE_LENGTH + 1];
    HENV      m_hEnv;
    HDBC      m_hDBC;

  public:
    CPLODBCSession();
    ~CPLODBCSession();

    int		EstablishSession( const char *pszDSN,
                                  const char *pszUserid,
                                  const char *pszPassword );
    const char  *GetLastError();

    // Essentially internal.

    int         CloseSession();

    int         Failed( int, HSTMT = NULL );
    HDBC        GetConnection() { return m_hDBC; }
    HENV        GetEnvironment()  { return m_hEnv; }
};

/**
 * Abstraction for statement, and resultset.
 *
 * Includes methods for executing an SQL statement, and for accessing the
 * resultset from that statement.  Also provides for executing other ODBC
 * requests that produce results sets such as SQLColumns() and SQLTables()
 * requests.
 */

class CPL_DLL CPLODBCStatement {

    CPLODBCSession     *m_poSession;
    HSTMT		m_hStmt;

    short          m_nColCount;
    char         **m_papszColNames;
    short         *m_panColType;
    SQLUINTEGER   *m_panColSize;
    short         *m_panColPrecision;
    short         *m_panColNullable;

    char         **m_papszColValues;

    int            Failed( int );

    char          *m_pszStatement;
    int            m_nStatementMax;
    int            m_nStatementLen;

  public:
    CPLODBCStatement( CPLODBCSession * );
    ~CPLODBCStatement();

    HSTMT          GetStatement() { return m_hStmt; }

    // Command buffer related.
    void           Clear();
    void           AppendEscaped( const char * );
    void           Append( const char * );
    void           Append( int );
    void           Append( double );
    int            Appendf( const char *, ... );
    const char    *GetCommand() { return m_pszStatement; }

    int            ExecuteSQL( const char * = NULL );

    // Results fetching
    int            Fetch( int nOrientation = SQL_FETCH_NEXT,
                          int nOffset = 0 );
    void           ClearColumnData();

    int            GetColCount();
    const char    *GetColName(int iCol);
    short          GetColType(int iCol);
    short          GetColSize(int iCol);
    short          GetColPrecision(int iCol);
    short          GetColNullable(int iCol);

    int            GetColId( const char * );
    const char    *GetColData( int, const char * = NULL );
    const char    *GetColData( const char *, const char * = NULL );

    // Fetch special metadata.
    int            GetColumns( const char *pszTable,
                               const char *pszCatalog = NULL,
                               const char *pszSchema = NULL );
    int            GetPrimaryKeys( const char *pszTable,
                                   const char *pszCatalog = NULL,
                                   const char *pszSchema = NULL );

    int            GetTables( const char *pszCatalog = NULL,
                              const char *pszSchema = NULL );

    void           DumpResult( FILE *fp, int bShowSchema = FALSE );

    static const char *GetTypeName( int );

    int            CollectResultsInfo();
};



#endif


