/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The OGRPolygon geometry class.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * $Log: ogrpolygon.cpp,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.26  2004/02/22 09:56:54  dron
 * Fix compirison casting problems in OGRPolygon::Equal().
 *
 * Revision 1.25  2004/02/21 15:36:14  warmerda
 * const correctness updates for geometry: bug 289
 *
 * Revision 1.24  2004/01/16 21:57:17  warmerda
 * fixed up EMPTY support
 *
 * Revision 1.23  2004/01/16 21:20:00  warmerda
 * Added EMPTY support
 *
 * Revision 1.22  2003/08/27 15:40:37  warmerda
 * added support for generating DB2 V7.2 compatible WKB
 *
 * Revision 1.21  2003/06/09 13:48:54  warmerda
 * added DB2 V7.2 byte order hack
 *
 * Revision 1.20  2003/05/28 19:16:43  warmerda
 * fixed up argument names and stuff for docs
 *
 * Revision 1.19  2003/03/07 21:28:56  warmerda
 * support 0x8000 style 3D WKB flags
 *
 * Revision 1.18  2002/09/11 13:47:17  warmerda
 * preliminary set of fixes for 3D WKB enum
 *
 * Revision 1.17  2002/05/02 19:44:53  warmerda
 * fixed 3D binary support for polygon/linearring
 *
 * Revision 1.16  2002/03/05 14:25:14  warmerda
 * expand tabs
 *
 * Revision 1.15  2001/11/01 17:20:33  warmerda
 * added DISABLE_OGRGEOM_TRANSFORM macro
 *
 * Revision 1.14  2001/09/21 16:24:20  warmerda
 * added transform() and transformTo() methods
 *
 * Revision 1.13  2001/07/18 05:03:05  warmerda
 *
 * Revision 1.12  1999/11/18 19:02:19  warmerda
 * expanded tabs
 *
 * Revision 1.11  1999/09/22 13:18:55  warmerda
 * Added the addRingDirectly() method.
 *
 * Revision 1.10  1999/09/13 02:27:33  warmerda
 * incorporated limited 2.5d support
 *
 * Revision 1.9  1999/07/27 00:48:12  warmerda
 * Added Equal() support
 *
 * Revision 1.8  1999/07/06 21:36:47  warmerda
 * tenatively added getEnvelope() and Intersect()
 *
 * Revision 1.7  1999/06/25 20:44:43  warmerda
 * implemented assignSpatialReference, carry properly
 *
 * Revision 1.6  1999/05/31 20:43:55  warmerda
 * added empty() method
 *
 * Revision 1.5  1999/05/31 14:59:06  warmerda
 * added documentation
 *
 * Revision 1.4  1999/05/23 05:34:41  warmerda
 * added support for clone(), multipolygons and geometry collections
 *
 * Revision 1.3  1999/05/20 14:35:44  warmerda
 * added support for well known text format
 *
 * Revision 1.2  1999/05/17 14:38:11  warmerda
 * Added new IPolygon style methods.
 *
 * Revision 1.1  1999/03/30 21:21:05  warmerda
 * New
 *
 */

#include "ogr_geometry.h"
#include "ogr_p.h"

/************************************************************************/
/*                             OGRPolygon()                             */
/************************************************************************/

/**
 * Create an empty polygon.
 */

OGRPolygon::OGRPolygon()

{
    nRingCount = 0;
    papoRings = NULL;
}

/************************************************************************/
/*                            ~OGRPolygon()                             */
/************************************************************************/

OGRPolygon::~OGRPolygon()

{
    empty();
}

/************************************************************************/
/*                               clone()                                */
/************************************************************************/

OGRGeometry *OGRPolygon::clone() const

{
    OGRPolygon  *poNewPolygon;

    poNewPolygon = new OGRPolygon;
    poNewPolygon->assignSpatialReference( getSpatialReference() );

    for( int i = 0; i < nRingCount; i++ )
    {
        poNewPolygon->addRing( papoRings[i] );
    }

    return poNewPolygon;
}

/************************************************************************/
/*                               empty()                                */
/************************************************************************/

void OGRPolygon::empty()

