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
#include <algorithm>          // for std::sort

#include "s63_pi.h"
#include "s63chart.h"
#include "mygeom63.h"
#include "cpl_csv.h"
#include "pi_s52s57.h"


#ifdef __WXOSX__
#include "GL/gl.h"
#include "GL/glu.h"
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


extern wxString         g_sencutil_bin;
wxString                s_last_sync_error;
extern unsigned int     g_backchannel_port;
extern unsigned int     g_frontchannel_port;
extern wxString         g_s57data_dir;
extern bool             g_bsuppress_log;
extern wxString         g_pi_filename;
extern wxString         g_SENCdir;
extern bool             g_benable_screenlog;
extern S63ScreenLogContainer           *g_pScreenLog;
extern S63ScreenLog                    *g_pPanelScreenLog;
extern bool             g_brendered_expired;

extern bool             g_b_validated;
extern bool             g_bSENCutil_valid;
extern bool             g_bLogActivity;
extern bool             g_bdisable_infowin;

int              s_PI_bInS57;         // Exclusion flag to prvent recursion in this class init call.

InfoWin                 *g_pInfo;
InfoWinDialog           *g_pInfoDlg;

wxDialog                *s_plogcontainer;
wxTextCtrl              *s_plogtc;
int                     nseq;

extern bool             pi_bopengl;
extern bool             g_GLOptionsSet;
extern bool             g_GLSetupOK;
extern s63_pi_event_handler_timer *g_pi_timer;

extern PFNGLGENBUFFERSPROC                 s_glGenBuffers;
extern PFNGLBINDBUFFERPROC                 s_glBindBuffer;
extern PFNGLBUFFERDATAPROC                 s_glBufferData;
extern PFNGLDELETEBUFFERSPROC              s_glDeleteBuffers;

#include <wx/arrimpl.cpp>
WX_DEFINE_ARRAY( float*, MyFloatPtrArray );

//    Arrays to temporarily hold SENC geometry
WX_DEFINE_OBJARRAY(PI_ArrayOfVE_Elements);
WX_DEFINE_OBJARRAY(PI_ArrayOfVC_Elements);
WX_DEFINE_OBJARRAY(ArrayOfLights);

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(ListOfPI_S57Obj);                // Implement a list of PI_S57 Objects

#ifdef BUILD_VBO
#ifdef ocpnUSE_GL
extern PFNGLGENBUFFERSPROC                 s_glGenBuffers;
extern PFNGLBINDBUFFERPROC                 s_glBindBuffer;
extern PFNGLBUFFERDATAPROC                 s_glBufferData;
extern PFNGLDELETEBUFFERSPROC              s_glDeleteBuffers;
#endif
#endif

extern bool              g_b_EnableVBO;

// ----------------------------------------------------------------------------
// Random Prototypes
// ----------------------------------------------------------------------------

double      round_msvc (double x)
{
    return(floor(x + 0.5));
}
#define round(x) round_msvc(x)

int DOUBLECMPFUNC(double *first, double *second)
{
    return (*first - *second);
}

void validate_SENC_util(void)
{
    //Verify that OCPNsenc actually exists, and runs, and is the correct version.
    wxString bin_test = g_sencutil_bin;

    if(wxNOT_FOUND != g_sencutil_bin.Find('\"'))
        bin_test = g_sencutil_bin.Mid(1).RemoveLast();

    wxString msg = _("Checking OCPNsenc utility at ");
    msg += _T("{");
    msg += bin_test;
    msg += _T("}");
    wxLogMessage(_T("s63_pi: ") + msg);


    if(!::wxFileExists(bin_test)){
        wxString msg = _("Cannot find the OCPNsenc utility at \n");
        msg += _T("{");
        msg += bin_test;
        msg += _T("}");
        OCPNMessageBox_PlugIn(NULL, msg, _("s63_pi Message"),  wxOK, -1, -1);
        wxLogMessage(_T("s63_pi: ") + msg);

        g_sencutil_bin.Clear();
        return;
    }

    //Will it run?
    wxArrayString ret_array;
    wxArrayString err_array;
    ret_array.Alloc(1000);
    err_array.Alloc(1000);
    wxString cmd = g_sencutil_bin;
#ifndef __WXMSW__
    cmd.Replace(" ", "\\ ");
#endif
    cmd += _T(" -a");                 // get version
    long rv = wxExecute(cmd, ret_array, err_array, wxEXEC_SYNC + wxEXEC_NOEVENTS );

    if(0 != rv) {
        wxString msg = _("Cannot execute OCPNsenc utility at \n");
        msg += _T("{");
        msg += bin_test;
        msg += _T("}");
        OCPNMessageBox_PlugIn(NULL, msg, _("s63_pi Message"),  wxOK, -1, -1);
        wxLogMessage(_T("s63_pi: ") + msg);

        g_sencutil_bin.Clear();
        return;
    }

    // Check results
    bool bad_ver = false;
    wxString ver_line;
    for(unsigned int i=0 ; i < ret_array.GetCount() ; i++){
        wxString line = ret_array[i];
        if(ret_array[i].Upper().Find(_T("VERSION")) != wxNOT_FOUND){
            ver_line = line;
            wxStringTokenizer tkz(line, _T(" "));
            while ( tkz.HasMoreTokens() ){
                wxString token = tkz.GetNextToken();
                double ver;
                if(token.ToDouble(&ver)){
                    if( ver < 1.03)
                        bad_ver = true;
                }
            }

        }
    }

    if(!ver_line.Length())                    // really old version.
          bad_ver = true;

    if(bad_ver) {
        wxString msg = _("OCPNsenc utility at \n");
        msg += _T("{");
        msg += bin_test;
        msg += _T("}\n");
        msg += _(" is incorrect version, reports as:\n\n");
        msg += ver_line;
        msg += _T("\n\n");
        wxString msg1;
        msg1.Printf(_("This version of S63_PI requires OCPNsenc of version 1.03 or later."));
        msg += msg1;
        OCPNMessageBox_PlugIn(NULL, msg, _("s63_pi Message"),  wxOK, -1, -1);
        wxLogMessage(_T("s63_pi: ") + msg);

        g_sencutil_bin.Clear();
        return;

    }

    wxLogMessage(_T("s63_pi: OCPNsenc Check OK...") + ver_line);

}


