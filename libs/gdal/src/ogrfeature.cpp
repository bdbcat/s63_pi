/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The OGRFeature class implementation.
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
 * $Log: ogrfeature.cpp,v $
 * Revision 1.1.1.1  2006/08/21 05:52:20  dsr
 * Initial import as opencpn, GNU Automake compliant.
 *
 * Revision 1.1.1.1  2006/04/19 03:23:29  dsr
 * Rename/Import to OpenCPN
 *
 * Revision 1.29  2003/05/28 19:16:42  warmerda
 * fixed up argument names and stuff for docs
 *
 * Revision 1.28  2003/04/08 20:57:28  warmerda
 * added RemapFields on OGRFeature
 *
 * Revision 1.27  2003/04/03 23:39:11  danmo
 * Small updates to C API docs (Normand S.)
 *
 * Revision 1.26  2003/03/31 15:55:42  danmo
 * Added C API function docs
 *
 * Revision 1.25  2003/01/08 22:03:44  warmerda
 * added StealGeometry() method on OGRFeature
 *
 * Revision 1.24  2002/11/12 19:42:41  warmerda
 * copy style string in SetFrom()
 *
 * Revision 1.23  2002/09/26 18:12:38  warmerda
 * added C support
 *
 * Revision 1.22  2002/04/25 16:06:26  warmerda
 * don't copy style string if not set during clone
 *
 * Revision 1.21  2002/04/24 20:00:30  warmerda
 * fix clone to copy fid as well
 *
 * Revision 1.20  2001/11/09 15:02:40  warmerda
 * dump render style
 *
 * Revision 1.19  2001/11/01 16:54:16  warmerda
 * added DestroyFeature
 *
 * Revision 1.18  2001/07/18 05:03:05  warmerda
 *
 * Revision 1.17  2001/06/01 14:32:27  warmerda
 * added CreateFeature factory method
 *
 * Revision 1.16  2001/02/06 14:14:09  warmerda
 * fixed up documentation
 *
 * Revision 1.15  2000/08/25 20:17:34  danmo
 * Init m_poStyleTable=NULL in constructor
 *
 * Revision 1.14  2000/08/18 21:26:53  svillene
 * Add representation
 *
 * Revision 1.13  2000/06/09 21:15:39  warmerda
 * fixed field copying
 *
 * Revision 1.12  1999/11/26 03:05:38  warmerda
 * added unset field support
 */

#include "ogr_feature.h"
#include "ogr_api.h"
#include "ogr_p.h"

/************************************************************************/
/*                             OGRFeature()                             */
/************************************************************************/

/**
 * Constructor
 *
 * Note that the OGRFeature will increment the reference count of it's
 * defining OGRFeatureDefn.  Destruction of the OGRFeatureDefn before
 * destruction of all OGRFeatures that depend on it is likely to result in
 * a crash.
 *
 * This method is the same as the C function OGR_F_Create().
 *
 * @param poDefnIn feature class (layer) definition to which the feature will
 * adhere.
 */

OGRFeature::OGRFeature( OGRFeatureDefn * poDefnIn )

{
    m_pszStyleString = NULL;
    m_poStyleTable = NULL;
    poDefnIn->Reference();
    poDefn = poDefnIn;

    nFID = OGRNullFID;

    poGeometry = NULL;

    // we should likely be initializing from the defaults, but this will
    // usually be a waste.
    pauFields = (OGRField *) CPLCalloc( poDefn->GetFieldCount(),
                                        sizeof(OGRField) );

    for( int i = 0; i < poDefn->GetFieldCount(); i++ )
    {
        pauFields[i].Set.nMarker1 = OGRUnsetMarker;
        pauFields[i].Set.nMarker2 = OGRUnsetMarker;
    }
}

/************************************************************************/
/*                            OGR_F_Create()                            */
/************************************************************************/
/**
 * Feature factory.
 *
 * Note that the OGRFeature will increment the reference count of it's
 * defining OGRFeatureDefn.  Destruction of the OGRFeatureDefn before
 * destruction of all OGRFeatures that depend on it is likely to result in
 * a crash.
 *
 * This function is the same as the CPP method OGRFeature::OGRFeature().
 *
 * @param hDefn handle to the feature class (layer) definition to
 * which the feature will adhere.
 *
 * @return an handle to the new feature object with null fields and
 * no geometry.
 */

OGRFeatureH OGR_F_Create( OGRFeatureDefnH hDefn )

{
    return (OGRFeatureH) new OGRFeature( (OGRFeatureDefn *) hDefn );
}

/************************************************************************/
/*                            ~OGRFeature()                             */
/************************************************************************/

OGRFeature::~OGRFeature()

{
    poDefn->Dereference();

    if( poGeometry != NULL )
        delete poGeometry;

    for( int i = 0; i < poDefn->GetFieldCount(); i++ )
    {
        OGRFieldDefn    *poFDefn = poDefn->GetFieldDefn(i);

        if( !IsFieldSet(i) )
            continue;

        switch( poFDefn->GetType() )
        {
          case OFTString:
            if( pauFields[i].String != NULL )
                VSIFree( pauFields[i].String );
            break;

          case OFTStringList:
            CSLDestroy( pauFields[i].StringList.paList );
            break;

          case OFTIntegerList:
          case OFTRealList:
            CPLFree( pauFields[i].IntegerList.paList );
            break;

          default:
            // should add support for wide strings.
            break;
        }
    }

    CPLFree( pauFields );
    CPLFree(m_pszStyleString);
}

/************************************************************************/
/*                           OGR_F_Destroy()                            */
/************************************************************************/
/**
 * Destroy feature
 *
 * The feature is deleted, but within the context of the GDAL/OGR heap.
 * This is necessary when higher level applications use GDAL/OGR from a
 * DLL and they want to delete a feature created within the DLL.  If the
 * delete is done in the calling application the memory will be freed onto
 * the application heap which is inappropriate.
 *
 * This function is the same as the CPP method OGRFeature::DestroyFeature().
 *
 * @param hFeat handle to the feature to destroy.
 */

void OGR_F_Destroy( OGRFeatureH hFeat )

