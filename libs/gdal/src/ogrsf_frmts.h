/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Classes related to format registration, and file opening.
 * Author:   Frank Warmerdam, warmerda@home.com
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
 *
 */

#ifndef _OGRSF_FRMTS_H_INCLUDED
#define _OGRSF_FRMTS_H_INCLUDED

#include "ogr_feature.h"

/**
 * \file ogrsf_frmts.h
 *
 * Classes related to registration of format support, and opening datasets.
 */

class OGRLayerAttrIndex;

/************************************************************************/
/*                               OGRLayer                               */
/************************************************************************/

/**
 * This class represents a layer of simple features, with access methods.
 *
 */

class CPL_DLL OGRLayer
{
  public:
                OGRLayer();
    virtual     ~OGRLayer();

    virtual OGRGeometry *GetSpatialFilter() = 0;
    virtual void        SetSpatialFilter( OGRGeometry * ) = 0;

    virtual OGRErr      SetAttributeFilter( const char * );

    virtual void        ResetReading() = 0;
    virtual OGRFeature *GetNextFeature() = 0;
    virtual OGRFeature *GetFeature( long nFID );
    virtual OGRErr      SetFeature( OGRFeature *poFeature );
    virtual OGRErr      CreateFeature( OGRFeature *poFeature );
    virtual OGRErr      DeleteFeature( long nFID );

    virtual OGRFeatureDefn *GetLayerDefn() = 0;

    virtual OGRSpatialReference *GetSpatialRef() { return NULL; }

    virtual int         GetFeatureCount( int bForce = TRUE );
    virtual OGRErr      GetExtent(OGREnvelope *psExtent, int bForce = TRUE);

    virtual int         TestCapability( const char * ) = 0;

    virtual const char *GetInfo( const char * );

    virtual OGRErr      CreateField( OGRFieldDefn *poField,
                                     int bApproxOK = TRUE );

    virtual OGRErr      SyncToDisk();

    OGRStyleTable       *GetStyleTable(){return m_poStyleTable;}
    void                 SetStyleTable(OGRStyleTable *poStyleTable){m_poStyleTable = poStyleTable;}

    virtual OGRErr       StartTransaction();
    virtual OGRErr       CommitTransaction();
    virtual OGRErr       RollbackTransaction();

    int                 Reference();
    int                 Dereference();
    int                 GetRefCount() const;

    /* consider these private */
    OGRErr               InitializeIndexSupport( const char * );
    OGRLayerAttrIndex   *GetIndex() { return m_poAttrIndex; }

 protected:
    OGRStyleTable       *m_poStyleTable;
//    OGRFeatureQuery     *m_poAttrQuery;
    OGRLayerAttrIndex   *m_poAttrIndex;

    int                  m_nRefCount;
};


/************************************************************************/
/*                            OGRDataSource                             */
/************************************************************************/

/**
 * This class represents a data source.  A data source potentially
 * consists of many layers (OGRLayer).  A data source normally consists
 * of one, or a related set of files, though the name doesn't have to be
 * a real item in the file system.
 *
 * When an OGRDataSource is destroyed, all it's associated OGRLayers objects
 * are also destroyed.
 */

class CPL_DLL OGRDataSource
{
  public:

    OGRDataSource();
    virtual     ~OGRDataSource();

    virtual const char  *GetName() = 0;

    virtual int         GetLayerCount() = 0;
    virtual OGRLayer    *GetLayer(int) = 0;
    virtual OGRLayer    *GetLayerByName(const char *);
    virtual OGRErr      DeleteLayer(int);

    virtual int         TestCapability( const char * ) = 0;

    virtual OGRLayer   *CreateLayer( const char *pszName,
                                     OGRSpatialReference *poSpatialRef = NULL,
                                     OGRwkbGeometryType eGType = wkbUnknown,
                                     char ** papszOptions = NULL );
    virtual OGRLayer   *CopyLayer( OGRLayer *poSrcLayer,
                                   const char *pszNewName,
                                   char **papszOptions = NULL );
    OGRStyleTable       *GetStyleTable(){return m_poStyleTable;}

