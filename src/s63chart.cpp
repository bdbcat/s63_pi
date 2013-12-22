/******************************************************************************
 *
 * Project:
 * Purpose:
 * Author:   David Register
 *
*/
// ============================================================================
// declarations
// ============================================================================


// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers


//  Why are these not in wx/prec.h?
#include "wx/dir.h"
#include "wx/stream.h"
#include "wx/wfstream.h"
#include "wx/tokenzr.h"
#include "wx/filename.h"
#include <wx/image.h>
#include <wx/dynlib.h>
#include <wx/textfile.h>
#include <wx/process.h>

#include <sys/stat.h>

#include "s63_pi.h"
#include "s63chart.h"
//#include "pis57chart.h"
#include "mygeom63.h"
#include "georef.h"
#include "cutil.h"



#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif



extern wxString         g_sencutil_bin;
wxString                s_last_sync_error;
extern unsigned int     g_backchannel_port;
extern unsigned int     g_frontchannel_port;
extern wxString         g_user_permit;
extern wxString         g_s57data_dir;

static int              s_PI_bInS57;         // Exclusion flag to prvent recursion in this class init call.

wxDialog                *s_plogcontainer;
wxTextCtrl              *s_plogtc;
int                     nseq;

#include <wx/arrimpl.cpp>
WX_DEFINE_ARRAY( float*, MyFloatPtrArray );

//    Arrays to temporarily hold SENC geometry
WX_DEFINE_OBJARRAY(PI_ArrayOfVE_Elements);
WX_DEFINE_OBJARRAY(PI_ArrayOfVC_Elements);

// ----------------------------------------------------------------------------
// Random Prototypes
// ----------------------------------------------------------------------------

double      round_msvc (double x)
{
    return(floor(x + 0.5));
}
#define round(x) round_msvc(x)


wxArrayString exec_SENCutil_sync( wxString cmd, bool bshowlog )
{
    wxArrayString ret_array;
    
    if( bshowlog ){
        ScreenLogMessage(_T("\n"));
    }

    long rv = wxExecute(cmd, ret_array, ret_array );
    
    if(-1 == rv) {
        ret_array.Add(_T("ERROR: Could not execute OCPNsenc utility\n"));
        ret_array.Add(cmd.Mid(0, 60) + _T("...") + _T("\n"));
    }
        
    return ret_array;
}

bool exec_results_check( wxArrayString &array )
{
    for(unsigned int i=0 ; i < array.GetCount() ; i++){
        wxString line = array[i];
        if(array[i].Upper().Find(_T("ERROR")) != wxNOT_FOUND){
            s_last_sync_error = array[i];
            return false;
        }
    }
        
    return true;
}


class UtilProcess: public wxProcess
{
public:
    UtilProcess();
    ~UtilProcess();
    
    void OnTerminate(int pid, int status);
    wxString    m_outstring;
    bool        term_happened;
    
};

UtilProcess::UtilProcess()
{
    term_happened = false;
}

UtilProcess::~UtilProcess()
{
}


void UtilProcess::OnTerminate(int pid, int status)
{
    wxInputStream *pis = GetInputStream();
    if(pis){
        while(pis->CanRead())
        {
            char c = pis->GetC();
            m_outstring += c;
        }
    }
    
    term_happened = true;
    
    wxPrintf(_T("%s"), m_outstring.c_str());
    if( s_plogtc )
        ScreenLogMessage(m_outstring);
}








// ============================================================================
// ThumbData implementation
// ============================================================================
#if 0
ThumbData::ThumbData()
{
    pDIBThumb = NULL;
}

ThumbData::~ThumbData()
{
    delete pDIBThumb;
}
#endif

// ----------------------------------------------------------------------------
// ChartS63 Implementation
// ----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS(ChartS63, PlugInChartBase)


ChartS63::ChartS63()
{

    m_senc_dir =  *GetpPrivateApplicationDataLocation() ;
    m_senc_dir += wxFileName::GetPathSeparator();
    m_senc_dir += _T("S63SENC");
    
    // Create ATON arrays, needed by S52PLIB
    pFloatingATONArray = new wxArrayPtrVoid;
    pRigidATONArray = new wxArrayPtrVoid;
    
    m_ChartType = PI_CHART_TYPE_S57;
    m_ChartFamily = PI_CHART_FAMILY_VECTOR;
    
    for( int i = 0; i < PI_PRIO_NUM; i++ )
        for( int j = 0; j < PI_LUPNAME_NUM; j++ )
            razRules[i][j] = NULL;
        
    m_Chart_Scale = 1;                              // Will be fetched during Init()
    m_Chart_Skew = 0.0;
    pDIB = NULL;
    m_pCloneBM = NULL;
    
    
//      ChartBaseBSBCTOR();
#if 0
      m_depth_unit_id = PI_DEPTH_UNIT_UNKNOWN;

      m_global_color_scheme = PI_GLOBAL_COLOR_SCHEME_RGB;

      m_bReadyToRender = false;

      m_Chart_Error_Factor = 0.;

      m_Chart_Scale = 10000;              // a benign value

      m_nCOVREntries = 0;
      m_pCOVRTable = NULL;
      m_pCOVRTablePoints = NULL;

      m_EdDate.Set(1, wxDateTime::Jan, 2000);

      m_lon_datum_adjust = 0.;
      m_lat_datum_adjust = 0.;

      m_projection = PI_PROJECTION_MERCATOR;             // default

      m_ChartType = PI_CHART_TYPE_PLUGIN;
      m_ChartFamily = PI_CHART_FAMILY_VECTOR;

      m_ppartial_bytes = NULL;

      m_pBMPThumb = NULL;

      m_ecr_length = 0;
#endif
}

ChartS63::~ChartS63()
{
#if 0
      //    Free the COVR tables

      for(unsigned int j=0 ; j<(unsigned int)m_nCOVREntries ; j++)
            free( m_pCOVRTable[j] );

      free( m_pCOVRTable );
      free( m_pCOVRTablePoints );

      free(m_ppartial_bytes);

      delete m_pBMPThumb;
#endif
//      ChartBaseBSBDTOR();
      
      delete pFloatingATONArray;
      delete pRigidATONArray;
      
}







#define BUF_LEN_MAX 4000


int ChartS63::Init( const wxString& name, int init_flags )
{
    //    Use a static semaphore flag to prevent recursion
    if( s_PI_bInS57 ) {
        return PI_INIT_FAIL_NOERROR;
    }
    s_PI_bInS57++;
    
    PI_InitReturn ret_val = PI_INIT_FAIL_NOERROR;
    
    m_FullPath = name;
    m_Description = m_FullPath;

    m_ChartType = PI_CHART_TYPE_PLUGIN;
    m_ChartFamily = PI_CHART_FAMILY_VECTOR;
    m_projection = PI_PROJECTION_MERCATOR;

    //  Get the base file name
    wxFileName tfn(name);
    wxString base_name = tfn.GetName();
 
    //  Parse the metadata
  
    wxTextFile meta_file( name );
    if( meta_file.Open() ){
        wxString line = meta_file.GetFirstLine();
        
        while( !meta_file.Eof() ){
            if(line.StartsWith( _T("cellbase:" ) ) ) {
                m_full_base_path = line.Mid(9);
            }
            else if(line.StartsWith( _T("cellpermit:" ) ) ) {
                m_cell_permit = line.Mid(11);
            }
            
            line = meta_file.GetNextLine();
        }                
    }             
    else{
        s_PI_bInS57--;
        return PI_INIT_FAIL_REMOVE;
    }
                
    if( PI_HEADER_ONLY == init_flags ){
        
       //      else if the ehdr file exists, we init from there (normal path for adding cell to dB)
        wxString efn = m_senc_dir;
        efn += wxFileName::GetPathSeparator();
        efn += base_name;
        efn += _T(".ehdr");
 
        wxRemoveFile( efn);
        
        if( wxFileName::FileExists(efn) ) {
        }
        else {                          //  we need to create the ehdr file.

            // build the SENC utility command line
            wxString temp_outfile = efn; //wxFileName::CreateTempFileName( _T("") );
            
            wxString cmd = g_sencutil_bin;
            cmd += _T(" -l ");                  // create secure header
            
            cmd += _T(" -i ");
            cmd += _T("\"");
            cmd += m_full_base_path;
            cmd += _T("\"");

            cmd += _T(" -o ");
            cmd += _T("\"");
            cmd += temp_outfile;
            cmd += _T("\"");
            
            cmd += _T(" -p ");
            cmd += m_cell_permit;
            
            cmd += _T(" -u ");
            cmd += g_user_permit;
            
            cmd += _T(" -b ");
            wxString port;
            port.Printf( _T("%d"), g_backchannel_port );
            cmd += port;

            cmd += _T(" -r ");
            cmd += _T("\"");
            cmd += g_s57data_dir;
            cmd += _T("\"");
            
            wxLogMessage( cmd );
            wxArrayString ehdr_result = exec_SENCutil_sync( cmd, true);
            
            //  Check results
            if( !exec_results_check( ehdr_result ) ) {
                m_extended_error = _T("Error executing cmd: ");
                m_extended_error += cmd;
                m_extended_error += _T("\n");
                m_extended_error += s_last_sync_error;
                
                ScreenLogMessage( _T("\n") );
                for(unsigned int i=0 ; i < ehdr_result.GetCount() ; i++){
                    ScreenLogMessage( ehdr_result[i] );
                    if(!ehdr_result[i].EndsWith(_T("\n")))
                        ScreenLogMessage( _T("\n") );
                }
                
                s_PI_bInS57--;
                return PI_INIT_FAIL_REMOVE;
            }
        }
        
        if( wxFileName::FileExists(efn) ) {
            //      We have the ehdr, so use it to init the chart
            bool init_result = InitFrom_ehdr( efn );
            if( init_result ) {
                ret_val = PI_INIT_OK;
                
                m_bReadyToRender = true;
            }
            else
                ret_val = PI_INIT_FAIL_RETRY;
            
        }
        else
            ret_val = PI_INIT_FAIL_REMOVE;
    }
        
    else if( PI_FULL_INIT == init_flags ){
    
        // see if there is a SENC available
        int sret = FindOrCreateSenc( m_full_base_path );
        if( sret != BUILD_SENC_OK ) {
            if( sret == BUILD_SENC_NOK_RETRY )
                ret_val = PI_INIT_FAIL_RETRY;
            else
                ret_val = PI_INIT_FAIL_REMOVE;
        } else
            
            //  Finish the init process
            ret_val = PostInit( init_flags, m_global_color_scheme );
                
        }
        
        
        
            
        
        
      s_PI_bInS57--;
      return ret_val;
}


wxString ChartS63::GetFileSearchMask(void)
{
      return _T("*.os63");
}

bool ChartS63::GetChartExtent(ExtentPI *pext)
{
    pext->NLAT = m_FullExtent.NLAT;
    pext->SLAT = m_FullExtent.SLAT;
    pext->ELON = m_FullExtent.ELON;
    pext->WLON = m_FullExtent.WLON;

    return true;
}

#if 0
int ChartS63::GetCOVREntries()
{
    return GetCOVREntries();
}

int ChartS63::GetCOVRTablePoints(int iTable)
{
    return GetCOVRTablePoints(iTable);
}

int  ChartS63::GetCOVRTablenPoints(int iTable)
{
    return GetCOVRTablenPoints(iTable);
}

float *ChartS63::GetCOVRTableHead(int iTable)
{
    return GetCOVRTableHead(iTable);
}

int ChartS63::GetNativeScale()
{
    return GetNativeScale();
}

#endif


double ChartS63::GetRasterScaleFactor()
{
    return 1;
}

bool ChartS63::IsRenderDelta(PlugIn_ViewPort &vp_last, PlugIn_ViewPort &vp_proposed)
{
    return true;
}

#if 0
void ChartS63::ChartBaseBSBCTOR()
{
      //    Init some private data

      pBitmapFilePath = NULL;

      pline_table = NULL;
      ifs_buf = NULL;

      cached_image_ok = 0;

      pRefTable = (Refpoint *)malloc(sizeof(Refpoint));
      nRefpoint = 0;
      cPoints.status = 0;
      bHaveEmbeddedGeoref = false;
      n_wpx = 0;
      n_wpy = 0;
      n_pwx = 0;
      n_pwy = 0;


      bUseLineCache = true;
      m_Chart_Skew = 0.0;

      pPixCache = NULL;

      pLineCache = NULL;

      m_bilinear_limit = 8;         // bilinear scaling only up to n

      ifs_bitmap = NULL;
      ifss_bitmap = NULL;
      ifs_hdr = NULL;

      for(int i = 0 ; i < N_BSB_COLORS ; i++)
            pPalettes[i] = NULL;

      bGeoErrorSent = false;
      m_Chart_DU = 0;
      m_cph = 0.;

      m_mapped_color_index = COLOR_RGB_DEFAULT;

      m_datum_str = _T("WGS84");                // assume until proven otherwise

      m_dtm_lat = 0.;
      m_dtm_lon = 0.;

      m_bIDLcross = false;

      m_dx = 0.;
      m_dy = 0.;
      m_proj_lat = 0.;
      m_proj_lon = 0.;
      m_proj_parameter = 0.;

      m_b_cdebug = 0;

#ifdef OCPN_USE_CONFIG
      wxFileConfig *pfc = pConfig;
      pfc->SetPath ( _T ( "/Settings" ) );
      pfc->Read ( _T ( "DebugBSBImg" ),  &m_b_cdebug, 0 );
#endif

}

void ChartBSB4::ChartBaseBSBDTOR()
{
      if(m_FullPath.Len())
      {
            wxString msg(_T("BSB4_PI:  Closing chart "));
            msg += m_FullPath;
            wxLogMessage(msg);
      }

      if(pBitmapFilePath)
            delete pBitmapFilePath;

      if(pline_table)
            free(pline_table);

      if(ifs_buf)
            free(ifs_buf);

      free(pRefTable);
//      free(pPlyTable);

      delete ifs_bitmap;
      delete ifs_hdr;
      delete ifss_bitmap;

      if(cPoints.status)
      {
          free(cPoints.tx );
          free(cPoints.ty );
          free(cPoints.lon );
          free(cPoints.lat );

          free(cPoints.pwx );
          free(cPoints.wpx );
          free(cPoints.pwy );
          free(cPoints.wpy );
      }

//    Free the line cache

      if(pLineCache)
      {
            CachedLine *pt;
            for(int ylc = 0 ; ylc < Size_Y ; ylc++)
            {
                  pt = &pLineCache[ylc];
                  if(pt->pPix)
                        free (pt->pPix);
            }
            free (pLineCache);
      }



      delete pPixCache;

//      delete pPixCacheBackground;
//      free(background_work_buffer);


      for(int i = 0 ; i < N_BSB_COLORS ; i++)
            delete pPalettes[i];

}
#endif

//    Report recommended minimum and maximum scale values for which use of this chart is valid

double ChartS63::GetNormalScaleMin(double canvas_scale_factor, bool b_allow_overzoom)
{
    double ppm = canvas_scale_factor / m_Chart_Scale; // true_chart_scale_on_display   = m_canvas_scale_factor / pixels_per_meter of displayed chart

    //Adjust overzoom factor based on  b_allow_overzoom option setting
    double oz_factor;
    if( b_allow_overzoom ) oz_factor = 256.;
    else
        oz_factor = 4.;

    ppm *= oz_factor;

    return canvas_scale_factor / ppm;
}

double ChartS63::GetNormalScaleMax(double canvas_scale_factor, int canvas_width)
{
    return 1.0e7;
}


double ChartS63::GetNearestPreferredScalePPM(double target_scale_ppm)
{
    return target_scale_ppm;
}


#if 0
double ChartS63::GetClosestValidNaturalScalePPM(double target_scale, double scale_factor_min, double scale_factor_max)
{
      double chart_1x_scale = GetPPM();

      double binary_scale_factor = 1.;



      //    Overzoom....
      if(chart_1x_scale > target_scale)
      {
            double binary_scale_factor_max = 1 / scale_factor_min;

            while(binary_scale_factor < binary_scale_factor_max)
            {
                  if(fabs((chart_1x_scale / binary_scale_factor ) - target_scale) < (target_scale * 0.05))
                        break;
                  if((chart_1x_scale / binary_scale_factor ) < target_scale)
                        break;
                  else
                        binary_scale_factor *= 2.;
            }
      }


      //    Underzoom.....
      else
      {
            int ibsf = 1;
            int isf_max = (int)scale_factor_max;
            while(ibsf < isf_max)
            {
                  if(fabs((chart_1x_scale * ibsf ) - target_scale) < (target_scale * 0.05))
                        break;

                  else if((chart_1x_scale * ibsf ) > target_scale)
                  {
                        if(ibsf > 1)
                              ibsf /= 2;
                        break;
                  }
                  else
                        ibsf *= 2;
            }

            binary_scale_factor = 1. / ibsf;
      }

      return  chart_1x_scale / binary_scale_factor;
}

#endif





