/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Classes for manipulating simple features that is not specific
 *           to a particular interface technology.
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
 */

#ifndef _OGR_GEOMETRY_H_INCLUDED
#define _OGR_GEOMETRY_H_INCLUDED

#include "ogr_core.h"
#include "ogr_spatialref.h"

/**
 * \file ogr_geometry.h
 *
 * Simple feature geometry classes.
 */

/**
 * Simple container for a position.
 */
class OGRRawPoint
{
  public:
          OGRRawPoint()
          {
                  x = y = 0.0;
          }
    double      x;
    double      y;
};


/************************************************************************/
/*                             OGRGeometry                              */
/************************************************************************/

/**
 * Abstract base class for all geometry classes.
 *
 * Note that the family of spatial analysis methods (Equal(), Disjoint(), ...,
 * ConvexHull(), Buffer(), ...) are not implemented at ths time.  Some other
 * required and optional geometry methods have also been omitted at this
 * time.
 */

class CPL_DLL OGRGeometry
{
  private:
    OGRSpatialReference * poSRS;                // may be NULL

  public:
                OGRGeometry();
    virtual     ~OGRGeometry();

    // standard IGeometry
    virtual int getDimension() const = 0;
    virtual int getCoordinateDimension() const = 0;
    virtual OGRBoolean  IsEmpty() const { return 0; }
    virtual OGRBoolean  IsSimple() const { return 1; }
    virtual void        empty() = 0;
    virtual OGRGeometry *clone() const = 0;
    virtual void getEnvelope( OGREnvelope * psEnvelope ) const = 0;

    // IWks Interface
    virtual int WkbSize() const = 0;
    virtual OGRErr importFromWkb( unsigned char *, int=-1 )=0;
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * ) const = 0;
    virtual OGRErr importFromWkt( char ** ppszInput ) = 0;
    virtual OGRErr exportToWkt( char ** ppszDstText ) const = 0;

    // non-standard
    virtual OGRwkbGeometryType getGeometryType() const = 0;
    virtual const char *getGeometryName() const = 0;
    virtual void   dumpReadable( FILE *, const char * = NULL );
    virtual void   flattenTo2D() = 0;

    void    assignSpatialReference( OGRSpatialReference * poSR );
    OGRSpatialReference *getSpatialReference( void ) const { return poSRS; }

    virtual OGRErr  transform( OGRCoordinateTransformation *poCT ) = 0;
    OGRErr  transformTo( OGRSpatialReference *poSR );

    // ISpatialRelation
    virtual OGRBoolean  Intersect( OGRGeometry * ) const;
    virtual OGRBoolean  Equal( OGRGeometry * ) const = 0;

#ifdef notdef

    // I presume all these should be virtual?  How many
    // should be pure?
    OGRGeometry *getBoundary();

    OGRBoolean  Disjoint( OGRGeometry * );
    OGRBoolean  Touch( OGRGeometry * );
    OGRBoolean  Cross( OGRGeometry * );
    OGRBoolean  Within( OGRGeometry * );
    OGRBoolean  Contains( OGRGeometry * );
    OGRBoolean  Overlap( OGRGeometry * );
    OGRBoolean  Relate( OGRGeometry *, const char * );

    double      Distance( OGRGeometry * );
    OGRGeometry *Intersection(OGRGeometry *);
    OGRGeometry *Buffer( double );
    OGRGeometry *ConvexHull();
    OGRGeometry *Union( OGRGeometry * );
    OGRGeometry *Difference( OGRGeometry * );
    OGRGeometry *SymmetricDifference( OGRGeometry * );
#endif

    // Special HACK for DB2 7.2 support
    static int bGenerate_DB2_V72_BYTE_ORDER;
};

/************************************************************************/
/*                               OGRPoint                               */
/************************************************************************/

/**
 * Point class.
 *
 * Implements SFCOM IPoint methods.
 */

class CPL_DLL OGRPoint : public OGRGeometry
{
    double      x;
    double      y;
    double      z;
    int         nQual;              // Point Positional accuracy declaration
                                    // Obtained from geometry record attributes
                                    // Field=ATTV
                                    //    Subfields ATTL = 402
                                    //              ATVL = 1-11

