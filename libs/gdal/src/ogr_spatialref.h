/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Classes for manipulating spatial reference systems in a
 *           platform non-specific manner.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
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
 * $Log: ogr_spatialref.h,v $
 * Cleanup
 *
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.60  2004/03/04 18:04:45  warmerda
 * added importFromDict() support
 *
 * Revision 1.59  2004/02/07 17:31:21  dron
 * Added OSRExportToUSGS() method.
 *
 * Revision 1.58  2004/02/05 17:07:59  dron
 * Support for HOM projection, specified by two points on centerline.
 *
 * Revision 1.57  2004/02/01 14:24:09  dron
 * Added OGRSpatialReference::importFromUSGS().
 *
 * Revision 1.56  2004/01/24 09:34:59  warmerda
 * added TransformEx support to capture per point reprojection failure
 *
 * Revision 1.55  2003/10/07 04:20:50  warmerda
 * added WMS AUTO: support
 *
 * Revision 1.54  2003/09/09 07:49:19  dron
 * Added exportToPCI() method.
 *
 * Revision 1.53  2003/08/31 14:51:30  dron
 * Added importFromPCI() method.
 *
 * Revision 1.52  2003/08/18 13:26:01  warmerda
 * added SetTMVariant() and related definitions
 *
 * Revision 1.51  2003/05/30 15:39:53  warmerda
 * Added override units capability for SetStatePlane()
 *
 * Revision 1.50  2003/05/28 19:16:42  warmerda
 * fixed up argument names and stuff for docs
 *
 * Revision 1.49  2003/03/12 14:25:01  warmerda
 * added NeedsQuoting() method
 *
 * Revision 1.48  2003/02/25 04:53:51  warmerda
 * added CopyGeogCSFrom() method
 *
 * Revision 1.47  2003/02/06 04:53:12  warmerda
 * added Fixup() method
 *
 * Revision 1.46  2003/01/08 18:14:28  warmerda
 * added FixupOrdering()
 */

#ifndef _OGR_SPATIALREF_H_INCLUDED
#define _OGR_SPATIALREF_H_INCLUDED

//#include "ogr_srs_api.h"

/**
 * \file ogr_spatialref.h
 *
 * Coordinate systems services.
 */

/************************************************************************/
/*                             OGR_SRSNode                              */
/************************************************************************/

/**
 * Objects of this class are used to represent value nodes in the parsed
 * representation of the WKT SRS format.  For instance UNIT["METER",1]
 * would be rendered into three OGR_SRSNodes.  The root node would have a
 * value of UNIT, and two children, the first with a value of METER, and the
 * second with a value of 1.
 *
 * Normally application code just interacts with the OGRSpatialReference
 * object, which uses the OGR_SRSNode to implement it's data structure;
 * however, this class is user accessable for detailed access to components
 * of an SRS definition.
 */

class CPL_DLL OGR_SRSNode
{
    char        *pszValue;

    int         nChildren;
    OGR_SRSNode **papoChildNodes;

    OGR_SRSNode *poParent;

    void        ClearChildren();
    int         NeedsQuoting() const;

  public:
                OGR_SRSNode(const char * = NULL);
                ~OGR_SRSNode();

    int         IsLeafNode() const { return nChildren == 0; }

    int         GetChildCount() const { return nChildren; }
    OGR_SRSNode *GetChild( int );
    const OGR_SRSNode *GetChild( int ) const;

    OGR_SRSNode *GetNode( const char * );
    const OGR_SRSNode *GetNode( const char * ) const;

    void        InsertChild( OGR_SRSNode *, int );
    void        AddChild( OGR_SRSNode * );
    int         FindChild( const char * ) const;
    void        DestroyChild( int );
    void        StripNodes( const char * );

    const char  *GetValue() const { return pszValue; }
    void        SetValue( const char * );

    void        MakeValueSafe();
    OGRErr      FixupOrdering();

    OGR_SRSNode *Clone() const;

    OGRErr      importFromWkt( char ** );
    OGRErr      exportToWkt( char ** ) const;
    OGRErr      exportToPrettyWkt( char **, int = 1) const;

    OGRErr      applyRemapper( const char *pszNode,
                               char **papszSrcValues,
                               char **papszDstValues,
                               int nStepSize = 1,
                               int bChildOfHit = FALSE );
};

/************************************************************************/
/*                         OGRSpatialReference                          */
/************************************************************************/

/**
 * This class respresents a OpenGIS Spatial Reference System, and contains
 * methods for converting between this object organization and well known
 * text (WKT) format.  This object is reference counted as one instance of
 * the object is normally shared between many OGRGeometry objects.
 *
 * Normally application code can fetch needed parameter values for this
 * SRS using GetAttrValue(), but in special cases the underlying parse tree
 * (or OGR_SRSNode objects) can be accessed more directly.
 *
 * See <a href="osr_tutorial.html">the tutorial</a> for more information on
 * how to use this class.
 */