    virtual OGRLayer *  ExecuteSQL( const char *pszStatement,
                                    OGRGeometry *poSpatialFilter,
                                    const char *pszDialect );
    virtual void        ReleaseResultSet( OGRLayer * poResultsSet );

    virtual OGRErr      SyncToDisk();

    int                 Reference();
    int                 Dereference();
    int                 GetRefCount() const;
    int                 GetSummaryRefCount() const;

  protected:

    OGRErr              ProcessSQLCreateIndex( const char * );
    OGRErr              ProcessSQLDropIndex( const char * );

    OGRStyleTable      *m_poStyleTable;
    int                 m_nRefCount;
};

/************************************************************************/
/*                             OGRSFDriver                              */
/************************************************************************/

/**
 * Represents an operational format driver.
 *
 * One OGRSFDriver derived class will normally exist for each file format
 * registered for use, regardless of whether a file has or will be opened.
 * The list of available drivers is normally managed by the
 * OGRSFDriverRegistrar.
 */

class CPL_DLL OGRSFDriver
{
  public:
    virtual     ~OGRSFDriver();

    virtual const char  *GetName() = 0;

    virtual OGRDataSource *Open( const char *pszName, int bUpdate=FALSE ) = 0;

    virtual int         TestCapability( const char * ) = 0;

    virtual OGRDataSource *CreateDataSource( const char *pszName,
                                             char ** = NULL );
    virtual OGRErr      DeleteDataSource( const char *pszName );

    virtual OGRDataSource *CopyDataSource( OGRDataSource *poSrcDS,
                                           const char *pszNewName,
                                           char **papszOptions = NULL );
};


/************************************************************************/
/*                         OGRSFDriverRegistrar                         */
/************************************************************************/

/**
 * Singleton manager for drivers.
 *
 */

class CPL_DLL OGRSFDriverRegistrar
{
    int         nDrivers;
    OGRSFDriver **papoDrivers;

                OGRSFDriverRegistrar();

    int         nOpenDSCount;
    char        **papszOpenDSRawName;
    OGRDataSource **papoOpenDS;
    OGRSFDriver **papoOpenDSDriver;

  public:

                ~OGRSFDriverRegistrar();

    static OGRSFDriverRegistrar *GetRegistrar();
    static OGRDataSource *Open( const char *pszName, int bUpdate=FALSE,
                                OGRSFDriver ** ppoDriver = NULL );

    OGRDataSource *OpenShared( const char *pszName, int bUpdate=FALSE,
                               OGRSFDriver ** ppoDriver = NULL );
    OGRErr      ReleaseDataSource( OGRDataSource * );

    void        RegisterDriver( OGRSFDriver * poDriver );

    int         GetDriverCount( void );
    OGRSFDriver *GetDriver( int iDriver );
    OGRSFDriver *GetDriverByName( const char * );

    int         GetOpenDSCount() { return nOpenDSCount; }
    OGRDataSource *GetOpenDS( int );
};

/* -------------------------------------------------------------------- */
/*      Various available registration methods.                         */
/* -------------------------------------------------------------------- */
CPL_C_START
void CPL_DLL OGRRegisterAll();

void CPL_DLL RegisterOGRShape();
void CPL_DLL RegisterOGRNTF();
void CPL_DLL RegisterOGRFME();
void CPL_DLL RegisterOGRSDTS();
void CPL_DLL RegisterOGRTiger();
void CPL_DLL RegisterOGRS57();
void CPL_DLL RegisterOGRTAB();
void CPL_DLL RegisterOGRMIF();
void CPL_DLL RegisterOGROGDI();
void CPL_DLL RegisterOGRODBC();
void CPL_DLL RegisterOGRPG();
void CPL_DLL RegisterOGROCI();
void CPL_DLL RegisterOGRDGN();
void CPL_DLL RegisterOGRGML();
void CPL_DLL RegisterOGRAVCBin();
void CPL_DLL RegisterOGRAVCE00();
void CPL_DLL RegisterOGRFME();
void CPL_DLL RegisterOGRREC();
void CPL_DLL RegisterOGRMEM();
void CPL_DLL RegisterOGRVRT();
void CPL_DLL RegisterOGRDODS();

CPL_C_END


#endif /* ndef _OGRSF_FRMTS_H_INCLUDED */