{
    if( papoRings != NULL )
    {
        for( int i = 0; i < nRingCount; i++ )
        {
            delete papoRings[i];
        }
        OGRFree( papoRings );
    }

    papoRings = NULL;
    nRingCount = 0;
}

/************************************************************************/
/*                          getGeometryType()                           */
/************************************************************************/

OGRwkbGeometryType OGRPolygon::getGeometryType() const

{
    if( getCoordinateDimension() == 3 )
        return wkbPolygon25D;
    else
        return wkbPolygon;
}

/************************************************************************/
/*                            getDimension()                            */
/************************************************************************/

int OGRPolygon::getDimension() const

{
    return 2;
}

/************************************************************************/
/*                       getCoordinateDimension()                       */
/************************************************************************/

int OGRPolygon::getCoordinateDimension() const

{
    for( int iRing = 0; iRing < nRingCount; iRing++ )
    {
        if( papoRings[iRing]->getCoordinateDimension() == 3 )
            return 3;
    }

    return 2;
}

/************************************************************************/
/*                            flattenTo2D()                             */
/************************************************************************/

void OGRPolygon::flattenTo2D()

{
    for( int iRing = 0; iRing < nRingCount; iRing++ )
        papoRings[iRing]->flattenTo2D();
}

/************************************************************************/
/*                          getGeometryName()                           */
/************************************************************************/

const char * OGRPolygon::getGeometryName() const

{
    return "POLYGON";
}

/************************************************************************/
/*                          getExteriorRing()                           */
/************************************************************************/

/**
 * Fetch reference to external polygon ring.
 *
 * Note that the returned ring pointer is to an internal data object of
 * the OGRPolygon.  It should not be modified or deleted by the application,
 * and the pointer is only valid till the polygon is next modified.  Use
 * the OGRGeometry::clone() method to make a separate copy within the
 * application.
 *
 * Relates to the SFCOM IPolygon::get_ExteriorRing() method.
 *
 * @return pointer to external ring.  May be NULL if the OGRPolygon is empty.
 */

OGRLinearRing *OGRPolygon::getExteriorRing()

{
    if( nRingCount > 0 )
        return papoRings[0];
    else
        return NULL;
}

const OGRLinearRing *OGRPolygon::getExteriorRing() const

{
    if( nRingCount > 0 )
        return papoRings[0];
    else
        return NULL;
}

/************************************************************************/
/*                        getNumInteriorRings()                         */
/************************************************************************/

/**
 * Fetch the number of internal rings.
 *
 * Relates to the SFCOM IPolygon::get_NumInteriorRings() method.
 *
 * @return count of internal rings, zero or more.
 */


int OGRPolygon::getNumInteriorRings() const

{
    if( nRingCount > 0 )
        return nRingCount-1;
    else
        return 0;
}

/************************************************************************/
/*                          getInteriorRing()                           */
/************************************************************************/

/**
 * Fetch reference to indicated internal ring.
 *
 * Note that the returned ring pointer is to an internal data object of
 * the OGRPolygon.  It should not be modified or deleted by the application,
 * and the pointer is only valid till the polygon is next modified.  Use
 * the OGRGeometry::clone() method to make a separate copy within the
 * application.
 *
 * Relates to the SFCOM IPolygon::get_InternalRing() method.
 *
 * @param iRing internal ring index from 0 to getNumInternalRings() - 1.
 *
 * @return pointer to external ring.  May be NULL if the OGRPolygon is empty.
 */

OGRLinearRing *OGRPolygon::getInteriorRing( int iRing )

{
    if( iRing < 0 || iRing >= nRingCount-1 )
        return NULL;
    else
        return papoRings[iRing+1];
}

const OGRLinearRing *OGRPolygon::getInteriorRing( int iRing ) const

{
    if( iRing < 0 || iRing >= nRingCount-1 )
        return NULL;
    else
        return papoRings[iRing+1];
}

/************************************************************************/
/*                              addRing()                               */
/************************************************************************/