{
    delete (OGRFeature *) hFeat;
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

/**
 * Feature factory.
 *
 * This is essentially a feature factory, useful for
 * applications creating features but wanting to ensure they
 * are created out of the OGR/GDAL heap.
 *
 * @param poDefn Feature definition defining schema.
 *
 * @return new feature object with null fields and no geometry.  May be
 * deleted with delete.
 */

OGRFeature *OGRFeature::CreateFeature( OGRFeatureDefn *poDefn )

{
    return new OGRFeature( poDefn );
}

/************************************************************************/
/*                           DestroyFeature()                           */
/************************************************************************/

/**
 * Destroy feature
 *
 * The feature is deleted, but within the context of the GDAL/OGR heap.
 * This is necessary when higher level applications use GDAL/OGR from a
 * DLL and they want to delete a feature created within the DLL.  If the
 * delete is done in the calling application the memory will be freed onto
 * the application heap which is inappropriate.
 *
 * This method is the same as the C function OGR_F_Destroy().
 *
 * @param poFeature the feature to delete.
 */

void OGRFeature::DestroyFeature( OGRFeature *poFeature )

{
    delete poFeature;
}

/************************************************************************/
/*                             GetDefnRef()                             */
/************************************************************************/

/**
 * \fn OGRFeatureDefn *OGRFeature::GetDefnRef();
 *
 * Fetch feature definition.
 *
 * This method is the same as the C function OGR_F_GetDefnRef().
 *
 * @return a reference to the feature definition object.
 */

/************************************************************************/
/*                          OGR_F_GetDefnRef()                          */
/************************************************************************/

/**
 * Fetch feature definition.
 *
 * This function is the same as the CPP method OGRFeature::GetDefnRef().
 *
 * @param hFeat handle to the feature to get the feature definition from.
 *
 * @return an handle to the feature definition object on which feature
 * depends.
 */

OGRFeatureDefnH OGR_F_GetDefnRef( OGRFeatureH hFeat )

{
    return ((OGRFeature *) hFeat)->GetDefnRef();
}

/************************************************************************/
/*                        SetGeometryDirectly()                         */
/************************************************************************/

/**
 * Set feature geometry.
 *
 * This method updates the features geometry, and operate exactly as
 * SetGeometry(), except that this method assumes ownership of the
 * passed geometry.
 *
 * This method is the same as the C function OGR_F_SetGeometryDirectly().
 *
 * @param poGeomIn new geometry to apply to feature.
 *
 * @return OGRERR_NONE if successful, or OGR_UNSUPPORTED_GEOMETRY_TYPE if
 * the geometry type is illegal for the OGRFeatureDefn (checking not yet
 * implemented).
 */

OGRErr OGRFeature::SetGeometryDirectly( OGRGeometry * poGeomIn )

{
    if( poGeometry != NULL )
        delete poGeometry;

    poGeometry = poGeomIn;

    // I should be verifying that the geometry matches the defn's type.

    return OGRERR_NONE;
}

/************************************************************************/
/*                     OGR_F_SetGeometryDirectly()                      */
/************************************************************************/

/**
 * Set feature geometry.
 *
 * This function updates the features geometry, and operate exactly as
 * SetGeometry(), except that this function assumes ownership of the
 * passed geometry.
 *
 * This function is the same as the CPP method
 * OGRFeature::SetGeometryDirectly.
 *
 * @param hFeat handle to the feature on which to apply the geometry.
 * @param hGeom handle to the new geometry to apply to feature.
 *
 * @return OGRERR_NONE if successful, or OGR_UNSUPPORTED_GEOMETRY_TYPE if
 * the geometry type is illegal for the OGRFeatureDefn (checking not yet
 * implemented).
 */

OGRErr OGR_F_SetGeometryDirectly( OGRFeatureH hFeat, OGRGeometryH hGeom )

{
    return ((OGRFeature *) hFeat)->SetGeometryDirectly((OGRGeometry *) hGeom);
}

/************************************************************************/
/*                            SetGeometry()                             */
/************************************************************************/

/**
 * Set feature geometry.
 *
 * This method updates the features geometry, and operate exactly as
 * SetGeometryDirectly(), except that this method does not assume ownership
 * of the passed geometry, but instead makes a copy of it.
 *
 * This method is the same as the C function OGR_F_SetGeometry().
 *
 * @param poGeomIn new geometry to apply to feature.
 *
 * @return OGRERR_NONE if successful, or OGR_UNSUPPORTED_GEOMETRY_TYPE if
 * the geometry type is illegal for the OGRFeatureDefn (checking not yet
 * implemented).
 */

OGRErr OGRFeature::SetGeometry( OGRGeometry * poGeomIn )

{
    if( poGeometry != NULL )
        delete poGeometry;

    if( poGeomIn != NULL )
        poGeometry = poGeomIn->clone();
    else
        poGeometry = NULL;

    // I should be verifying that the geometry matches the defn's type.

    return OGRERR_NONE;
}

/************************************************************************/
/*                         OGR_F_SetGeometry()                          */
/************************************************************************/

/**
 * Set feature geometry.
 *
 * This function updates the features geometry, and operate exactly as
 * SetGeometryDirectly(), except that this function does not assume ownership
 * of the passed geometry, but instead makes a copy of it.
 *
 * This function is the same as the CPP OGRFeature::SetGeometry().
 *
 * @param hFeat handle to the feature on which new geometry is applied to.
 * @param hGeom handle to the new geometry to apply to feature.
 *
 * @return OGRERR_NONE if successful, or OGR_UNSUPPORTED_GEOMETRY_TYPE if
 * the geometry type is illegal for the OGRFeatureDefn (checking not yet
 * implemented).
 */

OGRErr OGR_F_SetGeometry( OGRFeatureH hFeat, OGRGeometryH hGeom )

{
    return ((OGRFeature *) hFeat)->SetGeometry((OGRGeometry *) hGeom);
}

/************************************************************************/
/*                           StealGeometry()                            */
/************************************************************************/

/**
 * Take away ownership of geometry.
 *
 * Fetch the geometry from this feature, and clear the reference to the
 * geometry on the feature.  This is a mechanism for the application to
 * take over ownship of the geometry from the feature without copying.
 * Sort of an inverse to SetGeometryDirectly().
 *
 * After this call the OGRFeature will have a NULL geometry.
 *
 * @return the pointer to the geometry.
 */

OGRGeometry *OGRFeature::StealGeometry()

{
    OGRGeometry *poReturn = poGeometry;
    poGeometry = NULL;
    return poReturn;
}

/************************************************************************/
/*                           GetGeometryRef()                           */
/************************************************************************/

/**
 * \fn OGRGeometry *OGRFeature::GetGeometryRef();
 *
 * Fetch pointer to feature geometry.
 *
 * This method is the same as the C function OGR_F_GetGeometryRef().
 *
 * @return pointer to internal feature geometry.  This object should
 * not be modified.
 */

/************************************************************************/
/*                        OGR_F_GetGeometryRef()                        */
/************************************************************************/

/**
 * Fetch an handle to feature geometry.
 *
 * This function is the same as the CPP method OGRFeature::GetGeometryRef().
 *
 * @param hFeat handle to the feature to get geometry from.
 * @return an handle to internal feature geometry.  This object should
 * not be modified.
 */

OGRGeometryH OGR_F_GetGeometryRef( OGRFeatureH hFeat )

{
    return ((OGRFeature *) hFeat)->GetGeometryRef();
}

/************************************************************************/
/*                               Clone()                                */
/************************************************************************/

/**
 * Duplicate feature.
 *
 * The newly created feature is owned by the caller, and will have it's own
 * reference to the OGRFeatureDefn.
 *
 * This method is the same as the C function OGR_F_Clone().
 *
 * @return new feature, exactly matching this feature.
 */

OGRFeature *OGRFeature::Clone()

{
    OGRFeature  *poNew = new OGRFeature( poDefn );

    poNew->SetGeometry( poGeometry );

    for( int i = 0; i < poDefn->GetFieldCount(); i++ )
    {
        poNew->SetField( i, pauFields + i );
    }

    if( GetStyleString() != NULL )
        poNew->SetStyleString(GetStyleString());

    poNew->SetFID( GetFID() );

    return poNew;
}

/************************************************************************/
/*                            OGR_F_Clone()                             */
/************************************************************************/

/**
 * Duplicate feature.
 *
 * The newly created feature is owned by the caller, and will have it's own
 * reference to the OGRFeatureDefn.
 *
 * This function is the same as the CPP method OGRFeature::Clone().
 *
 * @param hFeat handle to the feature to clone.
 * @return an handle to the new feature, exactly matching this feature.
 */

OGRFeatureH OGR_F_Clone( OGRFeatureH hFeat )

{
    return (OGRFeatureH) ((OGRFeature *) hFeat)->Clone();
}

/************************************************************************/
/*                           GetFieldCount()                            */
/************************************************************************/

/**
 * \fn int OGRFeature::GetFieldCount();
 *
 * Fetch number of fields on this feature.  This will always be the same
 * as the field count for the OGRFeatureDefn.
 *
 * This method is the same as the C function OGR_F_GetFieldCount().
 *
 * @return count of fields.
 */

/************************************************************************/
/*                        OGR_F_GetFieldCount()                         */
/************************************************************************/

/**
 * Fetch number of fields on this feature.  This will always be the same
 * as the field count for the OGRFeatureDefn.
 *
 * This function is the same as the CPP method OGRFeature::GetFieldCount().
 *
 * @param hFeat handle to the feature to get the fields count from.
 * @return count of fields.
 */

int OGR_F_GetFieldCount( OGRFeatureH hFeat )

{
    return ((OGRFeature *) hFeat)->GetFieldCount();
}

/************************************************************************/
/*                          GetFieldDefnRef()                           */
/************************************************************************/

/**
 * \fn OGRFieldDefn *OGRFeature::GetFieldDefnRef( int iField );
 *
 * Fetch definition for this field.
 *
 * This method is the same as the C function OGR_F_GetFieldDefnRef().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field definition (from the OGRFeatureDefn).  This is an
 * internal reference, and should not be deleted or modified.
 */

/************************************************************************/
/*                       OGR_F_GetFieldDefnRef()                        */
/************************************************************************/

/**
 * Fetch definition for this field.
 *
 * This function is the same as the CPP method OGRFeature::GetFieldDefnRef().
 *
 * @param hFeat handle to the feature on which the field is found.
 * @param i the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return an handle to the field definition (from the OGRFeatureDefn).
 * This is an internal reference, and should not be deleted or modified.
 */

OGRFieldDefnH OGR_F_GetFieldDefnRef( OGRFeatureH hFeat, int i )

{
    return (OGRFieldDefnH) ((OGRFeature *) hFeat)->GetFieldDefnRef(i);
}

/************************************************************************/
/*                           GetFieldIndex()                            */
/************************************************************************/

/**
 * \fn int OGRFeature::GetFieldIndex( const char * pszName );
 *
 * Fetch the field index given field name.
 *
 * This is a cover for the OGRFeatureDefn::GetFieldIndex() method.
 *
 * This method is the same as the C function OGR_F_GetFieldIndex().
 *
 * @param pszName the name of the field to search for.
 *
 * @return the field index, or -1 if no matching field is found.
 */

/************************************************************************/
/*                        OGR_F_GetFieldIndex()                         */
/************************************************************************/

/**
 * Fetch the field index given field name.
 *
 * This is a cover for the OGRFeatureDefn::GetFieldIndex() method.
 *
 * This function is the same as the CPP method OGRFeature::GetFieldIndex().
 *
 * @param hFeat handle to the feature on which the field is found.
 * @param pszName the name of the field to search for.
 *
 * @return the field index, or -1 if no matching field is found.
 */

int OGR_F_GetFieldIndex( OGRFeatureH hFeat, const char *pszName )

{
    return ((OGRFeature *) hFeat)->GetFieldIndex( pszName );
}

/************************************************************************/
/*                             IsFieldSet()                             */
/************************************************************************/

/**
 * \fn int OGRFeature::IsFieldSet( int iField );
 *
 * Test if a field has ever been assigned a value or not.
 *
 * This method is the same as the C function OGR_F_IsFieldSet().
 *
 * @param iField the field to test.
 *
 * @return TRUE if the field has been set, otherwise false.
 */

/************************************************************************/
/*                          OGR_F_IsFieldSet()                          */
/************************************************************************/

/**
 * Test if a field has ever been assigned a value or not.
 *
 * This function is the same as the CPP method OGRFeature::IsFieldSet().
 *
 * @param hFeat handle to the feature on which the field is.
 * @param iField the field to test.
 *
 * @return TRUE if the field has been set, otherwise false.
 */

int OGR_F_IsFieldSet( OGRFeatureH hFeat, int iField )

{
    return ((OGRFeature *)hFeat)->IsFieldSet( iField );
}

/************************************************************************/
/*                             UnsetField()                             */
/************************************************************************/

/**
 * Clear a field, marking it as unset.
 *
 * This method is the same as the C function OGR_F_UnsetField().
 *
 * @param iField the field to unset.
 */

void OGRFeature::UnsetField( int iField )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL || !IsFieldSet(iField) )
        return;

    switch( poFDefn->GetType() )
    {
      case OFTRealList:
      case OFTIntegerList:
        CPLFree( pauFields[iField].IntegerList.paList );
        break;

      case OFTStringList:
        CSLDestroy( pauFields[iField].StringList.paList );
        break;

      case OFTString:
        CPLFree( pauFields[iField].String );
        break;

      default:
        break;
    }

    pauFields[iField].Set.nMarker1 = OGRUnsetMarker;
    pauFields[iField].Set.nMarker2 = OGRUnsetMarker;
}

