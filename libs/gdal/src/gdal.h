/******************************************************************************
 *
 * Name:     gdal.h
 * Project:  GDAL Core
 * Purpose:  GDAL Core C/Public declarations.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1998, 2002 Frank Warmerdam
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
 * $Log: gdal.h,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:28  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.77  2004/03/10 19:18:29  warmerda
 * updated date
 *
 * Revision 1.76  2004/03/01 18:30:44  warmerda
 * Updated release date.
 *
 * Revision 1.75  2004/02/25 09:03:15  dron
 * Added GDALPackedDMSToDec() and GDALDecToPackedDMS() functions.
 *
 * Revision 1.74  2004/02/19 15:55:52  warmerda
 * updated to 1.2.0
 *
 * Revision 1.73  2004/02/04 21:30:12  warmerda
 * ensure GDALGetDataTypeByName is exported
 *
 * Revision 1.72  2004/01/18 16:43:37  dron
 * Added GDALGetDataTypeByName() function.
 *
 * Revision 1.71  2003/07/18 04:46:48  sperkins
 * added CPL_DLL to GDALFillRaster
 *
 * Revision 1.70  2003/06/27 20:03:11  warmerda
 * updated version to 1.1.9
 *
 * Revision 1.69  2003/06/03 19:44:00  warmerda
 * added GDALRPCInfo support
 *
 * Revision 1.68  2003/05/06 05:20:38  sperkins
 * cleaned up comments
 *
 * Revision 1.67  2003/05/06 05:13:36  sperkins
 * added Fill() and GDALFillRaster()
 *
 * Revision 1.66  2003/05/02 19:47:57  warmerda
 * added C GetBandNumber and GetBandDataset entry points
 *
 * Revision 1.65  2003/04/30 17:13:48  warmerda
 * added docs for many C functions
 *
 * Revision 1.64  2003/04/30 15:48:31  warmerda
 * Fixed email address, trimmed log messages.
 *
 * Revision 1.63  2003/04/25 19:46:13  warmerda
 * added GDALDatasetRasterIO
 *
 * Revision 1.62  2003/03/18 06:01:03  warmerda
 * Added GDALFlushCache()
 *
 * Revision 1.61  2003/02/20 18:34:12  warmerda
 * added GDALGetRasterAccess()
 *
 * Revision 1.60  2003/01/27 21:55:52  warmerda
 * various documentation improvements
 *
 * Revision 1.59  2002/12/21 17:28:35  warmerda
 * actually, lets use 1.1.8.0
 *
 * Revision 1.58  2002/12/21 17:26:43  warmerda
 * updated version to 1.1.7.5
 *
 * Revision 1.57  2002/12/05 15:46:38  warmerda
 * added GDALReadTabFile()
 *
 * Revision 1.56  2002/11/23 18:07:41  warmerda
 * added DMD_CREATIONDATATYPES
 *
 * Revision 1.55  2002/10/24 14:18:29  warmerda
 * intermediate version update
 *
 * Revision 1.54  2002/09/11 14:17:38  warmerda
 * added C GDALSetDescription()
 *
 * Revision 1.53  2002/09/06 01:29:55  warmerda
 * added C entry points for GetAccess() and GetOpenDatasets()
 *
 * Revision 1.52  2002/09/04 06:52:35  warmerda
 * added GDALDestroyDriverManager
 *
 * Revision 1.51  2002/07/09 20:33:12  warmerda
 * expand tabs
 *
 * Revision 1.50  2002/06/12 21:13:27  warmerda
 * use metadata based driver info
 *
 * Revision 1.49  2002/05/28 18:55:46  warmerda
 * added GDALOpenShared() and GDALDumpOpenDatasets
 *
 * Revision 1.48  2002/05/14 21:38:32  warmerda
 * make INST_DATA overidable with binary patch
 *
 * Revision 1.47  2002/05/06 21:37:29  warmerda
 * added GDALGCPsToGeoTransform
 *
 * Revision 1.46  2002/04/24 16:25:04  warmerda
 * Ensure that GDAL{Read,Write}WorldFile() are exported on Windows.
 *
 * Revision 1.45  2002/04/19 12:22:05  dron
 * added GDALWriteWorldFile()
 *
 * Revision 1.44  2002/04/16 13:59:33  warmerda
 * added GDALVersionInfo
 *
 * Revision 1.43  2002/04/16 13:26:08  warmerda
 * upgrade to version 1.1.7
 */