/**
 * Add a ring to a polygon.
 *
 * If the polygon has no external ring (it is empty) this will be used as
 * the external ring, otherwise it is used as an internal ring.  The passed
 * OGRLinearRing remains the responsibility of the caller (an internal copy
 * is made).
 *
 * This method has no SFCOM analog.
 *
 * @param poNewRing ring to be added to the polygon.
 */

void OGRPolygon::addRing( OGRLinearRing * poNewRing )

{
    papoRings = (OGRLinearRing **) OGRRealloc( papoRings,
                                               sizeof(void*) * (nRingCount+1));

    papoRings[nRingCount] = new OGRLinearRing( poNewRing );

    nRingCount++;
}

/************************************************************************/
/*                          addRingDirectly()                           */
/************************************************************************/

/**
 * Add a ring to a polygon.
 *
 * If the polygon has no external ring (it is empty) this will be used as
 * the external ring, otherwise it is used as an internal ring.  Ownership
 * of the passed ring is assumed by the OGRPolygon, but otherwise this
 * method operates the same as OGRPolygon::AddRing().
 *
 * This method has no SFCOM analog.
 *
 * @param poNewRing ring to be added to the polygon.
 */

void OGRPolygon::addRingDirectly( OGRLinearRing * poNewRing )

{
    papoRings = (OGRLinearRing **) OGRRealloc( papoRings,
                                               sizeof(void*) * (nRingCount+1));

    papoRings[nRingCount] = poNewRing;

    nRingCount++;
}

/************************************************************************/
/*                              WkbSize()                               */
/*                                                                      */
/*      Return the size of this object in well known binary             */
/*      representation including the byte order, and type information.  */
/************************************************************************/

int OGRPolygon::WkbSize() const

{
    int         nSize = 9;
    int         b3D = getCoordinateDimension() == 3;

    for( int i = 0; i < nRingCount; i++ )
    {
        nSize += papoRings[i]->_WkbSize( b3D );
    }

    return nSize;
}

/************************************************************************/
/*                           importFromWkb()                            */
/*                                                                      */
/*      Initialize from serialized stream in well known binary          */
/*      format.                                                         */
/************************************************************************/

OGRErr OGRPolygon::importFromWkb( unsigned char * pabyData,
                                  int nSize )

{
    OGRwkbByteOrder     eByteOrder;
    int                 nDataOffset, b3D;

    if( nSize < 21 && nSize != -1 )
        return OGRERR_NOT_ENOUGH_DATA;

/* -------------------------------------------------------------------- */
/*      Get the byte order byte.                                        */
/* -------------------------------------------------------------------- */
    eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *pabyData);
    CPLAssert( eByteOrder == wkbXDR || eByteOrder == wkbNDR );

/* -------------------------------------------------------------------- */
/*      Get the geometry feature type.  For now we assume that          */
/*      geometry type is between 0 and 255 so we only have to fetch     */
/*      one byte.                                                       */
/* -------------------------------------------------------------------- */
#ifdef DEBUG
    OGRwkbGeometryType eGeometryType;

    if( eByteOrder == wkbNDR )
        eGeometryType = (OGRwkbGeometryType) pabyData[1];
    else
        eGeometryType = (OGRwkbGeometryType) pabyData[4];

    CPLAssert( eGeometryType == wkbPolygon );
#endif

    if( eByteOrder == wkbNDR )
        b3D = pabyData[4] & 0x80 || pabyData[2] & 0x80;
    else
        b3D = pabyData[1] & 0x80 || pabyData[3] & 0x80;

/* -------------------------------------------------------------------- */
/*      Do we already have some rings?                                  */
/* -------------------------------------------------------------------- */
    if( nRingCount != 0 )
    {
        for( int iRing = 0; iRing < nRingCount; iRing++ )
            delete papoRings[iRing];

        OGRFree( papoRings );
        papoRings = NULL;
    }

/* -------------------------------------------------------------------- */
/*      Get the ring count.                                             */
/* -------------------------------------------------------------------- */
    memcpy( &nRingCount, pabyData + 5, 4 );

    if( OGR_SWAP( eByteOrder ) )
        nRingCount = CPL_SWAP32(nRingCount);

    papoRings = (OGRLinearRing **) OGRMalloc(sizeof(void*) * nRingCount);

    nDataOffset = 9;
    if( nSize != -1 )
        nSize -= nDataOffset;