void ChartS63::SetColorScheme(int cs, bool bApplyImmediate)
{

    m_global_color_scheme = cs;


}


wxBitmap *ChartS63::GetThumbnail(int tnx, int tny, int cs)
{
    return NULL;
#if 0
      if(m_pBMPThumb && (m_pBMPThumb->GetWidth() == tnx) && (m_pBMPThumb->GetHeight() == tny) && (m_thumbcs == cs))
            return m_pBMPThumb;

      delete m_pBMPThumb;
      m_thumbcs = cs;

//    Calculate the size and divisors

      int divx = Size_X / tnx;
      int divy = Size_Y / tny;

      int div_factor = wxMin(divx, divy);

      int des_width = Size_X / div_factor;
      int des_height = Size_Y / div_factor;

      wxRect gts;
      gts.x = 0;                                // full chart
      gts.y = 0;
      gts.width = Size_X;
      gts.height = Size_Y;

      int this_bpp = 24;                       // for wxImage
//    Allocate the pixel storage needed for one line of chart bits
      unsigned char *pLineT = (unsigned char *)malloc((Size_X+1) * BPP/8);

//    Scale the data quickly
      unsigned char *pPixTN = (unsigned char *)malloc(des_width * des_height * this_bpp/8 );

      int ix = 0;
      int iy = 0;
      int iyd = 0;
      int ixd = 0;
      int yoffd;
      unsigned char *pxs;
      unsigned char *pxd;

      //    Temporarily set the color scheme
      int cs_tmp = m_global_color_scheme;
      SetColorScheme(cs, false);


      while(iyd < des_height)
      {
            if(0 == BSBGetScanline( pLineT, iy, 0, Size_X, 1))          // get a line
            {
                  free(pLineT);
                  free(pPixTN);
                  return NULL;
            }


            yoffd = iyd * des_width * this_bpp/8;                 // destination y

            ix = 0;
            ixd = 0;
            while(ixd < des_width )
            {
                  pxs = pLineT + (ix * BPP/8);
                  pxd = pPixTN + (yoffd + (ixd * this_bpp/8));
                  *pxd++ = *pxs++;
                  *pxd++ = *pxs++;
                  *pxd = *pxs;

                  ix += div_factor;
                  ixd++;

            }

            iy += div_factor;
            iyd++;
      }

      free(pLineT);

      //    Reset ColorScheme
      SetColorScheme(cs_tmp, false);




//#ifdef ocpnUSE_ocpnBitmap
//      m_pBMPThumb = new PIocpnBitmap(pPixTN, des_width, des_height, -1);
//#else
      wxImage thumb_image(des_width, des_height, pPixTN, true);
      m_pBMPThumb = new wxBitmap(thumb_image);
//#endif

      free(pPixTN);

#endif
    return m_pBMPThumb;

}

wxBitmap &ChartS63::RenderRegionView(const PlugIn_ViewPort& VPoint, const wxRegion &Region)
{
    SetVPParms( VPoint );

//        m_s57chart->SetLinePriorities();

    bool force_new_view = false;

    if( Region != m_last_Region )
        force_new_view = true;

    wxMemoryDC dc;
    /*bool bnew_view = */DoRenderViewOnDC( dc, VPoint, force_new_view );

    m_last_Region = Region;

    m_pCloneBM = GetCloneBitmap();

    m_last_Region = Region;

    return *m_pCloneBM;

}





//-----------------------------------------------------------------------
//          Pixel to Lat/Long Conversion helpers
//-----------------------------------------------------------------------
double polytrans( double* coeff, double lon, double lat );