#ifndef GDAL_H_INCLUDED
#define GDAL_H_INCLUDED

/**
 * \file gdal.h
 *
 * Public (C callable) GDAL entry points.
 */

#include "cpl_port.h"
#include "cpl_error.h"

/* -------------------------------------------------------------------- */
/*      GDAL Version Information.                                       */
/* -------------------------------------------------------------------- */
#ifndef GDAL_VERSION_NUM
#  define GDAL_VERSION_NUM      1200
#endif
#ifndef GDAL_RELEASE_DATE
#  define GDAL_RELEASE_DATE     20040310
#endif
#ifndef GDAL_RELEASE_NAME
#  define GDAL_RELEASE_NAME     "1.2.0.0"
#endif

/* -------------------------------------------------------------------- */
/*      Significant constants.                                          */
/* -------------------------------------------------------------------- */

CPL_C_START

/*! Pixel data types */
typedef enum {
    GDT_Unknown = 0,
    /*! Eight bit unsigned integer */           GDT_Byte = 1,
    /*! Sixteen bit unsigned integer */         GDT_UInt16 = 2,
    /*! Sixteen bit signed integer */           GDT_Int16 = 3,
    /*! Thirty two bit unsigned integer */      GDT_UInt32 = 4,
    /*! Thirty two bit signed integer */        GDT_Int32 = 5,
    /*! Thirty two bit floating point */        GDT_Float32 = 6,
    /*! Sixty four bit floating point */        GDT_Float64 = 7,
    /*! Complex Int16 */                        GDT_CInt16 = 8,
    /*! Complex Int32 */                        GDT_CInt32 = 9,
    /*! Complex Float32 */                      GDT_CFloat32 = 10,
    /*! Complex Float64 */                      GDT_CFloat64 = 11,
    GDT_TypeCount = 12          /* maximum type # + 1 */
} GDALDataType;

int CPL_DLL GDALGetDataTypeSize( GDALDataType );
int CPL_DLL GDALDataTypeIsComplex( GDALDataType );
const char CPL_DLL *GDALGetDataTypeName( GDALDataType );
GDALDataType CPL_DLL GDALGetDataTypeByName( const char * );
GDALDataType CPL_DLL GDALDataTypeUnion( GDALDataType, GDALDataType );

/*! Flag indicating read/write, or read-only access to data. */
typedef enum {
    /*! Read only (no update) access */ GA_ReadOnly = 0,
    /*! Read/write access. */           GA_Update = 1
} GDALAccess;

/*! Read/Write flag for RasterIO() method */
typedef enum {
    /*! Read data */   GF_Read = 0,
    /*! Write data */  GF_Write = 1
} GDALRWFlag;

/*! Types of color interpretation for raster bands. */
typedef enum
{
    GCI_Undefined=0,
    /*! Greyscale */                                      GCI_GrayIndex=1,
    /*! Paletted (see associated color table) */          GCI_PaletteIndex=2,
    /*! Red band of RGBA image */                         GCI_RedBand=3,
    /*! Green band of RGBA image */                       GCI_GreenBand=4,
    /*! Blue band of RGBA image */                        GCI_BlueBand=5,
    /*! Alpha (0=transparent, 255=opaque) */              GCI_AlphaBand=6,
    /*! Hue band of HLS image */                          GCI_HueBand=7,
    /*! Saturation band of HLS image */                   GCI_SaturationBand=8,
    /*! Lightness band of HLS image */                    GCI_LightnessBand=9,
    /*! Cyan band of CMYK image */                        GCI_CyanBand=10,
    /*! Magenta band of CMYK image */                     GCI_MagentaBand=11,
    /*! Yellow band of CMYK image */                      GCI_YellowBand=12,
    /*! Black band of CMLY image */                       GCI_BlackBand=13
} GDALColorInterp;

/*! Translate a GDALColorInterp into a user displayable string. */
const char CPL_DLL *GDALGetColorInterpretationName( GDALColorInterp );

