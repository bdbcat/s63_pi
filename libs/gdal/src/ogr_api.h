/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  C API for OGR Geometry, Feature, Layers, DataSource and drivers.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: ogr_api.h,v $
 * Cleanup
 *
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.17  2003/10/09 15:27:41  warmerda
 * added OGRLayer::DeleteFeature() support
 *
 * Revision 1.16  2003/09/04 14:01:44  warmerda
 * added OGRGetGenerate_DB2_V72_BYTE_ORDER
 *
 * Revision 1.15  2003/08/27 15:40:37  warmerda
 * added support for generating DB2 V7.2 compatible WKB
 *
 * Revision 1.14  2003/04/22 19:33:26  warmerda
 * Added synctodisk
 *
 * Revision 1.13  2003/04/08 21:21:13  warmerda
 * added OGRGetDriverByName
 *
 * Revision 1.12  2003/04/08 19:30:56  warmerda
 * added CopyLayer and CopyDataSource entry points
 *
 * Revision 1.11  2003/03/19 20:28:20  warmerda
 * added shared access, and reference counting apis
 *
 * Revision 1.10  2003/03/12 20:52:07  warmerda
 * implemented support for gml:Box
 *
 * Revision 1.9  2003/03/06 20:29:27  warmerda
 * added GML import/export entry points
 *
 * Revision 1.8  2003/03/05 05:08:49  warmerda
 * added GetLayerByName
 *
 * Revision 1.7  2003/03/03 05:05:54  warmerda
 * added support for DeleteDataSource and DeleteLayer
 *
 * Revision 1.6  2003/01/07 16:44:27  warmerda
 * added removeGeometry
 *
 * Revision 1.5  2003/01/06 21:37:00  warmerda
 * added CPL_DLL attribute on OGRBuildPolygon...
 *
 * Revision 1.4  2003/01/02 21:45:23  warmerda
 * move OGRBuildPolygonsFromEdges into C API
 *
 * Revision 1.3  2002/10/24 16:46:08  warmerda
 * removed bogus OGR_G_GetWkbSize()
 *
 * Revision 1.2  2002/09/26 19:00:07  warmerda
 * ensure all entry points CPL_DLL'ed
 *
 * Revision 1.1  2002/09/26 18:11:51  warmerda
 * New
 *
 */

#ifndef _OGR_API_H_INCLUDED
#define _OGR_API_H_INCLUDED

/**
 * \file ogr_api.h
 *
 * C API and defines for OGRFeature, OGRGeometry, and OGRDataSource
 * related classes.
 *
 * See also: ogr_geometry.h, ogr_feature.h, ogrsf_frmts.h
 */

#include "ogr_core.h"

CPL_C_START

/* -------------------------------------------------------------------- */
/*      Geometry related functions (ogr_geometry.h)                     */
/* -------------------------------------------------------------------- */
typedef void *OGRGeometryH;

#ifndef _DEFINED_OGRSpatialReferenceH
#define _DEFINED_OGRSpatialReferenceH

typedef void *OGRSpatialReferenceH;
typedef void *OGRCoordinateTransformationH;

#endif

struct _CPLXMLNode;

/* From base OGRGeometry class */

OGRErr CPL_DLL OGR_G_CreateFromWkb( unsigned char *, OGRSpatialReferenceH,
                                    OGRGeometryH * );
OGRErr CPL_DLL OGR_G_CreateFromWkt( char **, OGRSpatialReferenceH,
                                    OGRGeometryH * );
void   CPL_DLL OGR_G_DestroyGeometry( OGRGeometryH );
OGRGeometryH CPL_DLL OGR_G_CreateGeometry( OGRwkbGeometryType );