/************************************************************************/
/*                          OGR_F_UnsetField()                          */
/************************************************************************/

/**
 * Clear a field, marking it as unset.
 *
 * This function is the same as the CPP method OGRFeature::UnsetField().
 *
 * @param hFeat handle to the feature on which the field is.
 * @param iField the field to unset.
 */

void OGR_F_UnsetField( OGRFeatureH hFeat, int iField )

{
    ((OGRFeature *) hFeat)->UnsetField( iField );
}

/************************************************************************/
/*                           GetRawFieldRef()                           */
/************************************************************************/

/**
 * \fn OGRField *OGRFeature::GetRawFieldRef( int iField );
 *
 * Fetch a pointer to the internal field value given the index.
 *
 * This method is the same as the C function OGR_F_GetRawFieldRef().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the returned pointer is to an internal data structure, and should
 * not be freed, or modified.
 */

/************************************************************************/
/*                        OGR_F_GetRawFieldRef()                        */
/************************************************************************/

/**
 * Fetch an handle to the internal field value given the index.
 *
 * This function is the same as the CPP method OGRFeature::GetRawFieldRef().
 *
 * @param hFeat handle to the feature on which field is found.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the returned handle is to an internal data structure, and should
 * not be freed, or modified.
 */

OGRField *OGR_F_GetRawFieldRef( OGRFeatureH hFeat, int iField )

{
    return ((OGRFeature *)hFeat)->GetRawFieldRef( iField );
}

/************************************************************************/
/*                         GetFieldAsInteger()                          */
/************************************************************************/

/**
 * Fetch field value as integer.
 *
 * OFTString features will be translated using atoi().  OFTReal fields
 * will be cast to integer.   Other field types, or errors will result in
 * a return value of zero.
 *
 * This method is the same as the C function OGR_F_GetFieldAsInteger().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.
 */

int OGRFeature::GetFieldAsInteger( int iField )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return 0;

    if( !IsFieldSet(iField) )
        return 0;

    if( poFDefn->GetType() == OFTInteger )
        return pauFields[iField].Integer;
    else if( poFDefn->GetType() == OFTReal )
        return (int) pauFields[iField].Real;
    else if( poFDefn->GetType() == OFTString )
    {
        if( pauFields[iField].String == NULL )
            return 0;
        else
            return atoi(pauFields[iField].String);
    }
    else
        return 0;
}