/*! Types of color interpretations for a GDALColorTable. */
typedef enum
{
  /*! Grayscale (in GDALColorEntry.c1) */                      GPI_Gray=0,
  /*! Red, Green, Blue and Alpha in (in c1, c2, c3 and c4) */  GPI_RGB=1,
  /*! Cyan, Magenta, Yellow and Black (in c1, c2, c3 and c4)*/ GPI_CMYK=2,
  /*! Hue, Lightness and Saturation (in c1, c2, and c3) */     GPI_HLS=3
} GDALPaletteInterp;

/*! Translate a GDALPaletteInterp into a user displayable string. */
const char CPL_DLL *GDALGetPaletteInterpretationName( GDALPaletteInterp );

/* -------------------------------------------------------------------- */
/*      GDAL Specific error codes.                                      */
/*                                                                      */
/*      error codes 100 to 299 reserved for GDAL.                       */
/* -------------------------------------------------------------------- */
#define CPLE_WrongFormat        200

/* -------------------------------------------------------------------- */
/*      Define handle types related to various internal classes.        */
/* -------------------------------------------------------------------- */
typedef void *GDALMajorObjectH;
typedef void *GDALDatasetH;
typedef void *GDALRasterBandH;
typedef void *GDALDriverH;
typedef void *GDALProjDefH;
typedef void *GDALColorTableH;

/* -------------------------------------------------------------------- */
/*      Callback "progress" function.                                   */
/* -------------------------------------------------------------------- */

typedef int (*GDALProgressFunc)(double,const char *, void *);
int CPL_DLL GDALDummyProgress( double, const char *, void *);
int CPL_DLL GDALTermProgress( double, const char *, void *);
int CPL_DLL GDALScaledProgress( double, const char *, void *);
void CPL_DLL *GDALCreateScaledProgress( double, double,
                                        GDALProgressFunc, void * );
void CPL_DLL GDALDestroyScaledProgress( void * );

/* ==================================================================== */
/*      Registration/driver related.                                    */
/* ==================================================================== */

typedef struct {
    char      *pszOptionName;
    char      *pszValueType;   /* "boolean", "int", "float", "string",
                                  "string-select" */
    char      *pszDescription;
    char      **papszOptions;
} GDALOptionDefinition;

#define GDAL_DMD_LONGNAME "DMD_LONGNAME"
#define GDAL_DMD_HELPTOPIC "DMD_HELPTOPIC"
#define GDAL_DMD_MIMETYPE "DMD_MIMETYPE"
#define GDAL_DMD_EXTENSION "DMD_EXTENSION"
#define GDAL_DMD_CREATIONOPTIONLIST "DMD_CREATIONOPTIONLIST"
#define GDAL_DMD_CREATIONDATATYPES "DMD_CREATIONDATATYPES"

#define GDAL_DCAP_CREATE     "DCAP_CREATE"
#define GDAL_DCAP_CREATECOPY "DCAP_CREATECOPY"

void CPL_DLL GDALAllRegister( void );

GDALDatasetH CPL_DLL GDALCreate( GDALDriverH hDriver,
                                 const char *, int, int, int, GDALDataType,
                                 char ** );
GDALDatasetH CPL_DLL GDALCreateCopy( GDALDriverH, const char *, GDALDatasetH,
                                     int, char **, GDALProgressFunc, void * );

GDALDatasetH CPL_DLL GDALOpen( const char *pszFilename, GDALAccess eAccess );
GDALDatasetH CPL_DLL GDALOpenShared( const char *, GDALAccess );
int          CPL_DLL GDALDumpOpenDatasets( FILE * );

GDALDriverH CPL_DLL GDALGetDriverByName( const char * );
int CPL_DLL         GDALGetDriverCount();
GDALDriverH CPL_DLL GDALGetDriver( int );
int         CPL_DLL GDALRegisterDriver( GDALDriverH );
void        CPL_DLL GDALDeregisterDriver( GDALDriverH );
void        CPL_DLL GDALDestroyDriverManager( void );
CPLErr      CPL_DLL GDALDeleteDataset( GDALDriverH, const char * );

/* The following are deprecated */
const char CPL_DLL *GDALGetDriverShortName( GDALDriverH );
const char CPL_DLL *GDALGetDriverLongName( GDALDriverH );
const char CPL_DLL *GDALGetDriverHelpTopic( GDALDriverH );