int    CPL_DLL OGR_G_GetDimension( OGRGeometryH );
int    CPL_DLL OGR_G_GetCoordinateDimension( OGRGeometryH );
OGRGeometryH CPL_DLL OGR_G_Clone( OGRGeometryH );
void   CPL_DLL OGR_G_GetEnvelope( OGRGeometryH, OGREnvelope * );
OGRErr CPL_DLL OGR_G_ImportFromWkb( OGRGeometryH, unsigned char *, int );
OGRErr CPL_DLL OGR_G_ExportToWkb( OGRGeometryH, OGRwkbByteOrder, unsigned char*);
int    CPL_DLL OGR_G_WkbSize( OGRGeometryH hGeom );
OGRErr CPL_DLL OGR_G_ImportFromWkt( OGRGeometryH, char ** );
OGRErr CPL_DLL OGR_G_ExportToWkt( OGRGeometryH, char ** );
OGRwkbGeometryType CPL_DLL OGR_G_GetGeometryType( OGRGeometryH );
const char CPL_DLL *OGR_G_GetGeometryName( OGRGeometryH );
void   CPL_DLL OGR_G_DumpReadable( OGRGeometryH, FILE *, const char * );
void   CPL_DLL OGR_G_FlattenTo2D( OGRGeometryH );

#if defined(_CPL_MINIXML_H_INCLUDED)
OGRGeometryH CPL_DLL OGR_G_CreateFromGMLTree( const CPLXMLNode * );
CPLXMLNode CPL_DLL *OGR_G_ExportToGMLTree( OGRGeometryH );
CPLXMLNode CPL_DLL *OGR_G_ExportEnvelopeToGMLTree( OGRGeometryH );
#endif

void   CPL_DLL OGR_G_AssignSpatialReference( OGRGeometryH,
                                             OGRSpatialReferenceH );
OGRSpatialReferenceH CPL_DLL OGR_G_GetSpatialReference( OGRGeometryH );
OGRErr CPL_DLL OGR_G_Transform( OGRGeometryH, OGRCoordinateTransformationH );
OGRErr CPL_DLL OGR_G_TransformTo( OGRGeometryH, OGRSpatialReferenceH );

int    CPL_DLL OGR_G_Intersect( OGRGeometryH, OGRGeometryH );
int    CPL_DLL OGR_G_Equal( OGRGeometryH, OGRGeometryH );
void   CPL_DLL OGR_G_Empty( OGRGeometryH );

/* Methods for getting/setting vertices in points, line strings and rings */
int    CPL_DLL OGR_G_GetPointCount( OGRGeometryH );
double CPL_DLL OGR_G_GetX( OGRGeometryH, int );
double CPL_DLL OGR_G_GetY( OGRGeometryH, int );
double CPL_DLL OGR_G_GetZ( OGRGeometryH, int );
void   CPL_DLL OGR_G_GetPoint( OGRGeometryH, int iPoint,
                               double *, double *, double * );
void   CPL_DLL OGR_G_SetPoint( OGRGeometryH, int iPoint,
                               double, double, double );
void   CPL_DLL OGR_G_AddPoint( OGRGeometryH, double, double, double );

/* Methods for getting/setting rings and members collections */

int    CPL_DLL OGR_G_GetGeometryCount( OGRGeometryH );
OGRGeometryH CPL_DLL OGR_G_GetGeometryRef( OGRGeometryH, int );
OGRErr CPL_DLL OGR_G_AddGeometry( OGRGeometryH, OGRGeometryH );
OGRErr CPL_DLL OGR_G_AddGeometryDirectly( OGRGeometryH, OGRGeometryH );
OGRErr CPL_DLL OGR_G_RemoveGeometry( OGRGeometryH, int, int );

OGRGeometryH CPL_DLL OGRBuildPolygonFromEdges( OGRGeometryH hLinesAsCollection,
                                       int bBestEffort,
                                       int bAutoClose,
                                       double dfTolerance,
                                       OGRErr * peErr );

OGRErr CPL_DLL OGRSetGenerate_DB2_V72_BYTE_ORDER(
    int bGenerate_DB2_V72_BYTE_ORDER );

int CPL_DLL OGRGetGenerate_DB2_V72_BYTE_ORDER();