/************************************************************************/
/*                      OGR_F_GetFieldAsInteger()                       */
/************************************************************************/

/**
 * Fetch field value as integer.
 *
 * OFTString features will be translated using atoi().  OFTReal fields
 * will be cast to integer.   Other field types, or errors will result in
 * a return value of zero.
 *
 * This function is the same as the CPP method OGRFeature::GetFieldAsInteger().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.
 */

int OGR_F_GetFieldAsInteger( OGRFeatureH hFeat, int iField )

{
    return ((OGRFeature *)hFeat)->GetFieldAsInteger(iField);
}

/************************************************************************/
/*                          GetFieldAsDouble()                          */
/************************************************************************/

/**
 * Fetch field value as a double.
 *
 * OFTString features will be translated using atof().  OFTInteger fields
 * will be cast to double.   Other field types, or errors will result in
 * a return value of zero.
 *
 * This method is the same as the C function OGR_F_GetFieldAsDouble().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.
 */

double OGRFeature::GetFieldAsDouble( int iField )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return 0.0;

    if( !IsFieldSet(iField) )
        return 0.0;

    if( poFDefn->GetType() == OFTReal )
        return pauFields[iField].Real;
    else if( poFDefn->GetType() == OFTInteger )
        return pauFields[iField].Integer;
    else if( poFDefn->GetType() == OFTString )
    {
        if( pauFields[iField].String == NULL )
            return 0;
        else
            return atof(pauFields[iField].String);
    }
    else
        return 0.0;
}

/************************************************************************/
/*                       OGR_F_GetFieldAsDouble()                       */
/************************************************************************/

/**
 * Fetch field value as a double.
 *
 * OFTString features will be translated using atof().  OFTInteger fields
 * will be cast to double.   Other field types, or errors will result in
 * a return value of zero.
 *
 * This function is the same as the CPP method OGRFeature::GetFieldAsDouble().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.
 */

double OGR_F_GetFieldAsDouble( OGRFeatureH hFeat, int iField )

{
    return ((OGRFeature *)hFeat)->GetFieldAsDouble(iField);
}

/************************************************************************/
/*                          GetFieldAsString()                          */
/************************************************************************/

/**
 * Fetch field value as a string.
 *
 * OFTReal and OFTInteger fields will be translated to string using
 * sprintf(), but not necessarily using the established formatting rules.
 * Other field types, or errors will result in a return value of zero.
 *
 * This method is the same as the C function OGR_F_GetFieldAsString().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.  This string is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.
 */

const char *OGRFeature::GetFieldAsString( int iField )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );
    static char         szTempBuffer[160];
    unsigned int max_line = 80;

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return "";

    if( !IsFieldSet(iField) )
        return "";

    if( poFDefn->GetType() == OFTString )
    {
        if( pauFields[iField].String == NULL )
            return "";
        else
            return pauFields[iField].String;
    }
    else if( poFDefn->GetType() == OFTInteger )
    {
        sprintf( szTempBuffer, "%d", pauFields[iField].Integer );
        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        char    szFormat[64];

        if( poFDefn->GetWidth() != 0 )
        {
            sprintf( szFormat, "%%%d.%df",
                     poFDefn->GetWidth(), poFDefn->GetPrecision() );
        }
        else
            strcpy( szFormat, "%.16g" );

        sprintf( szTempBuffer, szFormat, pauFields[iField].Real );

        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTIntegerList )
    {
        char    szItem[32];
        int     i, nCount = pauFields[iField].IntegerList.nCount;

        sprintf( szTempBuffer, "(%d:", nCount );
        for( i = 0; i < nCount; i++ )
        {
            sprintf( szItem, "%d", pauFields[iField].IntegerList.paList[i] );
            if( strlen(szTempBuffer) + strlen(szItem) + 6 + 1
                > max_line/*sizeof(szTempBuffer)*/ )
            {
                break;
            }

            if( i > 0 )
                strcat( szTempBuffer, "," );

            strcat( szTempBuffer, szItem );
        }

        if( i < nCount )
            strcat( szTempBuffer, ",...)" );
        else
            strcat( szTempBuffer, ")" );

        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTRealList )
    {
        char    szItem[40];
        char    szFormat[64];
        int     i, nCount = pauFields[iField].RealList.nCount;

        if( poFDefn->GetWidth() != 0 )
        {
            sprintf( szFormat, "%%%d.%df",
                     poFDefn->GetWidth(), poFDefn->GetPrecision() );
        }
        else
            strcpy( szFormat, "%.16g" );

        sprintf( szTempBuffer, "(%d:", nCount );
        for( i = 0; i < nCount; i++ )
        {
            sprintf( szItem, szFormat, pauFields[iField].RealList.paList[i] );
            if( strlen(szTempBuffer) + strlen(szItem) + 6 + 1
                > max_line/*sizeof(szTempBuffer)*/ )
            {
                break;
            }

            if( i > 0 )
                strcat( szTempBuffer, "," );

            strcat( szTempBuffer, szItem );
        }

        if( i < nCount )
            strcat( szTempBuffer, ",...)" );
        else
            strcat( szTempBuffer, ")" );

        return szTempBuffer;
    }
    else if( poFDefn->GetType() == OFTStringList )
    {
        int     i, nCount = pauFields[iField].StringList.nCount;

        sprintf( szTempBuffer, "(%d:", nCount );
        for( i = 0; i < nCount; i++ )
        {
            const char  *pszItem = pauFields[iField].StringList.paList[i];

            if( strlen(szTempBuffer) + strlen(pszItem)  + 6 + 1
                > max_line/*sizeof(szTempBuffer)*/ )
            {
                break;
            }

            if( i > 0 )
                strcat( szTempBuffer, "," );

            strcat( szTempBuffer, pszItem );
        }

        if( i < nCount )
            strcat( szTempBuffer, ",...)" );
        else
            strcat( szTempBuffer, ")" );

        return szTempBuffer;
    }
    else
        return "";
}

/************************************************************************/
/*                       OGR_F_GetFieldAsString()                       */
/************************************************************************/

/**
 * Fetch field value as a string.
 *
 * OFTReal and OFTInteger fields will be translated to string using
 * sprintf(), but not necessarily using the established formatting rules.
 * Other field types, or errors will result in a return value of zero.
 *
 * This function is the same as the CPP method OGRFeature::GetFieldAsString().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.  This string is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.
 */

const char *OGR_F_GetFieldAsString( OGRFeatureH hFeat, int iField )

{
    return ((OGRFeature *)hFeat)->GetFieldAsString(iField);
}

/************************************************************************/
/*                       GetFieldAsIntegerList()                        */
/************************************************************************/

/**
 * Fetch field value as a list of integers.
 *
 * Currently this method only works for OFTIntegerList fields.
 *
 * This method is the same as the C function OGR_F_GetFieldAsIntegerList().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pnCount an integer to put the list count (number of integers) into.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.  If *pnCount is zero
 * on return the returned pointer may be NULL or non-NULL.
 */

const int *OGRFeature::GetFieldAsIntegerList( int iField, int *pnCount )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return NULL;

    if( !IsFieldSet(iField) )
        return NULL;

    if( poFDefn->GetType() == OFTIntegerList )
    {
        if( pnCount != NULL )
            *pnCount = pauFields[iField].IntegerList.nCount;

        return pauFields[iField].IntegerList.paList;
    }
    else
    {
        if( pnCount != NULL )
            *pnCount = 0;

        return NULL;
    }
}