class CPL_DLL OGRSpatialReference
{
    int         nRefCount;

    OGR_SRSNode *poRoot;

    int         bNormInfoSet;
    double      dfFromGreenwich;
    double      dfToMeter;
    double      dfToDegrees;

    OGRErr      ValidateProjection();
    int         IsAliasFor( const char *, const char * );
    void        GetNormInfo() const;

  public:
                OGRSpatialReference(const OGRSpatialReference&);
                OGRSpatialReference(const char * = NULL);

    virtual    ~OGRSpatialReference();

    OGRSpatialReference &operator=(const OGRSpatialReference&);

    int         Reference();
    int         Dereference();
    int         GetReferenceCount() const { return nRefCount; }

    OGRSpatialReference *Clone() const;
    OGRSpatialReference *CloneGeogCS() const;

    OGRErr      exportToWkt( char ** );
    OGRErr      exportToPrettyWkt( char **, int = FALSE) const;
    OGRErr      exportToProj4( char ** ) const;
    OGRErr      exportToPCI( char **, char **, double ** ) const;
    OGRErr      exportToUSGS( long *, long *, double **, long * ) const;
    OGRErr      exportToXML( char **, const char * = NULL ) const;
    OGRErr      importFromWkt( char ** );
    OGRErr      importFromProj4( const char * );
    OGRErr      importFromEPSG( int );
    OGRErr      importFromESRI( char ** );
    OGRErr      importFromPCI( const char *pszProj,
                               const char *pszUnits = NULL,
                               double *padfPrjParams = NULL );
    OGRErr      importFromUSGS( long iProjsys, long iZone,
                                double *padfPrjParams, long iDatum );
    OGRErr      importFromWMSAUTO( const char *pszAutoDef );
    OGRErr      importFromXML( const char * );
    OGRErr      importFromDict( const char *pszDict, const char *pszCode );

    OGRErr      morphToESRI();
    OGRErr      morphFromESRI();

    OGRErr      Validate();
    OGRErr      StripCTParms( OGR_SRSNode * = NULL );
    OGRErr      FixupOrdering();
    OGRErr      Fixup();

    // Machinary for accessing parse nodes
    OGR_SRSNode *GetRoot() { return poRoot; }
    const OGR_SRSNode *GetRoot() const { return poRoot; }
    void        SetRoot( OGR_SRSNode * );

    OGR_SRSNode *GetAttrNode(const char *);
    const OGR_SRSNode *GetAttrNode(const char *) const;
    const char  *GetAttrValue(const char *, int = 0) const;

    OGRErr      SetNode( const char *, const char * );
    OGRErr      SetNode( const char *, double );

    OGRErr      SetLinearUnits( const char *pszName, double dfInMeters );
    double      GetLinearUnits( char ** = NULL ) const;

    OGRErr      SetAngularUnits( const char *pszName, double dfInRadians );
    double      GetAngularUnits( char ** = NULL ) const;

    double      GetPrimeMeridian( char ** = NULL ) const;

    int         IsGeographic() const;
    int         IsProjected() const;
    int         IsLocal() const;
    int         IsSameGeogCS( const OGRSpatialReference * ) const;
    int         IsSame( const OGRSpatialReference * ) const;

    void        Clear();
    OGRErr      SetLocalCS( const char * );
    OGRErr      SetProjCS( const char * );
    OGRErr      SetProjection( const char * );
    OGRErr      SetGeogCS( const char * pszGeogName,
                           const char * pszDatumName,
                           const char * pszEllipsoidName,
                           double dfSemiMajor, double dfInvFlattening,
                           const char * pszPMName = NULL,
                           double dfPMOffset = 0.0,
                           const char * pszUnits = NULL,
                           double dfConvertToRadians = 0.0 );
    OGRErr      SetWellKnownGeogCS( const char * );
    OGRErr      CopyGeogCSFrom( const OGRSpatialReference * poSrcSRS );

    OGRErr      SetFromUserInput( const char * );

    OGRErr      SetTOWGS84( double, double, double,
                            double = 0.0, double = 0.0, double = 0.0,
                            double = 0.0 );
    OGRErr      GetTOWGS84( double *padfCoef, int nCoeff = 7 ) const;

    double      GetSemiMajor( OGRErr * = NULL ) const;
    double      GetSemiMinor( OGRErr * = NULL ) const;
    double      GetInvFlattening( OGRErr * = NULL ) const;

    OGRErr      SetAuthority( const char * pszTargetKey,
                              const char * pszAuthority,
                              int nCode );

    const char *GetAuthorityCode( const char * pszTargetKey ) const;
    const char *GetAuthorityName( const char * pszTargetKey ) const;