/* -------------------------------------------------------------------- */
/*      Feature related (ogr_feature.h)                                 */
/* -------------------------------------------------------------------- */

typedef void *OGRFieldDefnH;
typedef void *OGRFeatureDefnH;
typedef void *OGRFeatureH;

/* OGRFieldDefn */

OGRFieldDefnH CPL_DLL OGR_Fld_Create( const char *, OGRFieldType );
void   CPL_DLL OGR_Fld_Destroy( OGRFieldDefnH );

void   CPL_DLL OGR_Fld_SetName( OGRFieldDefnH, const char * );
const char CPL_DLL *OGR_Fld_GetNameRef( OGRFieldDefnH );
OGRFieldType CPL_DLL OGR_Fld_GetType( OGRFieldDefnH );
void   CPL_DLL OGR_Fld_SetType( OGRFieldDefnH, OGRFieldType );
OGRJustification CPL_DLL OGR_Fld_GetJustify( OGRFieldDefnH );
void   CPL_DLL OGR_Fld_SetJustify( OGRFieldDefnH, OGRJustification );
int    CPL_DLL OGR_Fld_GetWidth( OGRFieldDefnH );
void   CPL_DLL OGR_Fld_SetWidth( OGRFieldDefnH, int );
int    CPL_DLL OGR_Fld_GetPrecision( OGRFieldDefnH );
void   CPL_DLL OGR_Fld_SetPrecision( OGRFieldDefnH, int );
void   CPL_DLL OGR_Fld_Set( OGRFieldDefnH, const char *, OGRFieldType,
                            int, int, OGRJustification );

const char CPL_DLL *OGR_GetFieldTypeName( OGRFieldType );

/* OGRFeatureDefn */

OGRFeatureDefnH CPL_DLL OGR_FD_Create( const char * );
void   CPL_DLL OGR_FD_Destroy( OGRFeatureDefnH );
const char CPL_DLL *OGR_FD_GetName( OGRFeatureDefnH );
int    CPL_DLL OGR_FD_GetFieldCount( OGRFeatureDefnH );
OGRFieldDefnH CPL_DLL OGR_FD_GetFieldDefn( OGRFeatureDefnH, int );
int    CPL_DLL OGR_FD_GetFieldIndex( OGRFeatureDefnH, const char * );
void   CPL_DLL OGR_FD_AddFieldDefn( OGRFeatureDefnH, OGRFieldDefnH );
OGRwkbGeometryType CPL_DLL OGR_FD_GetGeomType( OGRFeatureDefnH );
void   CPL_DLL OGR_FD_SetGeomType( OGRFeatureDefnH, OGRwkbGeometryType );
int    CPL_DLL OGR_FD_Reference( OGRFeatureDefnH );
int    CPL_DLL OGR_FD_Dereference( OGRFeatureDefnH );
int    CPL_DLL OGR_FD_GetReferenceCount( OGRFeatureDefnH );

/* OGRFeature */

OGRFeatureH CPL_DLL OGR_F_Create( OGRFeatureDefnH );
void   CPL_DLL OGR_F_Destroy( OGRFeatureH );
OGRFeatureDefnH CPL_DLL OGR_F_GetDefnRef( OGRFeatureH );

OGRErr CPL_DLL OGR_F_SetGeometryDirectly( OGRFeatureH, OGRGeometryH );
OGRErr CPL_DLL OGR_F_SetGeometry( OGRFeatureH, OGRGeometryH );
OGRGeometryH CPL_DLL OGR_F_GetGeometryRef( OGRFeatureH );
OGRFeatureH CPL_DLL OGR_F_Clone( OGRFeatureH );
int    CPL_DLL OGR_F_Equal( OGRFeatureH, OGRFeatureH );

int    CPL_DLL OGR_F_GetFieldCount( OGRFeatureH );
OGRFieldDefnH CPL_DLL OGR_F_GetFieldDefnRef( OGRFeatureH, int );
int    CPL_DLL OGR_F_GetFieldIndex( OGRFeatureH, const char * );