/************************************************************************/
/*                    OGR_F_GetFieldAsIntegerList()                     */
/************************************************************************/

/**
 * Fetch field value as a list of integers.
 *
 * Currently this function only works for OFTIntegerList fields.
 *
 * This function is the same as the CPP method
 * OGRFeature::GetFieldAsIntegerList().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pnCount an integer to put the list count (number of integers) into.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.  If *pnCount is zero
 * on return the returned pointer may be NULL or non-NULL.
 */

const int *OGR_F_GetFieldAsIntegerList( OGRFeatureH hFeat, int iField,
                                  int *pnCount )

{
    return ((OGRFeature *)hFeat)->GetFieldAsIntegerList(iField, pnCount);
}

/************************************************************************/
/*                        GetFieldAsDoubleList()                        */
/************************************************************************/

/**
 * Fetch field value as a list of doubles.
 *
 * Currently this method only works for OFTRealList fields.
 *
 * This method is the same as the C function OGR_F_GetFieldAsDoubleList().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pnCount an integer to put the list count (number of doubles) into.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.  If *pnCount is zero
 * on return the returned pointer may be NULL or non-NULL.
 */

const double *OGRFeature::GetFieldAsDoubleList( int iField, int *pnCount )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return NULL;

    if( !IsFieldSet(iField) )
        return NULL;

    if( poFDefn->GetType() == OFTRealList )
    {
        if( pnCount != NULL )
            *pnCount = pauFields[iField].RealList.nCount;

        return pauFields[iField].RealList.paList;
    }
    else
    {
        if( pnCount != NULL )
            *pnCount = 0;

        return NULL;
    }
}

/************************************************************************/
/*                     OGR_F_GetFieldAsDoubleList()                     */
/************************************************************************/

/**
 * Fetch field value as a list of doubles.
 *
 * Currently this function only works for OFTRealList fields.
 *
 * This function is the same as the CPP method
 * OGRFeature::GetFieldAsDoubleList().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pnCount an integer to put the list count (number of doubles) into.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.  If *pnCount is zero
 * on return the returned pointer may be NULL or non-NULL.
 */

const double *OGR_F_GetFieldAsDoubleList( OGRFeatureH hFeat, int iField,
                                          int *pnCount )

{
    return ((OGRFeature *)hFeat)->GetFieldAsDoubleList(iField, pnCount);
}

/************************************************************************/
/*                        GetFieldAsStringList()                        */
/************************************************************************/

/**
 * Fetch field value as a list of strings.
 *
 * Currently this method only works for OFTStringList fields.
 *
 * This method is the same as the C function OGR_F_GetFieldAsStringList().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.
 */

char **OGRFeature::GetFieldAsStringList( int iField )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return NULL;

    if( !IsFieldSet(iField) )
        return NULL;

    if( poFDefn->GetType() == OFTStringList )
    {
        return pauFields[iField].StringList.paList;
    }
    else
    {
        return NULL;
    }
}

/************************************************************************/
/*                     OGR_F_GetFieldAsStringList()                     */
/************************************************************************/

/**
 * Fetch field value as a list of strings.
 *
 * Currently this method only works for OFTStringList fields.
 *
 * This function is the same as the CPP method
 * OGRFeature::GetFieldAsStringList().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 *
 * @return the field value.  This list is internal, and should not be
 * modified, or freed.  It's lifetime may be very brief.
 */

char **OGR_F_GetFieldAsStringList( OGRFeatureH hFeat, int iField )

{
    return ((OGRFeature *)hFeat)->GetFieldAsStringList(iField);
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to integer value.
 *
 * OFTInteger and OFTReal fields will be set directly.  OFTString fields
 * will be assigned a string representation of the value, but not necessarily
 * taking into account formatting constraints on this field.  Other field
 * types may be unaffected.
 *
 * This method is the same as the C function OGR_F_SetFieldInteger().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param nValue the value to assign.
 */

void OGRFeature::SetField( int iField, int nValue )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return;

    if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField].Integer = nValue;
        pauFields[iField].Set.nMarker2 = 0;
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField].Real = nValue;
    }
    else if( poFDefn->GetType() == OFTString )
    {
        char    szTempBuffer[64];

        sprintf( szTempBuffer, "%d", nValue );

        if( IsFieldSet( iField) )
            CPLFree( pauFields[iField].String );

        pauFields[iField].String = CPLStrdup( szTempBuffer );
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                       OGR_F_SetFieldInteger()                        */
/************************************************************************/

/**
 * Set field to integer value.
 *
 * OFTInteger and OFTReal fields will be set directly.  OFTString fields
 * will be assigned a string representation of the value, but not necessarily
 * taking into account formatting constraints on this field.  Other field
 * types may be unaffected.
 *
 * This function is the same as the CPP method OGRFeature::SetField().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param nValue the value to assign.
 */

void OGR_F_SetFieldInteger( OGRFeatureH hFeat, int iField, int nValue )

{
    ((OGRFeature *)hFeat)->SetField( iField, nValue );
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to double value.
 *
 * OFTInteger and OFTReal fields will be set directly.  OFTString fields
 * will be assigned a string representation of the value, but not necessarily
 * taking into account formatting constraints on this field.  Other field
 * types may be unaffected.
 *
 * This method is the same as the C function OGR_F_SetFieldDouble().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param dfValue the value to assign.
 */

void OGRFeature::SetField( int iField, double dfValue )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return;

    if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField].Real = dfValue;
    }
    else if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField].Integer = (int) dfValue;
        pauFields[iField].Set.nMarker2 = 0;
    }
    else if( poFDefn->GetType() == OFTString )
    {
        char    szTempBuffer[128];

        sprintf( szTempBuffer, "%.16g", dfValue );

        if( IsFieldSet( iField) )
            CPLFree( pauFields[iField].String );

        pauFields[iField].String = CPLStrdup( szTempBuffer );
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                        OGR_F_SetFieldDouble()                        */
/************************************************************************/

/**
 * Set field to double value.
 *
 * OFTInteger and OFTReal fields will be set directly.  OFTString fields
 * will be assigned a string representation of the value, but not necessarily
 * taking into account formatting constraints on this field.  Other field
 * types may be unaffected.
 *
 * This function is the same as the CPP method OGRFeature::SetField().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param dfValue the value to assign.
 */

void OGR_F_SetFieldDouble( OGRFeatureH hFeat, int iField, double dfValue )

{
    ((OGRFeature *)hFeat)->SetField( iField, dfValue );
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to string value.
 *
 * OFTInteger fields will be set based on an atoi() conversion of the string.
 * OFTReal fields will be set based on an atof() conversion of the string.
 * Other field types may be unaffected.
 *
 * This method is the same as the C function OGR_F_SetFieldString().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pszValue the value to assign.
 */

void OGRFeature::SetField( int iField, const char * pszValue )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return;

    if( poFDefn->GetType() == OFTString )
    {
        if( IsFieldSet(iField) )
            CPLFree( pauFields[iField].String );

        pauFields[iField].String = CPLStrdup( pszValue );
    }
    else if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField].Integer = atoi(pszValue);
        pauFields[iField].Set.nMarker2 = OGRUnsetMarker;
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField].Real = atof(pszValue);
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                        OGR_F_SetFieldString()                        */
/************************************************************************/