  public:
                OGRPoint();
                OGRPoint( double x, double y, double z = 0.0 );
    virtual     ~OGRPoint();

    // IWks Interface
    virtual int WkbSize() const;
    virtual OGRErr importFromWkb( unsigned char *, int=-1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * ) const;
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** ppszDstText ) const;

    // IGeometry
    virtual int getDimension() const;
    virtual int getCoordinateDimension() const;
    virtual OGRGeometry *clone() const;
    virtual void empty();
    virtual void getEnvelope( OGREnvelope * psEnvelope ) const;

    // IPoint
    double      getX() const { return x; }
    double      getY() const { return y; }
    double      getZ() const { return z; }
    int         getnQual() const { return nQual; }

    // Non standard
    void        setX( double xIn ) { x = xIn; }
    void        setY( double yIn ) { y = yIn; }
    void        setZ( double zIn ) { z = zIn; }
    void        setnQual( int q ) { nQual = q; }

    // ISpatialRelation
    virtual OGRBoolean  Equal( OGRGeometry * ) const;

    // Non standard from OGRGeometry
    virtual const char *getGeometryName() const;
    virtual OGRwkbGeometryType getGeometryType() const;
    virtual OGRErr  transform( OGRCoordinateTransformation *poCT );
    virtual void flattenTo2D();

};

/************************************************************************/
/*                               OGRCurve                               */
/************************************************************************/

/**
 * Abstract curve base class.
 */

class CPL_DLL OGRCurve : public OGRGeometry
{
  public:
            OGRCurve();
    virtual ~OGRCurve();
    // ICurve methods
    virtual double get_Length() const = 0;
    virtual void StartPoint(OGRPoint *) const = 0;
    virtual void EndPoint(OGRPoint *) const = 0;
    virtual int  get_IsClosed() const;
    virtual void Value( double, OGRPoint * ) const = 0;

};

/************************************************************************/
/*                            OGRLineString                             */
/************************************************************************/

/**
 * Concrete representation of a multi-vertex line.
 */

class CPL_DLL OGRLineString : public OGRCurve
{
  protected:
    int         nPointCount;
    OGRRawPoint *paoPoints;
    double      *padfZ;

    void        Make3D();
    void        Make2D();

  public:
                OGRLineString();
    virtual     ~OGRLineString();

    // IWks Interface
    virtual int WkbSize() const;
    virtual OGRErr importFromWkb( unsigned char *, int = -1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * ) const;
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** ppszDstText ) const;

    // IGeometry interface
    virtual int getDimension() const;
    virtual int getCoordinateDimension() const;
    virtual OGRGeometry *clone() const;
    virtual void empty();
    virtual void getEnvelope( OGREnvelope * psEnvelope ) const;

    // ICurve methods
    virtual double get_Length() const;
    virtual void StartPoint(OGRPoint *) const;
    virtual void EndPoint(OGRPoint *) const;
    virtual void Value( double, OGRPoint * ) const;

    // ILineString methods
    int         getNumPoints() const { return nPointCount; }
    void        getPoint( int, OGRPoint * ) const;
    double      getX( int i ) const { return paoPoints[i].x; }
    double      getY( int i ) const { return paoPoints[i].y; }
    double      getZ( int i ) const;

    // ISpatialRelation
    virtual OGRBoolean  Equal( OGRGeometry * ) const;

    // non standard.
    void        setNumPoints( int );
    void        setPoint( int, OGRPoint * );
    void        setPoint( int, double, double, double = 0.0 );
    void        setPoints( int, OGRRawPoint *, double * = NULL );
    void        setPoints( int, double * padfX, double * padfY,
                           double *padfZ = NULL );
    void        addPoint( OGRPoint * );
    void        addPoint( double, double, double = 0.0 );

    // non-standard from OGRGeometry
    virtual OGRwkbGeometryType getGeometryType() const;
    virtual const char *getGeometryName() const;
    virtual OGRErr  transform( OGRCoordinateTransformation *poCT );
    virtual void flattenTo2D();

};

/************************************************************************/
/*                            OGRLinearRing                             */
/*                                                                      */
/*      This is an alias for OGRLineString for now.                     */
/************************************************************************/