int    CPL_DLL OGR_F_IsFieldSet( OGRFeatureH, int );
void   CPL_DLL OGR_F_UnsetField( OGRFeatureH, int );
OGRField CPL_DLL *OGR_F_GetRawFieldRef( OGRFeatureH, int );

int    CPL_DLL OGR_F_GetFieldAsInteger( OGRFeatureH, int );
double CPL_DLL OGR_F_GetFieldAsDouble( OGRFeatureH, int );
const char CPL_DLL *OGR_F_GetFieldAsString( OGRFeatureH, int );
const int CPL_DLL *OGR_F_GetFieldAsIntegerList( OGRFeatureH, int, int * );
const double CPL_DLL *OGR_F_GetFieldAsDoubleList( OGRFeatureH, int, int * );
char  CPL_DLL **OGR_F_GetFieldAsStringList( OGRFeatureH, int );

void   CPL_DLL OGR_F_SetFieldInteger( OGRFeatureH, int, int );
void   CPL_DLL OGR_F_SetFieldDouble( OGRFeatureH, int, double );
void   CPL_DLL OGR_F_SetFieldString( OGRFeatureH, int, const char * );
void   CPL_DLL OGR_F_SetFieldIntegerList( OGRFeatureH, int, int, int * );
void   CPL_DLL OGR_F_SetFieldDoubleList( OGRFeatureH, int, int, double * );
void   CPL_DLL OGR_F_SetFieldStringList( OGRFeatureH, int, char ** );
void   CPL_DLL OGR_F_SetFieldRaw( OGRFeatureH, int, OGRField * );

long   CPL_DLL OGR_F_GetFID( OGRFeatureH );
OGRErr CPL_DLL OGR_F_SetFID( OGRFeatureH, long );
void   CPL_DLL OGR_F_DumpReadable( OGRFeatureH, FILE * );
OGRErr CPL_DLL OGR_F_SetFrom( OGRFeatureH, OGRFeatureH, int );

const char CPL_DLL *OGR_F_GetStyleString( OGRFeatureH );
void   CPL_DLL OGR_F_SetStyleString( OGRFeatureH, const char * );

/* -------------------------------------------------------------------- */
/*      ogrsf_frmts.h                                                   */
/* -------------------------------------------------------------------- */

typedef void *OGRLayerH;
typedef void *OGRDataSourceH;
typedef void *OGRSFDriverH;

/* OGRLayer */

OGRGeometryH CPL_DLL OGR_L_GetSpatialFilter( OGRLayerH );
void   CPL_DLL OGR_L_SetSpatialFilter( OGRLayerH, OGRGeometryH );
OGRErr CPL_DLL OGR_L_SetAttributeFilter( OGRLayerH, const char * );
void   CPL_DLL OGR_L_ResetReading( OGRLayerH );
OGRFeatureH CPL_DLL OGR_L_GetNextFeature( OGRLayerH );
OGRFeatureH CPL_DLL OGR_L_GetFeature( OGRLayerH, long );
OGRErr CPL_DLL OGR_L_SetFeature( OGRLayerH, OGRFeatureH );
OGRErr CPL_DLL OGR_L_CreateFeature( OGRLayerH, OGRFeatureH );
OGRErr CPL_DLL OGR_L_DeleteFeature( OGRLayerH, long );
OGRFeatureDefnH CPL_DLL OGR_L_GetLayerDefn( OGRLayerH );
OGRSpatialReferenceH CPL_DLL OGR_L_GetSpatialRef( OGRLayerH );
int    CPL_DLL OGR_L_GetFeatureCount( OGRLayerH, int );
OGRErr CPL_DLL OGR_L_GetExtent( OGRLayerH, OGREnvelope *, int );
int    CPL_DLL OGR_L_TestCapability( OGRLayerH, const char * );
OGRErr CPL_DLL OGR_L_CreateField( OGRLayerH, OGRFieldDefnH, int );
OGRErr CPL_DLL OGR_L_StartTransaction( OGRLayerH );
OGRErr CPL_DLL OGR_L_CommitTransaction( OGRLayerH );
OGRErr CPL_DLL OGR_L_RollbackTransaction( OGRLayerH );
int    CPL_DLL OGR_L_Reference( OGRLayerH );
int    CPL_DLL OGR_L_Dereference( OGRLayerH );
int    CPL_DLL OGR_L_GetRefCount( OGRLayerH );
OGRErr CPL_DLL OGR_L_SyncToDisk( OGRLayerH );