int ChartS63::vp_pix_to_latlong(PlugIn_ViewPort& vp, int pixx, int pixy, double *plat, double *plon)
{
#if 0
      if(bHaveEmbeddedGeoref)
      {
            double raster_scale = GetPPM() / vp.view_scale_ppm;

            int px = (int)(pixx*raster_scale) + Rsrc.x;
            int py = (int)(pixy*raster_scale) + Rsrc.y;
//            pix_to_latlong(px, py, plat, plon);

            if(1)
            {
                  double lon = polytrans( pwx, px, py );
                  lon = (lon < 0) ? lon + m_cph : lon - m_cph;
                  *plon = lon - m_lon_datum_adjust;
                  *plat = polytrans( pwy, px, py ) - m_lat_datum_adjust;
            }

            return 0;
      }
      else
      {
            double slat, slon;
            double xp, yp;

            if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
            {
                   //      Use Projected Polynomial algorithm

                  double raster_scale = GetPPM() / vp.view_scale_ppm;

                  //      Apply poly solution to vp center point
                  double easting, northing;
                  toTM(vp.clat + m_lat_datum_adjust, vp.clon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                  double xc = polytrans( cPoints.wpx, easting, northing );
                  double yc = polytrans( cPoints.wpy, easting, northing );

                  //    convert screen pixels to chart pixmap relative
                  double px = xc + (pixx- (vp.pix_width / 2))*raster_scale;
                  double py = yc + (pixy- (vp.pix_height / 2))*raster_scale;

                  //    Apply polynomial solution to chart relative pixels to get e/n
                  double east  = polytrans( cPoints.pwx, px, py );
                  double north = polytrans( cPoints.pwy, px, py );

                  //    Apply inverse Projection to get lat/lon
                  double lat,lon;
                  fromTM ( east, north, m_proj_lat, m_proj_lon, &lat, &lon );

                  //    Datum adjustments.....
//??                  lon = (lon < 0) ? lon + m_cph : lon - m_cph;
                  double slon_p = lon - m_lon_datum_adjust;
                  double slat_p = lat - m_lat_datum_adjust;

//                  printf("%8g %8g %8g %8g %g\n", slat, slat_p, slon, slon_p, slon - slon_p);
                  slon = slon_p;
                  slat = slat_p;

            }
            else if(m_projection == PI_PROJECTION_MERCATOR)
            {
                   //      Use Projected Polynomial algorithm

                  double raster_scale = GetPPM() / vp.view_scale_ppm;

                  //      Apply poly solution to vp center point
                  double easting, northing;
                  toSM_ECC(vp.clat + m_lat_datum_adjust, vp.clon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                  double xc = polytrans( cPoints.wpx, easting, northing );
                  double yc = polytrans( cPoints.wpy, easting, northing );

                  //    convert screen pixels to chart pixmap relative
                  double px = xc + (pixx- (vp.pix_width / 2))*raster_scale;
                  double py = yc + (pixy- (vp.pix_height / 2))*raster_scale;

                  //    Apply polynomial solution to chart relative pixels to get e/n
                  double east  = polytrans( cPoints.pwx, px, py );
                  double north = polytrans( cPoints.pwy, px, py );

                  //    Apply inverse Projection to get lat/lon
                  double lat,lon;
                  fromSM_ECC ( east, north, m_proj_lat, m_proj_lon, &lat, &lon );

                  //    Make Datum adjustments.....
                  double slon_p = lon - m_lon_datum_adjust;
                  double slat_p = lat - m_lat_datum_adjust;

                  slon = slon_p;
                  slat = slat_p;

//                  printf("vp.clon  %g    xc  %g   px   %g   east  %g  \n", vp.clon, xc, px, east);

            }
            else
            {
                  // Use a Mercator estimator, with Eccentricity corrrection applied
                  int dx = pixx - ( vp.pix_width  / 2 );
                  int dy = ( vp.pix_height / 2 ) - pixy;

                  xp = ( dx * cos ( vp.skew ) ) - ( dy * sin ( vp.skew ) );
                  yp = ( dy * cos ( vp.skew ) ) + ( dx * sin ( vp.skew ) );

                  double d_east = xp / vp.view_scale_ppm;
                  double d_north = yp / vp.view_scale_ppm;

                  fromSM_ECC ( d_east, d_north, vp.clat, vp.clon, &slat, &slon );
            }

            *plat = slat;

            if(slon < -180.)
                  slon += 360.;
            else if(slon > 180.)
                  slon -= 360.;
            *plon = slon;

            return 0;
      }
#endif
        return 1;
}




int ChartS63::latlong_to_pix_vp(double lat, double lon, int &pixx, int &pixy, PlugIn_ViewPort& vp)
{
#if 0
    int px, py;

    double alat, alon;

    if(bHaveEmbeddedGeoref)
    {
          double alat, alon;

          alon = lon + m_lon_datum_adjust;
          alat = lat + m_lat_datum_adjust;

          if(m_bIDLcross)
          {
                if(alon < 0.)
                      alon += 360.;
          }

          if(1)
          {
                /* change longitude phase (CPH) */
                double lonp = (alon < 0) ? alon + m_cph : alon - m_cph;
                double xd = polytrans( wpx, lonp, alat );
                double yd = polytrans( wpy, lonp, alat );
                px = (int)(xd + 0.5);
                py = (int)(yd + 0.5);


                double raster_scale = GetPPM() / vp.view_scale_ppm;

                pixx = (int)(((px - Rsrc.x) / raster_scale) + 0.5);
                pixy = (int)(((py - Rsrc.y) / raster_scale) + 0.5);

            return 0;
          }
    }
    else
    {
          double easting, northing;
          double xlon = lon;

                //  Make sure lon and lon0 are same phase
/*
          if((xlon * vp.clon) < 0.)
          {
                if(xlon < 0.)
                      xlon += 360.;
                else
                      xlon -= 360.;
          }

          if(fabs(xlon - vp.clon) > 180.)
          {
                if(xlon > vp.clon)
                      xlon -= 360.;
                else
                      xlon += 360.;
          }
*/


          if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
          {
                //      Use Projected Polynomial algorithm

                alon = lon + m_lon_datum_adjust;
                alat = lat + m_lat_datum_adjust;

                //      Get e/n from TM Projection
                toTM(alat, alon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                double xd = polytrans( cPoints.wpx, easting, northing );
                double yd = polytrans( cPoints.wpy, easting, northing );

                //      Apply poly solution to vp center point
                toTM(vp.clat + m_lat_datum_adjust, vp.clon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                double xc = polytrans( cPoints.wpx, easting, northing );
                double yc = polytrans( cPoints.wpy, easting, northing );

                //      Calculate target point relative to vp center
                double raster_scale = GetPPM() / vp.view_scale_ppm;

                int xs = (int)xc - (int)(vp.pix_width  * raster_scale / 2);
                int ys = (int)yc - (int)(vp.pix_height * raster_scale / 2);

                int pixx_p = (int)(((xd - xs) / raster_scale) + 0.5);
                int pixy_p = (int)(((yd - ys) / raster_scale) + 0.5);

//                printf("  %d  %d  %d  %d\n", pixx, pixx_p, pixy, pixy_p);

                pixx = pixx_p;
                pixy = pixy_p;

          }
          else if(m_projection == PI_PROJECTION_MERCATOR)
          {
                //      Use Projected Polynomial algorithm

                alon = lon + m_lon_datum_adjust;
                alat = lat + m_lat_datum_adjust;

                //      Get e/n from  Projection
                xlon = alon;
                if(m_bIDLcross)
                {
                      if(xlon < 0.)
                            xlon += 360.;
                }
                toSM_ECC(alat, xlon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                double xd = polytrans( cPoints.wpx, easting, northing );
                double yd = polytrans( cPoints.wpy, easting, northing );

                //      Apply poly solution to vp center point
                double xlonc = vp.clon;
                if(m_bIDLcross)
                {
                      if(xlonc < 0.)
                            xlonc += 360.;
                }

                toSM_ECC(vp.clat + m_lat_datum_adjust, xlonc + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                double xc = polytrans( cPoints.wpx, easting, northing );
                double yc = polytrans( cPoints.wpy, easting, northing );

                //      Calculate target point relative to vp center
                double raster_scale = GetPPM() / vp.view_scale_ppm;

                int xs = (int)xc - (int)(vp.pix_width  * raster_scale / 2);
                int ys = (int)yc - (int)(vp.pix_height * raster_scale / 2);

                int pixx_p = (int)(((xd - xs) / raster_scale) + 0.5);
                int pixy_p = (int)(((yd - ys) / raster_scale) + 0.5);

                pixx = pixx_p;
                pixy = pixy_p;

          }
          else
         {
                toSM_ECC(lat, xlon, vp.clat, vp.clon, &easting, &northing);

                double epix = easting  * vp.view_scale_ppm;
                double npix = northing * vp.view_scale_ppm;

                double dx = epix * cos ( vp.skew ) + npix * sin ( vp.skew );
                double dy = npix * cos ( vp.skew ) - epix * sin ( vp.skew );

                pixx = ( int ) /*rint*/( ( vp.pix_width  / 2 ) + dx );
                pixy = ( int ) /*rint*/( ( vp.pix_height / 2 ) - dy );
         }
                return 0;
    }
#endif
    return 1;
}

void ChartS63::latlong_to_chartpix(double lat, double lon, double &pixx, double &pixy)
{
#if 0
      double alat, alon;

      if(bHaveEmbeddedGeoref)
      {
            double alat, alon;

            alon = lon + m_lon_datum_adjust;
            alat = lat + m_lat_datum_adjust;

            if(m_bIDLcross)
            {
                  if(alon < 0.)
                        alon += 360.;
            }


            /* change longitude phase (CPH) */
            double lonp = (alon < 0) ? alon + m_cph : alon - m_cph;
            pixx = polytrans( wpx, lonp, alat );
            pixy = polytrans( wpy, lonp, alat );
      }
      else
      {
            double easting, northing;
            double xlon = lon;

            if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
            {
                //      Use Projected Polynomial algorithm

                  alon = lon + m_lon_datum_adjust;
                  alat = lat + m_lat_datum_adjust;

                //      Get e/n from TM Projection
                  toTM(alat, alon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                  pixx = polytrans( cPoints.wpx, easting, northing );
                  pixy = polytrans( cPoints.wpy, easting, northing );


            }
            else if(m_projection == PI_PROJECTION_MERCATOR)
            {
                //      Use Projected Polynomial algorithm

                  alon = lon + m_lon_datum_adjust;
                  alat = lat + m_lat_datum_adjust;

                //      Get e/n from  Projection
                  xlon = alon;
                  if(m_bIDLcross)
                  {
                        if(xlon < 0.)
                              xlon += 360.;
                  }
                  toSM_ECC(alat, xlon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                  pixx = polytrans( cPoints.wpx, easting, northing );
                  pixy = polytrans( cPoints.wpy, easting, northing );


            }
      }
#endif
}


void ChartS63::ComputeSourceRectangle(const PlugIn_ViewPort &vp, wxRect *pSourceRect)
{
#if 0
//    int pixxd, pixyd;

    //      This funny contortion is necessary to allow scale factors < 1, i.e. overzoom
    double binary_scale_factor = (wxRound(100000 * GetPPM() / vp.view_scale_ppm)) / 100000.;

//    if((binary_scale_factor > 1.0) && (fabs(binary_scale_factor - wxRound(binary_scale_factor)) < 1e-2))
//          binary_scale_factor = wxRound(binary_scale_factor);

    m_raster_scale_factor = binary_scale_factor;

    if(m_b_cdebug)printf(" ComputeSourceRect... PPM: %g  vp.view_scale_ppm: %g   m_raster_scale_factor: %g\n", GetPPM(), vp.view_scale_ppm, m_raster_scale_factor);

      double xd, yd;
      latlong_to_chartpix(vp.clat, vp.clon, xd, yd);


      pSourceRect->x = wxRound(xd - (vp.pix_width  * binary_scale_factor / 2));
      pSourceRect->y = wxRound(yd - (vp.pix_height * binary_scale_factor / 2));

      pSourceRect->width =  (int)wxRound(vp.pix_width  * binary_scale_factor) ;
      pSourceRect->height = (int)wxRound(vp.pix_height * binary_scale_factor) ;
#endif

}


//------------------------------------------------------------------------------
//      Local version of fgets for Binary Mode (SENC) file
//------------------------------------------------------------------------------
int ChartS63::my_fgets( char *buf, int buf_len_max, wxInputStream &ifs )
{
    char chNext;
    int nLineLen = 0;
    char *lbuf;
    
    lbuf = buf;
    
    while( !ifs.Eof() && nLineLen < buf_len_max ) {
        chNext = (char) ifs.GetC();
        
        /* each CR/LF (or LF/CR) as if just "CR" */
        if( chNext == 10 || chNext == 13 ) {
            chNext = '\n';
        }
        
        *lbuf = chNext;
        lbuf++, nLineLen++;
        
        if( chNext == '\n' ) {
            *lbuf = '\0';
            return nLineLen;
        }
    }
    
    *( lbuf ) = '\0';
    
    return nLineLen;
}

//    Read the .ehdr Header file and create required Chartbase data structures
bool ChartS63::InitFrom_ehdr( wxString &efn )
{
    bool ret_val = true;

    wxString ifs = efn;
    
    wxFileInputStream fpx_u( ifs );
    wxBufferedInputStream fpx( fpx_u );
    
    int MAX_LINE = 499999;
    char *buf = (char *) malloc( MAX_LINE + 1 );
    
    int dun = 0;
    
    wxString date_000, date_upd;
 
    m_pCOVRTablePoints = NULL;
    m_pCOVRTable = NULL;
    
    //  Create arrays to hold geometry objects temporarily
    MyFloatPtrArray *pAuxPtrArray = new MyFloatPtrArray;
    wxArrayInt *pAuxCntArray = new wxArrayInt;
    
    MyFloatPtrArray *pNoCovrPtrArray = new MyFloatPtrArray;
    wxArrayInt *pNoCovrCntArray = new wxArrayInt;
    
    
    
    while( !dun ) {
        
        if( my_fgets( buf, MAX_LINE, fpx ) == 0 ) {
            dun = 1;
            break;
        }
        
        else if( !strncmp( buf, "SENC", 4 ) ) {
            int senc_file_version;
            sscanf( buf, "SENC Version=%i", &senc_file_version );
            if( senc_file_version != CURRENT_SENC_FORMAT_VERSION ) {
                wxString msg( _T("   Wrong version on SENC file ") );
                msg.Append( efn );
                wxLogMessage( msg );
                
                dun = 1;
                ret_val = false;                   // error
                break;
            }
        }
        
        else if( !strncmp( buf, "DATEUPD", 7 ) ) {
            date_upd.Append( wxString( &buf[8], wxConvUTF8 ).BeforeFirst( '\n' ) );
        }
        
        else if( !strncmp( buf, "DATE000", 7 ) ) {
            date_000.Append( wxString( &buf[8], wxConvUTF8 ).BeforeFirst( '\n' ) );
        }
        
        else if( !strncmp( buf, "SCALE", 5 ) ) {
            int ins;
            sscanf( buf, "SCALE=%d", &ins );
            m_Chart_Scale = ins;
            
        }
        
        else if( !strncmp( buf, "NAME", 4 ) ) {
            m_Name = wxString( &buf[5], wxConvUTF8 ).BeforeFirst( '\n' );
        }
        
        else if( !strncmp( buf, "Chart Extents:", 14 ) ) {
            float elon, wlon, nlat, slat;
            sscanf( buf, "Chart Extents: %g %g %g %g", &elon, &wlon, &nlat, &slat );
            m_FullExtent.ELON = elon;
            m_FullExtent.WLON = wlon;
            m_FullExtent.NLAT = nlat;
            m_FullExtent.SLAT = slat;
            m_bExtentSet = true;
            
            //  Establish a common reference point for the chart
            m_ref_lat = ( m_FullExtent.NLAT + m_FullExtent.SLAT ) / 2.;
            m_ref_lon = ( m_FullExtent.WLON + m_FullExtent.ELON ) / 2.;
            
        }
        
        else if( !strncmp( buf, "OGRF", 4 ) ) {
            
            PI_S57ObjX *obj = new PI_S57ObjX( buf, &fpx );
            if( !strncmp( obj->FeatureName, "M_COVR", 6 ) ){

                wxString catcov_str = obj->GetAttrValueAsString( "CATCOV" );
                long catcov = 0;
                catcov_str.ToLong( &catcov );
    
                double area_ref_lat, area_ref_lon;
                ((PolyTessGeo *)obj->pPolyTessGeo)->GetRefPos( &area_ref_lat, &area_ref_lon );
                
                //      Get the raw geometry from the PolyTessGeo
                PolyTriGroup *pptg = ((PolyTessGeo *)obj->pPolyTessGeo)->Get_PolyTriGroup_head();
            
                float *ppolygeo = pptg->pgroup_geom;
            
                int ctr_offset = 0;
                for( int ic = 0; ic < pptg->nContours; ic++ ) {
                
                    int npt = pptg->pn_vertex[ic];
                    
                    if( npt >= 3 ) {
                        float *pf = (float *) malloc( 2 * npt * sizeof(float) );
                        float *pfr = pf;
                       float *pfi = &ppolygeo[ctr_offset];
                        float *pfir = pfi;
                        
                        for( int ip = 0; ip < npt; ip++ ) {
                            float easting = *pfir++;
                            float northing = *pfir++;
                            
                            //      Geom is is SM coords, so convert to lat/lon
                            double xll, yll;
                            fromSM_Plugin( easting, northing, m_ref_lat, m_ref_lon, &yll, &xll );
                            
                            //          Now store in chart cover array members
                            pfr[0] = yll;             // lat
                            pfr[1] = xll;             // lon
                            
                            pfr += 2;
                                                 
                        }

                        
                        if( catcov == 1 ) {
                            pAuxPtrArray->Add( pf );
                            pAuxCntArray->Add( npt );
                        }
                        else if( catcov == 2 ){
                            pNoCovrPtrArray->Add( pf );
                            pNoCovrCntArray->Add( npt );
                        }
                    }
                }
            }
                
        }               //OGRF
        
        
    }                       //while(!dun)
 
 
 //    Allocate the final storage for member coverage arrays
 
    m_nCOVREntries = pAuxCntArray->GetCount();
 
 //    If only one M_COVR,CATCOV=1 object was found,
 //    assign the geometry to the one and only COVR
 
    if( m_nCOVREntries == 1 ) {
        m_pCOVRTablePoints = (int *) malloc( sizeof(int) );
        *m_pCOVRTablePoints = pAuxCntArray->Item( 0 );
        m_pCOVRTable = (float **) malloc( sizeof(float *) );
        *m_pCOVRTable = (float *) malloc( pAuxCntArray->Item( 0 ) * 2 * sizeof(float) );
        memcpy( *m_pCOVRTable, pAuxPtrArray->Item( 0 ), pAuxCntArray->Item( 0 ) * 2 * sizeof(float) );
    }
 
    else if( m_nCOVREntries > 1 ) {
     //    Create new COVR entries
        m_pCOVRTablePoints = (int *) malloc( m_nCOVREntries * sizeof(int) );
        m_pCOVRTable = (float **) malloc( m_nCOVREntries * sizeof(float *) );
     
        for( unsigned int j = 0; j < (unsigned int) m_nCOVREntries; j++ ) {
            m_pCOVRTablePoints[j] = pAuxCntArray->Item( j );
            m_pCOVRTable[j] = (float *) malloc( pAuxCntArray->Item( j ) * 2 * sizeof(float) );
            memcpy( m_pCOVRTable[j], pAuxPtrArray->Item( j ), pAuxCntArray->Item( j ) * 2 * sizeof(float) );
        }
    }
 
    else {                                    // strange case, found no CATCOV=1 M_COVR objects
            wxString msg( _T("   ENC contains no useable M_COVR, CATCOV=1 features:  ") );
            msg.Append( m_FullPath );
            wxLogMessage( msg );
    }
        
        
        //      And for the NoCovr regions
    m_nNoCOVREntries = pNoCovrCntArray->GetCount();
 
    if( m_nNoCOVREntries ) {
     //    Create new NoCOVR entries
        m_pNoCOVRTablePoints = (int *) malloc( m_nNoCOVREntries * sizeof(int) );
        m_pNoCOVRTable = (float **) malloc( m_nNoCOVREntries * sizeof(float *) );
     
        for( unsigned int j = 0; j < (unsigned int) m_nNoCOVREntries; j++ ) {
            int npoints = pNoCovrCntArray->Item( j );
            m_pNoCOVRTablePoints[j] = npoints;
            m_pNoCOVRTable[j] = (float *) malloc( npoints * 2 * sizeof(float) );
            memcpy( m_pNoCOVRTable[j], pNoCovrPtrArray->Item( j ), npoints * 2 * sizeof(float) );
        }
    }
    else {
        m_pNoCOVRTablePoints = NULL;
        m_pNoCOVRTable = NULL;
    }
 
    delete pAuxPtrArray;
    delete pAuxCntArray;
    delete pNoCovrPtrArray;
    delete pNoCovrCntArray;
 
 
    if( 0 == m_nCOVREntries ) {                        // fallback
        wxString msg( _T("   ehdr contains no M_COVR features:  ") );
        msg.Append( efn );
        wxLogMessage( msg );
        
        msg =  _T("   Calculating Chart Extents as fallback.");
        wxLogMessage( msg );
 
    //    Populate simplified (exten-based) COVR structures
        if( m_bExtentSet ) {
            m_nCOVREntries = 1;             
            
            if( m_nCOVREntries == 1 ) {
                m_pCOVRTablePoints = (int *) malloc( sizeof(int) );
                *m_pCOVRTablePoints = 4;
                m_pCOVRTable = (float **) malloc( sizeof(float *) );
            
                float *pf = (float *) malloc( 2 * 4 * sizeof(float) );
                *m_pCOVRTable = pf;
                float *pfe = pf;
            
                *pfe++ = m_FullExtent.NLAT;
                *pfe++ = m_FullExtent.ELON;
            
                *pfe++ = m_FullExtent.NLAT;
                *pfe++ = m_FullExtent.WLON;
            
                *pfe++ = m_FullExtent.SLAT;
                *pfe++ = m_FullExtent.WLON;
            
                *pfe++ = m_FullExtent.SLAT;
                *pfe++ = m_FullExtent.ELON;
            }
        }
    }
    
    free( buf );
    
    
    //   Decide on pub date to show
    
    int d000 = 0;
    wxString sd000 =date_000.Mid( 0, 4 );
    wxCharBuffer dbuffer=sd000.ToUTF8();
    if(dbuffer.data())
        d000 = atoi(dbuffer.data() );
    
    int dupd = 0;
    wxString sdupd =date_upd.Mid( 0, 4 );
    wxCharBuffer ubuffer = sdupd.ToUTF8();
    if(ubuffer.data())
        dupd = atoi(ubuffer.data() );
    
    if( dupd > d000 )
        m_PubYear = sdupd;
    else
        m_PubYear = sd000;
    
    wxDateTime dt;
    dt.ParseDate( date_000 );
    
    if( !ret_val ) return false;
    
    return true;
}


//-----------------------------------------------------------------------------------------------
//    Find or Create a relevent SENC file from a given .000 ENC file
//    Returns with error code, and associated SENC file name in m_S57FileName
//-----------------------------------------------------------------------------------------------
PI_InitReturn ChartS63::FindOrCreateSenc( const wxString& name )
{
    //      Establish location for SENC files
    wxFileName SENCFileName = name;
    SENCFileName.SetExt( _T("es57") );
    
    //      Set the proper directory for the SENC files
    wxString SENCdir = m_senc_dir;
    
    if( !SENCdir.Len() )
        return PI_INIT_FAIL_RETRY;
    
    if( SENCdir.Last() != wxFileName::GetPathSeparator() )
        SENCdir.Append( wxFileName::GetPathSeparator() );
    
    wxFileName tsfn( SENCdir );
    tsfn.SetFullName( SENCFileName.GetFullName() );
    SENCFileName = tsfn;
    
    // Really can only Init and use S57 chart if the S52 Presentation Library is OK
    //    if( !ps52plib->m_bOK ) return INIT_FAIL_REMOVE;
    
    int build_ret_val = 1;
    
    bool bbuild_new_senc = false;
//    m_bneed_new_thumbnail = false;
    
    wxFileName FileName000( name );
    
    //      Look for SENC file in the target directory
    
    if( SENCFileName.FileExists() ) {
        
#if 0    
        wxFile f;
        if( f.Open( m_SENCFileName.GetFullPath() ) ) {
            if( f.Length() == 0 ) {
                f.Close();
                build_ret_val = BuildSENCFile( name, m_SENCFileName.GetFullPath() );
            } else                                      // file exists, non-zero
            {                                         // so check for new updates
            
            f.Seek( 0 );
            wxFileInputStream *pfpx_u = new wxFileInputStream( f );
            wxBufferedInputStream *pfpx = new wxBufferedInputStream( *pfpx_u );
            int dun = 0;
            int last_update = 0;
            int senc_file_version = 0;
            int force_make_senc = 0;
            char buf[256];
            char *pbuf = buf;
            wxDateTime ModTime000;
            int size000 = 0;
            wxString senc_base_edtn;
            
            while( !dun ) {
                if( my_fgets( pbuf, 256, *pfpx ) == 0 ) {
                    dun = 1;
                    force_make_senc = 1;
                    break;
                } else {
                    if( !strncmp( pbuf, "OGRF", 4 ) ) {
                        dun = 1;
                        break;
                    }
                    
                    wxString str_buf( pbuf, wxConvUTF8 );
                    wxStringTokenizer tkz( str_buf, _T("=") );
                    wxString token = tkz.GetNextToken();
                    
                    if( token.IsSameAs( _T("UPDT"), TRUE ) ) {
                        int i;
                        i = tkz.GetPosition();
                        last_update = atoi( &pbuf[i] );
                    }
                    
                    else if( token.IsSameAs( _T("SENC Version"), TRUE ) ) {
                        int i;
                        i = tkz.GetPosition();
                        senc_file_version = atoi( &pbuf[i] );
                    }
                    
                    else if( token.IsSameAs( _T("FILEMOD000"), TRUE ) ) {
                        int i;
                        i = tkz.GetPosition();
                        wxString str( &pbuf[i], wxConvUTF8 );
                        str.Trim();                               // gets rid of newline, etc...
                        if( !ModTime000.ParseFormat( str,
                            _T("%Y%m%d")/*(const wxChar *)"%Y%m%d"*/) ) ModTime000.SetToCurrent();
                        ModTime000.ResetTime();                   // to midnight
                    }
                    
                    else if( token.IsSameAs( _T("FILESIZE000"), TRUE ) ) {
                        int i;
                        i = tkz.GetPosition();
                        size000 = atoi( &pbuf[i] );
                    }
                    
                    else if( token.IsSameAs( _T("EDTN000"), TRUE ) ) {
                        int i;
                        i = tkz.GetPosition();
                        wxString str( &pbuf[i], wxConvUTF8 );
                        str.Trim();                               // gets rid of newline, etc...
                        senc_base_edtn = str;
                    }
                    
                }
            }
            
            delete pfpx;
            delete pfpx_u;
            f.Close();
            //              Anything to do?
            // force_make_senc = 1;
            //  SENC file version has to be correct for other tests to make sense
            if( senc_file_version != CURRENT_SENC_FORMAT_VERSION ) bbuild_new_senc = true;
                            
                            //  Senc EDTN must be the same as .000 file EDTN.
            //  This test catches the usual case where the .000 file is updated from the web,
            //  and all updates (.001, .002, etc.)  are subsumed.
            else if( !senc_base_edtn.IsSameAs( m_edtn000 ) ) bbuild_new_senc = true;
                            
                            else {
                                //    See if there are any new update files  in the ENC directory
                                int most_recent_update_file = GetUpdateFileArray( FileName000, NULL );
                                
                                if( last_update != most_recent_update_file ) bbuild_new_senc = true;
                            
                            //          Make two simple tests to see if the .000 file is "newer" than the SENC file representation
                                //          These tests may be redundant, since the DSID:EDTN test above should catch new base files
                                wxDateTime OModTime000;
                                FileName000.GetTimes( NULL, &OModTime000, NULL );
                                OModTime000.ResetTime();                      // to midnight
                                if( ModTime000.IsValid() ) if( OModTime000.IsLaterThan( ModTime000 ) ) bbuild_new_senc =
                                    true;
                                
                                int Osize000l = FileName000.GetSize().GetLo();
                                if( size000 != Osize000l ) bbuild_new_senc = true;
                            }
                            
                            if( force_make_senc ) bbuild_new_senc = true;
                            
                            if( bbuild_new_senc ) build_ret_val = BuildSENCFile( name,
                                m_SENCFileName.GetFullPath() );
                            
            }
        }
#endif        
//        build_ret_val = BuildSENCFile( name, SENCFileName.GetFullPath() );
//        bbuild_new_senc = true;
    }
    
    else                    // SENC file does not exist
    {
        build_ret_val = BuildSENCFile( name, SENCFileName.GetFullPath() );
        bbuild_new_senc = true;
    }
 
 
//    if( bbuild_new_senc )
//        m_bneed_new_thumbnail = true; // force a new thumbnail to be built in PostInit()
                        
    if( bbuild_new_senc ) {
        if( BUILD_SENC_NOK_PERMANENT == build_ret_val )
            return PI_INIT_FAIL_REMOVE;
        if( BUILD_SENC_NOK_RETRY == build_ret_val )
            return PI_INIT_FAIL_RETRY;
    }

    m_SENCFileName = SENCFileName;
    return PI_INIT_OK;
}


int ChartS63::BuildSENCFile( const wxString& FullPath_os63, const wxString& SENCFileName )
{
    
    // build the SENC utility command line
    wxString outfile = SENCFileName; //wxFileName::CreateTempFileName( _T("") );
    
    wxString cmd = g_sencutil_bin;
    cmd += _T(" -c ");                  // create secure SENC
    
    cmd += _T(" -i ");
    cmd += _T("\"");
    cmd += m_full_base_path;
    cmd += _T("\"");
    
    cmd += _T(" -o ");
    cmd += _T("\"");
    cmd += outfile;
    cmd += _T("\"");
    
    cmd += _T(" -p ");
    cmd += m_cell_permit;
    
    cmd += _T(" -u ");
    cmd += g_user_permit;
    
    cmd += _T(" -b ");
    wxString port;
    port.Printf( _T("%d"), g_backchannel_port );
    cmd += port;
    
    cmd += _T(" -r ");
    cmd += _T("\"");
    cmd += g_s57data_dir;
    cmd += _T("\"");
    
    wxLogMessage( cmd );
    
    ClearScreenLog();
    wxArrayString ehdr_result = exec_SENCutil_sync( cmd, true );
    
    //  Check results
    if( !exec_results_check( ehdr_result ) ) {
        ScreenLogMessage(_T("\n"));
        m_extended_error = _T("Error executing cmd: ");
        m_extended_error += cmd;
        m_extended_error += _T("\n");
        m_extended_error += s_last_sync_error;
        
        for(unsigned int i=0 ; i < ehdr_result.GetCount() ; i++){
            ScreenLogMessage( ehdr_result[i] );
            if(!ehdr_result[i].EndsWith(_T("\n")))
                ScreenLogMessage( _T("\n") );
        }
        
        return PI_INIT_FAIL_REMOVE;
    }
    
//    HideScreenLog();
    return BUILD_SENC_OK;
}

int ChartS63::_insertRules( PI_S57Obj *obj )
{
    int disPrioIdx = 0;
    int LUPtypeIdx = 0;
    
    PI_DisPrio DPRI = PI_GetObjectDisplayPriority( obj );
    // find display priority index       --talky version
    switch( DPRI ){
        case PI_PRIO_NODATA:
            disPrioIdx = 0;
            break;  // no data fill area pattern
        case PI_PRIO_GROUP1:
            disPrioIdx = 1;
            break;  // S57 group 1 filled areas
        case PI_PRIO_AREA_1:
            disPrioIdx = 2;
            break;  // superimposed areas
        case PI_PRIO_AREA_2:
            disPrioIdx = 3;
            break;  // superimposed areas also water features
        case PI_PRIO_SYMB_POINT:
            disPrioIdx = 4;
            break;  // point symbol also land features
        case PI_PRIO_SYMB_LINE:
            disPrioIdx = 5;
            break;  // line symbol also restricted areas
        case PI_PRIO_SYMB_AREA:
            disPrioIdx = 6;
            break;  // area symbol also traffic areas
        case PI_PRIO_ROUTEING:
            disPrioIdx = 7;
            break;  // routeing lines
        case PI_PRIO_HAZARDS:
            disPrioIdx = 8;
            break;  // hazards
        case PI_PRIO_MARINERS:
            disPrioIdx = 9;
            break;  // VRM & EBL, own ship
        default:
            break;
    }
    
    PI_LUPname TNAM = PI_GetObjectLUPName( obj );
    // find look up type index
    switch( TNAM ){
        case PI_SIMPLIFIED:
            LUPtypeIdx = 0;
            break; // points
        case PI_PAPER_CHART:
            LUPtypeIdx = 1;
            break; // points
        case PI_LINES:
            LUPtypeIdx = 2;
            break; // lines
        case PI_PLAIN_BOUNDARIES:
            LUPtypeIdx = 3;
            break; // areas
        case PI_SYMBOLIZED_BOUNDARIES:
            LUPtypeIdx = 4;
            break; // areas
        default:
            break;
    }
    
    // insert rules
    obj->nRef++;                         // Increment reference counter for delete check;
    obj->next = razRules[disPrioIdx][LUPtypeIdx];
    obj->child = NULL;
    razRules[disPrioIdx][LUPtypeIdx] = obj;
    
    return 1;
}









int ChartS63::BuildRAZFromSENCFile( const wxString& FullPath )
{
    int ret_val = 0;                    // default is OK

    
    //    Sanity check for existence of file
    wxFileName SENCFileName( FullPath );
    if( !SENCFileName.FileExists() ) {
        wxString msg( _T("   Cannot open eSENC file ") );
        msg.Append( SENCFileName.GetFullPath() );
        wxLogMessage( msg );
        return 1;
    }
    
    wxBufferedInputStream *pfpx;
    wxFileInputStream fpx_u( FullPath );

    SENCclient scli;
    
    if(0){
    //  configure the client
        scli.Attach( FullPath );
        if(!scli.m_OK) {
            scli.Close();
        
        //  Wait for output
            wxString outres;
            for(unsigned int t = 0 ; t < 5 ; t++) {
                outres = scli.GetServerOutput();
                if(outres.Len()){
                    break;
                }
            //            wxSleep(1);
            }
        
        
            wxString msg( _T("   Cannot start SENC server...") );
            msg += outres;
            wxLogMessage( msg );
        
            return 1;
        }

        wxBufferedInputStream fpx( scli );
        fpx.GetInputStreamBuffer()->SetBufferIO(200 * 1024);
        
        pfpx = &fpx;
    }

     else {
        if( !fpx_u.IsOk())
            return 1;
        pfpx = new wxBufferedInputStream( fpx_u );
    }
    
    
    
    int MAX_LINE = 499999;
    char *buf = (char *) malloc( MAX_LINE + 1 );
    
    int nGeoFeature;
    
    int object_count = 0;
    
    OGREnvelope Envelope;
    
    int dun = 0;
    
    char *hdr_buf = (char *) malloc( 1 );
    wxString date_000, date_upd;
    
    
    while( !dun ) {
        int err = my_fgets( buf, MAX_LINE, *pfpx );
        
        if( err == 0 ) {
            dun = 1;
            break;
        }
        else if( err < 0 ) {
            wxPrintf(_T("fgets err %d\n"), err);
            dun = 1;
            //            ret_val = 1;
            break;
        }
        
        if( !strncmp( buf, "OGRF", 4 ) ) {
            
            PI_S57ObjX *obj = new PI_S57ObjX( buf, pfpx );
            if( obj ) {
                
                //      Build/Maintain the ATON floating/rigid arrays
                if( GEO_POINT == obj->Primitive_type ) {
                    
                    // set floating platform
                    if( ( !strncmp( obj->FeatureName, "LITFLT", 6 ) )
                        || ( !strncmp( obj->FeatureName, "LITVES", 6 ) )
                        || ( !strncmp( obj->FeatureName, "BOY", 3 ) ) ) {
                        pFloatingATONArray->Add( obj );
                        }
                        
                        // set rigid platform
                        if( !strncmp( obj->FeatureName, "BCN", 3 ) ) {
                            pRigidATONArray->Add( obj );
                        }
                        
                        //    Mark the object as an ATON
                        if( ( !strncmp( obj->FeatureName, "LIT", 3 ) )
                            || ( !strncmp( obj->FeatureName, "LIGHTS", 6 ) )
                            || ( !strncmp( obj->FeatureName, "BCN", 3 ) )
                            || ( !strncmp( obj->FeatureName, "BOY", 3 ) ) ) {
                            obj->bIsAton = true;
                            }
                            
                }
                
                //      Ensure that Area objects actually describe a valid object
                if( GEO_AREA == obj->Primitive_type ) {
                    //                    if( !obj->BBObj.GetValid() ) {
                        //                        delete obj;
                        //                        continue;
                        //                    }
                }
                
                //      Get an S52PLIB context, and find the initial LUP
                bool bctx = PI_PLIBSetContext( obj );
                
                if( !bctx ) {
                    if( 1 /*g_bDebugS57*/ ) {
                        wxString msg( obj->FeatureName, wxConvUTF8 );
                        msg.Prepend( _T("   Could not find LUP for ") );
                        wxPrintf( msg + _T("\n") );
                    }
                    delete obj;
                } else {
                    //              Add linked object/LUP to the working set
                    _insertRules( obj );
                    
                    //              Establish Object's Display Category
                    obj->m_DisplayCat = PI_GetObjectDisplayCategory( obj );
                    
                    //              Establish chart reference position
                    obj->chart_ref_lat = m_ref_lat;
                    obj->chart_ref_lon = m_ref_lon;
                }
            }
            
            object_count++;
            
            continue;
            
        }               //OGRF
        
        else if( !strncmp( buf, "VETableStart", 12 ) ) {
            //    Use a wxArray for temp storage
            //    then transfer to a simple linear array
            PI_ArrayOfVE_Elements ve_array;
            
            int index = -1;
            int index_max = -1;
            int count;
            
            pfpx->Read( &index, sizeof(int) );
            
            while( -1 != index ) {
                pfpx->Read( (char *)&count, sizeof(int) );
                
                double *pPoints = NULL;
                if( count ) {
                    pPoints = (double *) malloc( count * 2 * sizeof(double) );
                    pfpx->Read( (char *)pPoints, count * 2 * sizeof(double) );
                }
                
                PI_VE_Element vee;
                vee.index = index;
                vee.nCount = count;
                vee.pPoints = pPoints;
                vee.max_priority = -99;            // Default
                
                ve_array.Add( vee );
                
                if( index > index_max ) index_max = index;
                
                //    Next element
                pfpx->Read( (char *)&index, sizeof(int) );
            }
            
            //    Create a hash map of VE_Element pointers as a chart class member
            int n_ve_elements = ve_array.GetCount();
            
            for( int i = 0; i < n_ve_elements; i++ ) {
                PI_VE_Element ve_from_array = ve_array.Item( i );
                PI_VE_Element *vep = new PI_VE_Element;
                vep->index = ve_from_array.index;
                vep->nCount = ve_from_array.nCount;
                vep->pPoints = ve_from_array.pPoints;
                
                m_ve_hash[vep->index] = (PI_VE_Element *)vep;
                
            }
            
        }
        
        else if( !strncmp( buf, "VCTableStart", 12 ) ) {
            //    Use a wxArray for temp storage
            //    then transfer to a simple linear array
            PI_ArrayOfVC_Elements vc_array;
            
            int index = -1;
            int index_max = -1;
            
            pfpx->Read( &index, sizeof(int) );
            
            while( -1 != index ) {
                
                double *pPoint = NULL;
                pPoint = (double *) malloc( 2 * sizeof(double) );
                pfpx->Read( pPoint, 2 * sizeof(double) );
                
                PI_VC_Element vce;
                vce.index = index;
                vce.pPoint = pPoint;
                
                vc_array.Add( vce );
                
                if( index > index_max ) index_max = index;
                
                //    Next element
                pfpx->Read( &index, sizeof(int) );
            }
            
            //    Create a hash map VC_Element pointers as a chart class member
            int n_vc_elements = vc_array.GetCount();
            
            for( int i = 0; i < n_vc_elements; i++ ) {
                PI_VC_Element vc_from_array = vc_array.Item( i );
                PI_VC_Element *vcp = new PI_VC_Element;
                vcp->index = vc_from_array.index;
                vcp->pPoint = vc_from_array.pPoint;
                
                m_vc_hash[vcp->index] = (PI_VC_Element *)vcp;
            }
        }
        
        else if( !strncmp( buf, "SENC", 4 ) ) {
            int senc_file_version;
            sscanf( buf, "SENC Version=%i", &senc_file_version );
            if( senc_file_version != CURRENT_SENC_FORMAT_VERSION ) {
                wxString msg( _T("   Wrong version on SENC file ") );
                msg.Append( FullPath );
                wxLogMessage( msg );
                
                dun = 1;
                ret_val = 1;                   // error
            }
        }
        
        else if( !strncmp( buf, "DATEUPD", 7 ) ) {
            date_upd.Append( wxString( &buf[8], wxConvUTF8 ).BeforeFirst( '\n' ) );
        }
        
        else if( !strncmp( buf, "DATE000", 7 ) ) {
            date_000.Append( wxString( &buf[8], wxConvUTF8 ).BeforeFirst( '\n' ) );
        }
        
        else if( !strncmp( buf, "SCALE", 5 ) ) {
            int ins;
            sscanf( buf, "SCALE=%d", &ins );
            m_Chart_Scale = ins;
        }
        
        else if( !strncmp( buf, "NAME", 4 ) ) {
            m_Name = wxString( &buf[5], wxConvUTF8 ).BeforeFirst( '\n' );
        }
        
        else if( !strncmp( buf, "NOGR", 4 ) ) {
            sscanf( buf, "NOGR=%d", &nGeoFeature );
        }
        
        else if( !strncmp( buf, "Chart Extents:", 14 ) ) {
            float elon, wlon, nlat, slat;
            sscanf( buf, "Chart Extents: %g %g %g %g", &elon, &wlon, &nlat, &slat );
            m_FullExtent.ELON = elon;
            m_FullExtent.WLON = wlon;
            m_FullExtent.NLAT = nlat;
            m_FullExtent.SLAT = slat;
            m_bExtentSet = true;
            
            //  Establish a common reference point for the chart
            m_ref_lat = ( m_FullExtent.NLAT + m_FullExtent.SLAT ) / 2.;
            m_ref_lon = ( m_FullExtent.WLON + m_FullExtent.ELON ) / 2.;
            
        }
        
    }                       //while(!dun)
    
    //      fclose(fpx);
    

//    scli.Close();
#if 0
    //  Wait for output
    for(unsigned int t = 0 ; t < 5 ; t++) {
        wxString outres = scli.GetServerOutput();
        if(outres.Len()){
//            int yyp = 4;
            break;
        }
        wxSleep(1);
    }
    
#endif    
    free( buf );
    
    free( hdr_buf );
    
    if(ret_val)
        return ret_val;
    
    
    //   Decide on pub date to show
        int d000 = 0;
        wxString sd000 =date_000.Mid( 0, 4 );
        wxCharBuffer dbuffer=sd000.ToUTF8();
        if(dbuffer.data())
            d000 = atoi(dbuffer.data() );
        
        int dupd = 0;
        wxString sdupd =date_upd.Mid( 0, 4 );
        wxCharBuffer ubuffer = sdupd.ToUTF8();
        if(ubuffer.data())
            dupd = atoi(ubuffer.data() );
        
        if( dupd > d000 )
            m_PubYear = sdupd;
        else
            m_PubYear = sd000;
        
        
        //    Set some base class values
            wxDateTime upd;
            upd.ParseFormat( date_upd, _T("%Y%m%d") );
            if( !upd.IsValid() ) upd.ParseFormat( _T("20000101"), _T("%Y%m%d") );
            
            upd.ResetTime();
            m_EdDate = upd;
            
            m_SE = m_edtn000;
            m_datum_str = _T("WGS84");
            
            m_SoundingsDatum = _T("MEAN LOWER LOW WATER");
            m_ID = SENCFileName.GetName();
            
            // Validate hash maps....
            
            PI_S57Obj *top;
            PI_S57Obj *nxx;
            
            for( int i = 0; i < PI_PRIO_NUM; ++i ) {
                for( int j = 0; j < PI_LUPNAME_NUM; j++ ) {
                    top = razRules[i][j];
                    while( top != NULL ) {
                        PI_S57Obj *obj = top;
                        
                        ///
                        for( int iseg = 0; iseg < obj->m_n_lsindex; iseg++ ) {
                            int seg_index = iseg * 3;
                            int *index_run = &obj->m_lsindex_array[seg_index];
                            
                            //  Get first connected node
                            int inode = *index_run++;
                            if( ( inode >= 0 ) ) {
                                if( m_vc_hash.find( inode ) == m_vc_hash.end() ) {
                                    //    Must be a bad index in the SENC file
                                    //    Stuff a recognizable flag to indicate invalidity
                                    index_run--;
                                    *index_run = -1;
                                    index_run++;
                                }
                            }
                            
                            //  Get the edge
                            //                              int enode = *index_run++;
                            index_run++;
                            
                            //  Get last connected node
                            int jnode = *index_run++;
                            if( ( jnode >= 0 ) ) {
                                if( m_vc_hash.find( jnode ) == m_vc_hash.end() ) {
                                    //    Must be a bad index in the SENC file
                                    //    Stuff a recognizable flag to indicate invalidity
                                    index_run--;
                                    *index_run = -2;
                                    index_run++;
                                }
                                
                            }
                        }
                        ///
                        nxx = top->next;
                        top = nxx;
                    }
                }
            }
    
    //  Set up the chart context
    m_this_chart_context = (chart_context *)calloc( sizeof(chart_context), 1);
    m_this_chart_context->m_pvc_hash = (void *)&m_vc_hash;
    m_this_chart_context->m_pve_hash = (void *)&m_ve_hash;
    
    m_this_chart_context->ref_lat = m_ref_lat;
    m_this_chart_context->ref_lon = m_ref_lon;
    m_this_chart_context->pFloatingATONArray = pFloatingATONArray;
    m_this_chart_context->pRigidATONArray = pRigidATONArray;
    m_this_chart_context->chart = NULL;
    
    //  Loop and populate all the objects
    for( int i = 0; i < PI_PRIO_NUM; ++i ) {
        for( int j = 0; j < PI_LUPNAME_NUM; j++ ) {
            top = razRules[i][j];
            while( top != NULL ) {
                PI_S57Obj *obj = top;
                obj->m_chart_context = m_this_chart_context;
                top = top->next;
            }
        }
    }
    
    
    return ret_val;
}




PI_InitReturn ChartS63::PostInit( int flags, int cs )
{
    //    SetExtentsFromCOVR();
    
    
    
    //    SENC file is ready, so build the RAZ structure
    if( 0 != BuildRAZFromSENCFile( m_SENCFileName.GetFullPath() ) ) {
        wxString msg( _T("   Cannot load SENC file ") );
        msg.Append( m_SENCFileName.GetFullPath() );
        wxLogMessage( msg );
        
        return PI_INIT_FAIL_RETRY;
    }
    
    //      Check for and if necessary rebuild Thumbnail
    //      Going to be in the global (user) SENC file directory
    
    #if 0
    wxString SENCdir = m_senc_dir;
    if( SENCdir.Last() != m_SENCFileName.GetPathSeparator() )
        SENCdir.Append( m_SENCFileName.GetPathSeparator() );
    
    wxFileName ThumbFileName( SENCdir, m_SENCFileName.GetName(), _T("BMP") );
    
    if( !ThumbFileName.FileExists() || m_bneed_new_thumbnail ) BuildThumbnail(
        ThumbFileName.GetFullPath() );
    #endif
    
    //  Update the member thumbdata structure
    #if 0
    if( ThumbFileName.FileExists() ) {
        wxBitmap *pBMP_NEW;
        #ifdef ocpnUSE_ocpnBitmap
        pBMP_NEW = new ocpnBitmap;
        #else
        pBMP_NEW = new wxBitmap;
        #endif
        if( pBMP_NEW->LoadFile( ThumbFileName.GetFullPath(), wxBITMAP_TYPE_BMP ) ) {
            delete pThumbData;
            pThumbData = new ThumbData;
            m_pDIBThumbDay = pBMP_NEW;
            //                    pThumbData->pDIBThumb = pBMP_NEW;
}
}
#endif

    //    Set the color scheme
    m_global_color_scheme = cs;
    SetColorScheme( cs, false );

//    Build array of contour values for later use by conditional symbology

//    BuildDepthContourArray();
    m_bReadyToRender = true;

    return PI_INIT_OK;
}



//      Rendering Support Methods

//-----------------------------------------------------------------------
//              Calculate and Set ViewPoint Constants
//-----------------------------------------------------------------------

void ChartS63::SetVPParms( const PlugIn_ViewPort &vpt )
{
    //  Set up local SM rendering constants
    m_pixx_vp_center = vpt.pix_width / 2;
    m_pixy_vp_center = vpt.pix_height / 2;
    m_view_scale_ppm = vpt.view_scale_ppm;
    
    toSM_Plugin( vpt.clat, vpt.clon, m_ref_lat, m_ref_lon, &m_easting_vp_center, &m_northing_vp_center );
}

bool ChartS63::AdjustVP( PlugIn_ViewPort &vp_last, PlugIn_ViewPort &vp_proposed )
{
    if( IsCacheValid() ) {
        
        //      If this viewpoint is same scale as last...
        if( vp_last.view_scale_ppm == vp_proposed.view_scale_ppm ) {
            
            double prev_easting_c, prev_northing_c;
            toSM_Plugin( vp_last.clat, vp_last.clon, m_ref_lat, m_ref_lon, &prev_easting_c, &prev_northing_c );
            
            double easting_c, northing_c;
            toSM_Plugin( vp_proposed.clat, vp_proposed.clon, m_ref_lat, m_ref_lon, &easting_c, &northing_c );
            
            //  then require this viewport to be exact integral pixel difference from last
            //  adjusting clat/clat and SM accordingly
            
            double delta_pix_x = ( easting_c - prev_easting_c ) * vp_proposed.view_scale_ppm;
            int dpix_x = (int) round ( delta_pix_x );
            double dpx = dpix_x;
            
            double delta_pix_y = ( northing_c - prev_northing_c ) * vp_proposed.view_scale_ppm;
            int dpix_y = (int) round ( delta_pix_y );
            double dpy = dpix_y;
            
            double c_east_d = ( dpx / vp_proposed.view_scale_ppm ) + prev_easting_c;
            double c_north_d = ( dpy / vp_proposed.view_scale_ppm ) + prev_northing_c;
            
            double xlat, xlon;
            fromSM_Plugin( c_east_d, c_north_d, m_ref_lat, m_ref_lon, &xlat, &xlon );
            
            vp_proposed.clon = xlon;
            vp_proposed.clat = xlat;
            
            return true;
        }
    }
    
    return false;
}

/*
 * bool s57chart::IsRenderDelta(ViewPort &vp_last, ViewPort &vp_proposed)
 * {
 * double last_center_easting, last_center_northing, this_center_easting, this_center_northing;
 * toSM ( vp_proposed.clat, vp_proposed.clon, ref_lat, ref_lon, &this_center_easting, &this_center_northing );
 * toSM ( vp_last.clat,     vp_last.clon,     ref_lat, ref_lon, &last_center_easting, &last_center_northing );
 * 
 * int dx = (int)round((last_center_easting  - this_center_easting)  * vp_proposed.view_scale_ppm);
 * int dy = (int)round((last_center_northing - this_center_northing) * vp_proposed.view_scale_ppm);
 * 
 * return((dx !=  0) || (dy != 0) || !(IsCacheValid()) || (vp_proposed.view_scale_ppm != vp_last.view_scale_ppm));
 }
 */
void ChartS63::GetValidCanvasRegion( const PlugIn_ViewPort& VPoint, wxRegion *pValidRegion )
{
    int rxl, rxr;
    int ryb, ryt;
    double easting, northing;
    double epix, npix;
    
    toSM_Plugin( m_FullExtent.SLAT, m_FullExtent.WLON, VPoint.clat, VPoint.clon, &easting, &northing );
    epix = easting * VPoint.view_scale_ppm;
    npix = northing * VPoint.view_scale_ppm;
    
    rxl = (int) round((VPoint.pix_width / 2) + epix);
    ryb = (int) round((VPoint.pix_height / 2) - npix);
    
    toSM_Plugin( m_FullExtent.NLAT, m_FullExtent.ELON, VPoint.clat, VPoint.clon, &easting, &northing );
    epix = easting * VPoint.view_scale_ppm;
    npix = northing * VPoint.view_scale_ppm;
    
    rxr = (int) round((VPoint.pix_width / 2) + epix);
    ryt = (int) round((VPoint.pix_height / 2) - npix);
    
    pValidRegion->Clear();
    pValidRegion->Union( rxl, ryt, rxr - rxl, ryb - ryt );
}

void ChartS63::SetLinePriorities( void )
{
    #if 0
    if( !ps52plib ) return;
    
    //      If necessary.....
    //      Establish line feature rendering priorities
    
    if( !m_bLinePrioritySet ) {
        ObjRazRules *top;
        ObjRazRules *crnt;
        
        for( int i = 0; i < PRIO_NUM; ++i ) {
            
            top = razRules[i][2];           //LINES
            while( top != NULL ) {
                ObjRazRules *crnt = top;
                top = top->next;
                ps52plib->SetLineFeaturePriority( crnt, i );
}

//    In the interest of speed, choose only the one necessary area boundary style index
int j;
if( ps52plib->m_nBoundaryStyle == SYMBOLIZED_BOUNDARIES ) j = 4;
else
    j = 3;

top = razRules[i][j];
while( top != NULL ) {
    crnt = top;
    top = top->next;               // next object
    ps52plib->SetLineFeaturePriority( crnt, i );
}

}
}

//      Mark the priority as set.
//      Generally only reset by Options Dialog post processing
m_bLinePrioritySet = true;
#endif
}

void ChartS63::ResetPointBBoxes( const PlugIn_ViewPort &vp_last, const PlugIn_ViewPort &vp_this )
{
    #if 0
    ObjRazRules *top;
    ObjRazRules *nxx;
    
    double box_margin = 0.25;
    
    //    Assume a 50x50 pixel box
    box_margin = ( 50. / vp_this.view_scale_ppm ) / ( 1852. * 60. );  //degrees
    
    for( int i = 0; i < PRIO_NUM; ++i ) {
        top = razRules[i][0];
        
        while( top != NULL ) {
            if( !top->obj->geoPtMulti )                      // do not reset multipoints
            {
                top->obj->bBBObj_valid = false;
                top->obj->BBObj.SetMin( top->obj->m_lon - box_margin,
                top->obj->m_lat - box_margin );
                top->obj->BBObj.SetMax( top->obj->m_lon + box_margin,
                top->obj->m_lat + box_margin );
}

nxx = top->next;
top = nxx;
}

top = razRules[i][1];

while( top != NULL ) {
    if( !top->obj->geoPtMulti )                      // do not reset multipoints
            {
                top->obj->bBBObj_valid = false;
                top->obj->BBObj.SetMin( top->obj->m_lon - box_margin,
                top->obj->m_lat - box_margin );
                top->obj->BBObj.SetMax( top->obj->m_lon + box_margin,
                top->obj->m_lat + box_margin );
}

nxx = top->next;
top = nxx;
}
}
#endif
}






bool ChartS63::DoRenderRegionViewOnDC( wxMemoryDC& dc, const PlugIn_ViewPort& VPoint,
                                       const wxRegion &Region, bool b_overlay )
{
    
    SetVPParms( VPoint );
    
    bool force_new_view = false;
    
    if( Region != m_last_Region ) force_new_view = true;
    
    //    ps52plib->PrepareForRender();
    
    #if 0
    if( m_plib_state_hash != ps52plib->GetStateHash() ) {
        m_bLinePrioritySet = false;                     // need to reset line priorities
        UpdateLUPs( this );                               // and update the LUPs
        ClearRenderedTextCache();                       // and reset the text renderer,
        //for the case where depth(height) units change
        ResetPointBBoxes( m_last_vp, VPoint );
}
#endif

    if( VPoint.view_scale_ppm != m_last_vp.view_scale_ppm ) {
        ResetPointBBoxes( m_last_vp, VPoint );
    }

    SetLinePriorities();

    bool bnew_view = DoRenderViewOnDC( dc, VPoint, force_new_view );

//    If quilting, we need to return a cloned bitmap instead of the original golden item
    if( VPoint.b_quilt ) {
        if( m_pCloneBM ) {
            if( ( m_pCloneBM->GetWidth() != VPoint.pix_width )|| ( m_pCloneBM->GetHeight() != VPoint.pix_height ) ) {
                delete m_pCloneBM;
                m_pCloneBM = NULL;
                }
        }
        if( NULL == m_pCloneBM )
            m_pCloneBM = new wxBitmap( VPoint.pix_width, VPoint.pix_height, -1 );
    
        wxMemoryDC dc_clone;
        dc_clone.SelectObject( *m_pCloneBM );
    
//    #ifdef ocpnUSE_DIBSECTION
//    ocpnMemDC memdc, dc_org;
//    #else
    wxMemoryDC memdc, dc_org;
//    #endif
    
//        pDIB->SelectIntoDC( dc_org );
        dc_org.SelectObject( *pDIB );
        
    //    Decompose the region into rectangles, and fetch them into the target dc
        wxRegionIterator upd( Region ); // get the requested rect list
        while( upd.HaveRects() ) {
            wxRect rect = upd.GetRect();
            dc_clone.Blit( rect.x, rect.y, rect.width, rect.height, &dc_org, rect.x, rect.y );
            upd++;
        }
    
        dc_clone.SelectObject( wxNullBitmap );
        dc_org.SelectObject( wxNullBitmap );
    
    //    Create a mask
        if( b_overlay ) {
            wxColour nodat = GetBaseGlobalColor( _T ( "NODTA" ) );
            wxColour nodat_sub = nodat;
        
//        #ifdef ocpnUSE_ocpnBitmap
//            nodat_sub = wxColour( nodat.Blue(), nodat.Green(), nodat.Red() );
//        #endif
            m_pMask = new wxMask( *m_pCloneBM, nodat_sub );
            m_pCloneBM->SetMask( m_pMask );
        }
    
        dc.SelectObject( *m_pCloneBM );
    } else {
//        pDIB->SelectIntoDC( dc );
        dc.SelectObject( *pDIB );
    }

    m_last_Region = Region;

    return bnew_view;

}

wxBitmap *ChartS63::GetCloneBitmap()
{
    wxRegion Region = m_last_Region;
    PlugIn_ViewPort VPoint = m_last_vp;
    
    if( m_pCloneBM ) {
        if( ( m_pCloneBM->GetWidth() != VPoint.pix_width )
            || ( m_pCloneBM->GetHeight() != VPoint.pix_height ) ) {
            delete m_pCloneBM;
        m_pCloneBM = NULL;
            }
    }
    if( NULL == m_pCloneBM ) m_pCloneBM = new wxBitmap( VPoint.pix_width, VPoint.pix_height,
        -1 );
    
    wxMemoryDC dc_clone;
    dc_clone.SelectObject( *m_pCloneBM );
    
//    #ifdef ocpnUSE_DIBSECTION
//    ocpnMemDC memdc, dc_org;
//    #else
    wxMemoryDC memdc, dc_org;
//    #endif
    
//    pDIB->SelectIntoDC( dc_org );
    dc_org.SelectObject( *pDIB );

    //    Decompose the region into rectangles, and fetch them into the target dc
    wxRegionIterator upd( Region ); // get the requested rect list
    while( upd.HaveRects() ) {
        wxRect rect = upd.GetRect();
        dc_clone.Blit( rect.x, rect.y, rect.width, rect.height, &dc_org, rect.x, rect.y );
        upd++;
    }
    
    dc_clone.SelectObject( wxNullBitmap );
    dc_org.SelectObject( wxNullBitmap );
    
    #if 0
    //    Create a mask
    if( b_overlay ) {
            wxColour nodat = GetBaseGlobalColor( _T ( "NODTA" ) );
        wxColour nodat_sub = nodat;
        
        #ifdef ocpnUSE_ocpnBitmap
        nodat_sub = wxColour( nodat.Blue(), nodat.Green(), nodat.Red() );
        #endif
        m_pMask = new wxMask( *m_pCloneBM, nodat_sub );
        m_pCloneBM->SetMask( m_pMask );
}
#endif
return m_pCloneBM;

}


bool ChartS63::RenderViewOnDC( wxMemoryDC& dc, const PlugIn_ViewPort& VPoint )
{
    //    CALLGRIND_START_INSTRUMENTATION
    
    SetVPParms( VPoint );
    
    //    ps52plib->PrepareForRender();
    
    //    if( m_plib_state_hash != ps52plib->GetStateHash() ) {
            //        m_bLinePrioritySet = false;                     // need to reset line priorities
    //        UpdateLUPs( this );                               // and update the LUPs
    //        ClearRenderedTextCache();                       // and reset the text renderer
    //    }
    
    SetLinePriorities();
    
    bool bnew_view = DoRenderViewOnDC( dc, VPoint, false );
    
//    pDIB->SelectIntoDC( dc );
    dc.SelectObject( *pDIB );  
    
    return bnew_view;
    
    //    CALLGRIND_STOP_INSTRUMENTATION
    
}

bool ChartS63::DoRenderViewOnDC( wxMemoryDC& dc, const PlugIn_ViewPort& VPoint, bool force_new_view )
{
    bool bnewview = false;
    wxPoint rul, rlr;
    bool bNewVP = false;
    
    bool bReallyNew = false;
    
    double easting_ul, northing_ul;
    double easting_lr, northing_lr;
    double prev_easting_ul = 0., prev_northing_ul = 0.;
    //    double prev_easting_lr, prev_northing_lr;
    
    if( PI_GetPLIBColorScheme() != m_lastColorScheme )
        bReallyNew = true;
    m_lastColorScheme = PI_GetPLIBColorScheme();
    
    if( VPoint.view_scale_ppm != m_last_vp.view_scale_ppm )
        bReallyNew = true;
    
    //      If the scale is very small, do not use the cache to avoid harmonic difficulties...
        if( VPoint.chart_scale > 1e8 )
            bReallyNew = true;
        
        wxRect dest( 0, 0, VPoint.pix_width, VPoint.pix_height );
        if( m_last_vprect != dest ) bReallyNew = true;
                                            m_last_vprect = dest;
        
        //    if( m_plib_state_hash != ps52plib->GetStateHash() ) {
            //        bReallyNew = true;
                                            //        m_plib_state_hash = ps52plib->GetStateHash();
                                            //    }
                                            
                                            if( bReallyNew ) {
                                                bNewVP = true;
                                                delete pDIB;
                                                pDIB = NULL;
                                                bnewview = true;
                                            }
                                            
                                            //      Calculate the desired rectangle in the last cached image space
                                            if( 0/*m_last_vp.IsValid()*/ ) {
                                                easting_ul = m_easting_vp_center - ( ( VPoint.pix_width / 2 ) / m_view_scale_ppm );
                                                northing_ul = m_northing_vp_center + ( ( VPoint.pix_height / 2 ) / m_view_scale_ppm );
                                                easting_lr = easting_ul + ( VPoint.pix_width / m_view_scale_ppm );
                                                northing_lr = northing_ul - ( VPoint.pix_height / m_view_scale_ppm );
                                                
                                                double last_easting_vp_center, last_northing_vp_center;
                                                toSM_Plugin( m_last_vp.clat, m_last_vp.clon, m_ref_lat, m_ref_lon, &last_easting_vp_center,
                                                      &last_northing_vp_center );
                                                
                                                prev_easting_ul = last_easting_vp_center
                                                - ( ( m_last_vp.pix_width / 2 ) / m_view_scale_ppm );
                                                prev_northing_ul = last_northing_vp_center
                                                + ( ( m_last_vp.pix_height / 2 ) / m_view_scale_ppm );
                                                //        prev_easting_lr = easting_ul + ( m_last_vp.pix_width / m_view_scale_ppm );
                                                //        prev_northing_lr = northing_ul - ( m_last_vp.pix_height / m_view_scale_ppm );
                                                
                                                double dx = ( easting_ul - prev_easting_ul ) * m_view_scale_ppm;
                                                double dy = ( prev_northing_ul - northing_ul ) * m_view_scale_ppm;
                                                
                                                rul.x = (int) round((easting_ul - prev_easting_ul) * m_view_scale_ppm);
                                                rul.y = (int) round((prev_northing_ul - northing_ul) * m_view_scale_ppm);
                                                
                                                rlr.x = (int) round((easting_lr - prev_easting_ul) * m_view_scale_ppm);
                                                rlr.y = (int) round((prev_northing_ul - northing_lr) * m_view_scale_ppm);
                                                
                                                if( ( fabs( dx - wxRound( dx ) ) > 1e-5 ) || ( fabs( dy - wxRound( dy ) ) > 1e-5 ) ) {
                                                    rul.x = 0;
                                                    rul.y = 0;
                                                    rlr.x = 0;
                                                    rlr.y = 0;
                                                    bNewVP = true;
                                                }
                                                
                                                else if( ( rul.x != 0 ) || ( rul.y != 0 ) ) {
                                                    bNewVP = true;
                                                }
                                            } else {
                                                rul.x = 0;
                                                rul.y = 0;
                                                rlr.x = 0;
                                                rlr.y = 0;
                                                bNewVP = true;
                                            }
                                            
                                            if( force_new_view ) bNewVP = true;
                                            
                                            //      Using regions, calculate re-usable area of pDIB
                                            
                                            wxRegion rgn_last( 0, 0, VPoint.pix_width, VPoint.pix_height );
                                            wxRegion rgn_new( rul.x, rul.y, rlr.x - rul.x, rlr.y - rul.y );
                                            rgn_last.Intersect( rgn_new );            // intersection is reusable portion
                                            
                                            if( bNewVP && ( NULL != pDIB ) && !rgn_last.IsEmpty() ) {
                                                int xu, yu, wu, hu;
                                                rgn_last.GetBox( xu, yu, wu, hu );
                                                
                                                int desx = 0;
                                                int desy = 0;
                                                int srcx = xu;
                                                int srcy = yu;
                                                
                                                if( rul.x < 0 ) {
                                                    srcx = 0;
                                                    desx = -rul.x;
                                                }
                                                if( rul.y < 0 ) {
                                                    srcy = 0;
                                                    desy = -rul.y;
                                                }
                                                
                                                wxMemoryDC dc_last;
//                                                pDIB->SelectIntoDC( dc_last );
                                                 dc_last.SelectObject( *pDIB );
                                                wxMemoryDC dc_new;
//                                                PixelCache *pDIBNew = new PixelCache( VPoint.pix_width, VPoint.pix_height, BPP );
                                                wxBitmap *pDIBNew = new wxBitmap( VPoint.pix_width, VPoint.pix_height, BPP );
//                                                pDIBNew->SelectIntoDC( dc_new );
                                                dc_new.SelectObject( *pDIBNew );
                                                
                                                //        printf("reuse blit %d %d %d %d %d %d\n",desx, desy, wu, hu,  srcx, srcy);
                                                dc_new.Blit( desx, desy, wu, hu, (wxDC *) &dc_last, srcx, srcy, wxCOPY );
                                                
                                                //        Ask the plib to adjust the persistent text rectangle list for this canvas shift
                                                //        This ensures that, on pans, the list stays in registration with the new text renders to come
                                                //        ps52plib->AdjustTextList( desx - srcx, desy - srcy, VPoint.pix_width, VPoint.pix_height );
                                                
                                                dc_new.SelectObject( wxNullBitmap );
                                                dc_last.SelectObject( wxNullBitmap );
                                                
                                                delete pDIB;
                                                pDIB = pDIBNew;
                                                
                                                //              OK, now have the re-useable section in place
                                                //              Next, build the new sections
                                                
//                                                pDIB->SelectIntoDC( dc );
                                                dc.SelectObject( *pDIB );
                                                wxRegion rgn_delta( 0, 0, VPoint.pix_width, VPoint.pix_height );
                                                wxRegion rgn_reused( desx, desy, wu, hu );
                                                rgn_delta.Subtract( rgn_reused );
                                                
                                                wxRegionIterator upd( rgn_delta ); // get the update rect list
                                                while( upd.HaveRects() ) {
                                                    wxRect rect = upd.GetRect();
                                                    
                                                    //      Build temp ViewPort on this region
                                                    
                                                    PlugIn_ViewPort temp_vp = VPoint;
                                                    double temp_lon_left, temp_lat_bot, temp_lon_right, temp_lat_top;
                                                    
                                                    double temp_northing_ul = prev_northing_ul - ( rul.y / m_view_scale_ppm )
                                                    - ( rect.y / m_view_scale_ppm );
                                                    double temp_easting_ul = prev_easting_ul + ( rul.x / m_view_scale_ppm )
                                                    + ( rect.x / m_view_scale_ppm );
                                                    fromSM_Plugin( temp_easting_ul, temp_northing_ul, m_ref_lat, m_ref_lon, &temp_lat_top,
                                                            &temp_lon_left );
                                                    
                                                    double temp_northing_lr = temp_northing_ul - ( rect.height / m_view_scale_ppm );
                                                    double temp_easting_lr = temp_easting_ul + ( rect.width / m_view_scale_ppm );
                                                    fromSM_Plugin( temp_easting_lr, temp_northing_lr, m_ref_lat, m_ref_lon, &temp_lat_bot,
                                                            &temp_lon_right );
                                                    
                                                    //            temp_vp.GetBBox().SetMin( temp_lon_left, temp_lat_bot );
                                                    //            temp_vp.GetBBox().SetMax( temp_lon_right, temp_lat_top );
                                                    
                                                    temp_vp.lat_min = temp_lat_bot;
                                                    temp_vp.lat_max = temp_lat_top;
                                                    temp_vp.lon_min = temp_lon_left;
                                                    temp_vp.lon_max = temp_lon_right;
                                                    
                                                    //      Allow some slop in the viewport
                                                    //    TODO Investigate why this fails if greater than 5 percent
                                                    //            double margin = wxMin(temp_vp.GetBBox().GetWidth(), temp_vp.GetBBox().GetHeight())
                                                    //                    * 0.05;
                                                    
                                                    //            temp_vp.GetBBox().EnLarge( margin );
                                                    
                                                    //      And Render it new piece on the target dc
                                                    //     printf("New Render, rendering %d %d %d %d \n", rect.x, rect.y, rect.width, rect.height);
                                                    
                                                    DCRenderRect( dc, temp_vp, &rect );
                                                    
                                                    upd++;
                                                }
                                                
                                                dc.SelectObject( wxNullBitmap );
                                                
                                                bnewview = true;
                                                
                                                //      Update last_vp to reflect the current cached bitmap
                                                m_last_vp = VPoint;
                                                
                                            }
                                            
                                            else if( bNewVP || ( NULL == pDIB ) ) {
                                                delete pDIB;
//                                                pDIB = new PixelCache( VPoint.pix_width, VPoint.pix_height, BPP );     // destination
                                                pDIB = new wxBitmap( VPoint.pix_width, VPoint.pix_height, BPP );     // destination
                                                
                                                wxRect full_rect( 0, 0, VPoint.pix_width, VPoint.pix_height );
//                                                pDIB->SelectIntoDC( dc );
                                                dc.SelectObject( *pDIB );
                                                
                                                //        Clear the text declutter list
                                                //        ps52plib->ClearTextList();
                                                
                                                DCRenderRect( dc, VPoint, &full_rect );
                                                
                                                dc.SelectObject( wxNullBitmap );
                                                
                                                bnewview = true;
                                                
                                                //      Update last_vp to reflect the current cached bitmap
                                                m_last_vp = VPoint;
                                                
                                            }
                                            
                                            return bnewview;
                                            
}

int ChartS63::DCRenderRect( wxMemoryDC& dcinput, const PlugIn_ViewPort& vp, wxRect* rect )
{
//    ScreenLogMessage(_T("Render\n"));
    
    int i;
    PI_S57Obj *top;
    PI_S57Obj *crnt;
    
    PlugIn_ViewPort tvp = vp;                    // undo const  TODO fix this in PLIB
    
    
    int depth = BPP;
    int pb_pitch = ( ( rect->width * depth / 8 ) );
    unsigned char *pixbuf = (unsigned char *) malloc( rect->height * pb_pitch );
    int width = rect->width;
    int height = rect->height;
    int pbx = rect->x;
    int pby = rect->y;
    
    // Preset background
    wxColour color = GetBaseGlobalColor( _T ( "NODTA" ) );
    unsigned char r, g, b;
    if( color.IsOk() ) {
        r = color.Red();
        g = color.Green();
        b = color.Blue();
    } else
        r = g = b = 0;
    
    if( depth == 24 ) {
        for( int i = 0; i < height; i++ ) {
            unsigned char *p = pixbuf + ( i * pb_pitch );
            for( int j = 0; j < width; j++ ) {
                *p++ = r;
                *p++ = g;
                *p++ = b;
            }
        }
    } else {
        int color_int = ( ( r ) << 16 ) + ( ( g ) << 8 ) + ( b );
        
        for( int i = 0; i < height; i++ ) {
            int *p = (int *) ( pixbuf + ( i * pb_pitch ) );
            for( int j = 0; j < width; j++ ) {
                *p++ = color_int;
            }
        }
    }
    
    //      Render the areas quickly
    for( i = 0; i < PI_PRIO_NUM; ++i ) {
        if( PI_GetPLIBBoundaryStyle() == PI_SYMBOLIZED_BOUNDARIES )
            top = razRules[i][4]; // Area Symbolized Boundaries
            else
                top = razRules[i][3];           // Area Plain Boundaries
                
                while( top != NULL ) {
                    crnt = top;
                    top = top->next;               // next object
                    PI_PLIBRenderAreaToDC( &dcinput, crnt, &tvp, *rect, pixbuf );
                }
    }
    
    //      Convert the Private render canvas into a bitmap
//    #ifdef ocpnUSE_ocpnBitmap
//    ocpnBitmap *pREN = new ocpnBitmap( pixbuf, width, height, depth );
//    #else
    wxImage *prender_image = new wxImage(width, height, false);
    prender_image->SetData((unsigned char*)pixbuf);
    wxBitmap *pREN = new wxBitmap(*prender_image);
    
//    #endif
    
    //      Map it into a temporary DC
    wxMemoryDC dc_ren;
    dc_ren.SelectObject( *pREN );
    
    //      Blit it onto the target dc
    dcinput.Blit( pbx, pby, width, height, (wxDC *) &dc_ren, 0, 0 );
    
    //      And clean up the mess
    dc_ren.SelectObject( wxNullBitmap );
    
//    #ifdef ocpnUSE_ocpnBitmap
//    free( pixbuf );
//    #else
    delete prender_image;           // the image owns the data
    // and so will free it in due course
//    #endif
    
    delete pREN;
    
    //      Render the rest of the objects/primitives
    DCRenderLPB( dcinput, vp, rect );
    
    return 1;
}

bool ChartS63::DCRenderLPB( wxMemoryDC& dcinput, const PlugIn_ViewPort& vp, wxRect* rect )
{
    int i;
    PI_S57Obj *top;
    PI_S57Obj *crnt;
    PlugIn_ViewPort tvp = vp;                    // undo const  TODO fix this in PLIB
    
    for( i = 0; i < PI_PRIO_NUM; ++i ) {
        //      Set up a Clipper for Lines
        wxDCClipper *pdcc = NULL;
        if( rect ) {
            //            wxRect nr = *rect;
            //         pdcc = new wxDCClipper(dcinput, nr);
        }
        
        if( PI_GetPLIBBoundaryStyle() == PI_SYMBOLIZED_BOUNDARIES )
            top = razRules[i][4]; // Area Symbolized Boundaries
            else
                top = razRules[i][3];           // Area Plain Boundaries
                while( top != NULL ) {
                    crnt = top;
                    top = top->next;               // next object
                    PI_PLIBRenderObjectToDC( &dcinput, crnt, &tvp );
                }
                
                top = razRules[i][2];           //LINES
                while( top != NULL ) {
                    PI_S57Obj *crnt = top;
                    top = top->next;
                    PI_PLIBRenderObjectToDC( &dcinput, crnt, &tvp );
                }
                
                if( PI_GetPLIBSymbolStyle() == PI_SIMPLIFIED )
                    top = razRules[i][0];       //SIMPLIFIED Points
                    else
                        top = razRules[i][1];           //Paper Chart Points Points
                        
                        while( top != NULL ) {
                            crnt = top;
                            top = top->next;
                            PI_PLIBRenderObjectToDC( &dcinput, crnt, &tvp );
                        }
                        
                        //      Destroy Clipper
                        if( pdcc ) delete pdcc;
    }
    
    /*
     *     printf("Render Lines                  %ldms\n", stlines.Time());
     *     printf("Render Simple Points          %ldms\n", stsim_pt.Time());
     *     printf("Render Paper Points           %ldms\n", stpap_pt.Time());
     *     printf("Render Symbolized Boundaries  %ldms\n", stasb.Time());
     *     printf("Render Plain Boundaries       %ldms\n\n", stapb.Time());
     */
    return true;
}


//----------------------------------------------------------------------------------
//      SENC Server Process container Implementation
//----------------------------------------------------------------------------------

ServerProcess::ServerProcess()
{
    term_happened = false;
}

ServerProcess::~ServerProcess()
{
}


void ServerProcess::OnTerminate(int pid, int status)
{
    wxInputStream *pis = GetInputStream();
    if(pis){
        while(pis->CanRead())
        {
            char c = pis->GetC();
            m_outstring += c;
        }
    }
    
    term_happened = true;
    
    wxPrintf(_T("ServerProcess::OnTerminate\n"));
    wxPrintf(_T("%s"), m_outstring.c_str());
}





//------------------------------------------------------------------------------
//    SENCclient Implementation
//------------------------------------------------------------------------------

SENCclient::SENCclient(void)
{
    m_sock = NULL;
    m_private_eof = false;
    m_OK = false;
    m_server_pid = 0;
    m_sproc = NULL;
}


void SENCclient::Attach(const wxString &senc_file_name)
{
    m_senc_file = senc_file_name;
    
    g_frontchannel_port++;
    
    if(1){
        //  Start the SENC server
        
        m_sproc = new ServerProcess;
        m_sproc->Redirect();
        wxString cmd = g_sencutil_bin;
        cmd += _T(" -t -s "); 
        cmd += senc_file_name;
        
        cmd += _T(" -b ");
        wxString port;
        port.Printf( _T("%d"), g_backchannel_port );
        cmd += port;
        
        cmd += _T(" -f ");
        port.Printf( _T("%d"), g_frontchannel_port );
        cmd += port;
        
        wxLogMessage(cmd);
        
        wxPrintf(_T(" Starting SENC server...\n") );
        m_server_pid = wxExecute(cmd, wxEXEC_ASYNC, m_sproc);
        
        if(m_server_pid)
            m_OK = true;
        
        //        wxSleep(2);
            
            unsigned int t = 0;
            if(m_OK) {
                m_OK = false;
                for( t=0 ; t < 100 ; t++){
                    if(!Open()){
                        m_OK = true;
                        break;
                    }
                    else
                        wxMilliSleep(100);
                }
            }
            
            if(m_OK){
                if(reset())
                    m_OK = false;
            }
            
            if( m_OK )
                wxPrintf(_T(" Open OK\n") );
            else{
                ScreenLogMessage( _T("   Error: Cannot start eSENC server: ") + g_sencutil_bin +_T("\n") );
            }
            
            
    }
}


SENCclient::~SENCclient()
{
    if(m_sproc)
        m_sproc->Detach();
    
}


wxString SENCclient::GetServerOutput()
{
    if(m_sproc &&  m_sproc->term_happened) {
        return m_sproc->m_outstring;
    }
    
    return _T("");
}


void SENCclient::Close()
{
    if( m_sock && m_sock->IsConnected() ){
        char c = 't';                           // terminate
        m_sock->Write(&c, 1);
    }
    else{
        if( m_sproc ){
            m_sproc->Detach();
            m_sproc->Kill(m_server_pid);
        }
    }
}


int SENCclient::Open(void)
{
    // Create the socket
    m_sock = new wxSocketClient();
    
    // Setup the event handler and subscribe to most events
    //    m_sock->SetEventHandler(*this, SOCKET_ID);
    //    m_sock->SetNotify(wxSOCKET_CONNECTION_FLAG |
    //    wxSOCKET_INPUT_FLAG |
    //    wxSOCKET_LOST_FLAG);
    //    m_sock->Notify(true);
    
    
    wxIPV4address addr;
    
    addr.Hostname( _T("127.0.0.1") );
    addr.Service(g_frontchannel_port);
    
    //          Connect to the server
    m_sock->Connect(addr, false);
    
    if(! m_sock->WaitOnConnect(2, 0) ){
        delete m_sock;
        m_sock = 0;
        return -2;
    }
    
    int ret_val;
    
    if( m_sock->IsConnected() )
        ret_val = 0;
    else{
        delete m_sock;
        m_sock = 0;
        ret_val = -1;
    }
    
    return ret_val;
}


int SENCclient::reset(void)
{
    int ret_val = 0;
    
    if( m_sock && m_sock->IsConnected() ){
        char c = 'r';
        m_sock->Write(&c, 1);
        if( m_sock->Error() ){
            ret_val = -2;
        }
        if(m_sock->LastCount() != 1) {
            ret_val = -3;
        }
    }
    else {
        ret_val = -4;
    }
    
    
    
    return ret_val;
}


int SENCclient::NetRead(void *destination, size_t length, size_t *read_actual)
{
    int retval = 0;
    size_t lc = 0;   
    
    if( m_sock && m_sock->IsConnected() ){
        char c = 'd';
        m_sock->Write(&c, 1);
        if( m_sock->Error() ){
            retval = -2;
            goto fast_return;
        }
        if(m_sock->LastCount() != 1) {
            retval = -3;
            goto fast_return;
        }
        
        int xlen = length;
        m_sock->Write(&xlen, sizeof(int));
        if( m_sock->Error() ){
            retval = -5;
            goto fast_return;
        }
        if(m_sock->LastCount() != 4) {
            retval = -6;
            goto fast_return;
        }
        
        m_sock->ReadMsg( destination, length );
        lc = m_sock->LastCount();
        if(lc != length) {
            retval = -8;
            goto fast_return;
        }
        if( m_sock->Error() ){
            retval = -7;
            goto fast_return;
        }
    }
    else
        retval = -4;
    
    fast_return:
    
    if( read_actual )
        *read_actual = lc;
    
    return retval;
}

#if 0
int SENCclient::UnRead(char *destination, int length)
{
    
    return 0;
    }
    #endif
    
    #if 0
    int SENCclient::fgets( char *buf, int buf_len_max )
    
    {
        char chNext;
        int nLineLen = 0;
        
        char *lbuf;
        
        lbuf = buf;
        
        while( !Eof() && nLineLen < buf_len_max ) {
            chNext = (char) GetC();
            
            /* each CR/LF (or LF/CR) as if just "CR" */
            if( chNext == 10 || chNext == 13 ) {
                chNext = '\n';
            }
            
            *lbuf = chNext;
            lbuf++, nLineLen++;
            
            if( chNext == '\n' ) {
                *lbuf = '\0';
                return nLineLen;
            }
        }
        *( lbuf ) = '\0';
        
        return nLineLen;
    }
    
    #endif
    
    #if 0
    int SENCclient::fgets( char *destination, int max_length)
    {
        int ret_val = 0;
        
        if( m_sock && m_sock->IsConnected() ){
            char c = 'f';
            m_sock->Write(&c, 1);
            if( m_sock->Errorsize_t SENCclient::OnSysRead(void *buffer, size_t size)
                () ){
                    ret_val = -2;
                    goto fast_return;
                }
                if(m_sock->LastCount() != 1) {
                    ret_val = -3;
                    goto fast_return;
                }
                
                m_sock->ReadMsg( destination, max_length );
            if( m_sock->Error() ){
                ret_val = -5;
                goto fast_return;
            }
            ret_val = m_sock->LastCount();
            char *dd = destination + ret_val;
            *dd = 0;
        }
        else {
            ret_val = -4;
        }
        
        
        fast_return:
        return ret_val;
    }
    #endif
    
    size_t SENCclient::OnSysRead(void *buffer, size_t size)
    {
        size_t read_actual;
        int stat =  NetRead(buffer, size, &read_actual );
        
        //    wxPrintf(_T("OnSysRead %d/%d\n"), read_actual, size );
        if( stat < 0 ){
            if( -8 == stat ) {
                m_lasterror = wxSTREAM_EOF;
                m_private_eof = true;
            }
            else {    
                m_lasterror = wxSTREAM_READ_ERROR;
                read_actual = 0;
            }
        }
        
        return read_actual;
    }
    
    
bool SENCclient::Eof() const
{
    return m_private_eof;
}
    

    
//------------------------------------------------------------------------------
//      Local version of fgets for Binary Mode (SENC) file
//------------------------------------------------------------------------------
int py_fgets( char *buf, int buf_len_max, wxInputStream *ifs )

{
    char chNext;
    int nLineLen = 0;
    char *lbuf;
    
    lbuf = buf;
    
    while( !ifs->Eof() && nLineLen < buf_len_max ) {
        int c = ifs->GetC();
         
        if(c != wxEOF ) {
            chNext = (char) c;
            /* each CR/LF (or LF/CR) as if just "CR" */
            if( chNext == 10 || chNext == 13 ) {
                chNext = '\n';
            }
        
            *lbuf = chNext;
            lbuf++, nLineLen++;
        
            if( chNext == '\n' ) {
                *lbuf = '\0';
                return nLineLen;
            }
        }
        else {
            *( lbuf ) = '\0';
            return nLineLen;
        }
    }
    
    *( lbuf ) = '\0';
    
    return nLineLen;
}

    
    
//----------------------------------------------------------------------------------
//      PI_S57Obj CTOR
//----------------------------------------------------------------------------------

PI_S57Obj::PI_S57Obj()
{
    att_array = NULL;
    attVal = NULL;
    n_attr = 0;

 //   bCS_Added = 0;
//    CSrules = NULL;
//    FText = NULL;
 //   bFText_Added = 0;
    geoPtMulti = NULL;
    geoPtz = NULL;
    geoPt = NULL;
    bIsClone = false;
    Scamin = 10000000;                              // ten million enough?
    nRef = 0;

    bIsAton = false;
    bIsAssociable = false;
    m_n_lsindex = 0;
    m_lsindex_array = NULL;
    m_n_edge_max_points = 0;

    bBBObj_valid = false;

    //        Set default (unity) auxiliary transform coefficients
    x_rate = 1.0;
    y_rate = 1.0;
    x_origin = 0.0;
    y_origin = 0.0;

    S52_Context = NULL;
}

//----------------------------------------------------------------------------------
//      PI_S57Obj DTOR
//----------------------------------------------------------------------------------

PI_S57Obj::~PI_S57Obj()
{
    //  Don't delete any allocated records of simple copy clones
    if( !bIsClone ) {
        if( attVal ) {
            for( unsigned int iv = 0; iv < attVal->GetCount(); iv++ ) {
                S57attVal *vv = attVal->Item( iv );
                void *v2 = vv->value;
                free( v2 );
                delete vv;
            }
            delete attVal;
        }
        free( att_array );

//        if( pPolyTessGeo ) delete pPolyTessGeo;

//        if( pPolyTrapGeo ) delete pPolyTrapGeo;

//        if( FText ) delete FText;

        if( geoPt ) free( geoPt );
        if( geoPtz ) free( geoPtz );
        if( geoPtMulti ) free( geoPtMulti );

        if( m_lsindex_array ) free( m_lsindex_array );
    }
}








//----------------------------------------------------------------------------------
//      PI_S57Obj CTOR
//----------------------------------------------------------------------------------

PI_S57ObjX::PI_S57ObjX()
{
    att_array = NULL;
    attVal = NULL;
    n_attr = 0;

 //   bCS_Added = 0;
//    CSrules = NULL;
//    FText = NULL;
 //   bFText_Added = 0;
    geoPtMulti = NULL;
    geoPtz = NULL;
    geoPt = NULL;
    bIsClone = false;
    Scamin = 10000000;                              // ten million enough?
    nRef = 0;

    bIsAton = false;
    bIsAssociable = false;
    m_n_lsindex = 0;
    m_lsindex_array = NULL;
    m_n_edge_max_points = 0;

    bBBObj_valid = false;

    //        Set default (unity) auxiliary transform coefficients
    x_rate = 1.0;
    y_rate = 1.0;
    x_origin = 0.0;
    y_origin = 0.0;

    S52_Context = NULL;
}

//----------------------------------------------------------------------------------
//      PI_S57Obj DTOR
//----------------------------------------------------------------------------------

PI_S57ObjX::~PI_S57ObjX()
{
    //  Don't delete any allocated records of simple copy clones
    if( !bIsClone ) {
        if( attVal ) {
            for( unsigned int iv = 0; iv < attVal->GetCount(); iv++ ) {
                S57attVal *vv = attVal->Item( iv );
                void *v2 = vv->value;
                free( v2 );
                delete vv;
            }
            delete attVal;
        }
        free( att_array );

//        if( pPolyTessGeo ) delete pPolyTessGeo;

//        if( pPolyTrapGeo ) delete pPolyTrapGeo;

//        if( FText ) delete FText;

        if( geoPt ) free( geoPt );
        if( geoPtz ) free( geoPtz );
        if( geoPtMulti ) free( geoPtMulti );

        if( m_lsindex_array ) free( m_lsindex_array );
    }
}

//----------------------------------------------------------------------------------
//      PI_S57ObjX CTOR from SENC file
//----------------------------------------------------------------------------------

PI_S57ObjX::PI_S57ObjX( char *first_line, wxInputStream *fpx )
{
    att_array = NULL;
    attVal = NULL;
    n_attr = 0;

//    pPolyTessGeo = NULL;
//    pPolyTrapGeo = NULL;
//    bCS_Added = 0;
//    CSrules = NULL;
//    FText = NULL;
//    bFText_Added = 0;
    bIsClone = false;

    geoPtMulti = NULL;
    geoPtz = NULL;
    geoPt = NULL;
    Scamin = 10000000;                              // ten million enough?
    nRef = 0;
    bIsAton = false;
    m_n_lsindex = 0;
    m_lsindex_array = NULL;
    S52_Context = NULL;

    //        Set default (unity) auxiliary transform coefficients
    x_rate = 1.0;
    y_rate = 1.0;
    x_origin = 0.0;
    y_origin = 0.0;

    if( strlen( first_line ) == 0 ) return;

    int FEIndex;

    int MAX_LINE = 499999;
    char *buf = (char *) malloc( MAX_LINE + 1 );
    int llmax = 0;

    char szFeatureName[20];

    char *br;
    char szAtt[20];
    char geoMatch[20];

    bool bMulti = false;

    char *hdr_buf = (char *) malloc( 1 );

    strcpy( buf, first_line );

//    while(!dun)
    {

        if( !strncmp( buf, "OGRF", 4 ) ) {
            attVal = new wxArrayOfS57attVal();

            FEIndex = atoi( buf + 19 );

            strncpy( szFeatureName, buf + 11, 6 );
            szFeatureName[6] = 0;
            strcpy( FeatureName, szFeatureName );

            //      Build/Maintain a list of found OBJL types for later use
            //      And back-reference the appropriate list index in S57Obj for Display Filtering

            iOBJL = -1; // deferred, done by OBJL filtering in the PLIB as needed

            //      Walk thru the attributes, adding interesting ones
            int hdr_len = 0;
            char *mybuf_ptr = NULL;
            char *hdr_end = NULL;

            int prim = -1;
            int attdun = 0;

            strcpy( geoMatch, "Dummy" );

            while( !attdun ) {
                if( hdr_len ) {
                    int nrl = my_bufgetlx( mybuf_ptr, hdr_end, buf, MAX_LINE );
                    if(nrl < 0)
                        goto bail_out;

                    mybuf_ptr += nrl;
                    if( 0 == nrl ) {
                        attdun = 1;
                        py_fgets( buf, MAX_LINE, fpx );     // this will be PolyGeo
                        break;
                    }
                }

                else
                    py_fgets( buf, MAX_LINE, fpx );

                if( !strncmp( buf, "HDRLEN", 6 ) ) {
                    hdr_len = atoi( buf + 7 );
                    char * tmp = hdr_buf;
                    hdr_buf = (char *) realloc( hdr_buf, hdr_len );
                    if (NULL == hdr_buf)
                    {
                        free ( tmp );
                        tmp = NULL;
                    }
                    else
                    {
                        fpx->Read( hdr_buf, hdr_len );
                        mybuf_ptr = hdr_buf;
                        hdr_end = hdr_buf + hdr_len;
                    }
                }

                else if( !strncmp( buf, geoMatch, 6 ) ) {
                    attdun = 1;
                    break;
                }

                else if( !strncmp( buf, "  MULT", 6 ) )         // Special multipoint
                        {
                    bMulti = true;
                    attdun = 1;
                    break;
                }

                else if( !strncmp( buf, "  PRIM", 6 ) ) {
                    prim = atoi( buf + 13 );
                    switch( prim ){
                        case 1: {
                            strcpy( geoMatch, "  POIN" );
                            break;
                        }

                        case 2:                            // linestring
                        {
                            strcpy( geoMatch, "  LINE" );
                            break;
                        }

                        case 3:                            // area as polygon
                        {
                            strcpy( geoMatch, "  POLY" );
                            break;
                        }

                        default:                            // unrecognized
                        {
                            break;
                        }

                    }       //switch
                }               // if PRIM

                bool iua = IsUsefulAttribute( buf );

                szAtt[0] = 0;

                if( iua ) {
                    S57attVal *pattValTmp = new S57attVal;

                    if( buf[10] == 'I' ) {
                        br = buf + 2;
                        int i = 0;
                        while( *br != ' ' ) {
                            szAtt[i++] = *br;
                            br++;
                        }

                        szAtt[i] = 0;

                        while( *br != '=' )
                            br++;

                        br += 2;

                        int AValInt = atoi( br );
                        int *pAVI = (int *) malloc( sizeof(int) );         //new int;
                        *pAVI = AValInt;
                        pattValTmp->valType = OGR_INT;
                        pattValTmp->value = pAVI;

                        //      Capture SCAMIN on the fly during load
                        if( !strcmp( szAtt, "SCAMIN" ) ) Scamin = AValInt;
                    }

                    else if( buf[10] == 'S' ) {
                        strncpy( szAtt, &buf[2], 6 );
                        szAtt[6] = 0;

                        br = buf + 15;

                        int nlen = strlen( br );
                        br[nlen - 1] = 0;                                 // dump the NL char
                        char *pAVS = (char *) malloc( nlen + 1 );
                        ;
                        strcpy( pAVS, br );

                        pattValTmp->valType = OGR_STR;
                        pattValTmp->value = pAVS;
                    }

                    else if( buf[10] == 'R' ) {
                        br = buf + 2;
                        int i = 0;
                        while( *br != ' ' ) {
                            szAtt[i++] = *br;
                            br++;
                        }

                        szAtt[i] = 0;

                        while( *br != '=' )
                            br++;

                        br += 2;

                        float AValfReal;
                        sscanf( br, "%f", &AValfReal );

                        double AValReal = AValfReal;        //FIXME this cast leaves trash in double

                        double *pAVR = (double *) malloc( sizeof(double) );   //new double;
                        *pAVR = AValReal;

                        pattValTmp->valType = OGR_REAL;
                        pattValTmp->value = pAVR;
                    }

                    else {
                        // unknown attribute type
                        //                        CPLError((CPLErr)0, 0,"Unknown Attribute Type %s", buf);
                    }

                    if( strlen( szAtt ) ) {
                        wxASSERT( strlen(szAtt) == 6);
                        att_array = (char *)realloc(att_array, 6*(n_attr + 1));

                        strncpy(att_array + (6 * sizeof(char) * n_attr), szAtt, 6);
                        n_attr++;

                        attVal->Add( pattValTmp );
                    } else
                        delete pattValTmp;

                }        //useful
            }               // while attdun

            //              Develop Geometry


            switch( prim ){
                case 1: {
                    if( !bMulti ) {
                        Primitive_type = GEO_POINT;

                        char tbuf[40];
                        float point_ref_lat, point_ref_lon;
                        sscanf( buf, "%s %f %f", tbuf, &point_ref_lat, &point_ref_lon );

                        py_fgets( buf, MAX_LINE, fpx );
                        int wkb_len = atoi( buf + 2 );
                        fpx->Read( buf, wkb_len );

                        float easting, northing;
                        npt = 1;
                        float *pfs = (float *) ( buf + 5 );                // point to the point
#ifdef ARMHF
                        float east, north;
                        memcpy(&east, pfs++, sizeof(float));
                        memcpy(&north, pfs, sizeof(float));
                        easting = east;
                        northing = north;
#else
                        easting = *pfs++;
                        northing = *pfs;
#endif
                        x = easting;                                    // and save as SM
                        y = northing;

                        //  Convert from SM to lat/lon for bbox
                        double xll, yll;
                        fromSM_Plugin( easting, northing, point_ref_lat, point_ref_lon, &yll, &xll );

                        m_lon = xll;
                        m_lat = yll;
                        lon_min = m_lon - .25;
                        lon_max = m_lon + .25;
                        lat_min = m_lat - .25;
                        lat_max = m_lat + .25;
                        bBBObj_valid = true;

//                        BBObj.SetMin( m_lon - .25, m_lat - .25 );
//                        BBObj.SetMax( m_lon + .25, m_lat + .25 );

                    } else {
                        Primitive_type = GEO_POINT;

                        char tbuf[40];
                        float point_ref_lat, point_ref_lon;
                        sscanf( buf, "%s %f %f", tbuf, &point_ref_lat, &point_ref_lon );

                        py_fgets( buf, MAX_LINE, fpx );
                        int wkb_len = atoi( buf + 2 );
                        fpx->Read( buf, wkb_len );

                        npt = *( (int *) ( buf + 5 ) );

                        geoPtz = (double *) malloc( npt * 3 * sizeof(double) );
                        geoPtMulti = (double *) malloc( npt * 2 * sizeof(double) );

                        double *pdd = geoPtz;
                        double *pdl = geoPtMulti;

                        float *pfs = (float *) ( buf + 9 );                 // start of data
                        for( int ip = 0; ip < npt; ip++ ) {
                            float easting, northing;
#ifdef ARMHF
                            float east, north, deep;
                            memcpy(&east, pfs++, sizeof(float));
                            memcpy(&north, pfs++, sizeof(float));
                            memcpy(&deep, pfs++, sizeof(float));

                            easting = east;
                            northing = north;

                            *pdd++ = east;
                            *pdd++ = north;
                            *pdd++ = deep;
#else
                            easting = *pfs++;
                            northing = *pfs++;
                            float depth = *pfs++;

                            *pdd++ = easting;
                            *pdd++ = northing;
                            *pdd++ = depth;
#endif
                            //  Convert point from SM to lat/lon for later use in decomposed bboxes
                            double xll, yll;
                            fromSM_Plugin( easting, northing, point_ref_lat, point_ref_lon, &yll, &xll );

                            *pdl++ = xll;
                            *pdl++ = yll;
                        }
                        // Capture bbox limits recorded in SENC record as lon/lat
                        float xmax = *pfs++;
                        float xmin = *pfs++;
                        float ymax = *pfs++;
                        float ymin = *pfs;

                        lon_min = xmin;
                        lon_max = xmax;
                        lat_min = ymin;
                        lat_max = ymax;
                        bBBObj_valid = true;

//                        BBObj.SetMin( xmin, ymin );
//                        BBObj.SetMax( xmax, ymax );

                    }
                    break;
                }

                case 2:                                                // linestring
                {
                    Primitive_type = GEO_LINE;

                    if( !strncmp( buf, "  LINESTRING", 12 ) ) {

                        char tbuf[40];
                        float line_ref_lat, line_ref_lon;
                        sscanf( buf, "%s %f %f", tbuf, &line_ref_lat, &line_ref_lon );

                        py_fgets( buf, MAX_LINE, fpx );
                        int sb_len = atoi( buf + 2 );

                        char *buft = (char *) malloc( sb_len );
                        fpx->Read( buft, sb_len );

                        npt = *( (int *) ( buft + 5 ) );

                        geoPt = (pt*) malloc( ( npt ) * sizeof(pt) );
                        pt *ppt = (pt *)geoPt;
                        float *pf = (float *) ( buft + 9 );
                        float xmax, xmin, ymax, ymin;


#ifdef ARMHF
                        for( int ip = 0; ip < npt; ip++ ) {
                            float east, north;
                            memcpy(&east, pf++, sizeof(float));
                            memcpy(&north, pf++, sizeof(float));

                            ppt->x = east;
                            ppt->y = north;
                            ppt++;
                        }
                        memcpy(&xmax, pf++, sizeof(float));
                        memcpy(&xmin, pf++, sizeof(float));
                        memcpy(&ymax, pf++, sizeof(float));
                        memcpy(&ymin, pf,   sizeof(float));

#else
                        // Capture SM points
                        for( int ip = 0; ip < npt; ip++ ) {
                            ppt->x = *pf++;
                            ppt->y = *pf++;
                            ppt++;
                        }

                        // Capture bbox limits recorded as lon/lat
                        xmax = *pf++;
                        xmin = *pf++;
                        ymax = *pf++;
                        ymin = *pf;
#endif
                        free( buft );

                        // set s57obj bbox as lat/lon
                        lon_min = xmin;
                        lon_max = xmax;
                        lat_min = ymin;
                        lat_max = ymax;

//                        BBObj.SetMin( xmin, ymin );
//                        BBObj.SetMax( xmax, ymax );
                        bBBObj_valid = true;

                        //  and declare x/y of the object to be average east/north of all points
                        double e1, e2, n1, n2;
                        toSM_Plugin( ymax, xmax, line_ref_lat, line_ref_lon, &e1, &n1 );
                        toSM_Plugin( ymin, xmin, line_ref_lat, line_ref_lon, &e2, &n2 );

                        x = ( e1 + e2 ) / 2.;
                        y = ( n1 + n2 ) / 2.;

                        //  Set the object base point
                        double xll, yll;
                        fromSM_Plugin( x, y, line_ref_lat, line_ref_lon, &yll, &xll );
                        m_lon = xll;
                        m_lat = yll;

                        //  Capture the edge and connected node table indices
                        py_fgets( buf, MAX_LINE, fpx );     // this will be "\n"
                        py_fgets( buf, MAX_LINE, fpx );     // this will be "LSINDEXLIST nnn"

//                          char test[100];
//                          strncpy(test, buf, 98);
//                          strcat(test, "\n");
//                          printf("%s", test);

                        sscanf( buf, "%s %d ", tbuf, &m_n_lsindex );

                        m_lsindex_array = (int *) malloc( 3 * m_n_lsindex * sizeof(int) );
                        fpx->Read( (char *)m_lsindex_array, 3 * m_n_lsindex * sizeof(int) );
                        m_n_edge_max_points = 0; //TODO this could be precalulated and added to next SENC format

                        py_fgets( buf, MAX_LINE, fpx );     // this should be \n

                    }

                    break;
                }

                case 3:                                                           // area as polygon
                {
                    Primitive_type = GEO_AREA;

                    if( !strncmp( FeatureName, "DEPARE", 6 )
                            || !strncmp( FeatureName, "DRGARE", 6 ) ) bIsAssociable = true;

 
                   
                    int ll = strlen( buf );
                    if( ll > llmax ) llmax = ll;

                    py_fgets( buf, MAX_LINE, fpx );     // this will be "  POLYTESSGEO"

                    if( !strncmp( buf, "  POLYTESSGEO", 13 ) ) {
                        float area_ref_lat, area_ref_lon;
                        int nrecl;
                        char tbuf[40];

                        sscanf( buf, " %s %d %f %f", tbuf, &nrecl, &area_ref_lat, &area_ref_lon );

                        if( nrecl ) {
                            char *polybuf = (char *) malloc( nrecl + 1 );
                            fpx->Read( polybuf, nrecl );
                            polybuf[nrecl] = 0;                     // endit
                            PolyTessGeo *ppg = new PolyTessGeo( (unsigned char *)polybuf, nrecl, FEIndex );
                            free( polybuf );

                            pPolyTessGeo = (void *)ppg;

                            //  Set the s57obj bounding box as lat/lon
//                            BBObj.SetMin( ppg->Get_xmin(), ppg->Get_ymin() );
//                            BBObj.SetMax( ppg->Get_xmax(), ppg->Get_ymax() );
                            lon_min = ppg->Get_xmin();
                            lon_max = ppg->Get_xmax();
                            lat_min = ppg->Get_ymin();
                            lat_max = ppg->Get_ymax();

                            bBBObj_valid = true;

                            //  and declare x/y of the object to be average east/north of all points
                            double e1, e2, n1, n2;
                            toSM_Plugin( ppg->Get_ymax(), ppg->Get_xmax(), area_ref_lat, area_ref_lon, &e1,
                                    &n1 );
                            toSM_Plugin( ppg->Get_ymin(), ppg->Get_xmin(), area_ref_lat, area_ref_lon, &e2,
                                    &n2 );

                            x = ( e1 + e2 ) / 2.;
                            y = ( n1 + n2 ) / 2.;

                            //  Set the object base point
                            double xll, yll;
                            fromSM_Plugin( x, y, area_ref_lat, area_ref_lon, &yll, &xll );
                            m_lon = xll;
                            m_lat = yll;

                            //  Capture the edge and connected node table indices
                            //                            py_fgets(buf, MAX_LINE, *pfpx);     // this will be "\n"
                            py_fgets( buf, MAX_LINE, fpx );     // this will be "LSINDEXLIST nnn"

                            sscanf( buf, "%s %d ", tbuf, &m_n_lsindex );

                            m_lsindex_array = (int *) malloc( 3 * m_n_lsindex * sizeof(int) );
                            fpx->Read( (char *)m_lsindex_array, 3 * m_n_lsindex * sizeof(int) );
                            m_n_edge_max_points = 0; //TODO this could be precalulated and added to next SENC format

                            py_fgets( buf, MAX_LINE, fpx );     // this should be \n

                        }
                    }
                    else {                      // not "POLYTESSGEO"
                        fpx->Ungetch(buf, strlen(buf) );
                    }

                    break;
                }
            }       //switch

            if( prim > 0 ) {
                Index = FEIndex;
            }
        }               //OGRF
    }                       //while(!dun)

    free( buf );
    free( hdr_buf );

bail_out:
    return;
}

//-------------------------------------------------------------------------------------------
//      Attributes in SENC file may not be needed, and can be safely ignored when creating PI_S57Obj
//      Look at a buffer, and return true or false according to a (default) definition
//-------------------------------------------------------------------------------------------

bool PI_S57ObjX::IsUsefulAttribute( char *buf )
{

    if( !strncmp( buf, "HDRLEN", 6 ) ) return false;

//      Dump the first 8 standard attributes
    /* -------------------------------------------------------------------- */
    /*      RCID                                                            */
    /* -------------------------------------------------------------------- */
    if( !strncmp( buf + 2, "RCID", 4 ) ) return false;

    /* -------------------------------------------------------------------- */
    /*      LNAM                                                            */
    /* -------------------------------------------------------------------- */
    if( !strncmp( buf + 2, "LNAM", 4 ) ) return false;

    /* -------------------------------------------------------------------- */
    /*      PRIM                                                            */
    /* -------------------------------------------------------------------- */
    else if( !strncmp( buf + 2, "PRIM", 4 ) ) return false;

    /* -------------------------------------------------------------------- */
    /*      SORDAT                                                          */
    /* -------------------------------------------------------------------- */
    else if( !strncmp( buf + 2, "SORDAT", 6 ) ) return false;

    /* -------------------------------------------------------------------- */
    /*      SORIND                                                          */
    /* -------------------------------------------------------------------- */
    else if( !strncmp( buf + 2, "SORIND", 6 ) ) return false;

    //      All others are "Useful"
    else
        return true;

#if (0)
    /* -------------------------------------------------------------------- */
    /*      GRUP                                                            */
    /* -------------------------------------------------------------------- */
    else if(!strncmp(buf, "  GRUP", 6))
    return false;

    /* -------------------------------------------------------------------- */
    /*      OBJL                                                            */
    /* -------------------------------------------------------------------- */
    else if(!strncmp(buf, "  OBJL", 6))
    return false;

    /* -------------------------------------------------------------------- */
    /*      RVER                                                            */
    /* -------------------------------------------------------------------- */
    else if(!strncmp(buf, "  RVER", 6))
    return false;

    /* -------------------------------------------------------------------- */
    /*      AGEN                                                            */
    /* -------------------------------------------------------------------- */
    else if(!strncmp(buf, "  AGEN", 6))
    return false;

    /* -------------------------------------------------------------------- */
    /*      FIDN                                                            */
    /* -------------------------------------------------------------------- */
    else if(!strncmp(buf, "  FIDN", 6))
    return false;

    /* -------------------------------------------------------------------- */
    /*      FIDS                                                            */
    /* -------------------------------------------------------------------- */
    else if(!strncmp(buf, "  FIDS", 6))
    return false;

//      UnPresent data
    else if(strstr(buf, "(null)"))
    return false;

    else
    return true;
#endif
}

#if 0
//------------------------------------------------------------------------------
//      Local version of fgets for Binary Mode (SENC) file
//------------------------------------------------------------------------------
int PI_S57Obj::my_fgets( char *buf, int buf_len_max, wxInputStream& ifs )

{
    char chNext;
    int nLineLen = 0;
    char *lbuf;

    lbuf = buf;

    while( !ifs.Eof() && nLineLen < buf_len_max ) {
        chNext = (char) ifs.GetC();

        /* each CR/LF (or LF/CR) as if just "CR" */
        if( chNext == 10 || chNext == 13 ) {
            chNext = '\n';
        }

        *lbuf = chNext;
        lbuf++, nLineLen++;

        if( chNext == '\n' ) {
            *lbuf = '\0';
            return nLineLen;
        }
    }

    *( lbuf ) = '\0';

    return nLineLen;
}
#endif
//------------------------------------------------------------------------------
//      Local version of bufgetl for Binary Mode (SENC) file
//------------------------------------------------------------------------------
int PI_S57ObjX::my_bufgetlx( char *ib_read, char *ib_end, char *buf, int buf_len_max )
{
    char chNext;
    int nLineLen = 0;
    char *lbuf;
    char *ibr = ib_read;

    lbuf = buf;

    while( ( nLineLen < buf_len_max ) && ( ibr < ib_end ) ) {
        chNext = *ibr++;

        /* each CR/LF (or LF/CR) as if just "CR" */
        if( chNext == 10 || chNext == 13 ) chNext = '\n';

        *lbuf++ = chNext;
        nLineLen++;

        if( chNext == '\n' ) {
            *lbuf = '\0';
            return nLineLen;
        }
    }

    *( lbuf ) = '\0';
    return nLineLen;
}

int PI_S57ObjX::GetAttributeIndex( const char *AttrSeek ) {
    char *patl = att_array;

    for(int i=0 ; i < n_attr ; i++) {
        if(!strncmp(patl, AttrSeek, 6)){
            return i;
            break;
        }

        patl += 6;
    }

    return -1;
}


wxString PI_S57ObjX::GetAttrValueAsString( const char *AttrName )
{
    wxString str;

    int idx = GetAttributeIndex(AttrName);

    if(idx >= 0) {

//      using idx to get the attribute value

        S57attVal *v = attVal->Item( idx );

        switch( v->valType ){
            case OGR_STR: {
                char *val = (char *) ( v->value );
                str.Append( wxString( val, wxConvUTF8 ) );
                break;
            }
            case OGR_REAL: {
                double dval = *(double*) ( v->value );
                str.Printf( _T("%g"), dval );
                break;
            }
            case OGR_INT: {
                int ival = *( (int *) v->value );
                str.Printf( _T("%d"), ival );
                break;
            }
            default: {
                str.Printf( _T("Unknown attribute type") );
                break;
            }
        }
    }
    return str;
}


//----------------------------------------------------------------------------------
//      render_canvas_parms Implementation
//----------------------------------------------------------------------------------

render_canvas_parms::render_canvas_parms()
{
    pix_buff = NULL;
}

render_canvas_parms::render_canvas_parms( int xr, int yr, int widthr, int heightr, wxColour color )
{
    depth = BPP;
    pb_pitch = ( widthr * depth / 8 );
    lclip = x;
    rclip = x + widthr - 1;
    pix_buff = (unsigned char *) malloc( heightr * pb_pitch );
    width = widthr;
    height = heightr;
    x = xr;
    y = yr;

    unsigned char r, g, b;
    if( color.IsOk() ) {
        r = color.Red();
        g = color.Green();
        b = color.Blue();
    } else
        r = g = b = 0;

    if( depth == 24 ) {
        for( int i = 0; i < height; i++ ) {
            unsigned char *p = pix_buff + ( i * pb_pitch );
            for( int j = 0; j < width; j++ ) {
                *p++ = r;
                *p++ = g;
                *p++ = b;
            }
        }
    } else {
        for( int i = 0; i < height; i++ ) {
            unsigned char *p = pix_buff + ( i * pb_pitch );
            for( int j = 0; j < width; j++ ) {
                *p++ = r;
                *p++ = g;
                *p++ = b;
                *p++ = 0;
            }
        }
    }

}

render_canvas_parms::~render_canvas_parms( void )
{
}

    