/**
 * Concrete representation of a closed ring.
 *
 * This class is functionally equivelent to an OGRLineString, but has a
 * separate identity to maintain alignment with the OpenGIS simple feature
 * data model.  It exists to serve as a component of an OGRPolygon.
 */

class CPL_DLL OGRLinearRing : public OGRLineString
{
  private:
    friend class OGRPolygon;

    // These are not IWks compatible ... just a convenience for OGRPolygon.
    virtual int _WkbSize( int b3D ) const;
    virtual OGRErr _importFromWkb( OGRwkbByteOrder, int b3D,
                                   unsigned char *, int=-1 );
    virtual OGRErr _exportToWkb( OGRwkbByteOrder, int b3D,
                                 unsigned char * ) const;

  public:
                        OGRLinearRing();
                        OGRLinearRing( OGRLinearRing * );
                        ~OGRLinearRing();

    // Non standard.
    virtual const char *getGeometryName() const;
    virtual OGRGeometry *clone() const;
    virtual int isClockwise() const;

    // IWks Interface - Note this isnt really a first class object
    // for the purposes of WKB form.  These methods always fail since this
    // object cant be serialized on its own.
    virtual int WkbSize() const;
    virtual OGRErr importFromWkb( unsigned char *, int=-1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * ) const;
};

/************************************************************************/
/*                              OGRSurface                              */
/************************************************************************/

/**
 * Abstract base class for 2 dimensional objects like polygons.
 */

class CPL_DLL OGRSurface : public OGRGeometry
{
  public:
    virtual double      get_Area() const = 0;
    virtual OGRErr      Centroid( OGRPoint * poPoint ) const = 0;
    virtual OGRErr      PointOnSurface( OGRPoint * poPoint ) const = 0;
};

/************************************************************************/
/*                              OGRPolygon                              */
/************************************************************************/

/**
 * Concrete class representing polygons.
 *
 * Note that the OpenGIS simple features polygons consist of one outer
 * ring, and zero or more inner rings.  A polygon cannot represent disconnected
 * regions (such as multiple islands in a political body).  The
 * OGRMultiPolygon must be used for this.
 */

class CPL_DLL OGRPolygon : public OGRSurface
{
    int         nRingCount;
    OGRLinearRing **papoRings;

  public:
                OGRPolygon();
    virtual     ~OGRPolygon();

    // Non standard (OGRGeometry).
    virtual const char *getGeometryName() const;
    virtual OGRwkbGeometryType getGeometryType() const;
    virtual OGRGeometry *clone() const;
    virtual void empty();
    virtual OGRErr  transform( OGRCoordinateTransformation *poCT );
    virtual void flattenTo2D();

    // ISurface Interface
    virtual double      get_Area() const;
    virtual int         Centroid( OGRPoint * poPoint ) const;
    virtual int         PointOnSurface( OGRPoint * poPoint ) const;

    // IWks Interface
    virtual int WkbSize() const;
    virtual OGRErr importFromWkb( unsigned char *, int = -1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * ) const;
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** ppszDstText ) const;

    // IGeometry
    virtual int getDimension() const;
    virtual int getCoordinateDimension() const;
    virtual void getEnvelope( OGREnvelope * psEnvelope ) const;

    // ISpatialRelation
    virtual OGRBoolean  Equal( OGRGeometry * ) const;

    // Non standard
    void        addRing( OGRLinearRing * );
    void        addRingDirectly( OGRLinearRing * );

    OGRLinearRing *getExteriorRing();
    const OGRLinearRing *getExteriorRing() const;
    int         getNumInteriorRings() const;
    OGRLinearRing *getInteriorRing( int );
    const OGRLinearRing *getInteriorRing( int ) const;
};

/************************************************************************/
/*                        OGRGeometryCollection                         */
/************************************************************************/

/**
 * A collection of 1 or more geometry objects.
 *
 * All geometries must share a common spatial reference system, and
 * Subclasses may impose additional restrictions on the contents.
 */

class CPL_DLL OGRGeometryCollection : public OGRGeometry
{
    int         nGeomCount;
    OGRGeometry **papoGeoms;

    int         nCoordinateDimension;

  public:
                OGRGeometryCollection();
    virtual     ~OGRGeometryCollection();