wxArrayString exec_SENCutil_sync( wxString cmd, bool bshowlog )
{
  //std::string a = cmd.ToStdString();
  //printf("exec_SENCutil_sync:  %s\n", a.c_str());

    wxArrayString ret_array;
    ret_array.Alloc(1000);

    if(!g_b_validated && !g_bSENCutil_valid){
        validate_SENC_util();
        g_b_validated = true;
    }

    if(!g_sencutil_bin.Length()){
        ret_array.Add(_T("ERROR: s63_pi could not execute OCPNsenc utility\n"));

        return ret_array;
    }
    wxString exec = g_sencutil_bin;
#ifndef __WXMSW__
    exec.Replace(" ", "\\ ");
#endif
    cmd.Prepend(exec + _T(" "));

    wxLogMessage( cmd );

    if( bshowlog ){
        ScreenLogMessage(_T("\n"));
    }

    bool bsuppress_last = g_bsuppress_log;
    g_bsuppress_log = !bshowlog;

    int flags = wxEXEC_SYNC;
    flags += wxEXEC_NOEVENTS;
#ifdef __WXMSW__
    //  If windows, we want to avoid disabling the currently active dialog.
    //  Reason:  the wxExecute call yields after disabling the dialog UI, and
    //  each yield seems to be recursive, so consuming GUI resources greatly
    //  until the next full yield() and the event queue drains.
    flags += wxEXEC_NODISABLE;
#endif

    long rv = wxExecute(cmd, ret_array, ret_array, flags );

    g_bsuppress_log = bsuppress_last;

    if(-1 == rv) {
        ret_array.Add(_T("ERROR: s63_pi could not execute OCPNsenc utility\n"));
        ret_array.Add(cmd.Mid(0, 60) + _T("...") + _T("\n"));
        s_last_sync_error = _T("NOEXEC");
    }

    if(g_bLogActivity){
        for(unsigned int i = 0 ; i < ret_array.GetCount() ; i++)
            wxLogMessage(ret_array[i]);
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


unsigned char *ChartS63::GetSENCCryptKeyBuffer( const wxString& FullPath, size_t* bufsize )
{

    unsigned char *cb = (unsigned char *)malloc(1024);

    if(bufsize)
        *bufsize = 1024;


    wxString tmp_file = wxFileName::CreateTempFileName( _T("") );

    //  Get the one-time pad key
    wxString cmd;
    cmd += _T(" -n ");

    cmd += _T(" -i ");
    cmd += _T("\"");
    cmd += FullPath;
    cmd += _T("\"");

    cmd += _T(" -o ");
    cmd += _T("\"");
    cmd += tmp_file;
    cmd += _T("\"");

    cmd += _T(" -u ");
    cmd += GetUserpermit();

    cmd += _T(" -e ");
    cmd += GetInstallpermit();

    if(g_benable_screenlog && (g_pPanelScreenLog || g_pScreenLog) ) {
        cmd += _T(" -b ");
        wxString port;
        port.Printf( _T("%d"), g_backchannel_port );
        cmd += port;
    }

    cmd += _T(" -p ");
    cmd += m_cell_permit;

    cmd += _T(" -z ");
    cmd += _T("\"");
    cmd += g_pi_filename;
    cmd += _T("\"");


    wxArrayString ehdr_result = exec_SENCutil_sync( cmd, false);

    //  Read the key
    wxFileInputStream *ifs = new wxFileInputStream(tmp_file);
    if ( !ifs->IsOk() ){
        ScreenLogMessage( _T("   Error: eSENC Key not built.\n "));
        return cb;
    }

    if( ifs->Read(cb, 1024).LastRead() != 1024) {
        ScreenLogMessage( _T("   Error: eSENC Key not read.\n "));
        delete ifs;
        wxRemoveFile(tmp_file);
        return cb;
    }

    delete ifs;
    wxRemoveFile(tmp_file);
    return cb;

}





//------------------------------------------------------------------------------
//    Simple stream cipher input stream
//------------------------------------------------------------------------------

CryptInputStream::CryptInputStream ( wxInputStream *stream )
:  m_parent_stream(stream),
m_owns(true),
m_cbuf(0), m_outbuf(0)
{
}

CryptInputStream::CryptInputStream ( wxInputStream &stream )
:  m_parent_stream(&stream),
m_owns(false),
m_cbuf(0), m_outbuf(0)
{
}


CryptInputStream::~CryptInputStream ( )
{
    if (m_owns)
        delete m_parent_stream;

    delete m_outbuf;
}

void CryptInputStream::SetCryptBuffer( unsigned char *buffer, size_t cbsize )
{
    m_cbuf = buffer;
    m_cbuf_size = cbsize;
    m_cb_offset = 0;
    if(!m_outbuf)
        m_outbuf = (unsigned char *)malloc(1024);
}



wxInputStream &CryptInputStream::Read(void *buffer, size_t bufsize)
{
    if(m_cbuf){

        m_parent_stream->Read(buffer, bufsize);

        size_t ibuf_count = 0;
        char *buf_crypt = (char *)buffer;
        size_t buf_left = bufsize;
        while(buf_left > 0){

            ibuf_count = bufsize;
            //  Do the crypto
            size_t c_idx = m_cb_offset;
            for(size_t index = 0 ; index < ibuf_count ; index++) {
                buf_crypt[index] ^= m_cbuf[c_idx];
                if(++c_idx >= m_cbuf_size)
                    c_idx = 0;
            }

            m_cb_offset = c_idx;

            buf_left -= ibuf_count;
        }

    }
    else
        m_parent_stream->Read(buffer, bufsize);

    return *m_parent_stream;

}

char CryptInputStream::GetC()
{
    unsigned char c;
    Read(&c, sizeof(c));
    return m_parent_stream->LastRead() ? c : wxEOF;

}

bool CryptInputStream::Eof()
{
    return m_parent_stream->Eof();
}

size_t CryptInputStream::Ungetch(const char* buffer, size_t size)
{
    return 0;
}

void CryptInputStream::Rewind()
{
    m_parent_stream->SeekI(0);
    m_cb_offset = 0;
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

    m_senc_dir =  g_SENCdir;

    // Create ATON arrays, needed by S52PLIB
    pFloatingATONArray = new wxArrayPtrVoid;
    pRigidATONArray = new wxArrayPtrVoid;

    m_ChartType = PI_CHART_TYPE_PLUGIN;
    m_ChartFamily = PI_CHART_FAMILY_VECTOR;

    for( int i = 0; i < PI_PRIO_NUM; i++ )
        for( int j = 0; j < PI_LUPNAME_NUM; j++ )
            razRules[i][j] = NULL;

    m_Chart_Scale = 1;                              // Will be fetched during Init()
    m_Chart_Skew = 0.0;
    pDIB = NULL;
    m_pCloneBM = NULL;

    m_bLinePrioritySet = false;
    m_this_chart_context = NULL;

    m_nCOVREntries = 0;
    m_pCOVRTable = NULL;
    m_pCOVRTablePoints = NULL;

    m_nNoCOVREntries = 0;
    m_pNoCOVRTable = NULL;
    m_pNoCOVRTablePoints = NULL;

    m_latest_update = 0;
    m_pcontour_array = new ArrayOfSortedDoubles;

    m_next_safe_contour = 1e6;
    m_plib_state_hash = 0;

    m_line_vertex_buffer = 0;

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

      //    Free the COVR tables

      for(unsigned int j=0 ; j<(unsigned int)m_nCOVREntries ; j++)
            free( m_pCOVRTable[j] );
      free( m_pCOVRTable );
      free( m_pCOVRTablePoints );

      for(unsigned int j=0 ; j<(unsigned int)m_nNoCOVREntries ; j++)
          free( m_pNoCOVRTable[j] );
      free( m_pNoCOVRTable );
      free( m_pNoCOVRTablePoints );

      //      free(m_ppartial_bytes);

//      delete m_pBMPThumb;


      FreeObjectsAndRules();

      delete pDIB;

      delete pFloatingATONArray;
      delete pRigidATONArray;

      free( m_this_chart_context );

      PI_VE_Hash::iterator it;
      for( it = m_ve_hash.begin(); it != m_ve_hash.end(); ++it ) {
          PI_VE_Element *value = it->second;
          if( value ) {
              free( value->pPoints );
              delete value;
          }
      }
      m_ve_hash.clear();

      PI_VC_Hash::iterator itc;
      for( itc = m_vc_hash.begin(); itc != m_vc_hash.end(); ++itc ) {
          PI_VC_Element *value = itc->second;
          if( value ) {
              free( value->pPoint );
              delete value;
          }
      }
      m_vc_hash.clear();

      PI_connected_segment_hash::iterator itcsc;
      for( itcsc = m_connector_hash.begin(); itcsc != m_connector_hash.end(); ++itcsc ) {
          PI_connector_segment *value = itcsc->second;
          if( value ) {
              delete value;
          }
      }
      m_connector_hash.clear();


      m_pcontour_array->Clear();
      delete m_pcontour_array;

      free(m_line_vertex_buffer);
}


void ChartS63::FreeObjectsAndRules()
{
    //      Delete the created PI_S57Obj array
    //      and any child lists

    PI_S57Obj *top;
    PI_S57Obj *nxx;
    for( int i = 0; i < PRIO_NUM; ++i ) {
        for( int j = 0; j < LUPNAME_NUM; j++ ) {

            top = razRules[i][j];
            while( top != NULL ) {

//  Needs 3.3.1618 ++
                if( top->S52_Context )
                    PI_PLIBFreeContext( top->S52_Context );


                nxx = top->next;

                top->nRef--;
                if( 0 == top->nRef )
                    delete top;

                top = nxx;
            }
        }
    }
}






#define BUF_LEN_MAX 4000

wxString ChartS63::Get_eHDR_Name( const wxString& name000 )
{
    wxFileName tfn(name000);
    wxString base_name = tfn.GetName();

    wxString efn = m_senc_dir;
    efn += wxFileName::GetPathSeparator();
    efn += base_name;
    efn += _T(".ehdr");

    return efn;
}


wxString ChartS63::Build_eHDR( const wxString& name000 )
{
    wxString ehdr_file_name = Get_eHDR_Name( name000 );

#if 0
    //  If required, build a temp file of update file array

    wxString tmp_up_file = wxFileName::CreateTempFileName( _T("") );
    wxTextFile up_file(tmp_up_file);
    if(m_up_file_array.GetCount()){
        up_file.Open();

        for(unsigned int i=0 ; i < m_up_file_array.GetCount() ; i++){
            up_file.AddLine(m_up_file_array[i]);
        }

        up_file.Write();
        up_file.Close();
    }
#endif

    //  Make sure the S63SENC directory exists
    wxFileName ehdrfile( ehdr_file_name );
        //      Make the target directory if needed
    if( true != wxFileName::DirExists( ehdrfile.GetPath() ) ) {
        if( !wxFileName::Mkdir( ehdrfile.GetPath() ) ) {
            ScreenLogMessage(_T("   Cannot create S63SENC file directory for ") + ehdrfile.GetFullPath() );
            return _T("");
        }
    }


    // build the SENC utility command line

    wxString cmd;
    cmd += _T(" -l ");                  // create secure header

    cmd += _T(" -i ");
    cmd += _T("\"");
    cmd += m_full_base_path;
    cmd += _T("\"");

    cmd += _T(" -o ");
    cmd += _T("\"");
    cmd += ehdr_file_name;
    cmd += _T("\"");

    cmd += _T(" -p ");
    cmd += m_cell_permit;

    cmd += _T(" -u ");
    cmd += GetUserpermit();

    cmd += _T(" -e ");
    cmd += GetInstallpermit();

    if(g_benable_screenlog /*&& (g_pPanelScreenLog || g_pScreenLog) */){
        cmd += _T(" -b ");
        wxString port;
        port.Printf( _T("%d"), g_backchannel_port );
        cmd += port;
    }

    cmd += _T(" -r ");
    cmd += _T("\"");
    cmd += g_s57data_dir;
    cmd += _T("\"");

#if 0
    if( m_up_file_array.GetCount() ){
        cmd += _T(" -m ");
        cmd += _T("\"");
        cmd += tmp_up_file;
        cmd += _T("\"");
    }
#endif

    cmd += _T(" -g ");
    cmd += _T("\"");
    cmd += m_FullPath;
    cmd += _T("\"");

    cmd += _T(" -z ");
    cmd += _T("\"");
    cmd += g_pi_filename;
    cmd += _T("\"");


    wxArrayString ehdr_result = exec_SENCutil_sync( cmd, true);

//    ::wxRemoveFile( tmp_up_file );

    //  Check results
    if( !exec_results_check( ehdr_result ) ) {
        m_extended_error = _T("Error executing cmd: ");
        m_extended_error += cmd;
        m_extended_error += _T("\n");
        m_extended_error += s_last_sync_error;

        ScreenLogMessage( _T("\n") );
        ScreenLogMessage( m_extended_error + _T("\n"));

        for(unsigned int i=0 ; i < ehdr_result.GetCount() ; i++){
            ScreenLogMessage( ehdr_result[i] );
            if(!ehdr_result[i].EndsWith(_T("\n")))
                ScreenLogMessage( _T("\n") );
        }

        return _T("");
    }
    else
        return ehdr_file_name;

}


int ChartS63::Init( const wxString& name_os63, int init_flags )
{
    wxLogMessage("********************Init()");
    //    Use a static semaphore flag to prevent recursion
    if( s_PI_bInS57 ) {
      wxLogMessage("Return semaphore");
        return PI_INIT_FAIL_NOERROR;
    }
    s_PI_bInS57++;

    g_brendered_expired = false;    // Reset the trip-wire
    g_pi_timer->Reset();

    if(!GetUserpermit().Len()) {
        s_PI_bInS57--;
      wxLogMessage("Return Userpermit");
        return PI_INIT_FAIL_REMOVE;
    }

    PI_InitReturn ret_val = PI_INIT_FAIL_NOERROR;

    m_FullPath = name_os63;
    m_Description = m_FullPath;

    m_ChartType = PI_CHART_TYPE_PLUGIN;
    m_ChartFamily = PI_CHART_FAMILY_VECTOR;
    m_projection = PI_PROJECTION_MERCATOR;


    //  Parse the metadata

    m_up_file_array.Clear();
    wxTextFile meta_file( name_os63 );
    if( meta_file.Open() ){
        wxString line = meta_file.GetFirstLine();

        while( !meta_file.Eof() ){
            if(line.StartsWith( _T("cellbase:" ) ) ) {
                m_full_base_path = line.Mid(9).BeforeFirst(';');
                wxString baseline = line.Mid(9);

                //      Capture the last update number
                wxString ck_comt = baseline.AfterFirst(';');
                wxStringTokenizer tkz(ck_comt, _T(","));
                while ( tkz.HasMoreTokens() ){
                    wxString token = tkz.GetNextToken();
                    wxString rest;
                    if(token.StartsWith(_T("EDTN="), &rest)){
                        long ck_edtn = -1;
                        rest.ToLong(&ck_edtn);
                        m_base_edtn = ck_edtn;
                    }
                }

            }
            else if(line.StartsWith( _T("cellpermit:" ) ) ) {
                m_cell_permit = line.Mid(11);
                wxStringTokenizer tkz(m_cell_permit, _T(","));
                wxString token = tkz.GetNextToken();
                token = tkz.GetNextToken();
                token = tkz.GetNextToken();
                token = tkz.GetNextToken();
                token = tkz.GetNextToken();
                m_data_server = token;
            }
            else if(line.StartsWith( _T("cellupdate:" ) ) ) {
                wxString upline = line.Mid(11);
                wxString upfile = upline.BeforeFirst(';');
                m_up_file_array.Add(upfile);

                //      Capture the last update number
                wxString ck_comt = upline.AfterFirst(';');
                wxStringTokenizer tkz(ck_comt, _T(","));
                while ( tkz.HasMoreTokens() ){
                    wxString token = tkz.GetNextToken();
                    wxString rest;
                    if(token.StartsWith(_T("UPDN="), &rest)){
                        long ck_updn = -1;
                        rest.ToLong(&ck_updn);
                        m_latest_update = wxMax(m_latest_update, ck_updn);
                    }
                }
            }

            line = meta_file.GetNextLine();
        }
    }
    else{

        //      Could not find the os63 file...
        //      So remove from database permanently to prevent thrashing
        wxString fn = name_os63;
//        RemoveChartFromDBInPlace( fn );

        s_PI_bInS57--;
       wxLogMessage("Return No-os63");

        return PI_INIT_FAIL_REMOVE;
    }

    // Craft the m_SE field
    m_SE.Printf(_T("%d/%d"),m_base_edtn, m_latest_update);


    //  os63 file is a placeholder, cells have not been imported yet.
    if( !m_full_base_path.Len() ){
        s_PI_bInS57--;
        wxLogMessage("Return fullbase");

        return PI_INIT_FAIL_REMOVE;
    }

    //  Base cell name may not be available, or otherwise corrupted
    wxFileName fn(m_full_base_path);
    if(fn.IsDir()){                             // must be a file, not a dir
        s_PI_bInS57--;
        wxLogMessage("Return fullbase1");

        return PI_INIT_FAIL_REMOVE;
    }
    if(!fn.FileExists()){                       // and file must exist
        s_PI_bInS57--;
        wxLogMessage("Return fullbase2");

        return PI_INIT_FAIL_REMOVE;
    }

    //  Check the permit expiry date
    wxDateTime permit_date;
    wxString expiry_date = m_cell_permit.Mid(8, 8);
    wxString ftime = expiry_date.Mid(0, 4) + _T(" ") + expiry_date.Mid(4,2) + _T(" ") + expiry_date.Mid(6,2);

#if wxCHECK_VERSION(2, 9, 0)
    wxString::const_iterator end;
    permit_date.ParseDate(ftime, &end);
#else
    permit_date.ParseDate(ftime.wchar_str());
#endif

    if( permit_date.IsValid()){
        wxDateTime now = wxDateTime::Now();
        m_bexpired = now.IsLaterThan(permit_date);
    }

    if( PI_HEADER_ONLY == init_flags ){

       //      else if the ehdr file exists, we init from there (normal path for adding cell to dB)
       wxString efn = Get_eHDR_Name(name_os63);

        if( wxFileName::FileExists(efn) ) {
        }
        else {                          //  we need to create the ehdr file.

            wxString ebuild = Build_eHDR(name_os63);
            if( !ebuild.Len() ) {
                s_PI_bInS57--;
                     wxLogMessage("Return ebuild");

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
            else {

                //      Init may have failed due to decrypt error...
                //      Allow one controlled retry
                wxRemoveFile( efn);
                wxString ebuild = Build_eHDR(name_os63);
                if( !ebuild.Len() ) {
                    ScreenLogMessage( _T("   Error: Second chance eHDR not built, PI_INIT_FAIL_REMOVE.\n "));
                    ret_val = PI_INIT_FAIL_REMOVE;
                }
                else {
                    bool init_result = InitFrom_ehdr( efn );
                    if( init_result ) {
                        ret_val = PI_INIT_OK;

                        m_bReadyToRender = true;
                    }
                    else {
                        ScreenLogMessage( _T("   Error: Second chance eHDR decrypt fail, PI_INIT_FAIL_REMOVE.\n "));
                        ret_val = PI_INIT_FAIL_REMOVE;
                    }
                }
            }

        }
        else{
            wxLogMessage("Return No ehdr");
            ret_val = PI_INIT_FAIL_REMOVE;
        }
    }

    else if( PI_FULL_INIT == init_flags ){

        //  Verify that the eHdr exists, if not, then rebuild it.
        wxString efn = Get_eHDR_Name(name_os63);

        if( !wxFileName::FileExists(efn) ){             //  we need to create the ehdr file.

            wxString ebuild = Build_eHDR(name_os63);
            if( !ebuild.Len() ) {
                s_PI_bInS57--;
                return PI_INIT_FAIL_REMOVE;
            }
        }

        // see if there is a SENC available
        int sret = FindOrCreateSenc( m_full_base_path );

//        wxString e;
//        e.Printf(_T("  FindOrCreateSenc returns %d\n"), sret);
//        ScreenLogMessage( e );

        if( sret != BUILD_SENC_OK ) {
            if( sret == BUILD_SENC_NOK_RETRY )
                ret_val = PI_INIT_FAIL_RETRY;
            else
                ret_val = PI_INIT_FAIL_REMOVE;
        } else {

            //  Finish the init process
            ret_val = PostInit( init_flags, m_global_color_scheme );

            //  SENC may not decrypt properly
            //  So try one more time
            if(PI_INIT_FAIL_RETRY == ret_val) {
                ScreenLogMessage( _T("   Warning: Retry eSENC.\n "));

                //      Establish location for SENC files
                wxFileName SENCFileName = m_full_base_path;
                SENCFileName.SetExt( _T("es57") );

                //      Set the proper directory for the SENC files
                wxString SENCdir = m_senc_dir;

                if( SENCdir.Last() != wxFileName::GetPathSeparator() )
                    SENCdir.Append( wxFileName::GetPathSeparator() );

                wxFileName tsfn( SENCdir );
                tsfn.SetFullName( SENCFileName.GetFullName() );
                SENCFileName = tsfn;

                wxRemoveFile( tsfn.GetFullPath() );

                int sret = FindOrCreateSenc( m_full_base_path );
                if( sret != BUILD_SENC_OK ) {
                    if( sret == BUILD_SENC_NOK_RETRY )
                        ret_val = PI_INIT_FAIL_RETRY;
                    else
                        ret_val = PI_INIT_FAIL_REMOVE;
                }
                else {
                    //  Finish the init process
                    ret_val = PostInit( init_flags, m_global_color_scheme );
                    if(0 != ret_val) {
                        ScreenLogMessage( _T("   Error: eSENC decrypt failed again.\n "));
                    }

                }
            }
        }
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



double ChartS63::GetRasterScaleFactor()
{
    return 1;
}

bool ChartS63::IsRenderDelta(PlugIn_ViewPort &vp_last, PlugIn_ViewPort &vp_proposed)
{
    return true;
}


//    Report recommended minimum and maximum scale values for which use of this chart is valid

double ChartS63::GetNormalScaleMin(double canvas_scale_factor, bool b_allow_overzoom)
{
//    if( b_allow_overzoom )
        return m_Chart_Scale * 0.125;
//    else
//        return m_Chart_Scale * 0.25;
}

double ChartS63::GetNormalScaleMax(double canvas_scale_factor, int canvas_width)
{
    return m_Chart_Scale * 4.0;
}


double ChartS63::GetNearestPreferredScalePPM(double target_scale_ppm)
{
    return target_scale_ppm;
}






void ChartS63::SetColorScheme(int cs, bool bApplyImmediate)
{

    m_global_color_scheme = cs;

    if( bApplyImmediate ) {
        delete pDIB;        // Toss any current cache
        pDIB = NULL;
    }

    //  Force a reload of text caches
    m_plib_state_hash = 0;


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
    if(m_bexpired)
        g_brendered_expired = true;

    SetVPParms( VPoint );

    PI_PLIBSetRenderCaps( PLIB_CAPS_LINE_BUFFER | PLIB_CAPS_SINGLEGEO_BUFFER | PLIB_CAPS_OBJSEGLIST | PLIB_CAPS_OBJCATMUTATE);
    PI_PLIBPrepareForNewRender();

    if( m_plib_state_hash != PI_GetPLIBStateHash() ) {
        m_bLinePrioritySet = false;                     // need to reset line priorities
        UpdateLUPsOnStateChange( );                     // and update the LUPs
        ResetPointBBoxes( m_last_vp, VPoint );
        SetSafetyContour();
        m_plib_state_hash = PI_GetPLIBStateHash();

    }


    if( VPoint.view_scale_ppm != m_last_vp.view_scale_ppm ) {
        ResetPointBBoxes( m_last_vp, VPoint );
    }

    SetLinePriorities();

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


int ChartS63::RenderRegionViewOnGL( const wxGLContext &glc, const PlugIn_ViewPort& VPoint,
                          const wxRegion &Region, bool b_use_stencil )
{
    if(m_bexpired)
        g_brendered_expired = true;

#ifdef ocpnUSE_GL


    SetVPParms( VPoint );

//    bool force_new_view = false;

//    if( Region != m_last_Region ) force_new_view = true;

    PI_PLIBSetRenderCaps( PLIB_CAPS_LINE_BUFFER | PLIB_CAPS_SINGLEGEO_BUFFER | PLIB_CAPS_OBJSEGLIST | PLIB_CAPS_OBJCATMUTATE);
    PI_PLIBPrepareForNewRender();

    if( m_plib_state_hash != PI_GetPLIBStateHash() ) {
        m_bLinePrioritySet = false;                     // need to reset line priorities
        UpdateLUPsOnStateChange( );                     // and update the LUPs
        ResetPointBBoxes( m_last_vp, VPoint );
        SetSafetyContour();
        m_plib_state_hash = PI_GetPLIBStateHash();

    }


    if( VPoint.view_scale_ppm != m_last_vp.view_scale_ppm ) {
        ResetPointBBoxes( m_last_vp, VPoint );
    }

    SetLinePriorities();

    //    How many rectangles in the Region?
    int n_rect = 0;
    wxRegionIterator clipit( Region );
    while( clipit.HaveRects() ) {
        clipit++;
        n_rect++;
    }

    //    Adjust for rotation
    glPushMatrix();

    if( fabs( VPoint.rotation ) > 0.01 )
        m_brotated = true;
    else
        m_brotated = false;


    //    Arbitrary optimization....
    //    It is cheaper to draw the entire screen if the rectangle count is large,
    //    as is the case for CM93 charts with non-rectilinear borders
    //    However, most (all?) pan operations on "normal" charts will be small rect count
    if(( n_rect < 4 ) && (!m_brotated)){
        wxRegionIterator upd( Region ); // get the Region rect list
        while( upd.HaveRects() ) {
            wxRect rect = upd.GetRect();

            //  Build synthetic ViewPort on this rectangle
            //  Especially, we want the BBox to be accurate in order to
            //  render only those objects actually visible in this region

            PlugIn_ViewPort temp_vp = VPoint;
            double temp_lon_left, temp_lat_bot, temp_lon_right, temp_lat_top;

            wxPoint p;
            p.x = rect.x;
            p.y = rect.y;
            GetCanvasLLPix( (PlugIn_ViewPort *)&VPoint, p, &temp_lat_top, &temp_lon_left);

            p.x += rect.width;
            p.y += rect.height;
            GetCanvasLLPix( (PlugIn_ViewPort *)&VPoint, p, &temp_lat_bot, &temp_lon_right);

            if( temp_lon_right < temp_lon_left )        // presumably crossing Greenwich
                temp_lon_right += 360.;


            temp_vp.lat_min = temp_lat_bot;
            temp_vp.lat_max = temp_lat_top;
            temp_vp.lon_min = temp_lon_left;
            temp_vp.lon_max = temp_lon_right;

            //SetClipRegionGL( glc, temp_vp, rect, true /*!b_overlay*/, b_use_stencil );
            DoRenderRectOnGL( glc, temp_vp, rect, b_use_stencil);

            upd++;
        }
    } else {
        //wxRect rect = Region.GetBox();

        //  Build synthetic ViewPort on this rectangle
        //  Especially, we want the BBox to be accurate in order to
        //  render only those objects actually visible in this region

        PlugIn_ViewPort temp_vp = VPoint;
        double temp_lon_left, temp_lat_bot, temp_lon_right, temp_lat_top;

        wxPoint p;
        p.x = VPoint.rv_rect.x;
        p.y =  VPoint.rv_rect.y;
        GetCanvasLLPix( (PlugIn_ViewPort *)&VPoint, p, &temp_lat_top, &temp_lon_left);

        p.x +=  VPoint.rv_rect.width;
        p.y +=  VPoint.rv_rect.height;
        GetCanvasLLPix( (PlugIn_ViewPort *)&VPoint, p, &temp_lat_bot, &temp_lon_right);

        if( temp_lon_right < temp_lon_left )        // presumably crossing Greenwich
                temp_lon_right += 360.;

        temp_vp.lat_min = temp_lat_bot;
        temp_vp.lat_max = temp_lat_top;
        temp_vp.lon_min = temp_lon_left;
        temp_vp.lon_max = temp_lon_right;

        wxRect rrot = VPoint.rv_rect;


        DoRenderRectOnGL( glc, temp_vp, rrot, b_use_stencil );

    }
//      Update last_vp to reflect current state
    m_last_vp = VPoint;
    m_last_Region = Region;

    glPopMatrix();

#endif
    return true;
}

void ChartS63::SetClipRegionGL( const wxGLContext &glc, const PlugIn_ViewPort& VPoint,
        const wxRegion &Region, bool b_render_nodta, bool b_useStencil )
{
#ifdef ocpnUSE_GL

    if( b_useStencil ) {
        //    Create a stencil buffer for clipping to the region
        glEnable( GL_STENCIL_TEST );
        glStencilMask( 0x1 );                 // write only into bit 0 of the stencil buffer
        glClear( GL_STENCIL_BUFFER_BIT );

        //    We are going to write "1" into the stencil buffer wherever the region is valid
        glStencilFunc( GL_ALWAYS, 1, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
    } else              //  Use depth buffer for clipping
    {
        glEnable( GL_DEPTH_TEST ); // to enable writing to the depth buffer
        glDepthFunc( GL_ALWAYS );  // to ensure everything you draw passes
        glDepthMask( GL_TRUE );    // to allow writes to the depth buffer

        glClear( GL_DEPTH_BUFFER_BIT ); // for a fresh start
    }

    //    As a convenience, while we are creating the stencil or depth mask,
    //    also render the default "NODTA" background if selected
    if( b_render_nodta ) {
        wxColour color = GetBaseGlobalColor( _T ( "NODTA" ) );
        float r, g, b;
        if( color.IsOk() ) {
            r = color.Red() / 255.;
            g = color.Green() / 255.;
            b = color.Blue() / 255.;
        } else
            r = g = b = 0;

        glColor3f( r, g, b );
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );  // enable color buffer

    } else {
        glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );   // disable color buffer
    }

    wxRegionIterator upd( Region ); // get the Region rect list
    while( upd.HaveRects() ) {
        wxRect rect = upd.GetRect();

        if( b_useStencil ) {
            glBegin( GL_QUADS );

            glVertex2f( rect.x, rect.y );
            glVertex2f( rect.x + rect.width, rect.y );
            glVertex2f( rect.x + rect.width, rect.y + rect.height );
            glVertex2f( rect.x, rect.y + rect.height );
            glEnd();

        } else              //  Use depth buffer for clipping
        {
//            if( g_bDebugS57 ) printf( "   Depth buffer Region rect:  %d %d %d %d\n", rect.x, rect.y,
//                    rect.width, rect.height );

            glBegin( GL_QUADS );

            //    Depth buffer runs from 0 at z = 1 to 1 at z = -1
            //    Draw the clip geometry at z = 0.5, giving a depth buffer value of 0.25
            //    Subsequent drawing at z=0 (depth = 0.5) will pass if using glDepthFunc(GL_GREATER);
            glVertex3f( rect.x, rect.y, 0.5 );
            glVertex3f( rect.x + rect.width, rect.y, 0.5 );
            glVertex3f( rect.x + rect.width, rect.y + rect.height, 0.5 );
            glVertex3f( rect.x, rect.y + rect.height, 0.5 );
            glEnd();

        }

        upd++;

    }
    if( b_useStencil ) {
        //    Now set the stencil ops to subsequently render only where the stencil bit is "1"
        glStencilFunc( GL_EQUAL, 1, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

    } else {
        glDepthFunc( GL_GREATER );                          // Set the test value
        glDepthMask( GL_FALSE );                            // disable depth buffer
    }

    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );  // re-enable color buffer
#endif
}

void ChartS63::SetClipRegionGL( const wxGLContext &glc, const PlugIn_ViewPort& VPoint, const wxRect &Rect,
                                bool b_render_nodta, bool b_useStencil )
{
#ifdef ocpnUSE_GL

    if( b_useStencil ) {
        //    Create a stencil buffer for clipping to the region
        glEnable( GL_STENCIL_TEST );
        glStencilMask( 0x1 );                 // write only into bit 0 of the stencil buffer
        glClear( GL_STENCIL_BUFFER_BIT );

        //    We are going to write "1" into the stencil buffer wherever the region is valid
        glStencilFunc( GL_ALWAYS, 1, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );

    } else              //  Use depth buffer for clipping
    {
        glEnable( GL_DEPTH_TEST ); // to enable writing to the depth buffer
        glDepthFunc( GL_ALWAYS );  // to ensure everything you draw passes
        glDepthMask( GL_TRUE );    // to allow writes to the depth buffer
        glClear( GL_DEPTH_BUFFER_BIT ); // for a fresh start
    }

    //    As a convenience, while we are creating the stencil or depth mask,
    //    also render the default "NODTA" background if selected
    if( b_render_nodta ) {
        wxColour color = GetBaseGlobalColor( _T ( "NODTA" ) );
        float r, g, b;
        if( color.IsOk() ) {
            r = color.Red() / 255.;
            g = color.Green() / 255.;
            b = color.Blue() / 255.;
        } else
            r = g = b = 0;
        glColor3f( r, g, b );
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );  // enable color buffer

    } else {
        glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );   // disable color buffer
    }

    if( b_useStencil ) {
        glBegin( GL_QUADS );

        glVertex2f( Rect.x, Rect.y );
        glVertex2f( Rect.x + Rect.width, Rect.y );
        glVertex2f( Rect.x + Rect.width, Rect.y + Rect.height );
        glVertex2f( Rect.x, Rect.y + Rect.height );
        glEnd();

    } else              //  Use depth buffer for clipping
    {
//        if( g_bDebugS57 ) printf( "   Depth buffer rect:  %d %d %d %d\n", Rect.x, Rect.y,
//                Rect.width, Rect.height );

        glBegin( GL_QUADS );

        //    Depth buffer runs from 0 at z = 1 to 1 at z = -1
        //    Draw the clip geometry at z = 0.5, giving a depth buffer value of 0.25
        //    Subsequent drawing at z=0 (depth = 0.5) will pass if using glDepthFunc(GL_GREATER);
        glVertex3f( Rect.x, Rect.y, 0.5 );
        glVertex3f( Rect.x + Rect.width, Rect.y, 0.5 );
        glVertex3f( Rect.x + Rect.width, Rect.y + Rect.height, 0.5 );
        glVertex3f( Rect.x, Rect.y + Rect.height, 0.5 );
        glEnd();

    }

    if( b_useStencil ) {
        //    Now set the stencil ops to subsequently render only where the stencil bit is "1"
        glStencilFunc( GL_EQUAL, 1, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

    } else {
        glDepthFunc( GL_GREATER );                          // Set the test value
        glDepthMask( GL_FALSE );                            // disable depth buffer
    }

    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );  // re-enable color buffer
#endif
}



bool ChartS63::DoRenderRectOnGL( const wxGLContext &glc, const PlugIn_ViewPort& VPoint, wxRect &rect, bool b_useStencil )
{
#ifdef ocpnUSE_GL

    int i;
    PI_S57Obj *top;
    PI_S57Obj *crnt;

    PlugIn_ViewPort tvp = VPoint;                    // undo const  TODO fix this in PLIB

    //    If the ViewPort is unrotated, we can use a simple (fast) scissor test instead
    //    of a stencil or depth buffer clipping algorithm.....
    /*
     if(fabs(VPoint.rotation) < 0.01)
     {

     glScissor(rect.x, VPoint.pix_height-rect.height-rect.y, rect.width, rect.height);
     glEnable(GL_SCISSOR_TEST);
     glDisable (GL_STENCIL_TEST);
     glDisable (GL_DEPTH_TEST);

     }
     */
    if( b_useStencil )
        glEnable( GL_STENCIL_TEST );
    else
        glEnable( GL_DEPTH_TEST );

    glDepthFunc( GL_GEQUAL );

    int dft;
    glGetIntegerv(GL_DEPTH_FUNC, &dft);

    //      Render the areas quickly
    for( i = 0; i < PRIO_NUM; ++i ) {
//        if( PI_GetPLIBBoundaryStyle() == SYMBOLIZED_BOUNDARIES )
//            top = razRules[i][4]; // Area Symbolized Boundaries
//        else
            top = razRules[i][3];           // Area Plain Boundaries


        while( top != NULL ) {
            crnt = top;
            top = top->next;               // next object

            if(m_brotated)
                glPushMatrix();

            glDepthFunc( GL_GEQUAL );

            // A little bit of trickery here.
            //  Most MSW and Mac platforms use Intel Integrated GPU.
            //  This shared GPU memory model does not perform well under high memory pressure conditions.
            //  It has been found safest to disable VBO for such platforms.
            //  But OCPN5 has no direct API for this control.  So we need to fool it.
            //  This trickery can go away whenever OCPN core is updated to allow this type of plugin to directly disable VBO when required.
#if defined(__WXMSW__) || defined(__WXMAC__)
            PI_PLIBSetRenderCaps( PLIB_CAPS_LINE_BUFFER | PLIB_CAPS_OBJSEGLIST | PLIB_CAPS_OBJCATMUTATE);
            crnt->geoPtMulti = (double *)1;
#endif
            PI_PLIBRenderAreaToGL( glc, crnt, &tvp, rect );

#if defined(__WXMSW__) || defined(__WXMAC__)
            crnt->geoPtMulti = (double *)0;
            PI_PLIBSetRenderCaps( PLIB_CAPS_LINE_BUFFER | PLIB_CAPS_SINGLEGEO_BUFFER | PLIB_CAPS_OBJSEGLIST | PLIB_CAPS_OBJCATMUTATE);
#endif

            // This patch obviated by above trickery, but left for posterity
            // Correct for an error in the core.
            // To avoid GL driver memory leakage, delete and reload the area VBO on each render.
            //  This is a performance hit, but unavoidable.
            //TODO  Fix in core, then conditional execute here based on core version
//             if( g_b_EnableVBO && pi_bopengl && s_glDeleteBuffers && (crnt->auxParm0 > 0)){
//                   s_glDeleteBuffers(1, (unsigned int *)&crnt->auxParm0);
//                   crnt->auxParm0 = -99;
//             }


            if(m_brotated){
                // There is a bug in OCPN 4.6 and prior in GLAP().
                // The clip region and parameters is corrupted rendering patterns.
                // We detect this case here, and reset the clip rectangle to the
                // proper (rv_rect) expanded value.
                //  Be careful in debugging.... GLAP only gets hit in category STANDARD and above.

                int df;
                glGetIntegerv(GL_DEPTH_FUNC, &df);

                if(df != dft){
                    glPopMatrix();
                    SetClipRegionGL( glc, VPoint, rect, false, b_useStencil );
                    glPushMatrix();
                    glDepthFunc( GL_GEQUAL );
                }

           }

            if(m_brotated)
                glPopMatrix();
        }
    }


    //    Render the lines and points
    for( i = 0; i < PRIO_NUM; ++i ) {
//        if( PI_GetPLIBBoundaryStyle() == SYMBOLIZED_BOUNDARIES )
//            top = razRules[i][4]; // Area Symbolized Boundaries
//        else
            top = razRules[i][3];           // Area Plain Boundaries
        while( top != NULL ) {
            crnt = top;
            top = top->next;               // next object
            PI_PLIBRenderObjectToGL( glc, crnt, &tvp, rect );
        }

        top = razRules[i][2];           //LINES
        while( top != NULL ) {
            PI_S57Obj *crnt = top;
            top = top->next;
            PI_PLIBRenderObjectToGL( glc, crnt, &tvp, rect );
        }

//        if( PI_GetPLIBSymbolStyle() == SIMPLIFIED )
            top = razRules[i][0];       //SIMPLIFIED Points
//        else
//            top = razRules[i][1];           //Paper Chart Points Points

        while( top != NULL ) {
            crnt = top;
            top = top->next;
            PI_PLIBRenderObjectToGL( glc, crnt, &tvp, rect );
        }

    }

    glDisable( GL_STENCIL_TEST );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_SCISSOR_TEST );
#endif          //#ifdef ocpnUSE_GL

    return true;
}



//-----------------------------------------------------------------------
//          Pixel to Lat/Long Conversion helpers
//-----------------------------------------------------------------------

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
}


//------------------------------------------------------------------------------
//      Local version of fgets for Binary Mode (SENC) file
//------------------------------------------------------------------------------
int ChartS63::my_fgets( char *buf, int buf_len_max, CryptInputStream &ifs )
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
    wxBufferedInputStream fpxb( fpx_u );
    CryptInputStream fpx(fpxb);
    int senc_file_version = 0;

    size_t crypt_size;
    unsigned char *cb = GetSENCCryptKeyBuffer( efn, &crypt_size );
    fpx.SetCryptBuffer( cb, crypt_size );

    // Verify the first 12 bytes
    char verf[13];
    verf[4] = 12;

    fpx.Read(verf, 12);
    fpx.Rewind();

    if(strncmp(verf, "SENC Version", 12)){
//        ScreenLogMessage( _T("   Info: ehdr decrypt failed first chance.\n "));
        free( cb );
        return false;
    }


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

            PI_S57ObjX *obj = new PI_S57ObjX( buf, &fpx, senc_file_version);
            if( !strncmp( obj->FeatureName, "M_COVR", 6 ) ){

                if(obj->pPolyTessGeo){
                    wxString catcov_str = obj->GetAttrValueAsString( "CATCOV" );
                    long catcov = 0;
                    catcov_str.ToLong( &catcov );

//                     double area_ref_lat, area_ref_lon;
//                     area_ref_lat = ((PolyTessGeo63 *)obj->pPolyTessGeo)->m_ref_lat;
//                     area_ref_lon = ((PolyTessGeo63 *)obj->pPolyTessGeo)->m_ref_lon;

                    //      Get the raw geometry from the PolyTessGeo
                    PolyTriGroup *pptg = ((PolyTessGeo63 *)obj->pPolyTessGeo)->Get_PolyTriGroup_head();

                    float *ppolygeo = pptg->pgroup_geom;

                    int ctr_offset = 0;

                    // We use only the first contour, which is by definition the external ring of the M_COVR feature
                    int ic = 0;
                    {
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
//                 else
//                     int yyp = 4;
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

            float *pf = m_pCOVRTable[j];
            for( int k=0 ; k < pAuxCntArray->Item(j); k++){
                printf( "%g  %g  \n", *pf, *(pf+1));
                pf += 2;
            }
            printf("\n");
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

    free( cb );

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
    wxFileName SENCFileName( name );
    SENCFileName.SetExt( _T("es57") );

    //      Set the proper directory for the SENC files
    wxString SENCdir = m_senc_dir;

    if( !SENCdir.Len() )
        return PI_INIT_FAIL_RETRY;

    if( SENCdir.Last() != wxFileName::GetPathSeparator() )
        SENCdir.Append( wxFileName::GetPathSeparator() );

    wxFileName tsfn( SENCdir + _T("a") );
    tsfn.SetFullName( SENCFileName.GetFullName() );
    SENCFileName = tsfn;


    int build_ret_val = 1;

    bool bbuild_new_senc = false;
//    m_bneed_new_thumbnail = false;

    wxFileName FileName000( name );

    m_bcrypt_buffer_OK = false;

    //      Look for SENC file in the target directory

    if( wxFileName::FileExists(SENCFileName.GetFullPath()) ) {

        int force_make_senc = 0;

        //      Open the senc file, and extract some useful information

        wxBufferedInputStream *pfpxb;
        wxFileInputStream fpx_u( SENCFileName.GetFullPath() );

        CryptInputStream *pfpx;
        if( fpx_u.IsOk()){
            pfpxb = new wxBufferedInputStream( fpx_u );

            pfpx = new CryptInputStream( pfpxb );

            m_crypt_buffer = GetSENCCryptKeyBuffer( SENCFileName.GetFullPath(), &m_crypt_size );
            pfpx->SetCryptBuffer( m_crypt_buffer, m_crypt_size );

            // Verify the first 4 bytes
            char verf[5];
            verf[4] = 0;

            pfpx->Read(verf, 4);
            pfpx->Rewind();

            int senc_update = 0;
            int senc_file_version = -1;
            long senc_base_edtn = 0;

            if(!strncmp(verf, "SENC", 4)) {
                bool dun = false;
                char buf[256];
                char *pbuf = buf;
//                int force_make_senc = 0;

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
                            senc_update = atoi( &pbuf[i] );
                        }

                        else if( token.IsSameAs( _T("SENC Version"), TRUE ) ) {
                            int i;
                            i = tkz.GetPosition();
                            senc_file_version = atoi( &pbuf[i] );
                        }

                        else if( token.IsSameAs( _T("EDTN000"), TRUE ) ) {
                            int i;
                            i = tkz.GetPosition();
                            wxString str( &pbuf[i], wxConvUTF8 );
                            str.Trim();                               // gets rid of newline, etc...
                            str.ToLong(&senc_base_edtn);
                        }
                    }
                }
            }

            else {
                ScreenLogMessage( _T("   Info: Existing eSENC file failed decrypt.\n "));
                bbuild_new_senc = true;
            }

            //  So far so good, check some more stuff
            if(!bbuild_new_senc){
                //  SENC file version has to be correct for other tests to make sense
                if( senc_file_version != CURRENT_SENC_FORMAT_VERSION ) {
                    ScreenLogMessage( _T("   Info: Existing eSENC SENC format mismatch.\n "));
                    bbuild_new_senc = true;
                }

                //  Senc EDTN must be the same as .000 file EDTN.
                //  This test catches the usual case where the .000 file is updated from the web,
                //  and all updates (.001, .002, etc.)  are subsumed.
                else if( senc_base_edtn != m_base_edtn ) {
                    wxString msg;
                    msg.Printf(_T("   Info: Existing eSENC base edition mismatch %d %d .\n "), senc_base_edtn, m_base_edtn);
                    ScreenLogMessage( msg );
                    bbuild_new_senc = true;
                }

                else {
                    //    Check the os63 file parse to see if the update number matches
                    if( senc_update != m_latest_update ) {
                        ScreenLogMessage( _T("   Info: Existing eSENC update mismatch.\n "));
                        bbuild_new_senc = true;
                    }
                }
            }

        }

        if( force_make_senc )
            bbuild_new_senc = true;

        if( bbuild_new_senc )
            build_ret_val = BuildSENCFile( name, SENCFileName.GetFullPath() );

    }

    else                    // SENC file does not exist
    {
        ScreenLogMessage( _T("   Info: eSENC file does not exist.\n "));

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
    else {
        //      Did not build a new SENC, so the crypt buffer used to verification
        //      is OK to use in PostInit stage.
        m_bcrypt_buffer_OK = true;
    }

    m_SENCFileName = SENCFileName;
    return PI_INIT_OK;
}


int ChartS63::BuildSENCFile( const wxString& FullPath_os63, const wxString& SENCFileName )
{
    int retval = BUILD_SENC_OK;

    if(!g_bdisable_infowin){
#ifdef __WXOSX__
        g_pInfoDlg = new InfoWinDialog( GetOCPNCanvasWindow(), _("Building eSENC") );
        g_pInfoDlg->Realize();
        g_pInfoDlg->Center();
#else
        g_pInfo = new InfoWin( GetOCPNCanvasWindow(), _("Building eSENC") );
        g_pInfo->Realize();
        g_pInfo->Center();
#endif
    }

#if 0
    //  Build the array of update filenames into a temporary file
    wxString tmp_up_file = wxFileName::CreateTempFileName( _T("") );
    wxTextFile up_file(tmp_up_file);
    if(m_up_file_array.GetCount()){
        up_file.Open();

        for(unsigned int i=0 ; i < m_up_file_array.GetCount() ; i++){
            up_file.AddLine(m_up_file_array[i]);
        }

        up_file.Write();
        up_file.Close();
    }
#endif

    wxFileName SENCfile( SENCFileName );
    //      Make the target directory if needed
    if( true != wxFileName::DirExists( SENCfile.GetPath() ) ) {
        if( !wxFileName::Mkdir( SENCfile.GetPath() ) ) {
            ScreenLogMessage(_T("   Cannot create S63SENC file directory for ") + SENCfile.GetFullPath() );
                return BUILD_SENC_NOK_RETRY;
        }
    }


    // build the SENC utility command line
    wxString outfile = SENCFileName;

    wxString cmd;
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
    cmd += GetUserpermit();

    cmd += _T(" -e ");
    cmd += GetInstallpermit();

    if(g_benable_screenlog && (g_pPanelScreenLog || g_pScreenLog) ){
        cmd += _T(" -b ");
        wxString port;
        port.Printf( _T("%d"), g_backchannel_port );
        cmd += port;
    }

    cmd += _T(" -r ");
    cmd += _T("\"");
    cmd += g_s57data_dir;
    cmd += _T("\"");

#if 0
    if( m_up_file_array.GetCount() ){
        cmd += _T(" -m ");
        cmd += _T("\"");
        cmd += tmp_up_file;
        cmd += _T("\"");
    }
#endif

    cmd += _T(" -g ");
    cmd += _T("\"");
    cmd += m_FullPath;
    cmd += _T("\"");


    cmd += _T(" -z ");
    cmd += _T("\"");
    cmd += g_pi_filename;
    cmd += _T("\"");


    ClearScreenLog();
    wxArrayString ehdr_result = exec_SENCutil_sync( cmd, true );

    //  Check results
    int rv = exec_results_check( ehdr_result );
    if( !rv ) {
        ScreenLogMessage(_T("\n"));
        m_extended_error = _T("Error executing cmd: ");
        m_extended_error += cmd;
        m_extended_error += _T("\n");
        m_extended_error += s_last_sync_error;

        ScreenLogMessage( m_extended_error + _T("\n"));
/*
        for(unsigned int i=0 ; i < ehdr_result.GetCount() ; i++){
            ScreenLogMessage( ehdr_result[i] );
            if(!ehdr_result[i].EndsWith(_T("\n")))
                ScreenLogMessage( _T("\n") );
        }
 */
        if( wxNOT_FOUND == s_last_sync_error.Find(_T("NOEXEC")) ){        // OCPNsenc ran, with errors
            //ScreenLogMessage( _T(" Removing bad eSENC file"));

            //TODO  this does not work, due to a fault in the Chrtdb remove code
            //wxRemoveFile( outfile );     //  so kill a bad SENC
        }

        retval = BUILD_SENC_NOK_PERMANENT;
    }
    else
        retval = BUILD_SENC_OK;

    if(g_pInfo){
        g_pInfo->Destroy();
        g_pInfo = NULL;
    }

    if(g_pInfoDlg){
        g_pInfoDlg->Destroy();
        g_pInfoDlg = NULL;
    }


    return retval;
}

int ChartS63::_insertRules( PI_S57Obj *obj )
{
    int disPrioIdx = 0;
    int LUPtypeIdx = 0;
//    int LUPtypeIdxAlt = 0;

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
//            LUPtypeIdxAlt = 1;
            break; // points
        case PI_PAPER_CHART:
            LUPtypeIdx = 0;
//            LUPtypeIdxAlt = 0;
            break; // points
        case PI_LINES:
            LUPtypeIdx = 2;
//            LUPtypeIdxAlt = 2;
            break; // lines
        case PI_PLAIN_BOUNDARIES:
            LUPtypeIdx = 3;
//            LUPtypeIdxAlt = 4;
            break; // areas
        case PI_SYMBOLIZED_BOUNDARIES:
            LUPtypeIdx = 3;
//            LUPtypeIdxAlt = 3;
            break; // areas
        default:
            LUPtypeIdx = 0;
//            LUPtypeIdxAlt = 0;
            break;
    }

    // insert rules
    obj->nRef++;                         // Increment reference counter for delete check;
    obj->next = razRules[disPrioIdx][LUPtypeIdx];
    obj->child = NULL;
    razRules[disPrioIdx][LUPtypeIdx] = obj;
//    razRules[disPrioIdx][LUPtypeIdxAlt] = obj;

    return 1;
}