    OGRErr      SetProjParm( const char *, double );
    double      GetProjParm( const char *, double =0.0, OGRErr* = NULL ) const;

    OGRErr      SetNormProjParm( const char *, double );
    double      GetNormProjParm( const char *, double=0.0, OGRErr* =NULL)const;

    static int  IsAngularParameter( const char * );
    static int  IsLongitudeParameter( const char * );
    static int  IsLinearParameter( const char * );

    /** Albers Conic Equal Area */
    OGRErr      SetACEA( double dfStdP1, double dfStdP2,
                         double dfCenterLat, double dfCenterLong,
                         double dfFalseEasting, double dfFalseNorthing );

    /** Azimuthal Equidistant */
    OGRErr      SetAE( double dfCenterLat, double dfCenterLong,
                       double dfFalseEasting, double dfFalseNorthing );

    /** Cylindrical Equal Area */
    OGRErr      SetCEA( double dfStdP1, double dfCentralMeridian,
                        double dfFalseEasting, double dfFalseNorthing );

    /** Cassini-Soldner */
    OGRErr      SetCS( double dfCenterLat, double dfCenterLong,
                       double dfFalseEasting, double dfFalseNorthing );

    /** Equidistant Conic */
    OGRErr      SetEC( double dfStdP1, double dfStdP2,
                       double dfCenterLat, double dfCenterLong,
                       double dfFalseEasting, double dfFalseNorthing );

    /** Eckert IV */
    OGRErr      SetEckertIV( double dfCentralMeridian,
                             double dfFalseEasting, double dfFalseNorthing );

    /** Eckert VI */
    OGRErr      SetEckertVI( double dfCentralMeridian,
                             double dfFalseEasting, double dfFalseNorthing );

    /** Equirectangular */
    OGRErr      SetEquirectangular(double dfCenterLat, double dfCenterLong,
                            double dfFalseEasting, double dfFalseNorthing );

    /** Gall Stereograpic */
    OGRErr      SetGS( double dfCentralMeridian,
                       double dfFalseEasting, double dfFalseNorthing );

    /** Gnomonic */
    OGRErr      SetGnomonic(double dfCenterLat, double dfCenterLong,
                            double dfFalseEasting, double dfFalseNorthing );

    OGRErr      SetHOM( double dfCenterLat, double dfCenterLong,
                        double dfAzimuth, double dfRectToSkew,
                        double dfScale,
                        double dfFalseEasting, double dfFalseNorthing );

    OGRErr      SetHOM2PNO( double dfCenterLat,
                            double dfLat1, double dfLong1,
                            double dfLat2, double dfLong2,
                            double dfScale,
                            double dfFalseEasting, double dfFalseNorthing );

    /** Krovak Oblique Conic Conformal */
    OGRErr      SetKrovak( double dfCenterLat, double dfCenterLong,
                           double dfAzimuth, double dfPseudoStdParallelLat,
                           double dfScale,
                           double dfFalseEasting, double dfFalseNorthing );

    /** Lambert Azimuthal Equal-Area */
    OGRErr      SetLAEA( double dfCenterLat, double dfCenterLong,
                         double dfFalseEasting, double dfFalseNorthing );

    /** Lambert Conformal Conic */
    OGRErr      SetLCC( double dfStdP1, double dfStdP2,
                        double dfCenterLat, double dfCenterLong,
                        double dfFalseEasting, double dfFalseNorthing );

    /** Lambert Conformal Conic 1SP */
    OGRErr      SetLCC1SP( double dfCenterLat, double dfCenterLong,
                           double dfScale,
                           double dfFalseEasting, double dfFalseNorthing );

    /** Lambert Conformal Conic (Belgium) */
    OGRErr      SetLCCB( double dfStdP1, double dfStdP2,
                         double dfCenterLat, double dfCenterLong,
                         double dfFalseEasting, double dfFalseNorthing );

    /** Miller Cylindrical */
    OGRErr      SetMC( double dfCenterLat, double dfCenterLong,
                       double dfFalseEasting, double dfFalseNorthing );

    /** Mercator */
    OGRErr      SetMercator( double dfCenterLat, double dfCenterLong,
                             double dfScale,
                             double dfFalseEasting, double dfFalseNorthing );

    /** Mollweide */
    OGRErr      SetMollweide( double dfCentralMeridian,
                              double dfFalseEasting, double dfFalseNorthing );

    /** New Zealand Map Grid */
    OGRErr      SetNZMG( double dfCenterLat, double dfCenterLong,
                         double dfFalseEasting, double dfFalseNorthing );

    /** Oblique Stereographic */
    OGRErr      SetOS( double dfOriginLat, double dfCMeridian,
                       double dfScale,
                       double dfFalseEasting,double dfFalseNorthing);