/* ==================================================================== */
/*      GDAL_GCP                                                        */
/* ==================================================================== */

/** Ground Control Point */
typedef struct
{
    /** Unique identifier, often numeric */
    char        *pszId;

    /** Informational message or "" */
    char        *pszInfo;

    /** Pixel (x) location of GCP on raster */
    double      dfGCPPixel;
    /** Line (y) location of GCP on raster */
    double      dfGCPLine;

    /** X position of GCP in georeferenced space */
    double      dfGCPX;

    /** Y position of GCP in georeferenced space */
    double      dfGCPY;

    /** Elevation of GCP, or zero if not known */
    double      dfGCPZ;
} GDAL_GCP;

void CPL_DLL GDALInitGCPs( int, GDAL_GCP * );
void CPL_DLL GDALDeinitGCPs( int, GDAL_GCP * );
GDAL_GCP CPL_DLL *GDALDuplicateGCPs( int, const GDAL_GCP * );

int CPL_DLL GDALGCPsToGeoTransform( int nGCPCount, const GDAL_GCP *pasGCPs,
                                    double *padfGeoTransform, int bApproxOK );
int CPL_DLL GDALInvGeoTransform( double *padfGeoTransformIn,
                                 double *padfInvGeoTransformOut );

/* ==================================================================== */
/*      major objects (dataset, and, driver, drivermanager).            */
/* ==================================================================== */

char CPL_DLL  **GDALGetMetadata( GDALMajorObjectH, const char * );
CPLErr CPL_DLL  GDALSetMetadata( GDALMajorObjectH, char **,
                                 const char * );
const char CPL_DLL *GDALGetMetadataItem( GDALMajorObjectH, const char *,
                                         const char * );
CPLErr CPL_DLL  GDALSetMetadataItem( GDALMajorObjectH,
                                     const char *, const char *,
                                     const char * );
const char CPL_DLL *GDALGetDescription( GDALMajorObjectH );
void       CPL_DLL  GDALSetDescription( GDALMajorObjectH, const char * );

/* ==================================================================== */
/*      GDALDataset class ... normally this represents one file.        */
/* ==================================================================== */

GDALDriverH CPL_DLL GDALGetDatasetDriver( GDALDatasetH );
void CPL_DLL   GDALClose( GDALDatasetH );
int CPL_DLL     GDALGetRasterXSize( GDALDatasetH );
int CPL_DLL     GDALGetRasterYSize( GDALDatasetH );
int CPL_DLL     GDALGetRasterCount( GDALDatasetH );
GDALRasterBandH CPL_DLL GDALGetRasterBand( GDALDatasetH, int );

CPLErr CPL_DLL  GDALAddBand( GDALDatasetH hDS, GDALDataType eType,
                             char **papszOptions );

CPLErr CPL_DLL GDALDatasetRasterIO(
    GDALDatasetH hDS, GDALRWFlag eRWFlag,
    int nDSXOff, int nDSYOff, int nDSXSize, int nDSYSize,
    void * pBuffer, int nBXSize, int nBYSize, GDALDataType eBDataType,
    int nBandCount, int *panBandCount,
    int nPixelSpace, int nLineSpace, int nBandSpace);

const char CPL_DLL *GDALGetProjectionRef( GDALDatasetH );
CPLErr CPL_DLL  GDALSetProjection( GDALDatasetH, const char * );
CPLErr CPL_DLL  GDALGetGeoTransform( GDALDatasetH, double * );
CPLErr CPL_DLL  GDALSetGeoTransform( GDALDatasetH, double * );

int CPL_DLL     GDALGetGCPCount( GDALDatasetH );
const char CPL_DLL *GDALGetGCPProjection( GDALDatasetH );
const GDAL_GCP CPL_DLL *GDALGetGCPs( GDALDatasetH );
CPLErr CPL_DLL  GDALSetGCPs( GDALDatasetH, int, const GDAL_GCP *,
                             const char * );

void CPL_DLL   *GDALGetInternalHandle( GDALDatasetH, const char * );
int CPL_DLL     GDALReferenceDataset( GDALDatasetH );
int CPL_DLL     GDALDereferenceDataset( GDALDatasetH );