/**
 * Set field to string value.
 *
 * OFTInteger fields will be set based on an atoi() conversion of the string.
 * OFTReal fields will be set based on an atof() conversion of the string.
 * Other field types may be unaffected.
 *
 * This function is the same as the CPP method OGRFeature::SetField().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param pszValue the value to assign.
 */

void OGR_F_SetFieldString( OGRFeatureH hFeat, int iField, const char *pszValue)

{
    ((OGRFeature *)hFeat)->SetField( iField, pszValue );
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to list of integers value.
 *
 * This method currently on has an effect of OFTIntegerList fields.
 *
 * This method is the same as the C function OGR_F_SetFieldIntegerList().
 *
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param nCount the number of values in the list being assigned.
 * @param panValues the values to assign.
 */

void OGRFeature::SetField( int iField, int nCount, int *panValues )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return;

    if( poFDefn->GetType() == OFTIntegerList )
    {
        OGRField        uField;

        uField.IntegerList.nCount = nCount;
        uField.IntegerList.paList = panValues;

        SetField( iField, &uField );
    }
}

/************************************************************************/
/*                     OGR_F_SetFieldIntegerList()                      */
/************************************************************************/

/**
 * Set field to list of integers value.
 *
 * This function currently on has an effect of OFTIntegerList fields.
 *
 * This function is the same as the CPP method OGRFeature::SetField().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param nCount the number of values in the list being assigned.
 * @param panValues the values to assign.
 */

void OGR_F_SetFieldIntegerList( OGRFeatureH hFeat, int iField,
                                int nCount, int *panValues )

{
    ((OGRFeature *)hFeat)->SetField( iField, nCount, panValues );
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to list of doubles value.
 *
 * This method currently on has an effect of OFTRealList fields.
 *
 * This method is the same as the C function OGR_F_SetFieldDoubleList().
 *
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param nCount the number of values in the list being assigned.
 * @param padfValues the values to assign.
 */

void OGRFeature::SetField( int iField, int nCount, double * padfValues )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return;

    if( poFDefn->GetType() == OFTRealList )
    {
        OGRField        uField;

        uField.RealList.nCount = nCount;
        uField.RealList.paList = padfValues;

        SetField( iField, &uField );
    }
}

/************************************************************************/
/*                      OGR_F_SetFieldDoubleList()                      */
/************************************************************************/

/**
 * Set field to list of doubles value.
 *
 * This function currently on has an effect of OFTRealList fields.
 *
 * This function is the same as the CPP method OGRFeature::SetField().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param nCount the number of values in the list being assigned.
 * @param padfValues the values to assign.
 */

void OGR_F_SetFieldDoubleList( OGRFeatureH hFeat, int iField,
                               int nCount, double *padfValues )

{
    ((OGRFeature *)hFeat)->SetField( iField, nCount, padfValues );
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field to list of strings value.
 *
 * This method currently on has an effect of OFTStringList fields.
 *
 * This method is the same as the C function OGR_F_SetFieldStringList().
 *
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param papszValues the values to assign.
 */

void OGRFeature::SetField( int iField, char ** papszValues )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return;

    if( poFDefn->GetType() == OFTStringList )
    {
        OGRField        uField;

        uField.StringList.nCount = CSLCount(papszValues);
        uField.StringList.paList = papszValues;

        SetField( iField, &uField );
    }
}

/************************************************************************/
/*                      OGR_F_SetFieldStringList()                      */
/************************************************************************/

/**
 * Set field to list of strings value.
 *
 * This function currently on has an effect of OFTStringList fields.
 *
 * This function is the same as the CPP method OGRFeature::SetField().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to set, from 0 to GetFieldCount()-1.
 * @param papszValues the values to assign.
 */

void OGR_F_SetFieldStringList( OGRFeatureH hFeat, int iField,
                               char ** papszValues )

{
    ((OGRFeature *)hFeat)->SetField( iField, papszValues );
}

/************************************************************************/
/*                              SetField()                              */
/************************************************************************/

/**
 * Set field.
 *
 * The passed value OGRField must be of exactly the same type as the
 * target field, or an application crash may occur.  The passed value
 * is copied, and will not be affected.  It remains the responsibility of
 * the caller.
 *
 * This method is the same as the C function OGR_F_SetFieldRaw().
 *
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param puValue the value to assign.
 */

void OGRFeature::SetField( int iField, OGRField * puValue )

{
    OGRFieldDefn        *poFDefn = poDefn->GetFieldDefn( iField );

    CPLAssert( poFDefn != NULL || iField == -1 );
    if( poFDefn == NULL )
        return;

    if( poFDefn->GetType() == OFTInteger )
    {
        pauFields[iField] = *puValue;
    }
    else if( poFDefn->GetType() == OFTReal )
    {
        pauFields[iField] = *puValue;
    }
    else if( poFDefn->GetType() == OFTString )
    {
        if( IsFieldSet( iField ) )
            CPLFree( pauFields[iField].String );

        if( puValue->String == NULL )
            pauFields[iField].String = NULL;
        else if( puValue->Set.nMarker1 == OGRUnsetMarker
                 && puValue->Set.nMarker2 == OGRUnsetMarker )
            pauFields[iField] = *puValue;
        else
            pauFields[iField].String = CPLStrdup( puValue->String );
    }
    else if( poFDefn->GetType() == OFTIntegerList )
    {
        int     nCount = puValue->IntegerList.nCount;

        if( IsFieldSet( iField ) )
            CPLFree( pauFields[iField].IntegerList.paList );

        if( puValue->Set.nMarker1 == OGRUnsetMarker
            && puValue->Set.nMarker2 == OGRUnsetMarker )
        {
            pauFields[iField] = *puValue;
        }
        else
        {
            pauFields[iField].IntegerList.paList =
                (int *) CPLMalloc(sizeof(int) * nCount);
            memcpy( pauFields[iField].IntegerList.paList,
                    puValue->IntegerList.paList,
                    sizeof(int) * nCount );
            pauFields[iField].IntegerList.nCount = nCount;
        }
    }
    else if( poFDefn->GetType() == OFTRealList )
    {
        int     nCount = puValue->RealList.nCount;

        if( IsFieldSet( iField ) )
            CPLFree( pauFields[iField].RealList.paList );

        if( puValue->Set.nMarker1 == OGRUnsetMarker
            && puValue->Set.nMarker2 == OGRUnsetMarker )
        {
            pauFields[iField] = *puValue;
        }
        else
        {
            pauFields[iField].RealList.paList =
                (double *) CPLMalloc(sizeof(double) * nCount);
            memcpy( pauFields[iField].RealList.paList,
                    puValue->RealList.paList,
                    sizeof(double) * nCount );
            pauFields[iField].RealList.nCount = nCount;
        }
    }
    else if( poFDefn->GetType() == OFTStringList )
    {
        if( IsFieldSet( iField ) )
            CSLDestroy( pauFields[iField].StringList.paList );

        if( puValue->Set.nMarker1 == OGRUnsetMarker
            && puValue->Set.nMarker2 == OGRUnsetMarker )
        {
            pauFields[iField] = *puValue;
        }
        else
        {
            pauFields[iField].StringList.paList =
                CSLDuplicate( puValue->StringList.paList );

            pauFields[iField].StringList.nCount = puValue->StringList.nCount;
            CPLAssert( CSLCount(puValue->StringList.paList)
                       == puValue->StringList.nCount );
        }
    }
    else
        /* do nothing for other field types */;
}