    /** Orthographic */
    OGRErr      SetOrthographic( double dfCenterLat, double dfCenterLong,
                                 double dfFalseEasting,double dfFalseNorthing);

    /** Polyconic */
    OGRErr      SetPolyconic( double dfCenterLat, double dfCenterLong,
                              double dfFalseEasting, double dfFalseNorthing );

    /** Polar Stereographic */
    OGRErr      SetPS( double dfCenterLat, double dfCenterLong,
                       double dfScale,
                       double dfFalseEasting, double dfFalseNorthing);

    /** Robinson */
    OGRErr      SetRobinson( double dfCenterLong,
                             double dfFalseEasting, double dfFalseNorthing );

    /** Sinusoidal */
    OGRErr      SetSinusoidal( double dfCenterLong,
                               double dfFalseEasting, double dfFalseNorthing );

    /** Stereographic */
    OGRErr      SetStereographic( double dfCenterLat, double dfCenterLong,
                                  double dfScale,
                                 double dfFalseEasting,double dfFalseNorthing);

    /** Swiss Oblique Cylindrical */
    OGRErr      SetSOC( double dfLatitudeOfOrigin, double dfCentralMeridian,
                        double dfFalseEasting, double dfFalseNorthing );

    /** Transverse Mercator */
    OGRErr      SetTM( double dfCenterLat, double dfCenterLong,
                       double dfScale,
                       double dfFalseEasting, double dfFalseNorthing );

    /** Transverse Mercator variants. */
    OGRErr      SetTMVariant( const char *pszVariantName,
                              double dfCenterLat, double dfCenterLong,
                              double dfScale,
                              double dfFalseEasting, double dfFalseNorthing );

    /** Tunesia Mining Grid  */
    OGRErr      SetTMG( double dfCenterLat, double dfCenterLong,
                        double dfFalseEasting, double dfFalseNorthing );

    /** Transverse Mercator (South Oriented) */
    OGRErr      SetTMSO( double dfCenterLat, double dfCenterLong,
                         double dfScale,
                         double dfFalseEasting, double dfFalseNorthing );

    /** VanDerGrinten */
    OGRErr      SetVDG( double dfCenterLong,
                        double dfFalseEasting, double dfFalseNorthing );

    /** Universal Transverse Mercator */
    OGRErr      SetUTM( int nZone, int bNorth = TRUE );
    int         GetUTMZone( int *pbNorth = NULL ) const;

    /** State Plane */
    OGRErr      SetStatePlane( int nZone, int bNAD83 = TRUE,
                               const char *pszOverrideUnitName = NULL,
                               double dfOverrideUnit = 0.0 );
};

/************************************************************************/
/*                     OGRCoordinateTransformation                      */
/*                                                                      */
/*      This is really just used as a base class for a private          */
/*      implementation.                                                 */
/************************************************************************/

/**
 * Object for transforming between coordinate systems.
 *
 * Also, see OGRCreateSpatialReference() for creating transformations.
 */

class CPL_DLL OGRCoordinateTransformation
{
public:
    virtual ~OGRCoordinateTransformation() {}

    // From CT_CoordinateTransformation

    /** Fetch internal source coordinate system. */
    virtual OGRSpatialReference *GetSourceCS() = 0;

    /** Fetch internal target coordinate system. */
    virtual OGRSpatialReference *GetTargetCS() = 0;

    // From CT_MathTransform

    /**
     * Transform points from source to destination space.
     *
     * This method is the same as the C function OCTTransform().
     *
     * The method TransformEx() allows extended success information to
     * be captured indicating which points failed to transform.
     *
     * @param nCount number of points to transform.
     * @param x array of nCount X vertices, modified in place.
     * @param y array of nCount Y vertices, modified in place.
     * @param z array of nCount Z vertices, modified in place.
     * @return TRUE on success, or FALSE if some or all points fail to
     * transform.
     */
    virtual int Transform( int nCount,
                           double *x, double *y, double *z = NULL ) = 0;

    /**
     * Transform points from source to destination space.
     *
     * This method is the same as the C function OCTTransformEx().
     *
     * @param nCount number of points to transform.
     * @param x array of nCount X vertices, modified in place.
     * @param y array of nCount Y vertices, modified in place.
     * @param z array of nCount Z vertices, modified in place.
     * @param pabSuccess array of per-point flags set to TRUE if that point
     * transforms, or FALSE if it does not.
     *
     * @return TRUE if some or all points transform successfully, or FALSE if
     * if none transform.
     */
    virtual int TransformEx( int nCount,
                             double *x, double *y, double *z = NULL,
                             int *pabSuccess = NULL ) = 0;

};

OGRCoordinateTransformation CPL_DLL *
OGRCreateCoordinateTransformation( OGRSpatialReference *poSource,
                                   OGRSpatialReference *poTarget );

#endif /* ndef _OGR_SPATIALREF_H_INCLUDED */