void ChartS63::UpdateLUPsOnStateChange( void )
{

    PI_S57Obj *top;
    //  Loop trrough all the objects, resetting the context for each
    for( int i = 0; i < PI_PRIO_NUM; ++i ) {
        for( int j = 0; j < PI_LUPNAME_NUM; j++ ) {
            top = razRules[i][j];
            while( top != NULL ) {
                PI_S57Obj *obj = top;
                PI_PLIBFreeContext( obj->S52_Context );
                obj->S52_Context = NULL;
                PI_PLIBSetContext( obj );
                top = top->next;
            }
        }
    }
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

    wxBufferedInputStream *pfpxb;
    wxFileInputStream fpx_u( FullPath );

    CryptInputStream *pfpx;

    int senc_file_version = 0;

#if 0
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

     else
#endif
     {
        if( !fpx_u.IsOk())
            return 1;
        pfpxb = new wxBufferedInputStream( fpx_u );
    }

    pfpx = new CryptInputStream( pfpxb );

    if(!m_bcrypt_buffer_OK) {
        m_crypt_buffer = GetSENCCryptKeyBuffer( FullPath, &m_crypt_size );
        pfpx->SetCryptBuffer( m_crypt_buffer, m_crypt_size );
    }
    else
        pfpx->SetCryptBuffer( m_crypt_buffer, m_crypt_size );


    // Verify the first 4 bytes
    char verf[5];
    verf[4] = 0;

    pfpx->Read(verf, 4);
    pfpx->Rewind();

    if(strncmp(verf, "SENC", 4)) {
        ScreenLogMessage( _T("   Error: eSENC decrypt failed.\n "));

        free( m_crypt_buffer );
        return 1;
    }


    int MAX_LINE = 499999;
    char *buf = (char *) malloc( MAX_LINE + 1 );

    int nGeoFeature;

    int object_count = 0;

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

            PI_S57ObjX *obj = new PI_S57ObjX( buf, pfpx, senc_file_version );
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

                    //              Establish objects base display priority
                    obj->m_DPRI = -1; //LUP->DPRI - '0';

                    //  Is this a catagory-movable object?
                    if( !strncmp(obj->FeatureName, "OBSTRN", 6) ||
                        !strncmp(obj->FeatureName, "WRECKS", 6) ||
                        !strncmp(obj->FeatureName, "DEPCNT", 6) ||
                        !strncmp(obj->FeatureName, "UWTROC", 6) )
                    {
                        obj->m_bcategory_mutable = true;
                    }
                    else{
                        obj->m_bcategory_mutable = false;
                    }

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

    free( m_crypt_buffer );

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

            //m_SE = m_edtn000;
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
                            int seg_index = iseg * 4;
                            int *index_run = &obj->m_lsindex_array[seg_index];

                            //  Get first connected node
                            int inode = *index_run;
                            if( ( inode ) ) {
                                if( m_vc_hash.find( inode ) == m_vc_hash.end() ) {
                                    //    Must be a bad index in the SENC file
                                    //    Stuff a recognizable flag to indicate invalidity
                                    *index_run = 0;
                                    m_vc_hash[0] = 0;
                                }
                            }
                            index_run++;

                            //  Get the edge
                            int enode = *index_run;
                            if( ( enode ) ) {
                                if( m_ve_hash.find( enode ) == m_ve_hash.end() ) {
                                    //    Must be a bad index in the SENC file
                                    //    Stuff a recognizable flag to indicate invalidity
                                    *index_run = 0;
                                    m_ve_hash[0] = 0;
                                }
                            }

                            index_run++;

                            //  Get last connected node
                            int jnode = *index_run;
                            if( ( jnode ) ) {
                                if( m_vc_hash.find( jnode ) == m_vc_hash.end() ) {
                                    //    Must be a bad index in the SENC file
                                    //    Stuff a recognizable flag to indicate invalidity
                                    *index_run = 0;
                                    m_vc_hash[0] = 0;
                                }
                            }
                            index_run++;

                            // Get the direction
                            //int iDir = *index_run;
                            index_run++;

                        }
                        nxx = top->next;
                        top = nxx;
                    }
                }
            }

    AssembleLineGeometry();

    //  Set up the chart context
    m_this_chart_context = (pi_chart_context *)calloc( sizeof(pi_chart_context), 1);
    m_this_chart_context->m_pvc_hash = (void *)&m_vc_hash;
    m_this_chart_context->m_pve_hash = (void *)&m_ve_hash;

    m_this_chart_context->ref_lat = m_ref_lat;
    m_this_chart_context->ref_lon = m_ref_lon;
    m_this_chart_context->pFloatingATONArray = pFloatingATONArray;
    m_this_chart_context->pRigidATONArray = pRigidATONArray;
    m_this_chart_context->chart = this;
    m_this_chart_context->safety_contour = 1e6;    // to be evaluated later
    m_this_chart_context->vertex_buffer = m_line_vertex_buffer;

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
    BuildDepthContourArray();
    m_bReadyToRender = true;

    return PI_INIT_OK;
}