CPLErr CPL_DLL  GDALBuildOverviews( GDALDatasetH, const char *, int, int *,
                                    int, int *, GDALProgressFunc, void * );
void CPL_DLL    GDALGetOpenDatasets( GDALDatasetH ***hDS, int *pnCount );
int CPL_DLL     GDALGetAccess( GDALDatasetH hDS );
void CPL_DLL    GDALFlushCache( GDALDatasetH hDS );

/* ==================================================================== */
/*      GDALRasterBand ... one band/channel in a dataset.               */
/* ==================================================================== */

GDALDataType CPL_DLL GDALGetRasterDataType( GDALRasterBandH );
void CPL_DLL    GDALGetBlockSize( GDALRasterBandH,
                                  int * pnXSize, int * pnYSize );

CPLErr CPL_DLL GDALRasterIO( GDALRasterBandH hRBand, GDALRWFlag eRWFlag,
                              int nDSXOff, int nDSYOff,
                              int nDSXSize, int nDSYSize,
                              void * pBuffer, int nBXSize, int nBYSize,
                              GDALDataType eBDataType,
                              int nPixelSpace, int nLineSpace );
CPLErr CPL_DLL GDALReadBlock( GDALRasterBandH, int, int, void * );
CPLErr CPL_DLL GDALWriteBlock( GDALRasterBandH, int, int, void * );
int CPL_DLL GDALGetRasterBandXSize( GDALRasterBandH );
int CPL_DLL GDALGetRasterBandYSize( GDALRasterBandH );
char CPL_DLL  **GDALGetRasterMetadata( GDALRasterBandH );
GDALAccess CPL_DLL GDALGetRasterAccess( GDALRasterBandH );
int CPL_DLL GDALGetBandNumber( GDALRasterBandH );
GDALDatasetH CPL_DLL GDALGetBandDataset( GDALRasterBandH );

GDALColorInterp CPL_DLL GDALGetRasterColorInterpretation( GDALRasterBandH );
CPLErr CPL_DLL GDALSetRasterColorInterpretation( GDALRasterBandH,
                                                 GDALColorInterp );
GDALColorTableH CPL_DLL GDALGetRasterColorTable( GDALRasterBandH );
CPLErr CPL_DLL GDALSetRasterColorTable( GDALRasterBandH, GDALColorTableH );
int CPL_DLL     GDALHasArbitraryOverviews( GDALRasterBandH );
int CPL_DLL             GDALGetOverviewCount( GDALRasterBandH );
GDALRasterBandH CPL_DLL GDALGetOverview( GDALRasterBandH, int );
double CPL_DLL GDALGetRasterNoDataValue( GDALRasterBandH, int * );
CPLErr CPL_DLL GDALSetRasterNoDataValue( GDALRasterBandH, double );
char CPL_DLL ** GDALGetRasterCategoryNames( GDALRasterBandH );
CPLErr CPL_DLL GDALSetRasterCategoryNames( GDALRasterBandH, char ** );
double CPL_DLL GDALGetRasterMinimum( GDALRasterBandH, int *pbSuccess );
double CPL_DLL GDALGetRasterMaximum( GDALRasterBandH, int *pbSuccess );
const char CPL_DLL *GDALGetRasterUnitType( GDALRasterBandH );
void CPL_DLL GDALComputeRasterMinMax( GDALRasterBandH hBand, int bApproxOK,
                                      double adfMinMax[2] );
CPLErr CPL_DLL GDALFlushRasterCache( GDALRasterBandH hBand );
CPLErr CPL_DLL GDALGetRasterHistogram( GDALRasterBandH hBand,
                                       double dfMin, double dfMax,
                                       int nBuckets, int *panHistogram,
                                       int bIncludeOutOfRange, int bApproxOK,
                                       GDALProgressFunc pfnProgress,
                                       void * pProgressData );
int CPL_DLL GDALGetRandomRasterSample( GDALRasterBandH, int, float * );
GDALRasterBandH CPL_DLL GDALGetRasterSampleOverview( GDALRasterBandH, int );
CPLErr CPL_DLL GDALFillRaster( GDALRasterBandH hBand, double dfRealValue,
		       double dfImaginaryValue );