    // Non standard (OGRGeometry).
    virtual const char *getGeometryName() const;
    virtual OGRwkbGeometryType getGeometryType() const;
    virtual OGRGeometry *clone() const;
    virtual void empty();
    virtual OGRErr  transform( OGRCoordinateTransformation *poCT );
    virtual void flattenTo2D();

    // IWks Interface
    virtual int WkbSize() const;
    virtual OGRErr importFromWkb( unsigned char *, int = -1 );
    virtual OGRErr exportToWkb( OGRwkbByteOrder, unsigned char * ) const;
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** ppszDstText ) const;

    // IGeometry methods
    virtual int getDimension() const;
    virtual int getCoordinateDimension() const;
    virtual void getEnvelope( OGREnvelope * psEnvelope ) const;

    // IGeometryCollection
    int         getNumGeometries() const;
    OGRGeometry *getGeometryRef( int );
    const OGRGeometry *getGeometryRef( int ) const;

    // ISpatialRelation
    virtual OGRBoolean  Equal( OGRGeometry * ) const;

    // Non standard
    virtual OGRErr addGeometry( const OGRGeometry * );
    virtual OGRErr addGeometryDirectly( OGRGeometry * );
    virtual OGRErr removeGeometry( int iIndex, int bDelete = TRUE );
};

/************************************************************************/
/*                           OGRMultiPolygon                            */
/************************************************************************/

/**
 * A collection of non-overlapping OGRPolygons.
 *
 * Note that the IMultiSurface class hasn't been modelled, nor have any
 * of it's methods.
 */

class CPL_DLL OGRMultiPolygon : public OGRGeometryCollection
{
  public:
            OGRMultiPolygon();
    // Non standard (OGRGeometry).
    virtual const char *getGeometryName() const;
    virtual OGRwkbGeometryType getGeometryType() const;
    virtual OGRGeometry *clone() const;
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** ) const;

    // Non standard
    virtual OGRErr addGeometryDirectly( OGRGeometry * );
};

/************************************************************************/
/*                            OGRMultiPoint                             */
/************************************************************************/

/**
 * A collection of OGRPoints.
 */

class CPL_DLL OGRMultiPoint : public OGRGeometryCollection
{
  private:
    OGRErr  importFromWkt_Bracketed( char ** );

  public:
            OGRMultiPoint();
    // Non standard (OGRGeometry).
    virtual const char *getGeometryName() const;
    virtual OGRwkbGeometryType getGeometryType() const;
    virtual OGRGeometry *clone() const;
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** ) const;

    // Non standard
    virtual OGRErr addGeometryDirectly( OGRGeometry * );
};

/************************************************************************/
/*                          OGRMultiLineString                          */
/************************************************************************/

/**
 * A collection of OGRLineStrings.
 */

class CPL_DLL OGRMultiLineString : public OGRGeometryCollection
{
  public:
            OGRMultiLineString();
            ~OGRMultiLineString();
    // Non standard (OGRGeometry).
    virtual const char *getGeometryName() const;
    virtual OGRwkbGeometryType getGeometryType() const;
    virtual OGRGeometry *clone() const;
    virtual OGRErr importFromWkt( char ** );
    virtual OGRErr exportToWkt( char ** ) const;

    // Non standard
    virtual OGRErr addGeometryDirectly( OGRGeometry * );
};


/************************************************************************/
/*                          OGRGeometryFactory                          */
/************************************************************************/

/**
 * Create geometry objects from well known text/binary.
 */

class CPL_DLL OGRGeometryFactory
{
  public:
    static OGRErr createFromWkb( unsigned char *, OGRSpatialReference *,
                                 OGRGeometry **, int = -1 );
    static OGRErr createFromWkt( char **, OGRSpatialReference *,
                                 OGRGeometry ** );

    static void   destroyGeometry( OGRGeometry * );
    static OGRGeometry *createGeometry( OGRwkbGeometryType );

    static OGRGeometry * forceToPolygon( OGRGeometry * );
    static OGRGeometry * forceToMultiPolygon( OGRGeometry * );
    static OGRGeometry * forceToMultiPoint( OGRGeometry * );
    static OGRGeometry * forceToMultiLineString( OGRGeometry * );
};

#endif /* ndef _OGR_GEOMETRY_H_INCLUDED */