void ChartS63::BuildDepthContourArray( void )
{
    //    Build array of contour values for later use by conditional symbology

    PI_S57Obj *top;
    for( int i = 0; i < PRIO_NUM; ++i ) {
        for( int j = 0; j < LUPNAME_NUM; j++ ) {

            top = razRules[i][j];
            while( top != NULL ) {
                if( !strncmp( top->FeatureName, "DEPCNT", 6 ) ) {
                    double valdco = 0.0;

                    //  Find the attribute VALDCO
                    char *curr_att = top->att_array;
                    int attrCounter = 0;
                    wxString curAttrName;

                    while( attrCounter < top->n_attr ) {
                        curAttrName = wxString(curr_att, wxConvUTF8, 6);

                        if(curAttrName == _T("VALDCO")){
                            S57attVal *pval = top->attVal->Item( attrCounter );
                            valdco = *( (double *) pval->value );

                            break;
                        }

                        curr_att += 6;
                        attrCounter++;
                    }



                    if( valdco > 0. ) {
                        bool bfound = false;
                        for(unsigned int i = 0 ; i < m_pcontour_array->GetCount() ; i++) {
                            if( fabs(m_pcontour_array->Item(i) - valdco ) < 1e-4 ){
                                bfound = true;
                                break;
                            }
                        }
                        if(!bfound)
                            m_pcontour_array->Add(valdco);
                    }
                }
                PI_S57Obj *nxx = top->next;
                top = nxx;
            }
        }
    }

    m_pcontour_array->Sort( DOUBLECMPFUNC );



}

void ChartS63::SetSafetyContour(void)
{
         // Iterate through the array of contours in this cell, choosing the best one to
         // render as a bold "safety contour" in the PLIB.

         //    This method computes the smallest chart DEPCNT:VALDCO value which
         //    is greater than or equal to the current PLIB mariner parameter S52_MAR_SAFETY_CONTOUR

     double mar_safety_contour = PI_GetPLIBMarinerSafetyContour();

     m_next_safe_contour = mar_safety_contour;      // default in the case mar_safety_contour
                                                    // is deeper than any cell contour present
     for(unsigned int i = 0 ; i < m_pcontour_array->GetCount() ; i++) {
         double h = m_pcontour_array->Item(i);
         if( h >= mar_safety_contour ){
             m_next_safe_contour = h;
             break;
         }
     }

     m_this_chart_context->safety_contour = m_next_safe_contour;                // Save in context
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
 * bool ChartS63::IsRenderDelta(ViewPort &vp_last, ViewPort &vp_proposed)
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

    //      If necessary.....
    //      Establish line feature rendering priorities

    if( !m_bLinePrioritySet ) {
        PI_S57Obj *obj;

        for( int i = 0; i < PRIO_NUM; ++i ) {

            obj = razRules[i][2];           //LINES
            while( obj != NULL ) {
                PI_PLIBSetLineFeaturePriority( obj, i );
                obj = obj->next;
            }

    //    In the interest of speed, choose only the one necessary area boundary style index
//            int j;
//             if( PI_GetPLIBBoundaryStyle() == SYMBOLIZED_BOUNDARIES )
//                 j = 4;
//             else
//            j = 3;

        // In s63_pi, we always use only the object array index "3", repopulating as necessary.
            int j = 3;
            obj = razRules[i][j];
            while( obj != NULL ) {
                PI_PLIBSetLineFeaturePriority( obj, i );
                obj = obj->next;
            }
        }
    }
    // Traverse the entire object list again, setting the priority of each line_segment_element
    // to the maximum priority seen for that segment
    for( int i = 0; i < PRIO_NUM; ++i ) {
        for( int j = 0; j < LUPNAME_NUM; j++ ) {
            PI_S57Obj *obj = razRules[i][j];
            while( obj ) {

                PI_VE_Element *pedge;
                PI_connector_segment *pcs;
                PI_line_segment_element *list = obj->m_ls_list;
                while( list ){
                    switch (list->type){
                        case TYPE_EE:

                            pedge = (PI_VE_Element *)list->private0;
                            if(pedge)
                                list->priority = pedge->max_priority;
                            break;

                        default:
                            pcs = (PI_connector_segment *)list->private0;
                            if(pcs)
                                list->priority = pcs->max_priority;
                            break;
                    }

                    list = list->next;
                }

                obj = obj->next;
            }
        }
    }

    //      Mark the priority as set.
    //      Generally only reset by Options Dialog post processing
    m_bLinePrioritySet = true;
}