/* OGRDataSource */

void   CPL_DLL OGR_DS_Destroy( OGRDataSourceH );
const char CPL_DLL *OGR_DS_GetName( OGRDataSourceH );
int    CPL_DLL OGR_DS_GetLayerCount( OGRDataSourceH );
OGRLayerH CPL_DLL OGR_DS_GetLayer( OGRDataSourceH, int );
OGRLayerH CPL_DLL OGR_DS_GetLayerByName( OGRDataSourceH, const char * );
OGRErr    CPL_DLL OGR_DS_DeleteLayer( OGRDataSourceH, int );
OGRLayerH CPL_DLL OGR_DS_CreateLayer( OGRDataSourceH, const char *,
                                      OGRSpatialReferenceH, OGRwkbGeometryType,
                                      char ** );
OGRLayerH CPL_DLL OGR_DS_CopyLayer( OGRDataSourceH, OGRLayerH, const char *,
                                    char ** );
int    CPL_DLL OGR_DS_TestCapability( OGRDataSourceH, const char * );
OGRLayerH CPL_DLL OGR_DS_ExecuteSQL( OGRDataSourceH, const char *,
                                     OGRGeometryH, const char * );
void   CPL_DLL OGR_DS_ReleaseResultSet( OGRDataSourceH, OGRLayerH );
int    CPL_DLL OGR_DS_Reference( OGRDataSourceH );
int    CPL_DLL OGR_DS_Dereference( OGRDataSourceH );
int    CPL_DLL OGR_DS_GetRefCount( OGRDataSourceH );
int    CPL_DLL OGR_DS_GetSummaryRefCount( OGRDataSourceH );
OGRErr CPL_DLL OGR_DS_SyncToDisk( OGRDataSourceH );

/* OGRSFDriver */

const char CPL_DLL *OGR_Dr_GetName( OGRSFDriverH );
OGRDataSourceH CPL_DLL OGR_Dr_Open( OGRSFDriverH, const char *, int );
int CPL_DLL OGR_Dr_TestCapability( OGRSFDriverH, const char * );
OGRDataSourceH CPL_DLL OGR_Dr_CreateDataSource( OGRSFDriverH, const char *,
                                                char ** );
OGRDataSourceH CPL_DLL OGR_Dr_CopyDataSource( OGRSFDriverH,  OGRDataSourceH,
                                              const char *, char ** );
OGRErr CPL_DLL OGR_Dr_DeleteDataSource( OGRSFDriverH, const char * );

/* OGRSFDriverRegistrar */

OGRDataSourceH CPL_DLL OGROpen( const char *, int, OGRSFDriverH * );
OGRDataSourceH CPL_DLL OGROpenShared( const char *, int, OGRSFDriverH * );
OGRErr  CPL_DLL OGRReleaseDataSource( OGRDataSourceH );
void    CPL_DLL OGRRegisterDriver( OGRSFDriverH );
int     CPL_DLL OGRGetDriverCount();
OGRSFDriverH CPL_DLL OGRGetDriver( int );
OGRSFDriverH CPL_DLL OGRGetDriverByName( const char * );
int     CPL_DLL OGRGetOpenDSCount();
OGRDataSourceH CPL_DLL OGRGetOpenDS( int iDS );


/* note: this is also declared in ogrsf_frmts.h */
void CPL_DLL OGRRegisterAll();

CPL_C_END

#endif /* ndef _OGR_API_H_INCLUDED */