/************************************************************************/
/*                      OGR_F_SetFieldRaw()                             */
/************************************************************************/

/**
 * Set field.
 *
 * The passed value OGRField must be of exactly the same type as the
 * target field, or an application crash may occur.  The passed value
 * is copied, and will not be affected.  It remains the responsibility of
 * the caller.
 *
 * This function is the same as the CPP method OGRFeature::SetField().
 *
 * @param hFeat handle to the feature that owned the field.
 * @param iField the field to fetch, from 0 to GetFieldCount()-1.
 * @param psValue handle on the value to assign.
 */

void OGR_F_SetFieldRaw( OGRFeatureH hFeat, int iField, OGRField *psValue )

{
    ((OGRFeature *)hFeat)->SetField( iField, psValue );
}

/************************************************************************/
/*                            DumpReadable()                            */
/************************************************************************/

/**
 * Dump this feature in a human readable form.
 *
 * This dumps the attributes, and geometry; however, it doesn't definition
 * information (other than field types and names), nor does it report the
 * geometry spatial reference system.
 *
 * This method is the same as the C function OGR_F_DumpReadable().
 *
 * @param fpOut the stream to write to, such as strout.
 */

void OGRFeature::DumpReadable( FILE * fpOut )

{
    if( fpOut == NULL )
        fpOut = stdout;

    fprintf( fpOut, "OGRFeature(%s):%ld\n", poDefn->GetName(), GetFID() );
    for( int iField = 0; iField < GetFieldCount(); iField++ )
    {
        OGRFieldDefn    *poFDefn = poDefn->GetFieldDefn(iField);

        fprintf( fpOut, "  %s (%s) = ",
                 poFDefn->GetNameRef(),
                 OGRFieldDefn::GetFieldTypeName(poFDefn->GetType()) );

        if( IsFieldSet( iField ) )
            fprintf( fpOut, "%s\n", GetFieldAsString( iField ) );
        else
            fprintf( fpOut, "(null)\n" );

    }

    if( GetStyleString() != NULL )
        fprintf( fpOut, "  Style = %s\n", GetStyleString() );

    if( poGeometry != NULL )
        poGeometry->dumpReadable( fpOut, "  " );

    fprintf( fpOut, "\n" );
}

/************************************************************************/
/*                         OGR_F_DumpReadable()                         */
/************************************************************************/

/**
 * Dump this feature in a human readable form.
 *
 * This dumps the attributes, and geometry; however, it doesn't definition
 * information (other than field types and names), nor does it report the
 * geometry spatial reference system.
 *
 * This function is the same as the CPP method OGRFeature::DumpReadable().
 *
 * @param hFeat handle to the feature to dump.
 * @param fpOut the stream to write to, such as strout.
 */

void OGR_F_DumpReadable( OGRFeatureH hFeat, FILE *fpOut )

{
    ((OGRFeature *) hFeat)->DumpReadable( fpOut );
}

/************************************************************************/
/*                               GetFID()                               */
/************************************************************************/

/**
 * \fn long OGRFeature::GetFID();
 *
 * Get feature identifier.
 *
 * This method is the same as the C function OGR_F_GetFID().
 *
 * @return feature id or OGRNullFID if none has been assigned.
 */

/************************************************************************/
/*                            OGR_F_GetFID()                            */
/************************************************************************/

/**
 * Get feature identifier.
 *
 * This function is the same as the CPP method OGRFeature::GetFID().
 *
 * @param hFeat handle to the feature from which to get the feature
 * identifier.
 * @return feature id or OGRNullFID if none has been assigned.
 */

long OGR_F_GetFID( OGRFeatureH hFeat )

{
    return ((OGRFeature *) hFeat)->GetFID();
}

/************************************************************************/
/*                               SetFID()                               */
/************************************************************************/

/**
 * Set the feature identifier.
 *
 * For specific types of features this operation may fail on illegal
 * features ids.  Generally it always succeeds.  Feature ids should be
 * greater than or equal to zero, with the exception of OGRNullFID (-1)
 * indicating that the feature id is unknown.
 *
 * This method is the same as the C function OGR_F_SetFID().
 *
 * @param nFID the new feature identifier value to assign.
 *
 * @return On success OGRERR_NONE, or on failure some other value.
 */

OGRErr OGRFeature::SetFID( long nFID )

{
    this->nFID = nFID;

    return OGRERR_NONE;
}

/************************************************************************/
/*                            OGR_F_SetFID()                            */
/************************************************************************/

/**
 * Set the feature identifier.
 *
 * For specific types of features this operation may fail on illegal
 * features ids.  Generally it always succeeds.  Feature ids should be
 * greater than or equal to zero, with the exception of OGRNullFID (-1)
 * indicating that the feature id is unknown.
 *
 * This function is the same as the CPP method OGRFeature::SetFID().
 *
 * @param hFeat handle to the feature to set the feature id to.
 * @param nFID the new feature identifier value to assign.
 *
 * @return On success OGRERR_NONE, or on failure some other value.
 */

OGRErr OGR_F_SetFID( OGRFeatureH hFeat, long nFID )

{
    return ((OGRFeature *) hFeat)->SetFID(nFID);
}

/************************************************************************/
/*                               Equal()                                */
/************************************************************************/

/**
 * Test if two features are the same.
 *
 * Two features are considered equal if the share them (pointer equality)
 * same OGRFeatureDefn, have the same field values, and the same geometry
 * (as tested by OGRGeometry::Equal()) as well as the same feature id.
 *
 * This method is the same as the C function OGR_F_Equal().
 *
 * @param poFeature the other feature to test this one against.
 *
 * @return TRUE if they are equal, otherwise FALSE.
 */

OGRBoolean OGRFeature::Equal( OGRFeature * poFeature )