void ChartS63::ResetPointBBoxes( const PlugIn_ViewPort &vp_last, const PlugIn_ViewPort &vp_this )
{
    PI_S57Obj *top;
    PI_S57Obj *nxx;

    double box_margin = 0.25;

    //    Assume a 10x10 pixel box
    box_margin = ( 10. / vp_this.view_scale_ppm ) / ( 1852. * 60. );  //degrees

    for( int i = 0; i < PRIO_NUM; ++i ) {
        top = razRules[i][0];
        while( top != NULL ) {
            if( !top->geoPtMulti )                      // do not reset multipoints
            {
                top->lon_min = top->m_lon - box_margin;
                top->lon_max = top->m_lon + box_margin;
                top->lat_min = top->m_lat - box_margin;
                top->lat_max = top->m_lat + box_margin;
                PI_UpdateContext(top);
            }

        nxx = top->next;
        top = nxx;
        }

        top = razRules[i][1];
        while( top != NULL ) {
            if( !top->geoPtMulti )                      // do not reset multipoints
            {
                top->lon_min = top->m_lon - box_margin;
                top->lon_max = top->m_lon + box_margin;
                top->lat_min = top->m_lat - box_margin;
                top->lat_max = top->m_lat + box_margin;
                PI_UpdateContext(top);
            }

            nxx = top->next;
            top = nxx;
        }
    }
}





bool ChartS63::DoRenderRegionViewOnDC( wxMemoryDC& dc, const PlugIn_ViewPort& VPoint,
                                       const wxRegion &Region, bool b_overlay )
{

    SetVPParms( VPoint );

    bool force_new_view = false;

    if( Region != m_last_Region ) force_new_view = true;

    PI_PLIBSetRenderCaps( PLIB_CAPS_LINE_BUFFER | PLIB_CAPS_SINGLEGEO_BUFFER | PLIB_CAPS_OBJSEGLIST | PLIB_CAPS_OBJCATMUTATE);
    PI_PLIBPrepareForNewRender();

    if( m_plib_state_hash != PI_GetPLIBStateHash() ) {
        m_bLinePrioritySet = false;                     // need to reset line priorities
        UpdateLUPsOnStateChange( );                     // and update the LUPs
        ResetPointBBoxes( m_last_vp, VPoint );
        SetSafetyContour();
        m_plib_state_hash = PI_GetPLIBStateHash();

    }


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

    SetVPParms( VPoint );

    PI_PLIBSetRenderCaps( PLIB_CAPS_LINE_BUFFER | PLIB_CAPS_SINGLEGEO_BUFFER | PLIB_CAPS_OBJSEGLIST | PLIB_CAPS_OBJCATMUTATE);
    PI_PLIBPrepareForNewRender();

    if( m_plib_state_hash != PI_GetPLIBStateHash() ) {
        m_bLinePrioritySet = false;                     // need to reset line priorities
        UpdateLUPsOnStateChange( );                     // and update the LUPs
        ResetPointBBoxes( m_last_vp, VPoint );
        SetSafetyContour();
        m_plib_state_hash = PI_GetPLIBStateHash();

    }

    SetLinePriorities();

    bool bnew_view = DoRenderViewOnDC( dc, VPoint, false );

//    pDIB->SelectIntoDC( dc );
    dc.SelectObject( *pDIB );

    return bnew_view;

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
        if( m_last_vprect != dest )
            bReallyNew = true;
         m_last_vprect = dest;


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
                                                dc_last.SelectObject( *pDIB );
                                                wxMemoryDC dc_new;
                                                wxBitmap *pDIBNew = new wxBitmap( VPoint.pix_width, VPoint.pix_height, BPP );
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
//        if( PI_GetPLIBBoundaryStyle() == PI_SYMBOLIZED_BOUNDARIES )
//            top = razRules[i][4]; // Area Symbolized Boundaries
//            else
                top = razRules[i][3];           // Area Plain Boundaries

                while( top != NULL ) {
                    crnt = top;
                    top = top->next;               // next object
                    //PolyTessGeo63 *ppt = (PolyTessGeo63 *)crnt->pPolyTessGeo;
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

//        if( PI_GetPLIBBoundaryStyle() == PI_SYMBOLIZED_BOUNDARIES )
//            top = razRules[i][4]; // Area Symbolized Boundaries
//            else
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

//                if( PI_GetPLIBSymbolStyle() == PI_SIMPLIFIED )
                    top = razRules[i][0];       //SIMPLIFIED Points
//                    else
//                        top = razRules[i][1];           //Paper Chart Points Points

                        while( top != NULL ) {
                            crnt = top;
                            top = top->next;
                            PI_PLIBRenderObjectToDC( &dcinput, crnt, &tvp );
                        }

                        //      Destroy Clipper
                        if( pdcc ) delete pdcc;
    }

    return true;
}

ListOfPI_S57Obj *ChartS63::GetObjRuleListAtLatLon(float lat, float lon, float select_radius,
                                        PlugIn_ViewPort *VPoint)
{

    ListOfPI_S57Obj *ret_ptr = new ListOfPI_S57Obj;

    //    Iterate thru the razRules array, by object/rule type

    PI_S57Obj *top;

    for( int i = 0; i < PRIO_NUM; ++i ) {
        // Points by type, array indices [0..1]

        int point_type = 0; //( PI_GetPLIBSymbolStyle() == SIMPLIFIED ) ? 0 : 1;
        top = razRules[i][point_type];

        while( top != NULL ) {
            if( top->npt == 1 )       // Do not select Multipoint objects (SOUNDG) yet.
                    {
                        if( PI_PLIBObjectRenderCheck( top, VPoint ) ) {
                            if( DoesLatLonSelectObject( lat, lon, select_radius, top ) )
                                ret_ptr->Append( top );
                        }
                    }

                    //    Check the child branch, if any.
                    //    This is where Multipoint soundings are captured individually
                    if( top->child ) {
                        PI_S57Obj *child_item = top->child;
                        while( child_item != NULL ) {
                            if( PI_PLIBObjectRenderCheck( child_item, VPoint ) ) {
                                if( DoesLatLonSelectObject( lat, lon, select_radius, child_item ) )
                                    ret_ptr->Append( child_item );
                            }

                            child_item = child_item->next;
                        }
                    }

                    top = top->next;
        }

        // Areas by boundary type, array indices [3..4]

        int area_boundary_type = 3; //( PI_GetPLIBBoundaryStyle() == PLAIN_BOUNDARIES ) ? 3 : 4;
        top = razRules[i][area_boundary_type];           // Area nnn Boundaries
        while( top != NULL ) {
            if( PI_PLIBObjectRenderCheck( top, VPoint ) ) {
                if( DoesLatLonSelectObject( lat, lon, select_radius, top ) )
                    ret_ptr->Append( top );
            }

            top = top->next;
        }         // while

        // Finally, lines
        top = razRules[i][2];           // Lines

        while( top != NULL ) {
            if( PI_PLIBObjectRenderCheck( top, VPoint ) ) {
                if( DoesLatLonSelectObject( lat, lon, select_radius, top ) )
                    ret_ptr->Append( top );
            }

            top = top->next;
        }
    }

    return ret_ptr;
}

bool ChartS63::DoesLatLonSelectObject( float lat, float lon, float select_radius, PI_S57Obj *obj )
{

    switch( obj->Primitive_type ){
        //  For single Point objects, the integral object bounding box contains the lat/lon of the object,
        //  possibly expanded by text or symbol rendering
        case GEO_POINT: {
            if( 1 == obj->npt ) {

                //  Special case for LIGHTS
                //  Sector lights have had their BBObj expanded to include the entire drawn sector
                //  This is too big for pick area, can be confusing....
                //  So make a temporary box at the light's lat/lon, with select_radius size
                if( !strncmp( obj->FeatureName, "LIGHTS", 6 ) ) {

                    if (  lon >= (obj->lon_min - select_radius) && lon <= (obj->lon_max + select_radius) &&
                        lat >= (obj->lat_min - select_radius) && lat <= (obj->lat_max + select_radius) )
                            return true;


#if 0
                    double olon, olat;
                    fromSM( ( obj->x * obj->x_rate ) + obj->x_origin,
                            ( obj->y * obj->y_rate ) + obj->y_origin, ref_lat, ref_lon, &olat,
                            &olon );

                    // Double the select radius to adjust for the fact that LIGHTS has
                    // a 0x0 BBox to start with, which makes it smaller than all other
                    // rendered objects.
                    wxBoundingBox sbox( olon - 2*select_radius, olat - 2*select_radius,
                                        olon + 2*select_radius, olat + 2*select_radius );

                    if( sbox.PointInBox( lon, lat, 0 ) )
                        return true;
#endif
                }

                else{
                    double rlat_min, rlat_max, rlon_min, rlon_max;
                    bool box_valid = PI_GetObjectRenderBox(obj, &rlat_min, &rlat_max, &rlon_min, &rlon_max);

                    if(!box_valid)
                        return false;

                   if (  lon >= (rlon_min - select_radius) && lon <= (rlon_max + select_radius) &&
                        lat >= (rlat_min - select_radius) && lat <= (rlat_max + select_radius) )
                        return TRUE;
                 }
            }

            //  For MultiPoint objects, make a bounding box from each point's lat/lon
            //  and check it
            else {

                //  Coarse test first
                if (  lon >= (obj->lon_min - select_radius) && lon <= (obj->lon_max + select_radius) &&
                    lat >= (obj->lat_min - select_radius) && lat <= (obj->lat_max + select_radius) )
                {

                //  Now decomposed soundings, one by one
                    double *pdl = obj->geoPtMulti;
                    for( int ip = 0; ip < obj->npt; ip++ ) {
                        double lon_point = *pdl++;
                        double lat_point = *pdl++;
                        if (  lon >= (lon_point - select_radius) && lon <= (lon_point + select_radius) &&
                            lat >= (lat_point - select_radius) && lat <= (lat_point + select_radius) )
                            return true;
                        }
                    }
                }

            break;
        }

        case GEO_AREA: {
            //  Coarse test first
            if (  lon >= (obj->lon_min - select_radius) && lon <= (obj->lon_max + select_radius) &&
                lat >= (obj->lat_min - select_radius) && lat <= (obj->lat_max + select_radius) )
            {
                return IsPointInObjArea( lat, lon, select_radius, obj );
            }

        }
#if 0
        case GEO_LINE: {
            if( obj->geoPt ) {
                //  Coarse test first
                if( !obj->BBObj.PointInBox( lon, lat, select_radius ) ) return false;

                //  Line geometry is carried in SM or CM93 coordinates, so...
                //  make the hit test using SM coordinates, converting from object points to SM using per-object conversion factors.

                double easting, northing;
                toSM( lat, lon, ref_lat, ref_lon, &easting, &northing );

                pt *ppt = obj->geoPt;
                int npt = obj->npt;

                double xr = obj->x_rate;
                double xo = obj->x_origin;
                double yr = obj->y_rate;
                double yo = obj->y_origin;

                double north0 = ( ppt->y * yr ) + yo;
                double east0 = ( ppt->x * xr ) + xo;
                ppt++;

                for( int ip = 1; ip < npt; ip++ ) {
                    double north = ( ppt->y * yr ) + yo;
                    double east = ( ppt->x * xr ) + xo;

                    //    A slightly less coarse segment bounding box check
                    if( northing >= ( fmin(north, north0) - select_radius ) ) if( northing
                        <= ( fmax(north, north0) + select_radius ) ) if( easting
                        >= ( fmin(east, east0) - select_radius ) ) if( easting
                        <= ( fmax(east, east0) + select_radius ) ) {
                            //                                                    index = ip;
                            return true;
                        }

                        north0 = north;
                    east0 = east;
                    ppt++;
                }
            }

            break;
        }
#endif
        case GEO_META:
        case GEO_PRIM:
        default:
            break;
    }

    return false;
}