CPLErr GDALComputeBandStats( GDALRasterBandH hBand, int nSampleStep,
                             double *pdfMean, double *pdfStdDev,
                             GDALProgressFunc pfnProgress,
                             void *pProgressData );
CPLErr GDALOverviewMagnitudeCorrection( GDALRasterBandH hBaseBand,
                                        int nOverviewCount,
                                        GDALRasterBandH *pahOverviews,
                                        GDALProgressFunc pfnProgress,
                                        void *pProgressData );

/* -------------------------------------------------------------------- */
/*      Helper functions.                                               */
/* -------------------------------------------------------------------- */
void CPL_DLL GDALSwapWords( void *pData, int nWordSize, int nWordCount,
                            int nWordSkip );
void CPL_DLL
    GDALCopyWords( void * pSrcData, GDALDataType eSrcType, int nSrcPixelOffset,
                   void * pDstData, GDALDataType eDstType, int nDstPixelOffset,
                   int nWordCount );

int CPL_DLL GDALReadWorldFile( const char *pszBaseFilename,
                       const char *pszExtension,
                       double * padfGeoTransform );
int CPL_DLL GDALWriteWorldFile( const char *pszBaseFilename,
                       const char *pszExtension,
                       double * padfGeoTransform );
int CPL_DLL GDALReadTabFile( const char *pszBaseFilename,
                             double *padfGeoTransform, char **ppszWKT,
                             int *pnGCPCount, GDAL_GCP **ppasGCPs );

const char CPL_DLL *GDALDecToDMS( double, const char *, int );
double CPL_DLL GDALPackedDMSToDec( double );
double CPL_DLL GDALDecToPackedDMS( double );

const char CPL_DLL *GDALVersionInfo( const char * );

typedef struct {
    double      dfLINE_OFF;
    double      dfSAMP_OFF;
    double      dfLAT_OFF;
    double      dfLONG_OFF;
    double      dfHEIGHT_OFF;

    double      dfLINE_SCALE;
    double      dfSAMP_SCALE;
    double      dfLAT_SCALE;
    double      dfLONG_SCALE;
    double      dfHEIGHT_SCALE;

    double      adfLINE_NUM_COEFF[20];
    double      adfLINE_DEN_COEFF[20];
    double      adfSAMP_NUM_COEFF[20];
    double      adfSAMP_DEN_COEFF[20];

    double	dfMIN_LONG;
    double      dfMIN_LAT;
    double      dfMAX_LONG;
    double	dfMAX_LAT;

} GDALRPCInfo;

int CPL_DLL GDALExtractRPCInfo( char **, GDALRPCInfo * );

/* ==================================================================== */
/*      Color tables.                                                   */
/* ==================================================================== */
/** Color tuple */
typedef struct
{
    /*! gray, red, cyan or hue */
    short      c1;

    /*! green, magenta, or lightness */
    short      c2;

    /*! blue, yellow, or saturation */
    short      c3;

    /*! alpha or blackband */
    short      c4;
} GDALColorEntry;

GDALColorTableH CPL_DLL GDALCreateColorTable( GDALPaletteInterp );
void CPL_DLL            GDALDestroyColorTable( GDALColorTableH );
GDALColorTableH CPL_DLL GDALCloneColorTable( GDALColorTableH );
GDALPaletteInterp CPL_DLL GDALGetPaletteInterpretation( GDALColorTableH );
int CPL_DLL             GDALGetColorEntryCount( GDALColorTableH );
const GDALColorEntry CPL_DLL *GDALGetColorEntry( GDALColorTableH, int );
int CPL_DLL GDALGetColorEntryAsRGB( GDALColorTableH, int, GDALColorEntry *);
void CPL_DLL GDALSetColorEntry( GDALColorTableH, int, const GDALColorEntry * );

/* ==================================================================== */
/*      GDAL Cache Management                                           */
/* ==================================================================== */

void CPL_DLL GDALSetCacheMax( int nBytes );
int CPL_DLL GDALGetCacheMax();
int CPL_DLL GDALGetCacheUsed();
int CPL_DLL GDALFlushCacheBlock();

CPL_C_END

#endif /* ndef GDAL_H_INCLUDED */