/* -------------------------------------------------------------------- */
/*      Get the rings.                                                  */
/* -------------------------------------------------------------------- */
    for( int iRing = 0; iRing < nRingCount; iRing++ )
    {
        OGRErr  eErr;

        papoRings[iRing] = new OGRLinearRing();
        eErr = papoRings[iRing]->_importFromWkb( eByteOrder, b3D,
                                                 pabyData + nDataOffset,
                                                 nSize );
        if( eErr != OGRERR_NONE )
        {
            nRingCount = iRing;
            return eErr;
        }

        if( nSize != -1 )
            nSize -= papoRings[iRing]->_WkbSize( b3D );

        nDataOffset += papoRings[iRing]->_WkbSize( b3D );
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkb()                             */
/*                                                                      */
/*      Build a well known binary representation of this object.        */
/************************************************************************/

OGRErr  OGRPolygon::exportToWkb( OGRwkbByteOrder eByteOrder,
                                 unsigned char * pabyData ) const

{
    int         nOffset;
    int         b3D = getCoordinateDimension() == 3;

/* -------------------------------------------------------------------- */
/*      Set the byte order.                                             */
/* -------------------------------------------------------------------- */
    pabyData[0] = DB2_V72_UNFIX_BYTE_ORDER((unsigned char) eByteOrder);

/* -------------------------------------------------------------------- */
/*      Set the geometry feature type.                                  */
/* -------------------------------------------------------------------- */
    GUInt32 nGType = getGeometryType();

    if( eByteOrder == wkbNDR )
        nGType = CPL_LSBWORD32( nGType );
    else
        nGType = CPL_MSBWORD32( nGType );

    memcpy( pabyData + 1, &nGType, 4 );

/* -------------------------------------------------------------------- */
/*      Copy in the raw data.                                           */
/* -------------------------------------------------------------------- */
    if( OGR_SWAP( eByteOrder ) )
    {
        int     nCount;

        nCount = CPL_SWAP32( nRingCount );
        memcpy( pabyData+5, &nCount, 4 );
    }
    else
    {
        memcpy( pabyData+5, &nRingCount, 4 );
    }

    nOffset = 9;

/* ==================================================================== */
/*      Serialize each of the rings.                                    */
/* ==================================================================== */
    for( int iRing = 0; iRing < nRingCount; iRing++ )
    {
        papoRings[iRing]->_exportToWkb( eByteOrder, b3D,
                                        pabyData + nOffset );

        nOffset += papoRings[iRing]->_WkbSize(b3D);
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromWkt()                            */
/*                                                                      */
/*      Instantiate from well known text format.  Currently this is     */
/*      `POLYGON ((x y, x y, ...),(x y, ...),...)'.                     */
/************************************************************************/

OGRErr OGRPolygon::importFromWkt( char ** ppszInput )

{
    char        szToken[OGR_WKT_TOKEN_MAX];
    const char  *pszInput = *ppszInput;
    int         iRing;

/* -------------------------------------------------------------------- */
/*      Clear existing rings.                                           */
/* -------------------------------------------------------------------- */
    if( nRingCount > 0 )
    {
        for( iRing = 0; iRing < nRingCount; iRing++ )
            delete papoRings[iRing];

        nRingCount = 0;
        CPLFree( papoRings );
    }

/* -------------------------------------------------------------------- */
/*      Read and verify the ``POLYGON'' keyword token.                  */
/* -------------------------------------------------------------------- */
    pszInput = OGRWktReadToken( pszInput, szToken );

    if( !EQUAL(szToken,"POLYGON") )
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      The next character should be a ( indicating the start of the    */
/*      list of rings.                                                  */
/* -------------------------------------------------------------------- */
    pszInput = OGRWktReadToken( pszInput, szToken );
    if( szToken[0] != '(' )
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      If the next token is EMPTY, then verify that we have proper     */
/*      EMPTY format will a trailing closing bracket.                   */
/* -------------------------------------------------------------------- */
    OGRWktReadToken( pszInput, szToken );
    if( EQUAL(szToken,"EMPTY") )
    {
        pszInput = OGRWktReadToken( pszInput, szToken );
        pszInput = OGRWktReadToken( pszInput, szToken );

        *ppszInput = (char *) pszInput;

        if( !EQUAL(szToken,")") )
            return OGRERR_CORRUPT_DATA;
        else
            return OGRERR_NONE;
    }

/* ==================================================================== */
/*      Read each ring in turn.  Note that we try to reuse the same     */
/*      point list buffer from ring to ring to cut down on              */
/*      allocate/deallocate overhead.                                   */
/* ==================================================================== */
    OGRRawPoint *paoPoints = NULL;
    int         nMaxPoints = 0, nMaxRings = 0;
    double      *padfZ = NULL;

    do
    {
        int     nPoints = 0;

/* -------------------------------------------------------------------- */
/*      Read points for one ring from input.                            */
/* -------------------------------------------------------------------- */
        pszInput = OGRWktReadPoints( pszInput, &paoPoints, &padfZ, &nMaxPoints,
                                     &nPoints );

        if( pszInput == NULL )
        {
            CPLFree( paoPoints );
            return OGRERR_CORRUPT_DATA;
        }

/* -------------------------------------------------------------------- */
/*      Do we need to grow the ring array?                              */
/* -------------------------------------------------------------------- */
        if( nRingCount == nMaxRings )
        {
            nMaxRings = nMaxRings * 2 + 1;
            papoRings = (OGRLinearRing **)
                CPLRealloc(papoRings, nMaxRings * sizeof(OGRLinearRing*));
        }

/* -------------------------------------------------------------------- */
/*      Create the new ring, and assign to ring list.                   */
/* -------------------------------------------------------------------- */
        papoRings[nRingCount] = new OGRLinearRing();
        papoRings[nRingCount]->setPoints( nPoints, paoPoints, padfZ );

        nRingCount++;

/* -------------------------------------------------------------------- */
/*      Read the delimeter following the ring.                          */
/* -------------------------------------------------------------------- */

        pszInput = OGRWktReadToken( pszInput, szToken );
    } while( szToken[0] == ',' );

/* -------------------------------------------------------------------- */
/*      freak if we don't get a closing bracket.                        */
/* -------------------------------------------------------------------- */
    CPLFree( paoPoints );
    CPLFree( padfZ );

    if( szToken[0] != ')' )
        return OGRERR_CORRUPT_DATA;

    *ppszInput = (char *) pszInput;
    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkt()                             */
/*                                                                      */
/*      Translate this structure into it's well known text format       */
/*      equivelent.  This could be made alot more CPU efficient!        */
/************************************************************************/

OGRErr OGRPolygon::exportToWkt( char ** ppszDstText ) const

{
    char        **papszRings;
    int         iRing, nCumulativeLength = 0;
    OGRErr      eErr;

/* -------------------------------------------------------------------- */
/*      Handle special empty case.                                      */
/* -------------------------------------------------------------------- */
    if( nRingCount == 0 )
    {
        *ppszDstText = CPLStrdup("POLYGON(EMPTY)");
        return OGRERR_NONE;
    }

/* -------------------------------------------------------------------- */
/*      Build a list of strings containing the stuff for each ring.     */
/* -------------------------------------------------------------------- */
    papszRings = (char **) CPLCalloc(sizeof(char *),nRingCount);

    for( iRing = 0; iRing < nRingCount; iRing++ )
    {
        eErr = papoRings[iRing]->exportToWkt( &(papszRings[iRing]) );
        if( eErr != OGRERR_NONE )
            return eErr;

        CPLAssert( EQUALN(papszRings[iRing],"LINEARRING (", 12) );
        nCumulativeLength += strlen(papszRings[iRing] + 11);
    }

/* -------------------------------------------------------------------- */
/*      Allocate exactly the right amount of space for the              */
/*      aggregated string.                                              */
/* -------------------------------------------------------------------- */
    *ppszDstText = (char *) VSIMalloc(nCumulativeLength + nRingCount + 11);

    if( *ppszDstText == NULL )
        return OGRERR_NOT_ENOUGH_MEMORY;

/* -------------------------------------------------------------------- */
/*      Build up the string, freeing temporary strings as we go.        */
/* -------------------------------------------------------------------- */
    strcpy( *ppszDstText, "POLYGON (" );

    for( iRing = 0; iRing < nRingCount; iRing++ )
    {
        if( iRing > 0 )
            strcat( *ppszDstText, "," );

        strcat( *ppszDstText, papszRings[iRing] + 11 );
        VSIFree( papszRings[iRing] );
    }

    strcat( *ppszDstText, ")" );

    CPLFree( papszRings );

    return OGRERR_NONE;
}

/************************************************************************/
/*                              get_Area()                              */
/************************************************************************/

double OGRPolygon::get_Area() const

{
    // notdef ... correct later.

    return 0.0;
}

/************************************************************************/
/*                              Centroid()                              */
/************************************************************************/

int OGRPolygon::Centroid( OGRPoint * ) const

{
    // notdef ... not implemented yet.

    return OGRERR_FAILURE;
}

/************************************************************************/
/*                           PointOnSurface()                           */
/************************************************************************/

int OGRPolygon::PointOnSurface( OGRPoint * ) const

{
    // notdef ... not implemented yet.

    return OGRERR_FAILURE;
}

/************************************************************************/
/*                            getEnvelope()                             */
/************************************************************************/

void OGRPolygon::getEnvelope( OGREnvelope * psEnvelope ) const

{
    OGREnvelope         oRingEnv;

    if( nRingCount == 0 )
        return;

    papoRings[0]->getEnvelope( psEnvelope );

    for( int iRing = 1; iRing < nRingCount; iRing++ )
    {
        papoRings[iRing]->getEnvelope( &oRingEnv );

        if( psEnvelope->MinX > oRingEnv.MinX )
            psEnvelope->MinX = oRingEnv.MinX;
        if( psEnvelope->MinY > oRingEnv.MinY )
            psEnvelope->MinY = oRingEnv.MinY;
        if( psEnvelope->MaxX < oRingEnv.MaxX )
            psEnvelope->MaxX = oRingEnv.MaxX;
        if( psEnvelope->MaxY < oRingEnv.MaxY )
            psEnvelope->MaxY = oRingEnv.MaxY;
    }
}

/************************************************************************/
/*                               Equal()                                */
/************************************************************************/

OGRBoolean OGRPolygon::Equal( OGRGeometry * poOther ) const

{
    OGRPolygon *poOPoly = (OGRPolygon *) poOther;

    if( poOPoly == this )
        return TRUE;

    if( poOther->getGeometryType() != getGeometryType() )
        return FALSE;

    if( getNumInteriorRings() != poOPoly->getNumInteriorRings() )
        return FALSE;

    if( !getExteriorRing()->Equal( poOPoly->getExteriorRing() ) )
        return FALSE;

    // we should eventually test the SRS.

    for( int iRing = 0; iRing < getNumInteriorRings(); iRing++ )
    {
        if( !getInteriorRing(iRing)->Equal(poOPoly->getInteriorRing(iRing)) )
            return FALSE;
    }

    return TRUE;
}

/************************************************************************/
/*                             transform()                              */
/************************************************************************/

OGRErr OGRPolygon::transform( OGRCoordinateTransformation *poCT )

{
#ifdef DISABLE_OGRGEOM_TRANSFORM
    return OGRERR_FAILURE;
#else
    for( int iRing = 0; iRing < nRingCount; iRing++ )
    {
        OGRErr  eErr;

        eErr = papoRings[iRing]->transform( poCT );
        if( eErr != OGRERR_NONE )
        {
            if( iRing != 0 )
            {
                CPLDebug("OGR",
                         "OGRPolygon::transform() failed for a ring other\n"
                         "than the first, meaning some rings are transformed\n"
                         "and some are not!\n" );

                return OGRERR_FAILURE;
            }

            return eErr;
        }
    }

    assignSpatialReference( poCT->GetTargetCS() );

    return OGRERR_NONE;
#endif
}