bool ChartS63::IsPointInObjArea( float lat, float lon, float select_radius, PI_S57Obj *obj )
{
    bool ret = false;

    if( obj->pPolyTessGeo ) {

        PolyTriGroup *ppg = ((PolyTessGeo63 *)obj->pPolyTessGeo)->Get_PolyTriGroup_head();

        TriPrim *pTP = ppg->tri_prim_head;

        MyPoint pvert_list[3];

        //  Polygon geometry is carried in SM coordinates, so...
        //  make the hit test thus.
        double easting, northing;
        toSM_Plugin( lat, lon, m_ref_lat, m_ref_lon, &easting, &northing );


        while( pTP ) {
            //  Coarse test
            if (  lon >= (pTP->minx) && lon <= (pTP->maxx) &&
                lat >= (pTP->miny) && lat <= (pTP->maxy) )
            {
                if(ppg->data_type == DATA_TYPE_DOUBLE) {

                    double *p_vertex = pTP->p_vertex;

                    switch( pTP->type ){
                        case PTG_TRIANGLE_FAN: {
                            for( int it = 0; it < pTP->nVert - 2; it++ ) {
                                pvert_list[0].x = p_vertex[0];
                                pvert_list[0].y = p_vertex[1];

                                pvert_list[1].x = p_vertex[( it * 2 ) + 2];
                                pvert_list[1].y = p_vertex[( it * 2 ) + 3];

                                pvert_list[2].x = p_vertex[( it * 2 ) + 4];
                                pvert_list[2].y = p_vertex[( it * 2 ) + 5];

                                if( G_PtInPolygon( (MyPoint *) pvert_list, 3, easting, northing ) ) {
                                    ret = true;
                                    break;
                                }
                            }
                            break;
                        }
                        case PTG_TRIANGLE_STRIP: {
                            for( int it = 0; it < pTP->nVert - 2; it++ ) {
                                pvert_list[0].x = p_vertex[( it * 2 )];
                                pvert_list[0].y = p_vertex[( it * 2 ) + 1];

                                pvert_list[1].x = p_vertex[( it * 2 ) + 2];
                                pvert_list[1].y = p_vertex[( it * 2 ) + 3];

                                pvert_list[2].x = p_vertex[( it * 2 ) + 4];
                                pvert_list[2].y = p_vertex[( it * 2 ) + 5];

                                if( G_PtInPolygon( (MyPoint *) pvert_list, 3, easting, northing ) ) {
                                    ret = true;
                                    break;
                                }
                            }
                            break;
                        }
                        case PTG_TRIANGLES: {
                            for( int it = 0; it < pTP->nVert; it += 3 ) {
                                pvert_list[0].x = p_vertex[( it * 2 )];
                                pvert_list[0].y = p_vertex[( it * 2 ) + 1];

                                pvert_list[1].x = p_vertex[( it * 2 ) + 2];
                                pvert_list[1].y = p_vertex[( it * 2 ) + 3];

                                pvert_list[2].x = p_vertex[( it * 2 ) + 4];
                                pvert_list[2].y = p_vertex[( it * 2 ) + 5];

                                if( G_PtInPolygon( (MyPoint *) pvert_list, 3, easting, northing ) ) {
                                    ret = true;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }  // double
                else{
                    float *p_vertex = (float *)pTP->p_vertex;

                    switch( pTP->type ){
                        case PTG_TRIANGLE_FAN: {
                            for( int it = 0; it < pTP->nVert - 2; it++ ) {
                                pvert_list[0].x = p_vertex[0];
                                pvert_list[0].y = p_vertex[1];

                                pvert_list[1].x = p_vertex[( it * 2 ) + 2];
                                pvert_list[1].y = p_vertex[( it * 2 ) + 3];

                                pvert_list[2].x = p_vertex[( it * 2 ) + 4];
                                pvert_list[2].y = p_vertex[( it * 2 ) + 5];

                                if( G_PtInPolygon( (MyPoint *) pvert_list, 3, easting, northing ) ) {
                                    ret = true;
                                    break;
                                }
                            }
                            break;
                        }
                        case PTG_TRIANGLE_STRIP: {
                            for( int it = 0; it < pTP->nVert - 2; it++ ) {
                                pvert_list[0].x = p_vertex[( it * 2 )];
                                pvert_list[0].y = p_vertex[( it * 2 ) + 1];

                                pvert_list[1].x = p_vertex[( it * 2 ) + 2];
                                pvert_list[1].y = p_vertex[( it * 2 ) + 3];

                                pvert_list[2].x = p_vertex[( it * 2 ) + 4];
                                pvert_list[2].y = p_vertex[( it * 2 ) + 5];

                                if( G_PtInPolygon( (MyPoint *) pvert_list, 3, easting, northing ) ) {
                                    ret = true;
                                    break;
                                }
                            }
                            break;
                        }
                        case PTG_TRIANGLES: {
                            for( int it = 0; it < pTP->nVert; it += 3 ) {
                                pvert_list[0].x = p_vertex[( it * 2 )];
                                pvert_list[0].y = p_vertex[( it * 2 ) + 1];

                                pvert_list[1].x = p_vertex[( it * 2 ) + 2];
                                pvert_list[1].y = p_vertex[( it * 2 ) + 3];

                                pvert_list[2].x = p_vertex[( it * 2 ) + 4];
                                pvert_list[2].y = p_vertex[( it * 2 ) + 5];

                                if( G_PtInPolygon( (MyPoint *) pvert_list, 3, easting, northing ) ) {
                                    ret = true;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }  // float
            }
            pTP = pTP->p_next;
        }

    }           // if pPolyTessGeo

#if 0
    else if( obj->pPolyTrapGeo ) {
        if( !obj->pPolyTrapGeo->IsOk() ) obj->pPolyTrapGeo->BuildTess();

        PolyTrapGroup *ptg = obj->pPolyTrapGeo->Get_PolyTrapGroup_head();

        //  Polygon geometry is carried in SM coordinates, so...
        //  make the hit test thus.
        //  However, since PolyTrapGeo geometry is (always??) in cm-93 coordinates, convert to sm as necessary
        double easting, northing;
        toSM( lat, lon, ref_lat, ref_lon, &easting, &northing );

        int ntraps = ptg->ntrap_count;
        trapz_t *ptraps = ptg->trap_array;
        MyPoint *segs = (MyPoint *) ptg->ptrapgroup_geom; //TODO convert MyPoint to wxPoint2DDouble globally

        MyPoint pvert_list[4];

        double y_rate = obj->y_rate;
        double y_origin = obj->y_origin;

        for( int i = 0; i < ntraps; i++, ptraps++ ) {
            //      Y test

            double hiy = ( ptraps->hiy * y_rate ) + y_origin;
            if( northing > hiy ) continue;

            double loy = ( ptraps->loy * y_rate ) + y_origin;
            if( northing < loy ) continue;

            //      Use the segment endpoints to calculate the corners of a trapezoid
            int lseg = ptraps->ilseg;
            int rseg = ptraps->irseg;

            //    Left edge
            double xmax = segs[lseg].x;
            double xmin = segs[lseg + 1].x;

            double ymax = segs[lseg].y;
            double ymin = segs[lseg + 1].y;

            double xt, yt, xca, xcb;

            if( ymax < ymin ) {
                xt = xmin;
                xmin = xmax;
                xmax = xt;                // interchange min/max
                yt = ymin;
                ymin = ymax;
                ymax = yt;
            }

            if( xmin == xmax ) {
                xca = xmin;
                xcb = xmin;
            } else {
                double slope = ( ymax - ymin ) / ( xmax - xmin );
                xca = xmin + ( ptraps->loy - ymin ) / slope;
                xcb = xmin + ( ptraps->hiy - ymin ) / slope;
            }

            pvert_list[0].x = ( xca * obj->x_rate ) + obj->x_origin;
            pvert_list[0].y = loy;

            pvert_list[1].x = ( xcb * obj->x_rate ) + obj->x_origin;
            pvert_list[1].y = hiy;

            //    Right edge
            xmax = segs[rseg].x;
            xmin = segs[rseg + 1].x;
            ymax = segs[rseg].y;
            ymin = segs[rseg + 1].y;

            if( ymax < ymin ) {
                xt = xmin;
                xmin = xmax;
                xmax = xt;
                yt = ymin;
                ymin = ymax;
                ymax = yt;
            }

            if( xmin == xmax ) {
                xca = xmin;
                xcb = xmin;
            } else {
                double slope = ( ymax - ymin ) / ( xmax - xmin );
                xca = xmin + ( ptraps->hiy - ymin ) / slope;
                xcb = xmin + ( ptraps->loy - ymin ) / slope;
            }

            pvert_list[2].x = ( xca * obj->x_rate ) + obj->x_origin;
            pvert_list[2].y = hiy;

            pvert_list[3].x = ( xcb * obj->x_rate ) + obj->x_origin;
            pvert_list[3].y = loy;

            if( G_PtInPolygon( (MyPoint *) pvert_list, 4, easting, northing ) ) {
                ret = true;
                break;
            }
        }
    }           // if pPolyTrapGeo
#endif
    return ret;
}




//      In GDAL-1.2.0, CSVGetField is not exported.......
//      So, make my own simplified copy
/************************************************************************/
/*                           MyCSVGetField()                            */
/*                                                                      */
/************************************************************************/

const char *MyCSVGetField( const char * pszFilename, const char * pszKeyFieldName,
                           const char * pszKeyFieldValue, CSVCompareCriteria eCriteria, const char * pszTargetField )

{
    char **papszRecord;
    int iTargetField;

    /* -------------------------------------------------------------------- */
    /*      Find the correct record.                                        */
    /* -------------------------------------------------------------------- */
    papszRecord = CSVScanFileByName( pszFilename, pszKeyFieldName, pszKeyFieldValue, eCriteria );

    if( papszRecord == NULL ) return "";

    /* -------------------------------------------------------------------- */
    /*      Figure out which field we want out of this.                     */
    /* -------------------------------------------------------------------- */
    iTargetField = CSVGetFileFieldId( pszFilename, pszTargetField );
    if( iTargetField < 0 ) return "";

    if( iTargetField >= CSLCount( papszRecord ) ) return "";

    return ( papszRecord[iTargetField] );
}

int CompareLights( PI_S57Light** l1ptr, PI_S57Light** l2ptr )
{
    PI_S57Light l1 = *(PI_S57Light*) *l1ptr;
    PI_S57Light l2 = *(PI_S57Light*) *l2ptr;

    int positionDiff = l1.position.Cmp( l2.position );
    if( positionDiff != 0 ) return positionDiff;

    double angle1, angle2;
    int attrIndex1 = l1.attributeNames.Index( _T("SECTR1") );
    int attrIndex2 = l2.attributeNames.Index( _T("SECTR1") );

    // This should put Lights without sectors last in the list.
    if( attrIndex1 == wxNOT_FOUND && attrIndex2 == wxNOT_FOUND ) return 0;
    if( attrIndex1 != wxNOT_FOUND && attrIndex2 == wxNOT_FOUND ) return -1;
    if( attrIndex1 == wxNOT_FOUND && attrIndex2 != wxNOT_FOUND ) return 1;

    l1.attributeValues.Item( attrIndex1 ).ToDouble( &angle1 );
    l2.attributeValues.Item( attrIndex2 ).ToDouble( &angle2 );

    if( angle1 == angle2 ) return 0;
    if( angle1 > angle2 ) return 1;
    return -1;
}



wxString ChartS63::CreateObjDescriptions( ListOfPI_S57Obj* obj_list )
{
    wxString ret_val;
    int attrCounter;
    wxString curAttrName, value;
    bool isLight = false;
    wxString className;
    wxString classDesc;
    wxString classAttributes;
    wxString objText;
    wxString lightsHtml;
    wxString positionString;
    ArrayOfLights lights;
    PI_S57Light* curLight = NULL;
    wxFileName file;


    for( ListOfPI_S57Obj::Node *node = obj_list->GetLast(); node; node = node->GetPrevious() ) {
        PI_S57Obj *current = node->GetData();
        positionString.Clear();
        objText.Clear();

        // Soundings have no information, so don't show them
 //       if( 0 == strncmp( current->LUP->OBCL, "SOUND", 5 ) ) continue;

        if( current->Primitive_type == GEO_META ) continue;
        if( current->Primitive_type == GEO_PRIM ) continue;

        className = wxString( current->FeatureName, wxConvUTF8 );

        // Lights get grouped together to make display look nicer.
        isLight = !strcmp( current->FeatureName, "LIGHTS" );

        //    Get the object's nice description from s57objectclasses.csv
        //    using cpl_csv from the gdal library

        const char *name_desc;
        if( g_s57data_dir.Len() ) {
            wxString oc_file = g_s57data_dir;
            oc_file.Append( _T("/s57objectclasses.csv") );
            name_desc = MyCSVGetField( oc_file.mb_str(), "Acronym",                  // match field
                                       current->FeatureName,            // match value
                                       CC_ExactString, "ObjectClass" );             // return field
        } else
            name_desc = "";

        // In case there is no nice description for this object class, use the 6 char class name
            if( 0 == strlen( name_desc ) ) {
                name_desc = current->FeatureName;
                classDesc = wxString( name_desc, wxConvUTF8, 1 );
                classDesc << wxString( name_desc + 1, wxConvUTF8 ).MakeLower();
            } else {
                classDesc = wxString( name_desc, wxConvUTF8 );
            }

#if 0
            //    Show LUP
            if( g_bDebugS57 ) {
                wxString index;
                index.Printf( _T("Feature Index: %d\n"), current->obj->Index );
            classAttributes << index;

            wxString LUPstring;
            LUPstring.Printf( _T("LUP RCID:  %d\n"), current->LUP->RCID );
            classAttributes << LUPstring;

            LUPstring = _T("    LUP ATTC: ");
            if( current->LUP->ATTCArray ) LUPstring += current->LUP->ATTCArray->Item( 0 );
            LUPstring += _T("\n");
            classAttributes << LUPstring;

            LUPstring = _T("    LUP INST: ");
            if( current->LUP->INST ) LUPstring += *( current->LUP->INST );
            LUPstring += _T("\n\n");
            classAttributes << LUPstring;

            }
#endif
            if( GEO_POINT == current->Primitive_type ) {
                double lon, lat;
//                fromSM_Plugin( ( current->x * current->x_rate ) + current->x_origin,
//                        ( current->y * current->y_rate ) + current->y_origin, m_ref_lat,
//                        m_ref_lon, &lat, &lon );

                // Use the m_lat/m_lon directly, since S63 chart type does not use offset/rate class members.
                lon = current->m_lon;
                lat = current->m_lat;

                if( lon > 180.0 ) lon -= 360.;

                positionString.Clear();
                positionString += toSDMM_PlugIn( 1, lat );
                positionString << _T(" ");
                positionString += toSDMM_PlugIn( 2, lon );


                if( isLight ) {
                    curLight = new PI_S57Light;
                    curLight->position = positionString;
                    curLight->hasSectors = false;
                    lights.Add( curLight );
                }

            }

            //    Get the Attributes and values, making sure they can be converted from UTF8
            if(current->att_array) {
                char *curr_att = current->att_array;

                attrCounter = 0;

                wxString attribStr;
                int noAttr = 0;
                attribStr << _T("<table border=0 cellspacing=0 cellpadding=0>");

                bool inDepthRange = false;

                while( attrCounter < current->n_attr ) {
                    //    Attribute name
                    curAttrName = wxString(curr_att, wxConvUTF8, 6);
                    noAttr++;

                    // Sort out how some kinds of attibutes are displayed to get a more readable look.
                    // DEPARE gets just its range. Lights are grouped.

                    if( isLight ) {
                        curLight->attributeNames.Add( curAttrName );
                        if( curAttrName.StartsWith( _T("SECTR") ) )
                            curLight->hasSectors = true;
                    } else {
                        if( curAttrName == _T("DRVAL1") ) {
                            attribStr << _T("<tr><td><font size=-1>");
                            inDepthRange = true;
                        } else if( curAttrName == _T("DRVAL2") ) {
                            attribStr << _T(" - ");
                            inDepthRange = false;
                        } else {
                            if( inDepthRange ) {
                                attribStr << _T("</font></td></tr>\n");
                                inDepthRange = false;
                            }
                            attribStr << _T("<tr><td valign=top><font size=-2>") << curAttrName;
                            attribStr << _T("</font></td><td>&nbsp;&nbsp;</td><td valign=top><font size=-1>");
                        }
                    }

                    // What we need to do...
                    // Change senc format, instead of (S), (I), etc, use the attribute types fetched from the S57attri...csv file
                    // This will be like (E), (L), (I), (F)
                    //  will affect lots of other stuff.  look for S57attVal.valType
                    // need to do this in creatsencrecord above, and update the senc format.

                    value = GetObjectAttributeValueAsString( current, attrCounter, curAttrName );

                    if( isLight ) {
                        curLight->attributeValues.Add( value );
                    } else {
                        if( curAttrName == _T("INFORM") || curAttrName == _T("NINFOM") ) value.Replace(
                            _T("|"), _T("<br>") );
                        attribStr << value;

                        if( !( curAttrName == _T("DRVAL1") ) ) {
                            attribStr << _T("</font></td></tr>\n");
                        }
                    }
                    attrCounter++;
                    curr_att += 6;

                }             //while attrCounter < current->obj->n_attr

                if( !isLight ) {
                    attribStr << _T("</table>\n");

                    objText += _T("<b>") + classDesc + _T("</b> <font size=-2>(") + className
                    + _T(")</font>") + _T("<br>");

                    if( positionString.Length() ) objText << _T("<font size=-2>") << positionString
                        << _T("</font><br>\n");

                    if( noAttr > 0 ) objText << attribStr;

                    if( node != obj_list->GetFirst() ) objText += _T("<hr noshade>");
                    objText += _T("<br>");
                    ret_val << objText;
                }

            }
    } // Object for loop


        // Add the additional info files
    wxArrayString files;
    file.Assign( GetFullPath() );

    wxString suppPath = file.GetPath();
    suppPath += wxFileName::GetPathSeparator();
    suppPath += file.GetName().Mid(0,2);
    suppPath += wxFileName::GetPathSeparator();
    suppPath += file.GetName();

    wxString AddFiles = wxString::Format(_T("<hr noshade><br><b>Additional info files attached to: </b> <font size=-2>%s</font><br><table border=0 cellspacing=0 cellpadding=3>"), file.GetFullName() );
    wxDir::GetAllFiles( suppPath, &files,  wxT("*.TXT")  );
    wxDir::GetAllFiles( suppPath, &files,  wxT("*.txt")  );
    //int nf = files.Count();
    if ( files.Count() > 0 )
    {
        for ( size_t i=0; i < files.Count(); i++){
            wxString fileToAdd = files.Item(i);
            file.Assign( files.Item(i) );
            AddFiles << wxString::Format( _T("<tr><td valign=top><font size=-2><a href=\"%s\">%s</a></font></td><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp</td>"), file.GetFullPath(), file.GetFullName() );
            if ( files.Count() > ++i){
                file.Assign( files.Item(i) );
                AddFiles << wxString::Format( _T("<td valign=top><font size=-2><a href=\"%s\">%s</a></font></td>"), file.GetFullPath(), file.GetFullName() );
            }
        }
        ret_val << AddFiles <<_T("</table>");
    }


#if 1
    if( lights.Count() > 0 ) {

        // For lights we now have all the info gathered but no HTML output yet, now
        // run through the data and build a merged table for all lights.

        lights.Sort( /*( CMPFUNC_wxObjArrayArrayOfLights )*/( &CompareLights ) );

        wxString lastPos;

        for( unsigned int curLightNo = 0; curLightNo < lights.Count(); curLightNo++ ) {
            PI_S57Light thisLight = /*(S57Light*)*/ lights.Item( curLightNo );
            int attrIndex;

            if( thisLight.position != lastPos ) {

                lastPos = thisLight.position;

                if( curLightNo > 0 ) lightsHtml << _T("</table>\n<hr noshade>\n");

                lightsHtml << _T("<b>Light</b> <font size=-2>(LIGHTS)</font><br>");
                lightsHtml << _T("<font size=-2>") << thisLight.position << _T("</font><br>\n");

                if( curLight->hasSectors ) lightsHtml
                    <<_("<font size=-2>(Sector angles are True Bearings from Seaward)</font><br>");

                lightsHtml << _T("<table>");
            }

            lightsHtml << _T("<tr>");
            lightsHtml << _T("<td><font size=-1>");

            attrIndex = thisLight.attributeNames.Index( _T("COLOUR") );
            if( attrIndex != wxNOT_FOUND ) {
                wxString color = thisLight.attributeValues.Item( attrIndex );
                if( color == _T("red(3)") ) lightsHtml
                    << _T("<table border=0><tr><td bgcolor=red>&nbsp;&nbsp;&nbsp;</td></tr></table> ");
                if( color == _T("green(4)") ) lightsHtml
                    << _T("<table border=0><tr><td bgcolor=green>&nbsp;&nbsp;&nbsp;</td></tr></table> ");
                if( color == _T("white(1)") ) lightsHtml
                    << _T("<table border=0><tr><td bgcolor=yellow>&nbsp;&nbsp;&nbsp;</td></tr></table> ");
            }

            lightsHtml << _T("</font></td><td><font size=-1><nobr><b>");

            attrIndex = thisLight.attributeNames.Index( _T("LITCHR") );
            if( attrIndex != wxNOT_FOUND ) {
                wxString character = thisLight.attributeValues.Item( attrIndex );
                lightsHtml << character.BeforeFirst( wxChar( '(' ) ) << _T(" ");
            }

            attrIndex = thisLight.attributeNames.Index( _T("SIGGRP") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << thisLight.attributeValues.Item( attrIndex );
                lightsHtml << _T(" ");
            }

            attrIndex = thisLight.attributeNames.Index( _T("SIGPER") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << thisLight.attributeValues.Item( attrIndex );
                lightsHtml << _T(" ");
            }

            attrIndex = thisLight.attributeNames.Index( _T("HEIGHT") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << thisLight.attributeValues.Item( attrIndex );
                lightsHtml << _T(" ");
            }

            attrIndex = thisLight.attributeNames.Index( _T("VALNMR") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << thisLight.attributeValues.Item( attrIndex );
                lightsHtml << _T(" ");
            }

            lightsHtml << _T("</b>");

            attrIndex = thisLight.attributeNames.Index( _T("SECTR1") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << _T("(") <<thisLight.attributeValues.Item( attrIndex );
                lightsHtml << _T(" - ");
                attrIndex = thisLight.attributeNames.Index( _T("SECTR2") );
                lightsHtml << thisLight.attributeValues.Item( attrIndex ) << _T(") ");
            }

            lightsHtml << _T("</nobr>");

            attrIndex = thisLight.attributeNames.Index( _T("CATLIT") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << _T("<nobr>");
                lightsHtml
                << thisLight.attributeValues.Item( attrIndex ).BeforeFirst(
                    wxChar( '(' ) );
                    lightsHtml << _T("</nobr> ");
            }

            attrIndex = thisLight.attributeNames.Index( _T("EXCLIT") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << _T("<nobr>");
                lightsHtml
                << thisLight.attributeValues.Item( attrIndex ).BeforeFirst(
                    wxChar( '(' ) );
                    lightsHtml << _T("</nobr> ");
            }

            attrIndex = thisLight.attributeNames.Index( _T("OBJNAM") );
            if( attrIndex != wxNOT_FOUND ) {
                lightsHtml << _T("<br><nobr>");
                lightsHtml << thisLight.attributeValues.Item( attrIndex ).Left( 1 ).Upper();
                lightsHtml << thisLight.attributeValues.Item( attrIndex ).Mid( 1 );
                lightsHtml << _T("</nobr> ");
            }

            lightsHtml << _T("</font></td>");
            lightsHtml << _T("</tr>");

            thisLight.attributeNames.Clear();
            thisLight.attributeValues.Clear();
//            delete thisLight;
        }
        lightsHtml << _T("</table><hr noshade>\n");
        ret_val = lightsHtml << ret_val;

        lights.Clear();
    }
#endif
    return ret_val;
}




wxString ChartS63::GetObjectAttributeValueAsString( PI_S57Obj *obj, int iatt, wxString curAttrName )
{
    wxString value;
    S57attVal *pval;

    pval = obj->attVal->Item( iatt );
    switch( pval->valType ){
        case OGR_STR: {
            if( pval->value ) {
                wxString val_str( (char *) ( pval->value ), wxConvUTF8 );
                long ival;
                if( val_str.ToLong( &ival ) ) {
                    if( 0 == ival ) value = _T("Unknown");
                    else {
                        wxString decode_val = GetAttributeDecode( curAttrName, ival );
                        if( !decode_val.IsEmpty() ) {
                            value = decode_val;
                            wxString iv;
                            iv.Printf( _T("(%d)"), (int) ival );
                            value.Append( iv );
                        } else
                            value.Printf( _T("%d"), (int) ival );
                    }
                }

                else if( val_str.IsEmpty() ) value = _T("Unknown");

                else {
                    value.Clear();
                    wxString value_increment;
                    wxStringTokenizer tk( val_str, wxT(",") );
                    int iv = 0;
                    while( tk.HasMoreTokens() ) {
                        wxString token = tk.GetNextToken();
                        long ival;
                        if( token.ToLong( &ival ) ) {
                            wxString decode_val = GetAttributeDecode( curAttrName, ival );
                            if( !decode_val.IsEmpty() ) value_increment = decode_val;
                            else
                                value_increment.Printf( _T(" %d"), (int) ival );

                            if( iv ) value_increment.Prepend( wxT(", ") );
                        }
                        value.Append( value_increment );

                        iv++;
                    }
                    value.Append( val_str );
                }
            } else
                value = _T("[NULL VALUE]");

            break;
        }

        case OGR_INT: {
            int ival = *( (int *) pval->value );
            wxString decode_val = GetAttributeDecode( curAttrName, ival );

            if( !decode_val.IsEmpty() ) {
                value = decode_val;
                wxString iv;
                iv.Printf( _T("(%d)"), ival );
                value.Append( iv );
            } else
                value.Printf( _T("(%d)"), ival );

            break;
        }
        case OGR_INT_LST:
            break;

        case OGR_REAL: {
            double dval = *( (double *) pval->value );
            wxString val_suffix = _T(" m");

            //    As a special case, convert some attribute values to feet.....
            if( ( curAttrName == _T("VERCLR") ) || ( curAttrName == _T("VERCLL") )
                || ( curAttrName == _T("HEIGHT") ) || ( curAttrName == _T("HORCLR") ) ) {
                switch( PI_GetPLIBDepthUnitInt() ){
                        case 0:                       // feet
                        case 2:                       // fathoms
                        dval = dval * 3 * 39.37 / 36;              // feet
                        val_suffix = _T(" ft");
                        break;
                        default:
                            break;
                    }
                }

                else if( ( curAttrName == _T("VALSOU") ) || ( curAttrName == _T("DRVAL1") )
                || ( curAttrName == _T("DRVAL2") ) ) {
                    switch( PI_GetPLIBDepthUnitInt() ){
                        case 0:                       // feet
                        dval = dval * 3 * 39.37 / 36;              // feet
                        val_suffix = _T(" ft");
                        break;
                        case 2:                       // fathoms
                        dval = dval * 3 * 39.37 / 36;              // fathoms
                        dval /= 6.0;
                        val_suffix = _T(" fathoms");
                        break;
                        default:
                            break;
                    }
                }

                else if( curAttrName == _T("SECTR1") ) val_suffix = _T("&deg;");
                else if( curAttrName == _T("SECTR2") ) val_suffix = _T("&deg;");
                else if( curAttrName == _T("ORIENT") ) val_suffix = _T("&deg;");
                else if( curAttrName == _T("VALNMR") ) val_suffix = _T(" Nm");
                else if( curAttrName == _T("SIGPER") ) val_suffix = _T("s");
                else if( curAttrName == _T("VALACM") ) val_suffix = _T(" Minutes/year");
                else if( curAttrName == _T("VALMAG") ) val_suffix = _T("&deg;");

               if( dval - floor( dval ) < 0.01 ) value.Printf( _T("%2.0f"), dval );
               else
                   value.Printf( _T("%4.1f"), dval );

               value << val_suffix;

               break;
        }

                        case OGR_REAL_LST: {
                            break;
                        }
    }
    return value;
}


wxString ChartS63::GetAttributeDecode( wxString& att, int ival )
{
    wxString ret_val = _T("");

#if 0
    //    Special case for "NATSUR", cacheing the strings for faster chart rendering
    if( att.IsSameAs( _T("NATSUR") ) ) {
        if( !m_natsur_hash[ival].IsEmpty() )            // entry available?
        {
            return m_natsur_hash[ival];
        }
    }
#endif
    if( !g_s57data_dir.Len() )
        return ret_val;

    //  Get the attribute code from the acronym
    const char *att_code;

    wxString file = g_s57data_dir;
    file.Append( _T("/s57attributes.csv") );

    if( !wxFileName::FileExists( file ) ) {
        wxString msg( _T("   Could not open ") );
        msg.Append( file );
        wxLogMessage( msg );

        return ret_val;
    }

    att_code = MyCSVGetField( file.mb_str(), "Acronym",                  // match field
                              att.mb_str(),               // match value
                              CC_ExactString, "Code" );             // return field

    // Now, get a nice description from s57expectedinput.csv
    //  This will have to be a 2-d search, using ID field and Code field

    // Ingest, and get a pointer to the ingested table for "Expected Input" file
    wxString ei_file = g_s57data_dir;
    ei_file.Append( _T("/s57expectedinput.csv") );

    if( !wxFileName::FileExists( ei_file ) ) {
        wxString msg( _T("   Could not open ") );
        msg.Append( ei_file );
        wxLogMessage( msg );

        return ret_val;
    }

    CSVTable *psTable = CSVAccess( ei_file.mb_str() );
    CSVIngest( ei_file.mb_str() );

    char **papszFields = NULL;
    int bSelected = FALSE;

    /* -------------------------------------------------------------------- */
    /*      Scan from in-core lines.                                        */
    /* -------------------------------------------------------------------- */
    int iline = 0;
    while( !bSelected && iline + 1 < psTable->nLineCount ) {
        iline++;
        papszFields = CSVSplitLine( psTable->papszLines[iline] );

        if( !strcmp( papszFields[0], att_code ) ) {
            if( atoi( papszFields[1] ) == ival ) {
                ret_val = wxString( papszFields[2], wxConvUTF8 );
                bSelected = TRUE;
            }
        }

        CSLDestroy( papszFields );
    }

//    if( att.IsSameAs( _T("NATSUR") ) ) m_natsur_hash[ival] = ret_val;            // cache the entry

    return ret_val;

}



void ChartS63::AssembleLineGeometry( void )
{
    // Walk the hash tables to get the required buffer size

    //  Start with the edge hash table
    size_t nPoints = 0;
    PI_VE_Hash::iterator it;
    for( it = m_ve_hash.begin(); it != m_ve_hash.end(); ++it ) {
        PI_VE_Element *pedge = it->second;
        if( pedge ) {
            nPoints += pedge->nCount;
        }
    }


    int ndelta = 0;
    //  Get the end node connected segments.  To do this, we
    //  walk the Feature array and process each feature that potetially has a LINE type element
    for( int i = 0; i < PRIO_NUM; ++i ) {
        for( int j = 0; j < LUPNAME_NUM; j++ ) {
            PI_S57Obj *obj = razRules[i][j];
            while(obj)
            {
//                 if(obj->Index == 4947)
//                     int yyp = 4;

                for( int iseg = 0; iseg < obj->m_n_lsindex; iseg++ ) {
                    int seg_index = iseg * 4;
                    int *index_run = &obj->m_lsindex_array[seg_index];

                    //  Get first connected node
                    unsigned int inode = *index_run++;

                    //  Get the edge
                    unsigned int venode = *index_run++;
                    PI_VE_Element *pedge = 0;
                    pedge = m_ve_hash[venode];

                    //  Get end connected node
                    unsigned int enode = *index_run++;

                    //  Get first connected node
                    PI_VC_Element *ipnode = 0;
                    ipnode = m_vc_hash[inode];

                    //  Get end connected node
                    PI_VC_Element *epnode = 0;
                    epnode = m_vc_hash[enode];

                    // Get the direction
                    int iDir = *index_run;
                    index_run++;

                    if( ipnode ) {
                        if(pedge && pedge->nCount)
                        {
                            //      The initial node exists and connects to the start of an edge
                            wxString key;
                            key.Printf(_T("CE%d%d"), inode, venode);

                            if(m_connector_hash.find( key ) == m_connector_hash.end()){
                                ndelta += 2;
                                PI_connector_segment *pcs = new PI_connector_segment;
                                pcs->type = TYPE_CE;
                                pcs->start = ipnode;
                                pcs->end = pedge;
                                pcs->dir = iDir;
                                m_connector_hash[key] = pcs;
                            }
                        }
                    }

                    if(pedge && pedge->nCount){

                    }   //pedge

                    // end node
                    if( epnode ) {
                        if(ipnode){
                            if(pedge && pedge->nCount){

                                wxString key;
                                key.Printf(_T("EC%d%d"), venode, enode);

                                if(m_connector_hash.find( key ) == m_connector_hash.end()){
                                    ndelta += 2;
                                    PI_connector_segment *pcs = new PI_connector_segment;
                                    pcs->type = TYPE_EC;
                                    pcs->start = pedge;
                                    pcs->end = epnode;
                                    pcs->dir = iDir;
                                    m_connector_hash[key] = pcs;
                                }
                            }
                            else {
                                wxString key;
                                key.Printf(_T("CC%d%d"), inode, enode);

                                if(m_connector_hash.find( key ) == m_connector_hash.end()){
                                    ndelta += 2;
                                    PI_connector_segment *pcs = new PI_connector_segment;
                                    pcs->type = TYPE_CC;
                                    pcs->start = ipnode;
                                    pcs->end = epnode;
                                    pcs->dir = iDir;
                                    m_connector_hash[key] = pcs;
                                }
                            }
                        }
                    }
                }  // for
                obj = obj->next;
            }
        }
    }

    //  We have the total VBO point count, and a nice hashmap of the connector segments
    nPoints += ndelta;

    size_t vbo_byte_length = 2 * nPoints * sizeof(float);
    m_vbo_byte_length = vbo_byte_length;

    m_line_vertex_buffer = (float *)malloc( vbo_byte_length);
    float *lvr = m_line_vertex_buffer;
    size_t offset = 0;

    //      Copy and convert the edge points from doubles to floats,
    //      and recording each segment's offset in the array
    for( it = m_ve_hash.begin(); it != m_ve_hash.end(); ++it ) {
        PI_VE_Element *pedge = it->second;
        if( pedge ) {
            double *pp = pedge->pPoints;
            for(int i = 0 ; i < pedge->nCount ; i++){
                double x = *pp++;
                double y = *pp++;

                *lvr++ = (float)x;
                *lvr++ = (float)y;
            }

            pedge->vbo_offset = offset;
            offset += pedge->nCount * 2 * sizeof(float);

        }
    }

    //      Now iterate on the hashmap, adding the connector segments in the hashmap to the VBO buffer
    double e0, n0, e1, n1;
    double *ppt;
    PI_VC_Element *ipnode;
    PI_VC_Element *epnode;
    PI_VE_Element *pedge;
    PI_connected_segment_hash::iterator itc;
    for( itc = m_connector_hash.begin(); itc != m_connector_hash.end(); ++itc )
    {
        wxString key = itc->first;
        PI_connector_segment *pcs = itc->second;

        switch(pcs->type){
            case TYPE_CC:
                ipnode = (PI_VC_Element *)pcs->start;
                epnode = (PI_VC_Element *)pcs->end;

                ppt = ipnode->pPoint;
                e0 = *ppt++;
                n0 = *ppt;

                ppt = epnode->pPoint;
                e1 = *ppt++;
                n1 = *ppt;

                *lvr++ = (float)e0;
                *lvr++ = (float)n0;
                *lvr++ = (float)e1;
                *lvr++ = (float)n1;

                pcs->vbo_offset = offset;
                offset += 4 * sizeof(float);

                break;

            case TYPE_CE:
                ipnode = (PI_VC_Element *)pcs->start;
                ppt = ipnode->pPoint;
                e0 = *ppt++;
                n0 = *ppt;

                if(pcs->dir == 1){
                    pedge = (PI_VE_Element *)pcs->end;
                    e1 = pedge->pPoints[ 0 ];
                    n1 = pedge->pPoints[ 1 ];
                }
                else{
                    pedge = (PI_VE_Element *)pcs->end;
                    //e1 = pedge->pPoints[ 0 ];
                    //n1 = pedge->pPoints[ 1 ];
                    e1 = pedge->pPoints[ (2 * (pedge->nCount - 1))];
                    n1 = pedge->pPoints[ (2 * (pedge->nCount - 1)) + 1];

                }


                *lvr++ = (float)e0;
                *lvr++ = (float)n0;
                *lvr++ = (float)e1;
                *lvr++ = (float)n1;

                pcs->vbo_offset = offset;
                offset += 4 * sizeof(float);
                break;

            case TYPE_EC:
                if(pcs->dir == 1){
                    pedge = (PI_VE_Element *)pcs->start;
                    e0 = pedge->pPoints[ (2 * (pedge->nCount - 1))];
                    n0 = pedge->pPoints[ (2 * (pedge->nCount - 1)) + 1];
                }
                else{
                    pedge = (PI_VE_Element *)pcs->start;
                    e0 = pedge->pPoints[ 0 ];
                    n0 = pedge->pPoints[ 1 ];
                }

                epnode = (PI_VC_Element *)pcs->end;
                ppt = epnode->pPoint;
                e1 = *ppt++;
                n1 = *ppt;

                *lvr++ = (float)e0;
                *lvr++ = (float)n0;
                *lvr++ = (float)e1;
                *lvr++ = (float)n1;

                pcs->vbo_offset = offset;
                offset += 4 * sizeof(float);

                break;
            default:
                break;
        }
    }

    // Now ready to walk the object array again, building the per-object list of renderable segments
    for( int i = 0; i < PRIO_NUM; ++i ) {
        for( int j = 0; j < LUPNAME_NUM; j++ ) {
            PI_S57Obj *obj = razRules[i][j];
            while (obj)
            {
                PI_line_segment_element *list_top = new PI_line_segment_element;
                list_top->n_points = 0;
                list_top->next = 0;

                PI_line_segment_element *le_current = list_top;

                for( int iseg = 0; iseg < obj->m_n_lsindex; iseg++ ) {
                    int seg_index = iseg * 4;
                    int *index_run = &obj->m_lsindex_array[seg_index];

                    //  Get first connected node
                    unsigned int inode = *index_run++;

                    //  Get the edge
                    unsigned int venode = *index_run++;
                    PI_VE_Element *pedge = 0;
                    pedge = m_ve_hash[venode];

                    //  Get end connected node
                    unsigned int enode = *index_run++;

                    // Get the direction
                    //int iDir = *index_run;
                    index_run++;

                    //  Get first connected node
                    PI_VC_Element *ipnode = 0;
                    ipnode = m_vc_hash[inode];

                    //  Get end connected node
                    PI_VC_Element *epnode = 0;
                    epnode = m_vc_hash[enode];

                    double e0, n0, e1, n1;

                    if( ipnode ) {
                        double *ppt = ipnode->pPoint;
                        e0 = *ppt++;
                        n0 = *ppt;

                        if(pedge && pedge->nCount)
                        {
                            wxString key;
                            key.Printf(_T("CE%d%d"), inode, venode);

                            if(m_connector_hash.find( key ) != m_connector_hash.end()){

                                PI_connector_segment *pcs = m_connector_hash[key];

                                PI_line_segment_element *pls = new PI_line_segment_element;
                                pls->next = 0;
                                pls->vbo_offset = pcs->vbo_offset;
                                pls->n_points = 2;
                                pls->priority = 0;
                                pls->private0 = pcs;
                                pls->type = TYPE_CE;

                                //  Get the bounding box
                                e1 = pedge->pPoints[0];
                                n1 = pedge->pPoints[1];

                                double lat0, lon0, lat1, lon1;
                                fromSM_Plugin( e0, n0, m_ref_lat, m_ref_lon, &lat0, &lon0 );
                                fromSM_Plugin( e1, n1, m_ref_lat, m_ref_lon, &lat1, &lon1 );

                                pls->lat_max = wxMax(lat0, lat1);
                                pls->lat_min = wxMin(lat0, lat1);
                                pls->lon_max = wxMax(lon0, lon1);
                                pls->lon_min = wxMin(lon0, lon1);

                                le_current->next = pls;             // hook it up
                                le_current = pls;
                            }
                        }
                    }

                    if(pedge && pedge->nCount){
                        PI_line_segment_element *pls = new PI_line_segment_element;
                        pls->next = 0;
                        pls->vbo_offset = pedge->vbo_offset;
                        pls->n_points = pedge->nCount;
                        pls->priority = 0;
                        pls->private0 = pedge;
                        pls->type = TYPE_EE;

                        double lat_max = -100;
                        double lat_min = 100;
                        double lon_max = -400;
                        double lon_min = 400;

                        for(int i= 0 ; i < pedge->nCount ; i++){
                            double lat, lon;
                            e0 = pedge->pPoints[ (2 * i)];
                            n0 = pedge->pPoints[ (2 * i) + 1];

                            fromSM_Plugin( e0, n0, m_ref_lat, m_ref_lon, &lat, &lon );
                            lat_max = wxMax(lat_max, lat);
                            lat_min = wxMin(lat_min, lat);
                            lon_max = wxMax(lon_max, lon);
                            lon_min = wxMin(lon_min, lon);
                        }

                        pls->lat_max = lat_max;
                        pls->lat_min = lat_min;
                        pls->lon_max = lon_max;
                        pls->lon_min = lon_min;

                        le_current->next = pls;             // hook it up
                        le_current = pls;

                        e0 = pedge->pPoints[ (2 * (pedge->nCount - 1))];
                        n0 = pedge->pPoints[ (2 * (pedge->nCount - 1)) + 1];


                    }   //pedge

                    // end node
                    if( epnode ) {
                        double *ppt = epnode->pPoint;
                        e1 = *ppt++;
                        n1 = *ppt;

                        if(ipnode){
                            if(pedge && pedge->nCount){

                                wxString key;
                                key.Printf(_T("EC%d%d"), venode, enode);

                                if(m_connector_hash.find( key ) != m_connector_hash.end()){
                                    PI_connector_segment *pcs = m_connector_hash[key];

                                    PI_line_segment_element *pls = new PI_line_segment_element;
                                    pls->next = 0;
                                    pls->vbo_offset = pcs->vbo_offset;
                                    pls->n_points = 2;
                                    pls->priority = 0;
                                    pls->private0 = pcs;
                                    pls->type = TYPE_EC;

                                    double lat0, lon0, lat1, lon1;
                                    fromSM_Plugin( e0, n0, m_ref_lat, m_ref_lon, &lat0, &lon0 );
                                    fromSM_Plugin( e1, n1, m_ref_lat, m_ref_lon, &lat1, &lon1 );

                                    pls->lat_max = wxMax(lat0, lat1);
                                    pls->lat_min = wxMin(lat0, lat1);
                                    pls->lon_max = wxMax(lon0, lon1);
                                    pls->lon_min = wxMin(lon0, lon1);

                                    le_current->next = pls;             // hook it up
                                    le_current = pls;
                                }
                            }
                            else {
                                wxString key;
                                key.Printf(_T("CC%d%d"), inode, enode);

                                if(m_connector_hash.find( key ) != m_connector_hash.end()){
                                    PI_connector_segment *pcs = m_connector_hash[key];

                                    PI_line_segment_element *pls = new PI_line_segment_element;
                                    pls->next = 0;
                                    pls->vbo_offset = pcs->vbo_offset;
                                    pls->n_points = 2;
                                    pls->priority = 0;
                                    pls->private0 = pcs;
                                    pls->type = TYPE_CC;

                                    double lat0, lon0, lat1, lon1;
                                    fromSM_Plugin( e0, n0, m_ref_lat, m_ref_lon, &lat0, &lon0 );
                                    fromSM_Plugin( e1, n1, m_ref_lat, m_ref_lon, &lat1, &lon1 );

                                    pls->lat_max = wxMax(lat0, lat1);
                                    pls->lat_min = wxMin(lat0, lat1);
                                    pls->lon_max = wxMax(lon0, lon1);
                                    pls->lon_min = wxMin(lon0, lon1);

                                    le_current->next = pls;             // hook it up
                                    le_current = pls;
                                }
                            }
                        }
                    }
                }  // for

                //  All done, so assign the list to the object
                obj->m_ls_list = list_top->next;    // skipping the empty first placeholder element
                delete list_top;

                obj = obj->next;

            }
        }
    }

}


void ChartS63::BuildLineVBO( void )
{

#ifdef BUILD_VBO
#ifdef ocpnUSE_GL

    if(!g_b_EnableVBO)
        return;

    if(m_LineVBO_name == -1){

        //      Create the VBO
        GLuint vboId;
        (s_glGenBuffers)(1, &vboId);

        // bind VBO in order to use
        (s_glBindBuffer)(GL_ARRAY_BUFFER, vboId);

        // upload data to VBO
        glEnableClientState(GL_VERTEX_ARRAY);             // activate vertex coords array
        (s_glBufferData)(GL_ARRAY_BUFFER, m_vbo_byte_length, m_line_vertex_buffer, GL_STATIC_DRAW);

        glDisableClientState(GL_VERTEX_ARRAY);            // deactivate vertex array
        (s_glBindBuffer)(GL_ARRAY_BUFFER, 0);

        //  Loop and populate all the objects
        for( int i = 0; i < PRIO_NUM; ++i ) {
            for( int j = 0; j < LUPNAME_NUM; j++ ) {
                PI_S57Obj *obj = razRules[i][j];
                obj->auxParm2 = vboId;
            }
        }

        m_LineVBO_name = vboId;

    }
#endif
#endif
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
int py_fgets( char *buf, int buf_len_max, CryptInputStream *ifs )

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
    pPolyTessGeo = NULL;

    bIsAton = false;
    bIsAssociable = false;
    m_n_lsindex = 0;
    m_lsindex_array = NULL;
    m_n_edge_max_points = 0;


    //        Set default (unity) auxiliary transform coefficients
    x_rate = 1.0;
    y_rate = 1.0;
    x_origin = 0.0;
    y_origin = 0.0;

    S52_Context = NULL;
    m_ls_list = 0;

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

        if( geoPt ) free( geoPt );
        if( geoPtz ) free( geoPtz );
        if( geoPtMulti ) free( geoPtMulti );

//        if( pPolyTessGeo ) delete (PolyTessGeo63*)pPolyTessGeo;
        if( pPolyTessGeo ) {
#ifdef ocpnUSE_GL
            bool b_useVBO = g_b_EnableVBO  && (auxParm0 > 0);    // VBO allowed?
            PolyTessGeo63  *pPTG = (PolyTessGeo63*)pPolyTessGeo;
            PolyTriGroup *ppg_vbo = pPTG->Get_PolyTriGroup_head();
            if (b_useVBO && ppg_vbo && auxParm0 > 0 && ppg_vbo->single_buffer && s_glDeleteBuffers) {
                 s_glDeleteBuffers(1, (GLuint *)&auxParm0);
            }
#endif
            delete (PolyTessGeo63*)pPolyTessGeo;
        }

//        if(S52_Context) delete (S52PLIB_Context *)S52_Context;

        if( m_lsindex_array ) free( m_lsindex_array );

        if(m_ls_list){
            PI_line_segment_element *element = m_ls_list;
            while(element){
                PI_line_segment_element *next = element->next;
                delete element;
                element = next;
            }
        }

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
    m_bcategory_mutable = false;

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

    //        Set default (unity) auxiliary transform coefficients
    x_rate = 1.0;
    y_rate = 1.0;
    x_origin = 0.0;
    y_origin = 0.0;

    S52_Context = NULL;
    child = NULL;
    next = NULL;
    pPolyTessGeo = NULL;

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

        if( geoPt ) free( geoPt );
        if( geoPtz ) free( geoPtz );
        if( geoPtMulti ) free( geoPtMulti );

        if( pPolyTessGeo ) delete (PolyTessGeo63*)pPolyTessGeo;

        if( m_lsindex_array ) free( m_lsindex_array );
    }
}

//----------------------------------------------------------------------------------
//      PI_S57ObjX CTOR from SENC file
//----------------------------------------------------------------------------------

PI_S57ObjX::PI_S57ObjX( char *first_line, CryptInputStream *fpx, int senc_file_version )
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
    child = NULL;
    next = NULL;
    m_bcategory_mutable = false;

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
                        double point_ref_lat, point_ref_lon;
                        sscanf( buf, "%s %lf %lf", tbuf, &point_ref_lat, &point_ref_lon );

                        py_fgets( buf, MAX_LINE, fpx );
                        int wkb_len = atoi( buf + 2 );
                        fpx->Read( buf, wkb_len );

                        double easting, northing;
                        npt = 1;
                        double *pfs = (double *) ( buf + 5 );                // point to the point
#ifdef ARMHF
                        double east, north;
                        memcpy(&east, pfs++, sizeof(double));
                        memcpy(&north, pfs, sizeof(double));
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
                        lon_min = m_lon;
                        lon_max = m_lon;
                        lat_min = m_lat;
                        lat_max = m_lat;

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
                        float xmax, xmin, ymax, ymin;
#ifdef ARMHF
                        memcpy(&xmax, pfs++, sizeof(float));
                        memcpy(&xmin, pfs++, sizeof(float));
                        memcpy(&ymax, pfs++, sizeof(float));
                        memcpy(&ymin, pfs++, sizeof(float));

#else
                        xmax = *pfs++;
                        xmin = *pfs++;
                        ymax = *pfs++;
                        ymin = *pfs;
#endif
                        lon_min = xmin;
                        lon_max = xmax;
                        lat_min = ymin;
                        lat_max = ymax;

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

                        m_lsindex_array = (int *) malloc( 4 * m_n_lsindex * sizeof(int) );
                        fpx->Read( (char *)m_lsindex_array, 4 * m_n_lsindex * sizeof(int) );
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
                            PolyTessGeo63 *ppg = new PolyTessGeo63( (unsigned char *)polybuf, nrecl, FEIndex, senc_file_version );
                            free( polybuf );

                            pPolyTessGeo = (void *)ppg;

                            //  Set the s57obj bounding box as lat/lon
//                            BBObj.SetMin( ppg->Get_xmin(), ppg->Get_ymin() );
//                            BBObj.SetMax( ppg->Get_xmax(), ppg->Get_ymax() );
                            lon_min = ppg->Get_xmin();
                            lon_max = ppg->Get_xmax();
                            lat_min = ppg->Get_ymin();
                            lat_max = ppg->Get_ymax();


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

                            m_lsindex_array = (int *) malloc( 4 * m_n_lsindex * sizeof(int) );
                            fpx->Read( (char *)m_lsindex_array, 4 * m_n_lsindex * sizeof(int) );
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



int Intersect(MyPoint, MyPoint, MyPoint, MyPoint) ;
int CCW(MyPoint, MyPoint, MyPoint) ;


/*************************************************************************
 *
 *
 * FUNCTION:   G_PtInPolygon
 *
 * PURPOSE
 * This routine determines if the point passed is in the polygon. It uses
 * the classical polygon hit-testing algorithm: a horizontal ray starting
 * at the point is extended infinitely rightwards and the number of
 * polygon edges that intersect the ray are counted. If the number is odd,
 * the point is inside the polygon.
 *
 * Polygon is assumed OPEN, not CLOSED.
 * RETURN VALUE
 * (bool) TRUE if the point is inside the polygon, FALSE if not.
 *************************************************************************/


int  G_PtInPolygon(MyPoint *rgpts, int wnumpts, float x, float y)
{

    MyPoint  *ppt, *ppt1 ;
    int   i ;
    MyPoint  pt1, pt2, pt0 ;
    int   wnumintsct = 0 ;


    pt0.x = x;
    pt0.y = y;

    pt1 = pt2 = pt0 ;
    pt2.x = 1.e8;

    // Now go through each of the lines in the polygon and see if it
    // intersects
    for (i = 0, ppt = rgpts ; i < wnumpts-1 ; i++, ppt++)
    {
        ppt1 = ppt;
        ppt1++;
        if (Intersect(pt0, pt2, *ppt, *(ppt1)))
            wnumintsct++ ;
    }

    // And the last line
    if (Intersect(pt0, pt2, *ppt, *rgpts))
        wnumintsct++ ;

    return (wnumintsct&1) ;

}


/*************************************************************************
 *
 *              0
 * FUNCTION:   Intersect
 *
 * PURPOSE
 * Given two line segments, determine if they intersect.
 *
 * RETURN VALUE
 * TRUE if they intersect, FALSE if not.
 *************************************************************************/


int Intersect(MyPoint p1, MyPoint p2, MyPoint p3, MyPoint p4) {
//    int i;
//    i = CCW(p1, p2, p3);
//    i = CCW(p1, p2, p4);
//    i = CCW(p3, p4, p1);
//    i = CCW(p3, p4, p2);
    return ((( CCW(p1, p2, p3) * CCW(p1, p2, p4)) <= 0)
    && (( CCW(p3, p4, p1) * CCW(p3, p4, p2)  <= 0) )) ;

}
/*************************************************************************
 *
 *
 * FUNCTION:   CCW (CounterClockWise)
 *
 * PURPOSE
 * Determines, given three points, if when travelling from the first to
 * the second to the third, we travel in a counterclockwise direction.
 *
 * RETURN VALUE
 * (int) 1 if the movement is in a counterclockwise direction, -1 if
 * not.
 *************************************************************************/


int CCW(MyPoint p0, MyPoint p1, MyPoint p2) {
    double dx1, dx2 ;
    double dy1, dy2 ;

    dx1 = p1.x - p0.x ; dx2 = p2.x - p0.x ;
    dy1 = p1.y - p0.y ; dy2 = p2.y - p0.y ;

    /* This is a slope comparison: we don't do divisions because
     * of divide by zero possibilities with pure horizontal and pure
     * vertical lines.
     */
    return ((dx1 * dy2 > dy1 * dx2) ? 1 : -1) ;

}