{
    if( poFeature == this )
        return TRUE;

    if( GetFID() != poFeature->GetFID() )
        return FALSE;

    if( GetDefnRef() != poFeature->GetDefnRef() )
        return FALSE;

    //notdef: add testing of attributes at a later date.

    if( GetGeometryRef() != NULL
        && (!GetGeometryRef()->Equal( poFeature->GetGeometryRef() ) ) )
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                            OGR_F_Equal()                             */
/************************************************************************/

/**
 * Test if two features are the same.
 *
 * Two features are considered equal if the share them (handle equality)
 * same OGRFeatureDefn, have the same field values, and the same geometry
 * (as tested by OGR_G_Equal()) as well as the same feature id.
 *
 * This function is the same as the CPP method OGRFeature::Equal().
 *
 * @param hFeat handle to one of the feature.
 * @param hOtherFeat handle to the other feature to test this one against.
 *
 * @return TRUE if they are equal, otherwise FALSE.
 */

int OGR_F_Equal( OGRFeatureH hFeat, OGRFeatureH hOtherFeat )

{
    return ((OGRFeature *) hFeat)->Equal( (OGRFeature *) hOtherFeat );
}


/************************************************************************/
/*                              SetFrom()                               */
/************************************************************************/

/**
 * Set one feature from another.
 *
 * Overwrite the contents of this feature from the geometry and attributes
 * of another.  The poSrcFeature does not need to have the same
 * OGRFeatureDefn.  Field values are copied by corresponding field names.
 * Field types do not have to exactly match.  SetField() method conversion
 * rules will be applied as needed.
 *
 * This method is the same as the C function OGR_F_SetFrom().
 *
 * @param poSrcFeature the feature from which geometry, and field values will
 * be copied.
 *
 * @param bForgiving TRUE if the operation should continue despite lacking
 * output fields matching some of the source fields.
 *
 * @return OGRERR_NONE if the operation succeeds, even if some values are
 * not transferred, otherwise an error code.
 */

OGRErr OGRFeature::SetFrom( OGRFeature * poSrcFeature, int bForgiving )

{
    OGRErr      eErr;

    SetFID( OGRNullFID );

/* -------------------------------------------------------------------- */
/*      Set the geometry.                                               */
/* -------------------------------------------------------------------- */
    eErr = SetGeometry( poSrcFeature->GetGeometryRef() );
    if( eErr != OGRERR_NONE )
        return eErr;

/* -------------------------------------------------------------------- */
/*      Copy feature style string.                                      */
/* -------------------------------------------------------------------- */
    if( poSrcFeature->GetStyleString() != NULL )
        SetStyleString( poSrcFeature->GetStyleString() );

/* -------------------------------------------------------------------- */
/*      Set the fields by name.                                         */
/* -------------------------------------------------------------------- */
    int         iField, iDstField;

    for( iField = 0; iField < poSrcFeature->GetFieldCount(); iField++ )
    {
        iDstField = GetFieldIndex(
            poSrcFeature->GetFieldDefnRef(iField)->GetNameRef() );

        if( iDstField == -1 )
        {
            if( bForgiving )
                continue;
            else
                return OGRERR_FAILURE;
        }

        if( !poSrcFeature->IsFieldSet(iField) )
        {
            UnsetField( iDstField );
            continue;
        }

        switch( poSrcFeature->GetFieldDefnRef(iField)->GetType() )
        {
          case OFTInteger:
            SetField( iDstField, poSrcFeature->GetFieldAsInteger( iField ) );
            break;

          case OFTReal:
            SetField( iDstField, poSrcFeature->GetFieldAsDouble( iField ) );
            break;

          case OFTString:
            SetField( iDstField, poSrcFeature->GetFieldAsString( iField ) );
            break;

          default:
            if( poSrcFeature->GetFieldDefnRef(iField)->GetType()
                == GetFieldDefnRef(iDstField)->GetType() )
            {
                SetField( iDstField, poSrcFeature->GetRawFieldRef(iField) );
            }
            else if( !bForgiving )
                return OGRERR_FAILURE;
            break;
        }
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                           OGR_F_SetFrom()                            */
/************************************************************************/

/**
 * Set one feature from another.
 *
 * Overwrite the contents of this feature from the geometry and attributes
 * of another.  The hOtherFeature does not need to have the same
 * OGRFeatureDefn.  Field values are copied by corresponding field names.
 * Field types do not have to exactly match.  OGR_F_SetField*() function
 * conversion rules will be applied as needed.
 *
 * This function is the same as the CPP method OGRFeature::SetFrom().
 *
 * @param hFeat handle to the feature to set to.
 * @param hOtherFeat handle to the feature from which geometry,
 * and field values will be copied.
 *
 * @param bForgiving TRUE if the operation should continue despite lacking
 * output fields matching some of the source fields.
 *
 * @return OGRERR_NONE if the operation succeeds, even if some values are
 * not transferred, otherwise an error code.
 */

OGRErr OGR_F_SetFrom( OGRFeatureH hFeat, OGRFeatureH hOtherFeat,
                      int bForgiving )

{
    return ((OGRFeature *) hFeat)->SetFrom( (OGRFeature *) hOtherFeat,
                                           bForgiving );
}

/************************************************************************/
/*                             GetStyleString()                         */
/************************************************************************/

/**
 * Fetch style string for this feature.
 *
 * Set the OGR Feature Style Specification for details on the format of
 * this string, and ogr_featurestyle.h for services available to parse it.
 *
 * This method is the same as the C function OGR_F_GetStyleString().
 *
 * @return a reference to a representation in string format, or NULL if
 * there isn't one.
 */

const char *OGRFeature::GetStyleString()
{
    if (m_pszStyleString)
      return m_pszStyleString;
    else
      return NULL;
}

/************************************************************************/
/*                        OGR_F_GetStyleString()                        */
/************************************************************************/

/**
 * Fetch style string for this feature.
 *
 * Set the OGR Feature Style Specification for details on the format of
 * this string, and ogr_featurestyle.h for services available to parse it.
 *
 * This function is the same as the CPP method OGRFeature::GetStyleString().
 *
 * @param hFeat handle to the feature to get the style from.
 * @return a reference to a representation in string format, or NULL if
 * there isn't one.
 */

const char *OGR_F_GetStyleString( OGRFeatureH hFeat )
{
    return ((OGRFeature *)hFeat)->GetStyleString();
}

/************************************************************************/
/*                             SetStyleString()                         */
/************************************************************************/

/**
 * Set feature style string.
 *
 * This method is the same as the C function OGR_F_SetStyleString().
 *
 * @param pszString the style string to apply to this feature, cannot be NULL.
 */

void OGRFeature::SetStyleString(const char *pszString)
{
    if (m_pszStyleString)
      CPLFree(m_pszStyleString);

    m_pszStyleString = CPLStrdup(pszString);

}

/************************************************************************/
/*                        OGR_F_SetStyleString()                        */
/************************************************************************/

/**
 * Set feature style string.
 *
 * This function is the same as the CPP method OGRFeature::SetStyleString().
 *
 * @param hFeat handle to the feature to set style to.
 * @param pszStyle the style string to apply to this feature, cannot be NULL.
 */

void OGR_F_SetStyleString( OGRFeatureH hFeat, const char *pszStyle )

{
    ((OGRFeature *)hFeat)->SetStyleString( pszStyle );
}

/************************************************************************/
/*                           SetStyleTable()                            */
/************************************************************************/
void OGRFeature::SetStyleTable(OGRStyleTable *poStyleTable)
{
    m_poStyleTable = poStyleTable;
}

/************************************************************************/
/*                            RemapFields()                             */
/*                                                                      */
/*      This is used to transform a feature "in place" from one         */
/*      feature defn to another with minimum work.                      */
/************************************************************************/

OGRErr OGRFeature::RemapFields( OGRFeatureDefn *poNewDefn,
                                int *panRemapSource )

{
    int  iDstField;
    OGRField *pauNewFields;

    if( poNewDefn == NULL )
        poNewDefn = poDefn;

    pauNewFields = (OGRField *) CPLCalloc( poNewDefn->GetFieldCount(),
                                           sizeof(OGRField) );

    for( iDstField = 0; iDstField < poDefn->GetFieldCount(); iDstField++ )
    {
        if( panRemapSource[iDstField] == -1 )
        {
            pauNewFields[iDstField].Set.nMarker1 = OGRUnsetMarker;
            pauNewFields[iDstField].Set.nMarker2 = OGRUnsetMarker;
        }
        else
        {
            memcpy( pauNewFields + iDstField,
                    pauFields + panRemapSource[iDstField],
                    sizeof(OGRField) );
        }
    }

    /*
    ** We really should be freeing memory for old columns that
    ** are no longer present.  We don't for now because it is a bit messy
    ** and would take too long to test.
    */

/* -------------------------------------------------------------------- */
/*      Apply new definition and fields.                                */
/* -------------------------------------------------------------------- */
    CPLFree( pauFields );
    pauFields = pauNewFields;

    poDefn = poNewDefn;

    return OGRERR_NONE;
}
