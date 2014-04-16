/******************************************************************************
 *
 * Project:  S64_pi
 * Purpose:  S63_pi Plugin core
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2013 by David S. Register   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/textfile.h>
#include "wx/tokenzr.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/file.h"
#include "wx/stream.h"
#include "wx/wfstream.h"
#include <wx/statline.h>
#include <wx/progdlg.h>

#include "s63_pi.h"
#include "s63chart.h"
#include "src/myiso8211/iso8211.h"
#include "dsa_utils.h"

//      Some PlugIn global variables
wxString                        g_sencutil_bin;
S63ScreenLogContainer           *g_pScreenLog;
S63ScreenLog                    *g_pPanelScreenLog;
unsigned int                    g_backchannel_port;
unsigned int                    g_frontchannel_port;
wxString                        g_s57data_dir;

wxString                        g_userpermit;
wxString                        g_installpermit;
s63_pi                          *g_pi;
wxString                        g_pi_filename;
wxString                        g_SENCdir;

bool                            g_bsuppress_log;
wxProgressDialog                *g_pprog;
wxString                        g_old_installpermit;
wxString                        g_old_userpermit;

//      A prototype of the default IHO.PUB public key file
wxString i0(_T("// BIG p"));
wxString i1(_T("FCA6 82CE 8E12 CABA 26EF CCF7 110E 526D B078 B05E DECB CD1E B4A2 08F3 AE16 17AE 01F3 5B91 A47E 6DF6 3413 C5E1 2ED0 899B CD13 2ACD 50D9 9151 BDC4 3EE7 3759 2E17."));
wxString i2(_T("// BIG q"));
wxString i3(_T("962E DDCC 369C BA8E BB26 0EE6 B6A1 26D9 346E 38C5."));
wxString i4(_T("// BIG g"));
wxString i5(_T("6784 71B2 7A9C F44E E91A 49C5 147D B1A9 AAF2 44F0 5A43 4D64 8693 1D2D 1427 1B9E 3503 0B71 FD73 DA17 9069 B32E 2935 630E 1C20 6235 4D0D A20A 6C41 6E50 BE79 4CA4."));
wxString i6(_T("// BIG y"));
wxString i7(_T("963F 14E3 2BA5 3729 28F2 4F15 B073 0C49 D31B 28E5 C764 1002 564D B959 95B1 5CF8 800E D54E 3548 67B8 2BB9 597B 1582 69E0 79F0 C4F4 926B 1776 1CC8 9EB7 7C9B 7EF8."));

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new s63_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}



static int ExtensionCompare( const wxString& first, const wxString& second )
{
    wxFileName fn1( first );
    wxFileName fn2( second );
    wxString ext1( fn1.GetExt() );
    wxString ext2( fn2.GetExt() );
    
    return ext1.Cmp( ext2 );
}



//---------------------------------------------------------------------------------------------------------
//
//    PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

#include "default_pi.xpm"


//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

s63_pi::s63_pi(void *ppimgr)
      :opencpn_plugin_111(ppimgr)
{
      // Create the PlugIn icons
      m_pplugin_icon = new wxBitmap(default_pi);

      g_pi = this;              // Store a global handle to the PlugIn itself
      
      m_event_handler = new s63_pi_event_handler(this);

      wxFileName fn_exe(GetOCPN_ExePath());

      //        Specify the location of the OCPNsenc helper.
      g_sencutil_bin = fn_exe.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + _T("OCPNsenc");
      
      
#ifdef __WXMSW__
      g_sencutil_bin = _T("\"") + fn_exe.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + 
           _T("plugins\\s63_pi\\OCPNsenc.exe\"");
#endif 
           
#ifdef __WXOSX__
      fn_exe.RemoveLastDir();     
      g_sencutil_bin = _T("\"") + fn_exe.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + 
           _T("PlugIns/s63_pi/OCPNsenc\"");
#endif 

      //        Set up a globally accesible string pointing to the eSENC storage location     
      g_SENCdir = *GetpPrivateApplicationDataLocation();
      g_SENCdir += wxFileName::GetPathSeparator();
      g_SENCdir += _T("s63");
      g_SENCdir += wxFileName::GetPathSeparator();
      g_SENCdir += _T("s63SENC");
           
      
      g_backchannel_port = 49500; //49152;       //ports 49152–65535 are unallocated
      
      g_pScreenLog = NULL; 
      g_pPanelScreenLog = NULL;
      
      g_frontchannel_port = 50000;
      
      g_s57data_dir = *GetpSharedDataLocation();
      g_s57data_dir += _T("s57data");
      
      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();
    
      m_up_text = NULL;
      LoadConfig();
      
      
      
}

s63_pi::~s63_pi()
{
      delete m_pplugin_icon;
}

int s63_pi::Init(void)
{
//    ScreenLogMessage( _T("s63_pi Init()\n") );
    
    //  Get the path of the PlugIn itself
    g_pi_filename = GetPlugInPath(this);
    
    AddLocaleCatalog( _T("s63_pi") );

      //    Build an arraystring of dynamically loadable chart class names
    m_class_name_array.Add(_T("ChartS63"));

    
    //  Make sure the Certificate directory exists, and is populated with the most current IHO.PUB key file
    wxString dir = GetCertificateDir();
    
    if( !wxFileName::DirExists( dir ) ){
        wxFileName::Mkdir(dir, 0777, wxPATH_MKDIR_FULL);
    }
    
    wxString iho_pub = dir + wxFileName::GetPathSeparator() + _T("IHO.PUB");
    if(!::wxFileExists( iho_pub )){
        wxTextFile file(iho_pub);
        file.Create();
        file.AddLine(i0, wxTextFileType_Dos);
        file.AddLine(i1, wxTextFileType_Dos);
        file.AddLine(i2, wxTextFileType_Dos);
        file.AddLine(i3, wxTextFileType_Dos);
        file.AddLine(i4, wxTextFileType_Dos);
        file.AddLine(i5, wxTextFileType_Dos);
        file.AddLine(i6, wxTextFileType_Dos);
        file.AddLine(i7, wxTextFileType_Dos);
        
        file.Write();
        file.Close();
    }
    
    return (INSTALLS_PLUGIN_CHART_GL | INSTALLS_TOOLBOX_PAGE | WANTS_PLUGIN_MESSAGING);

}

bool s63_pi::DeInit(void)
{
    SaveConfig();
    return true;
}

int s63_pi::GetAPIVersionMajor()
{
      return MY_API_VERSION_MAJOR;
}

int s63_pi::GetAPIVersionMinor()
{
      return MY_API_VERSION_MINOR;
}

int s63_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int s63_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *s63_pi::GetPlugInBitmap()
{
      return m_pplugin_icon;
}

wxString s63_pi::GetCommonName()
{
      return _("S63");
}


wxString s63_pi::GetShortDescription()
{
      return _("S63 PlugIn for OpenCPN");
}


wxString s63_pi::GetLongDescription()
{
      return _("S63 PlugIn for OpenCPN\n\
Provides support of S63 charts.\n\n\
");

}

wxArrayString s63_pi::GetDynamicChartClassNameArray()
{
      return m_class_name_array;
}

void s63_pi::SetPluginMessage(wxString &message_id, wxString &message_body)
{
    if(message_id == _T("S63_CALLBACK_PRIVATE_1") ){
        ImportCells();
    }
    
}



//      Options Dialog Page management

void s63_pi::OnSetupOptions(){

    //  Create the S63 Options panel, and load it

    m_s63chartPanelWin = AddOptionsPage( PI_OPTIONS_PARENT_CHARTS, _("S63 Charts") );

    wxBoxSizer *chartPanelSizer = new wxBoxSizer( wxVERTICAL );
    m_s63chartPanelWin->SetSizer( chartPanelSizer );

    int border_size = 2;

    wxBoxSizer* cmdButtonSizer = new wxBoxSizer( wxVERTICAL );
    chartPanelSizer->Add( cmdButtonSizer, 0, wxALL, border_size );

    //  Chart cell permit listbox, etc
    wxStaticBoxSizer* sbSizerLB= new wxStaticBoxSizer( new wxStaticBox( m_s63chartPanelWin, wxID_ANY, _("Installed S63 Cell Permits") ), wxVERTICAL );
    
    wxBoxSizer* bSizer17;
    bSizer17 = new wxBoxSizer( wxHORIZONTAL );

    m_permit_list = new OCPNPermitList( m_s63chartPanelWin );
    
    wxListItem col0;
    col0.SetId(0);
    col0.SetText( _("Cell Name") );
    m_permit_list->InsertColumn(0, col0);
    
    wxListItem col1;
    col1.SetId(1);
    col1.SetText( _("Data Server ID") );
    m_permit_list->InsertColumn(1, col1);
    
    wxListItem col2;
    col2.SetId(2);
    col2.SetText( _("Expiration Date") );
    m_permit_list->InsertColumn(2, col2);
    
    wxString permit_dir = GetPermitDir();
    m_permit_list->BuildList( permit_dir );

    bSizer17->Add( m_permit_list, 1, wxALL|wxEXPAND, 5 );
    
    wxBoxSizer* bSizer18;
    bSizer18 = new wxBoxSizer( wxVERTICAL );
    
    m_buttonImportPermit = new wxButton( m_s63chartPanelWin, wxID_ANY, _("Import Cell Permits..."), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer18->Add( m_buttonImportPermit, 0, wxALL, 5 );
    
    m_buttonRemovePermit = new wxButton( m_s63chartPanelWin, wxID_ANY, _("Remove Permits"), wxDefaultPosition, wxDefaultSize, 0 );
    m_buttonRemovePermit->Enable( false );
    bSizer18->Add( m_buttonRemovePermit, 0, wxALL, 5 );

    m_buttonImportCells = new wxButton( m_s63chartPanelWin, wxID_ANY, _("Import Charts/Updates..."), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer18->Add( m_buttonImportCells, 0, wxALL, 5 );
    
    bSizer17->Add( bSizer18, 0, wxEXPAND, 5 );
    sbSizerLB->Add( bSizer17, 1, wxEXPAND, 5 );
    
    chartPanelSizer->Add( sbSizerLB, 0, wxEXPAND, 5 );
    chartPanelSizer->AddSpacer( 5 );
    

    //  Certificate listbox, etc
    wxStaticBoxSizer* sbSizerLBCert= new wxStaticBoxSizer( new wxStaticBox( m_s63chartPanelWin, wxID_ANY, _("Installed S63 Certificates/Keys") ), wxVERTICAL );
    
    wxBoxSizer* bSizer17C = new wxBoxSizer( wxHORIZONTAL );
    
    m_cert_list = new OCPNCertificateList( m_s63chartPanelWin );
    
    wxListItem col0c;
    col0c.SetId(0);
    col0c.SetText( _("Certificate Name") );
    m_cert_list->InsertColumn(0, col0c);
    
#if 0    
    wxListItem col1;
    col1.SetId(1);
    col1.SetText( _("Data Server ID") );
    m_permit_list->InsertColumn(1, col1);
    
    wxListItem col2;
    col2.SetId(2);
    col2.SetText( _("Expiration Date") );
    m_permit_list->InsertColumn(2, col2);
    
#endif

    m_cert_list->BuildList( GetCertificateDir() );
    
    
    bSizer17C->Add( m_cert_list, 1, wxALL|wxFIXED_MINSIZE, 5 );
    
    wxBoxSizer* bSizer18C = new wxBoxSizer( wxVERTICAL );
    
    m_buttonImportCert = new wxButton( m_s63chartPanelWin, wxID_ANY, _("Import Certificate..."), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer18C->Add( m_buttonImportCert, 0, wxALL, 5 );
    
    bSizer17C->Add( bSizer18C, 0, wxEXPAND, 5 );
    sbSizerLBCert->Add( bSizer17C, 1, wxEXPAND, 5 );
    
    chartPanelSizer->Add( sbSizerLBCert, 0, wxEXPAND, 5 );
    chartPanelSizer->AddSpacer( 5 );
    
    //  User Permit
    wxStaticBoxSizer* sbSizerUP= new wxStaticBoxSizer( new wxStaticBox( m_s63chartPanelWin, wxID_ANY, _("UserPermit") ), wxHORIZONTAL );
    m_up_text = new wxStaticText(m_s63chartPanelWin, wxID_ANY, _T(""));
    
    if(g_userpermit.Len())
        m_up_text->SetLabel( GetUserpermit() );
    sbSizerUP->Add(m_up_text, wxEXPAND);
 
    m_buttonNewUP = new wxButton( m_s63chartPanelWin, wxID_ANY, _("New Userpermit..."), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizerUP->Add( m_buttonNewUP, 0, wxALL | wxALIGN_RIGHT, 5 );
    
    chartPanelSizer->AddSpacer( 5 );
    chartPanelSizer->Add( sbSizerUP, 0, wxEXPAND, 5 );

    //  Install Permit
    wxStaticBoxSizer* sbSizerIP= new wxStaticBoxSizer( new wxStaticBox( m_s63chartPanelWin, wxID_ANY, _("InstallPermit") ), wxHORIZONTAL );
    m_ip_text = new wxStaticText(m_s63chartPanelWin, wxID_ANY, _T(""));
    
    if(g_installpermit.Len())
        m_ip_text->SetLabel( GetInstallpermit() );
    sbSizerIP->Add(m_ip_text, wxEXPAND);
    
    m_buttonNewIP = new wxButton( m_s63chartPanelWin, wxID_ANY, _("New Installpermit..."), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizerIP->Add( m_buttonNewIP, 0, wxALL | wxALIGN_RIGHT, 5 );
    
    chartPanelSizer->AddSpacer( 5 );
    chartPanelSizer->Add( sbSizerIP, 0, wxEXPAND, 5 );
    
    chartPanelSizer->AddSpacer( 15 );
    wxStaticLine *psl = new wxStaticLine(m_s63chartPanelWin, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    chartPanelSizer->Add( psl, 0, wxEXPAND, 5 );
    chartPanelSizer->AddSpacer( 15 );
    
    
    if(g_pScreenLog) {
        g_pScreenLog->Close();
        delete g_pScreenLog;
        g_pScreenLog = NULL;
    }

    wxStaticBoxSizer* sbSizerSL= new wxStaticBoxSizer( new wxStaticBox( m_s63chartPanelWin, wxID_ANY, _("S63_pi Log") ), wxVERTICAL );
    
    g_backchannel_port++;
    g_pPanelScreenLog = new S63ScreenLog( m_s63chartPanelWin );
    sbSizerSL->Add( g_pPanelScreenLog, 1, wxEXPAND, 5 );

    chartPanelSizer->Add( sbSizerSL, 1, wxEXPAND, 5 );

    m_cert_list->SetMinSize(wxSize(-1, 80));
    m_s63chartPanelWin->Layout();


    //  Connect to Events
    m_buttonImportPermit->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(s63_pi_event_handler::OnImportPermitClick), NULL, m_event_handler );

    m_buttonRemovePermit->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(s63_pi_event_handler::OnRemovePermitClick), NULL, m_event_handler );
    
    m_buttonImportCells->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                   wxCommandEventHandler(s63_pi_event_handler::OnImportCellsClick), NULL, m_event_handler );
    
    m_buttonNewUP->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(s63_pi_event_handler::OnNewUserpermitClick), NULL, m_event_handler );
    
    m_buttonNewIP->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                            wxCommandEventHandler(s63_pi_event_handler::OnNewInstallpermitClick), NULL, m_event_handler );
    
    m_permit_list->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED,
                  wxListEventHandler( s63_pi_event_handler::OnSelectPermit ), NULL, m_event_handler );
    
    m_buttonImportCert->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler(s63_pi_event_handler::OnImportCertClick), NULL, m_event_handler );
    
}

void s63_pi::OnCloseToolboxPanel(int page_sel, int ok_apply_cancel)
{
    m_up_text = NULL;
    
    if(g_pPanelScreenLog){
        g_pPanelScreenLog->Close();
        delete g_pPanelScreenLog;
    }
    
    g_backchannel_port++;
    g_pScreenLog = new S63ScreenLogContainer( GetOCPNCanvasWindow() );
    g_pScreenLog->Centre();
    
}

wxString s63_pi::GetPermitDir()
{
    wxString os63_dirname = *GetpPrivateApplicationDataLocation();
    os63_dirname += wxFileName::GetPathSeparator();
    os63_dirname += _T("s63");
    os63_dirname += wxFileName::GetPathSeparator();
    os63_dirname += _T("s63charts");
    
    return os63_dirname;
}


Catalog31 *s63_pi::CreateCatalog31(const wxString &file31)
{
    Catalog31 *rv = new Catalog31();
    
    DDFModule poModule;
    if( poModule.Open( file31.mb_str() ) ) {
        poModule.Rewind();
        
        //    Read and parse the file
        //    Each record corresponds to a file in the exchange set
        
        DDFRecord *pr = poModule.ReadRecord();                              // Record 0
        
        while(pr){
            
            Catalog_Entry31 *pentry = new Catalog_Entry31;
            
            //  Look for records whose bas file name is the same as the .000 file, and is numeric extension
            //  And decide whether to add them to update array
            
            char *u = NULL;
            u = (char *) ( pr->GetStringSubfield( "CATD", 0, "FILE", 0 ) );
            if( u ) {
                wxString file = wxString( u, wxConvUTF8 );
                
#ifndef __WXMSW__                
                file.Replace(_T("\\"), _T("/"));
#endif                
                
                pentry->m_filename = file;
            }
           
           u = (char *) ( pr->GetStringSubfield( "CATD", 0, "COMT", 0 ) );
           if(u){
               wxString comt = wxString( u, wxConvUTF8 );
               pentry->m_comt = comt;
           }
           
           rv->Add(pentry);
           
           pr = poModule.ReadRecord();
        }
        
    }
    
    
    return rv;
}




int s63_pi::ImportCells( void )
{
    m_bSSE26_shown = false;
    bool b_error = false;
    g_pprog = NULL;
    
    //  Get the ENC_ROOT directory
    wxString enc_root_dir;
    
    wxDirDialog *DiropenDialog = new wxDirDialog( NULL, _("Select S63 exchange set root directory (usually ENC_ROOT)"),
                                                  m_last_enc_root_dir);
    int dirresponse = DiropenDialog->ShowModal();
    if( dirresponse == wxID_OK ){
        enc_root_dir = DiropenDialog->GetPath();
        m_last_enc_root_dir = enc_root_dir;
        SaveConfig();
    }
    else {
        enc_root_dir = _T("");
    }
    
    if( !enc_root_dir.Len() ){
        return 0;
    }

    wxString msg = _("OpenCPN can create eSENC files as cells are imported.\n\n");
    msg += _("Note:\n");
    msg += _("This process may take some time.\n");
    msg += _("The process may be Cancelled by the user at any time.\n");
    msg += _("eSENCS not yet processed will be created as needed by OpenCPN.\n\n");
    msg += _("Create eSENCs on Import?\n");
    
    
    int dret = OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                          msg,
                          _T("s63_pi Message"),  wxYES_NO, -1, -1);
              
    bool bSENC = (dret == wxID_YES);

    //  Read the SERIAL.ENC file to absolutlely identify the Data Server ID
    wxString data_server_string;
    wxFileName serial_enc(enc_root_dir);
 //   serial_enc.RemoveLastDir();
    serial_enc.SetFullName(_T("SERIAL.ENC"));
    wxString t = serial_enc.GetFullPath();
    if( serial_enc.FileExists() ){
        wxTextFile tf( serial_enc.GetFullPath() );
        tf.Open();
        if( !tf.Eof() ){
            wxString str = tf.GetFirstLine();
            data_server_string = str.Mid(0, 2);
        }
    }
    
    
    // Read and parse the CATALOG.031
    wxString cat_file = enc_root_dir + wxFileName::GetPathSeparator() + _T("CATALOG.031");
    m_catalog = CreateCatalog31(cat_file);

    //  Make a list of all the unique cell names appearing in the exchange set
    wxArrayString unique_cellname_array;
    for(unsigned int i=0 ; i < m_catalog->Count() ; i++){
        wxString file = m_catalog->Item(i).m_filename;
        wxFileName fn( file );
        wxString ext = fn.GetExt();
        
        long tmp;
        //  Files of interest have numeric extension, except for CATALOG.031
        //  They also must have a digit as 3rd character, to distinguish from ENC Signature files (Spec 5.3.2)
        if( ext.ToLong( &tmp ) && (fn.GetName() != _T("CATALOG")) ){
            wxString num_test = fn.GetName().Mid(2,1);
            long tnum;
            if(num_test.ToLong(&tnum)){
                wxString tent_cell_file = file;
                wxCharBuffer buffer=tent_cell_file.ToUTF8();             // Check file namme for convertability
            
                if( buffer.data() ) {
                
                //      Strip the extension, and    
                //      Add to array iff not already there
                    wxFileName fna(file);
                    fna.SetExt(_T(""));
                    bool bfound = false;
                    for(unsigned int j=0 ; j < unique_cellname_array.Count() ; j++) {
                        if(fna.GetName() == unique_cellname_array[j]){
                            bfound = true;
                            break;
                        }
                    }
                
                    if(!bfound)
                        unique_cellname_array.Add(fna.GetName());
                }
            }
        }
    }

    
    //  Walk the unique cell list, and
    //  search high and low for a .os63 file that matches
    
    wxString os63_dirname = GetPermitDir();

    //  Get a list of all the os63 files in the directory corresponding to the Data Server identified above
    os63_dirname += wxFileName::GetPathSeparator();
    os63_dirname += data_server_string;
    wxArrayString os63_file_array;
    wxDir::GetAllFiles(os63_dirname, &os63_file_array, _T("*.os63"));

    if( 0 == os63_file_array.GetCount() ){
        wxString msg = _("Security Scheme Error\n\n SSE 10 - Permits not available for this data provider.\n");
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              msg,
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi: ") + msg);
        
        return 0;
    }
    
    if(bSENC){
        g_pprog = new wxProgressDialog(_T("s63_pi"), _("Creating eSenc"), unique_cellname_array.Count(),
                                       g_pScreenLog,  wxPD_CAN_ABORT | wxPD_AUTO_HIDE );
    }
    
    unsigned int nproc = 0;
    
    for(unsigned int i=0 ; i < unique_cellname_array.Count() ; i++){

        if(bSENC){
            if(g_pprog){
                g_pprog->Raise();
                wxString msg;
                msg.Printf(_T("Building eSENC %d/%d\n"), nproc+1, unique_cellname_array.Count()); 
                g_pprog->Update(nproc, msg);
                if( nproc != unique_cellname_array.Count() ){    // not done yet
                                wxSleep(4);
                                
                                if( !g_pprog->Update(nproc, _T("")) ){
                                    g_pprog->Update(unique_cellname_array.Count(), _T(""));
                                    ScreenLogMessage(_T("eSENC building cancelled\n"));
                                    bSENC = false;
                                    g_pprog = 0;
                                }
                }
            }
        }
        
        for(unsigned int j=0 ; j < os63_file_array.Count() ; j++){
            wxFileName fn1(unique_cellname_array[i]);
            wxFileName fn2(os63_file_array[j]);
            
            if(fn1.GetName() == fn2.GetName()){         // found the matching os63 file
                
                wxString base_file_name;
                wxString base_comt;
                wxString cell_name = fn1.GetName(); 
                wxString os63_filename = fn2.GetFullPath();
                //  Examine the Catalog.031 again
                //  Find the base cell, if present, and build an array of relevent updates
                
                wxDateTime date000;
                long edtn;
                
                wxArrayString cell_array;
                for(size_t k=0 ; k < m_catalog->GetCount() ; k++){
                    
                    wxString file = m_catalog->Item(k).m_filename;
                    wxFileName fn( file );
                    wxString ext = fn.GetExt();
                    
                    long tmp;
                    //  Files of interest have the same base name as the target .000 cell,
                    //  and have numeric extension
                    if( ext.ToLong( &tmp ) && ( fn.GetName() == cell_name ) ) {
                            
                         wxString comt = m_catalog->Item(k).m_comt;
                            
                            //      Check updates for applicability
                        if(0 == tmp) {    // the base .000 cell
                            base_file_name = file;
                            base_comt = comt;
                            wxStringTokenizer tkz(comt, _T(","));
                            while ( tkz.HasMoreTokens() ){
                                wxString token = tkz.GetNextToken();
                                wxString rest;
                                if(token.StartsWith(_T("EDTN="), &rest))
                                    rest.ToLong(&edtn);
                                else if(token.StartsWith(_T("UADT="), &rest)){       
                                    date000.ParseFormat( rest, _T("%Y%m%d") );
                                    if( !date000.IsValid() )
                                        date000.ParseFormat( _T("20000101"), _T("%Y%m%d") );
                                    date000.ResetTime();
                                }
                            }
                        }
                        else {
                            cell_array.Add( file );             // Must be checked for validity later
                        }
                    }
                }
            
            //      Sort the candidates
                cell_array.Sort( ExtensionCompare );
            
            //      Walk the sorted array of updates, appending the CATALOG m_comt field to the file name.
            
                for(unsigned int i=0 ; i < cell_array.Count() ; i++) {
                    for(unsigned int j=0 ; j < m_catalog->Count() ; j++){
                        if(m_catalog->Item(j).m_filename == cell_array[i]){
                            cell_array[i] += _T(";") + m_catalog->Item(j).m_comt;
                            break;
                        }
                    }
                }

             // Now authenticate the base cell file, if it is actually in the Exchange Set
                if(base_file_name.Len()){
                    int base_auth = AuthenticateCell( enc_root_dir + wxFileName::GetPathSeparator() + base_file_name );
                    if(base_auth != 0){        // failed to authenticate
                        b_error = true;
                        continue;               // so do nothing
                    }
                }
                
                 
            //  Update the os63 file
                wxTextFile os63file( os63_filename );
                wxString line;
                wxString str;
                
                os63file.Open();
            
                //      Read the file, to see  if there is already a cellbase entry
                bool base_present = false;
                for ( str = os63file.GetLastLine(); os63file.GetCurrentLine() > 0; str = os63file.GetPrevLine() ) {
                    if(str.StartsWith(_T("cellbase:"))){
                        base_present = true;
                        break;
                    }
                }
 
                long base_installed_edtn = -1;
                wxDateTime base_installed_UADT;
                
                //      Branch on results
                if(base_present){               // This could be an update or replace
                    // Check the EDTN of the currently installed base cell
                    wxString base_comt = str.AfterFirst(';');
                    wxStringTokenizer tkz(base_comt, _T(","));
                    while ( tkz.HasMoreTokens() ){
                        wxString token = tkz.GetNextToken();
                        wxString rest;
                        if(token.StartsWith(_T("EDTN="), &rest)){
                            rest.ToLong(&base_installed_edtn);
                        }
                        else if(token.StartsWith(_T("ISDT="), &rest)){       
                            base_installed_UADT.ParseFormat( rest, _T("%Y%m%d") );
                            if( !base_installed_UADT.IsValid() )
                                base_installed_UADT.ParseFormat( _T("20000101"), _T("%Y%m%d") );
                            base_installed_UADT.ResetTime();
                        }
                        
                    }
                    
                    //  If the exchange set contains a base cell, then...
                    if(base_file_name.Len()){
                    //  If the currently installed base EDTN is the same as the base being imported, then do nothing
                        if(base_installed_edtn > 0){
                            if(base_installed_edtn == edtn){
                                edtn = base_installed_edtn;
                                date000 = base_installed_UADT;
                            }
                            else {
//                                int yyp = 5;                    // TODO a new base cell edition is coming in
                            }
                        }
                    }
                    else {                        // Exchange set does not contain base, so must be update
                        edtn = base_installed_edtn;
                        date000 = base_installed_UADT;
                    }
                    
                    
                }
                else {                          // this must be an initial import
                    line = _T("cellbase:");
                    line += enc_root_dir + wxFileName::GetPathSeparator();
                    line += base_file_name;
                    line += _T(";");
                    line += base_comt;
                    os63file.AddLine(line);

                }

                
                
                //  Check the updates array for validity against edtn amd date000
                for(unsigned int i=0 ; i < cell_array.Count() ; i++){
                    wxString up_comt = cell_array[i].AfterFirst(';');
                    long update_edtn;
                    long update_updn;
                    wxDateTime update_time;
                    wxStringTokenizer tkz(up_comt, _T(","));
                    while ( tkz.HasMoreTokens() ){
                        wxString token = tkz.GetNextToken();
                        wxString rest;
                        if(token.StartsWith(_T("EDTN="), &rest))
                            rest.ToLong(&update_edtn);
                        else if(token.StartsWith(_T("ISDT="), &rest)){       
                            update_time.ParseFormat( rest, _T("%Y%m%d") );
                            if( !update_time.IsValid() )
                                update_time.ParseFormat( _T("20000101"), _T("%Y%m%d") );
                            update_time.ResetTime();
                        }
                        else if(token.StartsWith(_T("UPDN="), &rest)){
                            rest.ToLong(&update_updn);
                        }
                            
                    }
                    
                
                    if(update_time.IsValid() && date000.IsValid() && 
                        ( !update_time.IsEarlierThan( date000 ) ) && ( update_edtn == edtn ) )  {
                        
                        int installed_updn = 0;
                    
                        //      Check the entire file to see if this update is already in place
                        //      If so, do not add it again.
                        bool b_exists = false;
                        for ( str = os63file.GetLastLine(); os63file.GetCurrentLine() > 0; str = os63file.GetPrevLine() ) {
                            if(str.StartsWith(_T("cellupdate:"))){
                                wxString ck_comt = str.AfterFirst(';');
                                wxStringTokenizer tkz(ck_comt, _T(","));
                                while ( tkz.HasMoreTokens() ){
                                    wxString token = tkz.GetNextToken();
                                    wxString rest;
                                    if(token.StartsWith(_T("UPDN="), &rest)){
                                        long ck_updn = -1;
                                        rest.ToLong(&ck_updn);
                                        
                                        if(ck_updn == update_updn)
                                            b_exists = true;
                                        
                                        installed_updn = wxMax(installed_updn, ck_updn);        // capture latest update
                                        
                                        break;          // done with this entry
                                    }
                                }
                            }
                            if(b_exists)
                                break;                  // already have this update, so do nothing
                        }
                        
                        //      Check update for valid sequence
                        //      That is, update_updn == installed_updn + 1
                        if(!b_exists){
                            if(update_updn != installed_updn + 1){
                                wxString msg = _("Security Scheme Error\n\n SSE 23 - Non sequential update, previous update(s) missing.\nTry reloading from the base media. \n If the problem persists contact your data supplier.\n\n");
                                wxString m1;
                                m1.Printf(_T("cell:"));
                                msg += m1;
                                msg += fn2.GetFullPath();
                                msg += _T("\n");
                                m1.Printf(_T("Latest intalled update: %d\n"), installed_updn);
                                msg += m1;
                                m1.Printf(_T("Attempted update: %d\n"), update_updn);
                                msg += m1;
                                OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                                                      msg,
                                                      _T("s63_pi Message"),  wxOK, -1, -1);
                                
                                ScreenLogMessage( msg );
                                wxLogMessage(_T("s63_pi: ") + msg);
                                
                                b_error = true;
                                break;                  // so stop the for() loop here
                                                        // and be done with the update array
                            }
                            
                        }
                            
                        if(!b_exists){
                            
                            //  Authenticate the update cells, one by one.
                            int base_auth = AuthenticateCell( enc_root_dir + wxFileName::GetPathSeparator() + cell_array[i].BeforeFirst(';') );
                            if(base_auth != 0){        // failed to authenticate

                                ScreenLogMessage(_("Cell update NOT authenticated\n") );
                                ScreenLogMessage(_("This update and all subsequent updates to this cell are NOT applied.\n\n") );
                                b_error = true;
                                break;                  // so stop the for() loop here
                                                        // and be done with the update array
                            }
                            else {
                                line = _T("cellupdate:");
                                line += enc_root_dir + wxFileName::GetPathSeparator();
                                line += cell_array[i];
                                os63file.AddLine(line);
                            }
                        }
                    }
                }
                
            
                os63file.Write();
                os63file.Close();
    
                nproc++;
               
                //  Add the chart(cell) to the OCPN database
                ScreenLogMessage(_T("Adding cell to database: ") + os63_filename + _T("\n"));
                
                int rv_add = AddChartToDBInPlace( os63_filename, false );
                if(!rv_add) {
                    ScreenLogMessage(_T("   Error adding cell to database: ") + os63_filename + _T("\n\n"));
                    b_error = true;
                    
//                    wxRemoveFile( os63_filename );
//                    rv = rv_add;
//                    return rv;
                }
                else {
                    ScreenLogMessage(_T("Cell added successfully\n\n") );
                    
                    //  Build the eSENC inline, if requested
                    if(bSENC){
                        wxString msg;
                        msg.Printf(_T("Building eSENC %d/%d\n"), nproc, unique_cellname_array.Count()); 
                        ScreenLogMessage( msg );
                        ChartS63 *pch = new ChartS63();
                        if(pch){
                            if( PI_INIT_OK != pch->Init( os63_filename, PI_FULL_INIT) ){
                                b_error = true;
                                ScreenLogMessage( _T("eSENC build ERROR\n") );
                            }
                            else
                                ScreenLogMessage( _T("eSENC built successfully\n") );
                            
                            delete pch;
                        }
                        
                        if(0/*g_pprog*/){
                            g_pprog->Raise();
                            wxString msg;
                            msg.Printf(_T("Building eSENC %d/%d\n"), nproc, unique_cellname_array.Count()); 
                            g_pprog->Update(nproc, msg);
                            if( nproc != unique_cellname_array.Count() ){    // not done yet
                                wxSleep(4);
                            
                                if( !g_pprog->Update(nproc, _T("")) ){
                                    g_pprog->Update(unique_cellname_array.Count(), _T(""));
                                    ScreenLogMessage(_T("eSENC building cancelled\n"));
                                    bSENC = false;
                                    g_pprog = 0;
                                }
                            }
                        }
                    }
                    
                }
                    
                
                break;
                
            }
        }
    }
        

    if(g_pprog){
        g_pprog->Update(10000, _T(""));
        
        g_pprog->Hide();
        g_pprog->Close();
        g_pprog->Destroy();
    }
    
    if(!b_error)    
        ScreenLogMessage(_T("Finished Cell Update\n"));
    else
        ScreenLogMessage(_T("Finished Cell Update,  ERRORS encountered\n"));
    
        
    return 0;
}

int s63_pi::ImportCert(void)
{
    
    //  Get the Certificate (actually the .PUB key file) file from a dialog
    wxString key_file_name;
    wxFileDialog *openDialog = new wxFileDialog( NULL, _("Select Public Key File"),
                                                 m_SelectPermit_dir, wxT(""),
                                                 _("PUB files (*.PUB)|*.PUB|txt files (*.txt)|*.txt|All files (*.*)|*.*"), wxFD_OPEN );
    int response = openDialog->ShowModal();
    if( response == wxID_OK )
        key_file_name = openDialog->GetPath();
    else
        return 0;                       // cancelled
        
    wxFileName fn(key_file_name);

    ScreenLogMessage(_T("Checking SA Digital Certificate format\n") );
    bool bfs = check_enc_signature_format( fn.GetFullPath() );
    if(bfs){
        ScreenLogMessage(_T("SA Digital Certificate format OK\n") );
    }
    else {
        wxString msg = _("Security Scheme Error\n\nSSE 08 - SA Digital Certificate file incorrect format.\n");
        msg += _("A valid certificate can be obtained from the IHO website or your data supplier.\n");
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              msg,
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi: ") + msg);
        
        ScreenLogMessage(_T("SA Digital Certificate file incorrect format.\n\n") );
        
        return 1;
    }
    
    
    //  Make sure the directories exist...
    wxString dir = GetCertificateDir();
    
    if( !wxFileName::DirExists( dir ) ){
        wxFileName::Mkdir(dir, 0777, wxPATH_MKDIR_FULL);
    }
    
    wxString msg;
    if(::wxCopyFile(key_file_name, GetCertificateDir() + wxFileName::GetPathSeparator() + fn.GetFullName()) )
        msg = _("Certificate Key imported successfully\n");
    else
        msg = _("Certificate Key import FAILED\n");
    

    OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(), msg,
                             _T("s63_pi Message"),  wxOK, -1, -1);
    
    m_cert_list->BuildList( GetCertificateDir() );
    
    return 0;
                             
}

int s63_pi::ImportCellPermits(void)
{

    //  Get the PERMIT.TXT file from a dialog
    wxString permit_file_name;
    wxFileDialog *openDialog = new wxFileDialog( NULL, _("Select PERMIT.TXT File"),
                                                 m_SelectPermit_dir, wxT(""),
                                    _("TXT files (*.TXT)|*.TXT|All files (*.*)|*.*"), wxFD_OPEN );
    int response = openDialog->ShowModal();
    if( response == wxID_OK )
        permit_file_name = openDialog->GetPath();
    
    wxFileName fn(permit_file_name);
    m_SelectPermit_dir = fn.GetPath();          // save for later
    SaveConfig();
    
 

    //  Open PERMIT.TXT as text file

    //  Validate file format

    //  In a loop, process the individual cell permits
    int n_permits = 0;
    if(permit_file_name.Len()){
        wxTextFile permit_file( permit_file_name );
        if( permit_file.Open() ){
            wxString line = permit_file.GetFirstLine();

            while( !permit_file.Eof() ){
                if(line.StartsWith( _T(":ENC" ) ) ) {
                    wxString cell_line = permit_file.GetNextLine();
                    while(!permit_file.Eof() && !cell_line.StartsWith( _T(":") ) ){

                        //      Process a single cell permit
                        ProcessCellPermit( cell_line );
                        n_permits++;

                        cell_line = permit_file.GetNextLine();
                    }

                    if( !permit_file.Eof() )
                        line = cell_line;
                    else
                        line = _T("");
                }
                else
                    line = permit_file.GetNextLine();

            }
        }
    }

    if( !n_permits){
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              _T("Security Scheme Error\n\nSSE 11 - Cell permit not found"),
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi:  SSE 11 – Cell permit not found" ));
    }
    
    
    //  Set status
    
    if(m_permit_list){
        wxString permit_dir = GetPermitDir();
        
        m_permit_list->BuildList( permit_dir );
    }
    
    return 0;
}



int s63_pi::ProcessCellPermit( wxString &permit )
{
    int rv = 0;

    //  Parse the cell permit file entry
    wxStringTokenizer tkz(permit, _T(","));
    wxString cellpermitstring = tkz.GetNextToken();
    wxString service_level_indicator = tkz.GetNextToken();
    wxString edition_number = tkz.GetNextToken();
    wxString data_server_ID = tkz.GetNextToken();
    wxString comment = tkz.GetNextToken();

    //  A simple length test for poorly formatted cell permits
    if( cellpermitstring.Length() != 64) {
        wxString msg = _T("Security Scheme Error\n\nSSE 12 – Cell permit format is incorrect\n\nIncorrect cell permit line starts with ");
        msg += cellpermitstring.Mid(0, 16);
        msg += _T("...");
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              msg,
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi: ") + msg);
        
        return 1;
    }
 
    //  Go to the SENC utility to validate the encrypted cell permit checksum
    wxString cmd = g_sencutil_bin;
    cmd += _T(" -d ");                  // validate cell permit
    
    cmd += _T(" -p ");
    cmd += cellpermitstring;
    
    cmd += _T(" -u ");
    cmd += GetUserpermit();
    
    cmd += _T(" -e ");
    cmd += GetInstallpermit();
    
    wxLogMessage( cmd );
    wxArrayString valup_result = exec_SENCutil_sync( cmd, false);
    
    for(unsigned int i=0 ; i < valup_result.GetCount() ; i++){
        wxString line = valup_result[i];
        if(line.Upper().Find(_T("ERROR")) != wxNOT_FOUND){
            wxString msg = _("Security Scheme Error\n\nSSE 13 - Cell Permit is invalid (checksum is incorrect)\n");
            msg += _("or the Cell Permit is for a different system.\n\n");
            msg += _("Invalid cell permit starts with ");
            msg += cellpermitstring.Mid(0, 24);
            msg += _T("...");
            OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                                  msg,
                                  _T("s63_pi Message"),  wxOK, -1, -1);
                                  
            wxLogMessage(_T("s63_pi: ") + msg);
                                  
            return 1;
                                  
        }
    }
    
    
    
    wxString cell_name = cellpermitstring.Mid(0, 8);
    wxString expiry_date = cellpermitstring.Mid(8, 8);
    wxString eck1 = cellpermitstring.Mid(16, 16);
    wxString eck2 = cellpermitstring.Mid(32, 16);
    wxString permit_checksum = cellpermitstring.Mid(48, 16);

    wxString base_file_name;
    wxString base_comt;
    

    // 10.5.4          Check Cell Permit Check Sum
    // 10.5.5          Check Cell Permit Expiry Date
    // 10.5.6          Check Data Server ID

#if 0    
    //Examine the Catalog.031 as previously parsed
    //  Find the base cell, if present, and build an array of relevent updates
    
    wxDateTime date000;
    long edtn;
    
    wxArrayString cell_array;
    bool b_found_cell = false;
    for(size_t i=0 ; i < m_catalog->GetCount() ; i++){

        wxString file = m_catalog->Item(i).m_filename;
        wxFileName fn( file );
        wxString ext = fn.GetExt();

        long tmp;
        //  Files of interest have the same base name is the target .000 cell,
        //  and have numeric extension
        if( ext.ToLong( &tmp ) && ( fn.GetName() == cell_name ) ) {
            wxString tent_cell_file = file;
            wxCharBuffer buffer=tent_cell_file.ToUTF8();             // Check file namme for convertability

            if( buffer.data() ) {   
                b_found_cell = true;

                wxString comt = m_catalog->Item(i).m_comt;
                
                //      Check updates for applicability
                if(0 == tmp) {    // the base .000 cell
                    base_file_name = file;
                    base_comt = comt;
                    wxStringTokenizer tkz(comt, _T(","));
                    while ( tkz.HasMoreTokens() ){
                        wxString token = tkz.GetNextToken();
                        wxString rest;
                        if(token.StartsWith(_T("EDTN="), &rest))
                            rest.ToLong(&edtn);
                        else if(token.StartsWith(_T("UADT="), &rest)){       
                            date000.ParseFormat( rest, _T("%Y%m%d") );
                            if( !date000.IsValid() )
                                date000.ParseFormat( _T("20000101"), _T("%Y%m%d") );
                        date000.ResetTime();
                        }
                    }
                }
                else {
                    if(comt.Len()){
                        long update_edtn;
                        wxDateTime update_time;
                        wxStringTokenizer tkz(comt, _T(","));
                        while ( tkz.HasMoreTokens() ){
                            wxString token = tkz.GetNextToken();
                            wxString rest;
                            if(token.StartsWith(_T("EDTN="), &rest))
                                rest.ToLong(&update_edtn);
                            else if(token.StartsWith(_T("ISDT="), &rest)){       
                                update_time.ParseFormat( rest, _T("%Y%m%d") );
                                if( !update_time.IsValid() )
                                    update_time.ParseFormat( _T("20000101"), _T("%Y%m%d") );
                                update_time.ResetTime();
                            }
                        }
                        
                        if(update_time.IsValid() && date000.IsValid()){
                            if( ( !update_time.IsEarlierThan( date000 ) ) && ( update_edtn == edtn ) )  
                                cell_array.Add( file );                    
                        }
                    }
                }
            }
        }
    }

    //      Sort the candidates
    cell_array.Sort( ExtensionCompare );
    
    //      Walk the sorted array, appending the CATALOG m_comt field to the file name.
    
    for(unsigned int i=0 ; i < cell_array.Count() ; i++) {
        for(unsigned int j=0 ; j < m_catalog->Count() ; j++){
            if(m_catalog->Item(j).m_filename == cell_array[i]){
                cell_array[i] += _T(";") + m_catalog->Item(j).m_comt;
                break;
            }
        }
    }
    
                
    
    if( !b_found_cell ) {
        ScreenLogMessage( _T("   Error: Cannot find ENC cell base or update in specified exchange set...")
                + cell_name + _T("\n"));
        return -1;
    }

    if( !base_file_name.Len() ) {
        ScreenLogMessage( _T("   Error: Cannot find ENC cell base in specified exchange set...")
        + cell_name + _T("\n"));
        return -1;
    }
#endif    
    //  Create the text file 

    wxString os63_filename = GetPermitDir();
    os63_filename += wxFileName::GetPathSeparator();
    os63_filename += data_server_ID;
    os63_filename += wxFileName::GetPathSeparator();
    os63_filename += cell_name;
    os63_filename += _T(".os63");

    //TODO  Check if file exists...What then?  a dialog asking to replace?

    if( wxFileName::FileExists( os63_filename ) )
        wxRemoveFile( os63_filename );

    //  Create the target dir if necessary
    wxFileName tfn( os63_filename );
    if( true != tfn.DirExists( tfn.GetPath() ) ) {
        if( !wxFileName::Mkdir( tfn.GetPath(), 0777, wxPATH_MKDIR_FULL ) ) {
            wxString msg = _T("   Error: Cannot create directory ");
            msg += tfn.GetPath();
            msg += _T("\n");
            ScreenLogMessage( msg );
            return -1;
        }
    }

    wxTextFile os63file( os63_filename );
    if( !os63file.Create() ){
        wxString msg;
        msg = _("   Error: Cannot create ");
        msg += os63_filename;
        msg += _T("\n");
        ScreenLogMessage( msg );
        return -1;
    }

//      Populate the base os63 file....
    wxString line;

    line = _T("cellpermit:");
    line += permit;
    os63file.AddLine(line);
#if 0
    line = _T("cellbase:");
    line += enc_root_dir + wxFileName::GetPathSeparator();
    line += base_file_name;
    line += _T(";");
    line += base_comt;
    os63file.AddLine(line);

    for(unsigned int i=0 ; i < cell_array.Count() ; i++){
        line = _T("cellupdate:");
        line += enc_root_dir + wxFileName::GetPathSeparator();
        line += cell_array[i];
        os63file.AddLine(line);
    }
#endif        
    os63file.Write();
    os63file.Close();

#if 0    
    //  Add the chart(cell) to the OCPN database
    ScreenLogMessage(_T("Adding cell to database: ") + os63_filename + _T("\n"));
    int rv_add = 1;//AddChartToDBInPlace( os63_filename, false );
    if(!rv_add) {
        ScreenLogMessage(_T("   Error adding cell to database: ") + os63_filename + _T("\n"));
        wxRemoveFile( os63_filename );
        rv = rv_add;
        return rv;
    }
#endif
    return rv;
}


int s63_pi::RemoveCellPermit( void )
{
    //  Which permits?
    if(m_permit_list){
        
        wxArrayString permits;
        
        long itemIndex = -1;
        for ( ;; )
        {
            itemIndex = m_permit_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
            if ( itemIndex == -1 )
                break;
        
            int index = m_permit_list->GetItemData( itemIndex );
            wxString permit_file = m_permit_list->m_permit_file_array[index];
            
            permits.Add(permit_file);
        }
        
        for(unsigned int i=0 ; i < permits.GetCount() ; i++){
        
            RemoveChartFromDBInPlace( permits[i] );
        
        //      Kill the permit file
            ::wxRemoveFile( permits[i] );

        //      Rebuild the permit list
            wxString permit_dir = GetPermitDir();
            
            m_permit_list->BuildList( permit_dir );
        }
    }
    
    return 0;
}

wxString s63_pi::GetCertificateDir()
{
    wxString dir = *GetpPrivateApplicationDataLocation();
    dir += wxFileName::GetPathSeparator();
    dir += _T("s63");
    dir += wxFileName::GetPathSeparator();
    dir += _T("s63_certificates");
    
    return dir;
}
    
    
int s63_pi::AuthenticateCell( const wxString & cell_file )
{
    ScreenLogMessage(_T("Authenticating ") + cell_file + _T("\n\n") );
    
    //  Locate the ENC signature file
    
    wxFileName fn_cell(cell_file);
    wxString cell_name = fn_cell.GetFullName();
    
    wxString sig_name = cell_name;
    wxChar np = sig_name[2];
    sig_name[2] = np + 0x18;
    
    wxString sig_file = fn_cell.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR ) + sig_name ;

    ScreenLogMessage(_T("Checking signature file format\n") );
    bool bf = check_enc_signature_format( sig_file );
    if(bf){
        ScreenLogMessage(_T("Signature file format OK\n") );
    }
    else {
        wxString msg = _("Security Scheme Error\n\nSSE 24 - ENC Signature format is incorrect.\n\n");
        msg += _T("Cell name: ");
        msg += cell_file;
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              msg,
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi: ") + msg);
        
        ScreenLogMessage(_T("Signature file format incorrect.\n\n") );
        
        return 24;
    }
        
        
    wxString key_file = GetCertificateDir() + wxFileName::GetPathSeparator() + _T("IHO.PUB");
        
    ScreenLogMessage(_T("Checking SA Digital Certificate format\n") );
    bool bfs = check_enc_signature_format( key_file );
    if(bfs){
        ScreenLogMessage(_T("SA Digital Certificate format OK\n") );
    }
    else {
        wxString msg = _("Security Scheme Error\n\nSSE 08 - SA Digital Certificate file incorrect format.\n");
        msg += _("A valid certificate can be obtained from the IHO website or your data supplier.\n");
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              msg,
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi: ") + msg);
        
        ScreenLogMessage(_T("SA Digital Certificate file incorrect format.\n\n") );
        
        return 8;
    }
        

    ScreenLogMessage(_T("Authenticating Signed Data Server Certificate\n") );
    
    //  Try to authenticate the signature file against the default IHO.PUB public key
   
    bool iho_result = validate_enc_signature( sig_file, key_file );

    bool b_auth = iho_result;
    
    //  If no joy, try to authenticate against any public key file found in the CertificateDir
    wxString key_found;
    if(!b_auth){
        wxArrayString key_array;
        unsigned int n_files = wxDir::GetAllFiles(GetCertificateDir(), &key_array, _T("*.PUB"));
        
        for(unsigned int i=0 ; i < n_files ; i++){
            if( validate_enc_signature( sig_file, key_array[i] ) ){
                b_auth = true;
                key_found = key_array[i];
                break;
            }
        }
    }

    if( !b_auth ){
        ScreenLogMessage(_T("Certificate Authentication FAILED\n\n") );
        
        wxString msg = _T("Security Scheme Error\n\nSSE 06 - The SA Signed Data Server Certificate is invalid.\n");
        msg += _T("The SA may have issued a new public key or the ENC may originate from another service.\n");
        msg += _T("A new SA public key can be obtained from the IHO website or from your data supplier.\n\n");
        msg += _T("Cell name: ");
        msg += cell_file;
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              msg,
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi: ") + msg);
        
        return 6;
    }
    else {
        if( !iho_result && !m_bSSE26_shown ) {
            wxString msg = _T("Security Scheme Warning\n\nSSE 26 – ENC is not authenticated by the IHO acting as the SA.\n\n");
            msg += _T("Cell name: ");
            msg += cell_file;
            OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                                  msg,
                                  _T("s63_pi Message"),  wxOK, -1, -1);
                                  
                                  wxLogMessage(_T("s63_pi: ") + msg);
            m_bSSE26_shown = true;                      
        }
        
        ScreenLogMessage(_T("Certificate Authentication Successful\n") );
    }
    
    ScreenLogMessage(_T("Authenticating ENC cell contents\n") );
    
    b_auth = validate_enc_cell( sig_file, cell_file );
    if(!b_auth){
        wxString msg = _T("Security Scheme Error\n\nSSE 09 – ENC Signature is invalid.\n");
        msg += _T("Cell name: ");
        msg += cell_file;
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(),
                              msg,
                              _T("s63_pi Message"),  wxOK, -1, -1);
        
        wxLogMessage(_T("s63_pi: ") + msg);
        
        ScreenLogMessage(_T("ENC Authentication FAILED\n\n") );
        
        return 9;
    }
    else{
        ScreenLogMessage(_T("ENC Authentication Successful\n\n") );
    }
    
    return 0;
}



int s63_pi::pi_error( wxString msg )
{
    return 0;
}

bool s63_pi::LoadConfig( void )
{
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;
    
    if( pConf ) {
        pConf->SetPath( _T("/PlugIns/S63") );
        
        //      Defaults
        g_installpermit = _T("Y");    
        g_userpermit = _T("X");    
        
        pConf->Read( _T("PermitDir"), &m_SelectPermit_dir );
        pConf->Read( _T("Userpermit"), &g_userpermit );
        pConf->Read( _T("Installpermit"), &g_installpermit );
        pConf->Read( _T("LastENCROOT"), &m_last_enc_root_dir);
    }        
     
    return true;
}

bool s63_pi::SaveConfig( void )
{
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;
    
    if( pConf ) {
        pConf->SetPath( _T("/PlugIns/S63") );
        
        pConf->Write( _T("PermitDir"), m_SelectPermit_dir );
        pConf->Write( _T("Userpermit"), g_userpermit );
        pConf->Write( _T("Installpermit"), g_installpermit );
        pConf->Write( _T("LastENCROOT"), m_last_enc_root_dir );
        
    }

    return true;
}

void s63_pi::GetNewUserpermit(void)
{
    g_old_userpermit = g_userpermit;
    
    g_userpermit = _T("");
    wxString new_permit = GetUserpermit();
    
    if( new_permit != _T("Invalid")){
        g_userpermit = new_permit;
        g_pi->SaveConfig();
        
        if(m_up_text) {
            m_up_text->SetLabel( g_userpermit );
        }
    }
    else
        g_userpermit = g_old_userpermit;
        
}


void s63_pi::GetNewInstallpermit(void)
{
    g_old_installpermit = g_installpermit;
    
    g_installpermit = _T("");
    wxString new_permit = GetInstallpermit();
    
    if( new_permit != _T("Invalid")){
        g_installpermit = new_permit;
        g_pi->SaveConfig();
        
        if(m_ip_text) {
            m_ip_text->SetLabel( g_installpermit );
        }
    }
    else
        g_installpermit = g_old_installpermit;
    
}



// An Event handler class to catch events from S63 UI dialog
//      Implementation

s63_pi_event_handler::s63_pi_event_handler(s63_pi *parent)
{
    m_parent = parent;
}

s63_pi_event_handler::~s63_pi_event_handler()
{
}


void s63_pi_event_handler::OnImportPermitClick( wxCommandEvent &event )
{
    m_parent->ImportCellPermits();
}

void s63_pi_event_handler::OnRemovePermitClick( wxCommandEvent &event )
{
    m_parent->RemoveCellPermit();
}

void s63_pi_event_handler::OnImportCellsClick( wxCommandEvent &event )
{
    SendPluginMessage(_T("S63_CALLBACK_PRIVATE_1"), wxEmptyString);
    
//    m_parent->ImportCells();
}


void s63_pi_event_handler::OnSelectPermit( wxListEvent& event )
{
    m_parent->EnablePermitRemoveButton(true);
}

void s63_pi_event_handler::OnNewUserpermitClick( wxCommandEvent& event )
{
    m_parent->GetNewUserpermit();
}

void s63_pi_event_handler::OnNewInstallpermitClick( wxCommandEvent& event )
{
    m_parent->GetNewInstallpermit();
}

void s63_pi_event_handler::OnImportCertClick( wxCommandEvent &event )
{
    m_parent->ImportCert();
}

//      Private logging functions
void ScreenLogMessage(wxString s)
{
    if(!g_pScreenLog && !g_pPanelScreenLog){
        g_pScreenLog = new S63ScreenLogContainer( GetOCPNCanvasWindow() );
        g_pScreenLog->Centre();
        
    }
    
    if( g_pScreenLog ) {
        g_pScreenLog->LogMessage(s);
    }
    else if( g_pPanelScreenLog ){
        g_pPanelScreenLog->LogMessage(s);
    }
}
void HideScreenLog(void)
{
    if( g_pScreenLog ) {
        g_pScreenLog->Hide();
    }
    else if( g_pPanelScreenLog ) {
        g_pPanelScreenLog->Hide();
    }
    
}

void ClearScreenLog(void)
{
    if( g_pScreenLog ) {
        g_pScreenLog->ClearLog();
    }
    else if( g_pPanelScreenLog ) {
        g_pPanelScreenLog->ClearLog();
    }
    
}





//      On Screen log container

S63ScreenLogContainer::S63ScreenLogContainer( wxWindow *parent )
{
    Create( parent, -1, _T("S63_pi Log"), wxDefaultPosition, wxSize(500,400) );
    m_slog = new S63ScreenLog( this );
    
    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );
    SetSizer( itemBoxSizer2 );
 
    itemBoxSizer2->Add( m_slog, 1, wxEXPAND, 5 );
    
    Hide();
}

S63ScreenLogContainer::~S63ScreenLogContainer()
{
    if( m_slog  ) 
        m_slog->Destroy();
}

void S63ScreenLogContainer::LogMessage(wxString &s)
{
    if( m_slog  ) {
        m_slog->LogMessage( s );
        Show();
    }
}

void S63ScreenLogContainer::ClearLog(void)
{
    if( m_slog  ) {
        m_slog->ClearLog();
    }
}



#define SERVER_ID       5000
#define SOCKET_ID       5001

BEGIN_EVENT_TABLE(S63ScreenLog, wxWindow)
EVT_SIZE(S63ScreenLog::OnSize)
EVT_SOCKET(SERVER_ID,  S63ScreenLog::OnServerEvent)
EVT_SOCKET(SOCKET_ID,  S63ScreenLog::OnSocketEvent)
END_EVENT_TABLE()

S63ScreenLog::S63ScreenLog(wxWindow *parent):
    wxWindow( parent, -1, wxDefaultPosition, wxDefaultSize)    
{
    
//    Create(parent, -1, _T("S63_pi Log"), wxDefaultPosition, wxDefaultSize,
//                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER /*| wxDIALOG_NO_PARENT*/ );
           

    wxBoxSizer *LogSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( LogSizer );

    m_plogtc = new wxTextCtrl(this, -1, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    LogSizer->Add(m_plogtc, 1, wxEXPAND, 0);
    
    
    m_nseq = 0;
    
    
    // Create a server socket to catch "back channel" messages from SENC utility
    
    // Create the address - defaults to localhost:0 initially
    wxIPV4address addr;
    addr.Service(g_backchannel_port);
    addr.AnyAddress();
    
    // Create the socket
    m_server = new wxSocketServer(addr);
    
    // We use Ok() here to see if the server is really listening
    if (! m_server->Ok())
    {
        m_plogtc->AppendText(_("S63_pi backchannel could not listen at the specified port !\n"));
    }
    else
    {
        m_plogtc->AppendText(_("S63_pi backchannel server listening.\n\n"));
    }
    
    // Setup the event handler and subscribe to connection events
    m_server->SetEventHandler(*this, SERVER_ID);
    m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
    m_server->Notify(true);
    
    
}

S63ScreenLog::~S63ScreenLog()
{
    delete m_plogtc;
}

void S63ScreenLog::OnSize( wxSizeEvent& event)
{
    Layout();
}

void S63ScreenLog::LogMessage(wxString &s)
{
    if( m_plogtc  ) {
        wxString seq;
        seq.Printf(_T("%6d: "), m_nseq++);
        
        wxString sp = s;

        if(sp[0] == '\r'){
            int lp = m_plogtc->GetInsertionPoint();
            int nol = m_plogtc->GetNumberOfLines();
            int ll = m_plogtc->GetLineLength(nol-1);
            
            if(ll)
                m_plogtc->Remove(lp-ll, lp);
            m_plogtc->SetInsertionPoint(lp - ll );
            m_plogtc->WriteText(s.Mid(1));
        }
        else {
            m_plogtc->AppendText(seq);
            m_plogtc->AppendText(sp);
        }
        
        m_plogtc->SetInsertionPointEnd();
        Show();
    }
}

void S63ScreenLog::ClearLog(void)
{
//    if(m_plogtc){
//        m_plogtc->Clear();
//    }
}

void S63ScreenLog::OnServerEvent(wxSocketEvent& event)
{
    wxString s; // = _("OnServerEvent: ");
    wxSocketBase *sock;
    
    switch(event.GetSocketEvent())
    {
        case wxSOCKET_CONNECTION :
//            s.Append(_("wxSOCKET_CONNECTION\n"));
            break;
        default                  :
            s.Append(_("Unexpected event !\n"));
            break;
    }
    
    m_plogtc->AppendText(s);
    
    // Accept new connection if there is one in the pending
    // connections queue, else exit. We use Accept(false) for
    // non-blocking accept (although if we got here, there
    // should ALWAYS be a pending connection).
    
    sock = m_server->Accept(false);
    
    if (sock)
    {
//       m_plogtc->AppendText(_("New client connection accepted\n\n"));
    }
    else
    {
        m_plogtc->AppendText(_("Error: couldn't accept a new connection\n\n"));
        return;
    }
    
    sock->SetEventHandler(*this, SOCKET_ID);
    sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
    sock->Notify(true);
    sock->SetFlags(wxSOCKET_BLOCK);
    
    
}

void S63ScreenLog::OnSocketEvent(wxSocketEvent& event)
{
    wxString s; // = _("OnSocketEvent: ");
    wxSocketBase *sock = event.GetSocket();
    
    // First, print a message
    switch(event.GetSocketEvent())
    {
        case wxSOCKET_INPUT : 
//            s.Append(_("wxSOCKET_INPUT\n"));
            break;
        case wxSOCKET_LOST  :
//            s.Append(_("wxSOCKET_LOST\n"));
            break;
        default             :
            s.Append(_("Unexpected event !\n"));
            break;
    }
    
    m_plogtc->AppendText(s);
    
    // Now we process the event
    switch(event.GetSocketEvent())
    {
        case wxSOCKET_INPUT:
        {
            // We disable input events, so that the test doesn't trigger
            // wxSocketEvent again.
            sock->SetNotify(wxSOCKET_LOST_FLAG);
            
            char buf[160];
            
            sock->ReadMsg( buf, sizeof(buf) );
            size_t rlen = sock->LastCount();
            if(rlen < sizeof(buf))
                buf[rlen] = '\0';
            else
                buf[0] = '\0';
            
            if(rlen) {
                wxString msg(buf, wxConvUTF8);
                if(!g_bsuppress_log)
                    LogMessage(msg);
            }
            
            // Enable input events again.
            sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
            break;
        }
                case wxSOCKET_LOST:
                {
                    
                    // Destroy() should be used instead of delete wherever possible,
                    // due to the fact that wxSocket uses 'delayed events' (see the
                    // documentation for wxPostEvent) and we don't want an event to
                    // arrive to the event handler (the frame, here) after the socket
                    // has been deleted. Also, we might be doing some other thing with
                    // the socket at the same time; for example, we might be in the
                    // middle of a test or something. Destroy() takes care of all
                    // this for us.
                    
//                    m_plogtc->AppendText(_("Deleting socket.\n\n"));
                    sock->Destroy();
                    break;
                }
                default: ;
    }
    
}

//-------------------------------------------------------------------------------------
//
//      OCPNPermitList implementation
//      List control for management of cell permits
//
//-------------------------------------------------------------------------------------

OCPNPermitList::OCPNPermitList(wxWindow *parent)
{
    Create( parent, -1, wxDefaultPosition, wxSize(-1, 150), wxLC_REPORT | wxLC_HRULES );
    
}

OCPNPermitList::~OCPNPermitList()
{
}

void OCPNPermitList::BuildList( const wxString &permit_dir )
{
    
    DeleteAllItems();
    
    if( wxDir::Exists(permit_dir) ){
        
        m_permit_file_array.Clear();
        wxArrayString file_array;
        size_t nfiles = wxDir::GetAllFiles(permit_dir, &file_array, _T("*.os63"));
        
        for (size_t i = 0; i < nfiles; i++){
            wxTextFile file(file_array[i]);
            if(file.Open()){
                wxString line = file.GetFirstLine();
                
                while( !file.Eof() ){
                    if(line.StartsWith( _T("cellpermit" ) ) ) {
                        
                        //      Keep an array of file names, store index in item
                        //      May be useful for list managment, e.g.item deletion
                        int pfa_index = m_permit_file_array.Add( file_array[i] );
                        
                        wxString permit_string = line.Mid(11);
                        
                        wxListItem li;
                        li.SetId( i );
                        li.SetData( pfa_index );
                        li.SetText( _T("") );
                        
                        long itemIndex = InsertItem( li );
                        
                        SetItem(itemIndex, 0, permit_string.Mid(0,8));
 
                        wxString sdate = permit_string.Mid(8, 8);
                        wxDateTime exdate;
                        exdate.ParseFormat(sdate, _T("%Y%m%d"));
                        
                        wxString fdate = exdate.FormatDate();
                        
                        wxStringTokenizer tkz(line.AfterFirst(':'), _T(",") );
                        wxString token = tkz.GetNextToken();
                        token = tkz.GetNextToken();
                        token = tkz.GetNextToken();
                        token = tkz.GetNextToken();             // Data server ID
                        
                        //      Set Data Server ID string
                        SetItem(itemIndex, 1, token);
                        

                        //TODO why, on GTK, can I not set an item/column colour?
                        wxListItem lid;
                        lid.SetId( itemIndex );
                        lid.SetColumn(2);
//                        lid.SetTextColour(*wxRED );
                        lid.SetText(fdate);
                        SetItem(lid);
                        
//                       SetItemTextColour(itemIndex, *wxRED);
                        
                        break;
                        
                    }
                    else
                        line = file.GetNextLine();
                }
            }
        }
    }
                    
            
    
#ifdef __WXOSX__
    SetColumnWidth( 0, wxLIST_AUTOSIZE );
    SetColumnWidth( 1, wxLIST_AUTOSIZE );
    SetColumnWidth( 2, wxLIST_AUTOSIZE );
#else
    SetColumnWidth( 0, wxLIST_AUTOSIZE_USEHEADER );
    SetColumnWidth( 1, wxLIST_AUTOSIZE_USEHEADER );
    SetColumnWidth( 2, wxLIST_AUTOSIZE_USEHEADER );
#endif
    
    
}

//-------------------------------------------------------------------------------------
//
//      OCPNCertificate implementation
//      List control for management of certificates/public keys
//
//-------------------------------------------------------------------------------------

OCPNCertificateList::OCPNCertificateList(wxWindow *parent)
{
    Create( parent, -1, wxDefaultPosition, wxSize(-1, 150), wxLC_REPORT | wxLC_HRULES);
    
}

OCPNCertificateList::~OCPNCertificateList()
{
}

void OCPNCertificateList::BuildList( const wxString &cert_dir )
{
    
    DeleteAllItems();
    
    if( wxDir::Exists(cert_dir) ){
        
 //       m_permit_file_array.Clear();
        wxArrayString file_array;
        size_t nfiles = wxDir::GetAllFiles(cert_dir, &file_array, _T("*.PUB"));
        
        for (size_t i = 0; i < nfiles; i++){
            wxTextFile file(file_array[i]);
            if(file.Open()){
                wxString line = file.GetFirstLine();
                
                while( !file.Eof() ){
                    if(line.Upper().Find( _T("BIG" ) ) != wxNOT_FOUND ) {
                        
                        //      Keep an array of file names, store index in item
                        //      May be useful for list managment, e.g.item deletion
//                        int pfa_index = m_permit_file_array.Add( file_array[i] );
                        
//                        wxString permit_string = line.Mid(11);
                        
                        wxListItem li;
                        li.SetId( i );
//                        li.SetData( pfa_index );
                        li.SetText( _T("") );
                        
                        long itemIndex = InsertItem( li );
      
                        wxFileName fn( file_array[i] );
                        
                        SetItem(itemIndex, 0, fn.GetFullName());
  
#if 0                        
                        wxString sdate = permit_string.Mid(8, 8);
                        wxDateTime exdate;
                        exdate.ParseFormat(sdate, _T("%Y%m%d"));
                        
                        wxString fdate = exdate.FormatDate();
                        
                        wxStringTokenizer tkz(line.AfterFirst(':'), _T(",") );
                        wxString token = tkz.GetNextToken();
                        token = tkz.GetNextToken();
                        token = tkz.GetNextToken();
                        token = tkz.GetNextToken();             // Data server ID
                        
                        //      Set Data Server ID string
                        SetItem(itemIndex, 1, token);
                        
                        
                        //TODO why, on GTK, can I not set an item/column colour?
                        wxListItem lid;
                        lid.SetId( itemIndex );
                        lid.SetColumn(2);
                        //                        lid.SetTextColour(*wxRED );
                        lid.SetText(fdate);
                        SetItem(lid);
                        
                        //                       SetItemTextColour(itemIndex, *wxRED);
#endif                        
                        break;
                        
                    }
                    else
                        line = file.GetNextLine();
                }
            }
        }
    }
    
    
    
    #ifdef __WXOSX__
    SetColumnWidth( 0, wxLIST_AUTOSIZE );
//    SetColumnWidth( 1, wxLIST_AUTOSIZE );
//    SetColumnWidth( 2, wxLIST_AUTOSIZE );
    #else
    SetColumnWidth( 0, wxLIST_AUTOSIZE_USEHEADER );
//    SetColumnWidth( 1, wxLIST_AUTOSIZE_USEHEADER );
//    SetColumnWidth( 2, wxLIST_AUTOSIZE_USEHEADER );
    #endif
    
    
}






/*!
 * GetUserpermitDialog type definition
 */

IMPLEMENT_DYNAMIC_CLASS( GetUserpermitDialog, wxDialog )
/*!
 * GetUserpermitDialog event table definition
 */BEGIN_EVENT_TABLE( GetUserpermitDialog, wxDialog )
 
 ////@begin GetUserpermitDialog event table entries
 
 EVT_BUTTON( ID_GETUP_CANCEL, GetUserpermitDialog::OnCancelClick )
 EVT_BUTTON( ID_GETUP_OK, GetUserpermitDialog::OnOkClick )
 EVT_BUTTON( ID_GETUP_TEST, GetUserpermitDialog::OnTestClick )
 EVT_TEXT(ID_GETUP_UP, GetUserpermitDialog::OnUpdated)
 
 ////@end GetUserpermitDialog event table entries
 
 END_EVENT_TABLE()
 
 /*!
  * GetUserpermitDialog constructors
  */
 
 GetUserpermitDialog::GetUserpermitDialog()
 {
 }
 
 GetUserpermitDialog::GetUserpermitDialog( wxWindow* parent, wxWindowID id, const wxString& caption,
                                         const wxPoint& pos, const wxSize& size, long style )
 {
     
     long wstyle = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER;
     wxDialog::Create( parent, id, caption, pos, size, wstyle );
     
     CreateControls();
     GetSizer()->SetSizeHints( this );
     Centre();
     
 }
 
 GetUserpermitDialog::~GetUserpermitDialog()
 {
     delete m_PermitCtl;
 }
 
 /*!
  * GetUserpermitDialog creator
  */
 
 bool GetUserpermitDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                                  const wxPoint& pos, const wxSize& size, long style )
 {
     SetExtraStyle( GetExtraStyle() | wxWS_EX_BLOCK_EVENTS );
     long wstyle = style;
#ifdef __WXMAC__
     wstyle |= wxSTAY_ON_TOP;
#endif
     
     wxDialog::Create( parent, id, caption, pos, size, wstyle );
     
     CreateControls();
     GetSizer()->SetSizeHints( this );
     Centre();
     
     return TRUE;
 }
 
 /*!
  * Control creation for GetUserpermitDialog
  */
 
 void GetUserpermitDialog::CreateControls()
 {
     GetUserpermitDialog* itemDialog1 = this;
     
     wxBoxSizer* itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );
     itemDialog1->SetSizer( itemBoxSizer2 );
     
     wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox( itemDialog1, wxID_ANY,
                                                               _("Enter Userpermit") );
     
     wxStaticBoxSizer* itemStaticBoxSizer4 = new wxStaticBoxSizer( itemStaticBoxSizer4Static,
                                                                   wxVERTICAL );
     itemBoxSizer2->Add( itemStaticBoxSizer4, 0, wxEXPAND | wxALL, 5 );
     
     wxStaticText* itemStaticText5 = new wxStaticText( itemDialog1, wxID_STATIC, _T(""),
     wxDefaultPosition, wxDefaultSize, 0 );
     itemStaticBoxSizer4->Add( itemStaticText5, 0,
                               wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );
     
     m_PermitCtl = new wxTextCtrl( itemDialog1, ID_GETUP_UP, _T(""), wxDefaultPosition,
     wxSize( 180, -1 ), 0 );
     itemStaticBoxSizer4->Add( m_PermitCtl, 0,
                               wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5 );
 
     wxBoxSizer* itemBoxSizerTest = new wxBoxSizer( wxHORIZONTAL );
     itemBoxSizer2->Add( itemBoxSizerTest, 0, wxALIGN_LEFT | wxALL, 5 );
     
     m_testBtn = new wxButton(itemDialog1, ID_GETUP_TEST, _("Test Userpermit"));
     m_testBtn->Disable();
     itemBoxSizerTest->Add( m_testBtn, 0, wxALIGN_LEFT | wxALL, 5 );

     wxStaticBox* itemStaticBoxTestResults = new wxStaticBox( itemDialog1, wxID_ANY,
                                                                  _("Test Results"), wxDefaultPosition, wxSize(500, 40) );
     
     wxStaticBoxSizer* itemStaticBoxSizerTest = new wxStaticBoxSizer( itemStaticBoxTestResults,  wxVERTICAL );
     itemBoxSizerTest->Add( itemStaticBoxSizerTest, 0,  wxALIGN_RIGHT |wxALL, 5 );
     
     
     m_TestResult = new wxStaticText( itemDialog1, -1, _T(""), wxDefaultPosition, wxSize( 180, -1 ), 0 );
     
     itemStaticBoxSizerTest->Add( m_TestResult, 0, wxALIGN_LEFT | wxALL, 5 );
     
     
     
     wxBoxSizer* itemBoxSizer16 = new wxBoxSizer( wxHORIZONTAL );
     itemBoxSizer2->Add( itemBoxSizer16, 0, wxALIGN_RIGHT | wxALL, 5 );
     
     m_CancelButton = new wxButton( itemDialog1, ID_GETUP_CANCEL, _("Cancel"), wxDefaultPosition,
     wxDefaultSize, 0 );
     itemBoxSizer16->Add( m_CancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
     m_CancelButton->SetDefault();
     
     m_OKButton = new wxButton( itemDialog1, ID_GETUP_OK, _("OK"), wxDefaultPosition,
     wxDefaultSize, 0 );
     itemBoxSizer16->Add( m_OKButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
     
     m_OKButton->Disable();
     
     m_PermitCtl->AppendText(g_old_userpermit);
     
 }
 
 
 bool GetUserpermitDialog::ShowToolTips()
 {
     return TRUE;
 }
 
 void GetUserpermitDialog::OnTestClick( wxCommandEvent& event )
 {
     wxString cmd = g_sencutil_bin;
     cmd += _T(" -y ");                  // validate Userpermit
     
     cmd += _T(" -u ");
     cmd += m_PermitCtl->GetValue();
     
     wxLogMessage( cmd );
     wxArrayString valup_result = exec_SENCutil_sync( cmd, false);

     bool berr = false;
     for(unsigned int i=0 ; i < valup_result.GetCount() ; i++){
         wxString line = valup_result[i];
         if(line.Upper().Find(_T("ERROR")) != wxNOT_FOUND){
             if( line.Upper().Find(_T("S63_PI")) != wxNOT_FOUND)  {
                 m_TestResult->SetLabel(line.Trim());
             }
             else {
                m_TestResult->SetLabel(_("Userpermit invalid"));
             }
             berr = true;
             m_OKButton->Disable();
             break;
         }
     }
     if(!berr){
         m_TestResult->SetLabel(_("Userpermit OK"));
         m_OKButton->Enable();
     }
 }
 
 void GetUserpermitDialog::OnCancelClick( wxCommandEvent& event )
 {
    EndModal(2);
 }
 
 void GetUserpermitDialog::OnOkClick( wxCommandEvent& event )
 {
     if( m_PermitCtl->GetValue().Length() == 0 ) 
         EndModal(1);
     else {
        g_userpermit = m_PermitCtl->GetValue();
        g_pi->SaveConfig();
     
        EndModal(0);
     }
 }
 
 void GetUserpermitDialog::OnUpdated( wxCommandEvent& event )
 {
     if( m_PermitCtl->GetValue().Length() )
         m_testBtn->Enable();
     else
         m_testBtn->Disable();
 }
 
 





wxString GetUserpermit(void)
{
    if(g_userpermit.Len())
        return g_userpermit;
    else {
        GetUserpermitDialog dlg(NULL);
        dlg.SetSize(500,-1);
        dlg.Centre();
        int ret = dlg.ShowModal();
        if(ret == 0)
            return g_userpermit;
        else 
            return _T("Invalid");
    }
}


/*!
 * GetInstallpermitDialog type definition
 */

IMPLEMENT_DYNAMIC_CLASS( GetInstallpermitDialog, wxDialog )
/*!
 * GetInstallpermitDialog event table definition
 */BEGIN_EVENT_TABLE( GetInstallpermitDialog, wxDialog )
 
 ////@begin GetInstallpermitDialog event table entries
 
 EVT_BUTTON( ID_GETIP_CANCEL, GetInstallpermitDialog::OnCancelClick )
 EVT_BUTTON( ID_GETIP_OK, GetInstallpermitDialog::OnOkClick )
 EVT_BUTTON( ID_GETIP_TEST, GetInstallpermitDialog::OnTestClick )
 EVT_TEXT(ID_GETIP_IP, GetInstallpermitDialog::OnUpdated)
 
 ////@end GetInstallpermitDialog event table entries
 
 END_EVENT_TABLE()
 
 /*!
  * GetInstallpermitDialog constructors
  */
 
 GetInstallpermitDialog::GetInstallpermitDialog()
 {
 }
 
 GetInstallpermitDialog::GetInstallpermitDialog( wxWindow* parent, wxWindowID id, const wxString& caption,
                                         const wxPoint& pos, const wxSize& size, long style )
 {
     
     long wstyle = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER;
     wxDialog::Create( parent, id, caption, pos, size, wstyle );
     
     CreateControls();
     GetSizer()->SetSizeHints( this );
     Centre();
     
 }
 
 GetInstallpermitDialog::~GetInstallpermitDialog()
 {
     delete m_PermitCtl;
 }
 
 /*!
  * GetInstallpermitDialog creator
  */
 
 bool GetInstallpermitDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                                  const wxPoint& pos, const wxSize& size, long style )
 {
     SetExtraStyle( GetExtraStyle() | wxWS_EX_BLOCK_EVENTS );
     
     long wstyle = style;
#ifdef __WXMAC__
     wstyle |= wxSTAY_ON_TOP;
#endif
     
     wxDialog::Create( parent, id, caption, pos, size, wstyle );
     
     CreateControls();
     GetSizer()->SetSizeHints( this );
     Centre();
     
     return TRUE;
 }
 
 /*!
  * Control creation for GetInstallpermitDialog
  */
 
 void GetInstallpermitDialog::CreateControls()
 {
     GetInstallpermitDialog* itemDialog1 = this;
     
     wxBoxSizer* itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );
     itemDialog1->SetSizer( itemBoxSizer2 );
     
     wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox( itemDialog1, wxID_ANY,
                                                               _("Enter Installpermit") );
     
     wxStaticBoxSizer* itemStaticBoxSizer4 = new wxStaticBoxSizer( itemStaticBoxSizer4Static,
                                                                   wxVERTICAL );
     itemBoxSizer2->Add( itemStaticBoxSizer4, 0, wxEXPAND | wxALL, 5 );
     
     wxStaticText* itemStaticText5 = new wxStaticText( itemDialog1, wxID_STATIC, _T(""),
     wxDefaultPosition, wxDefaultSize, 0 );
     itemStaticBoxSizer4->Add( itemStaticText5, 0,
                               wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );
     
     m_PermitCtl = new wxTextCtrl( itemDialog1, ID_GETIP_IP, _T(""), wxDefaultPosition,
     wxSize( 180, -1 ), 0 );
     itemStaticBoxSizer4->Add( m_PermitCtl, 0,
                               wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5 );
 
     wxBoxSizer* itemBoxSizerTest = new wxBoxSizer( wxHORIZONTAL );
     itemBoxSizer2->Add( itemBoxSizerTest, 0, wxALIGN_LEFT | wxALL, 5 );
     
     m_testBtn = new wxButton(itemDialog1, ID_GETIP_TEST, _("Test Installpermit"));
     m_testBtn->Disable();
     itemBoxSizerTest->Add( m_testBtn, 0, wxALIGN_LEFT | wxALL, 5 );

     wxStaticBox* itemStaticBoxTestResults = new wxStaticBox( itemDialog1, wxID_ANY,
                                                                  _("Test Results"), wxDefaultPosition, wxSize(500, 40) );
     
     wxStaticBoxSizer* itemStaticBoxSizerTest = new wxStaticBoxSizer( itemStaticBoxTestResults,  wxVERTICAL );
     itemBoxSizerTest->Add( itemStaticBoxSizerTest, 0,  wxALIGN_RIGHT |wxALL, 5 );
     
     
     m_TestResult = new wxStaticText( itemDialog1, -1, _T(""), wxDefaultPosition, wxSize( 180, -1 ), 0 );
     
     itemStaticBoxSizerTest->Add( m_TestResult, 0, wxALIGN_LEFT | wxALL, 5 );
     
     
     wxBoxSizer* itemBoxSizer16 = new wxBoxSizer( wxHORIZONTAL );
     itemBoxSizer2->Add( itemBoxSizer16, 0, wxALIGN_RIGHT | wxALL, 5 );
     
     m_CancelButton = new wxButton( itemDialog1, ID_GETIP_CANCEL, _("Cancel"), wxDefaultPosition,
     wxDefaultSize, 0 );
     itemBoxSizer16->Add( m_CancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
     m_CancelButton->SetDefault();
     
     m_OKButton = new wxButton( itemDialog1, ID_GETIP_OK, _("OK"), wxDefaultPosition,
     wxDefaultSize, 0 );
     itemBoxSizer16->Add( m_OKButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
     
     
     m_PermitCtl->AppendText(g_old_installpermit);
     
 }
 
 
 bool GetInstallpermitDialog::ShowToolTips()
 {
     return TRUE;
 }
 
 void GetInstallpermitDialog::OnTestClick( wxCommandEvent& event )
 {
     wxString cmd = g_sencutil_bin;
     cmd += _T(" -k ");                  // validate Installpermit
     
     cmd += _T(" -e ");
     cmd += m_PermitCtl->GetValue();
     
     cmd += _T(" -u ");
     cmd += g_userpermit;
     
     
     
     wxLogMessage( cmd );
     wxArrayString valup_result = exec_SENCutil_sync( cmd, false);

     bool berr = false;
     for(unsigned int i=0 ; i < valup_result.GetCount() ; i++){
         wxString line = valup_result[i];
         if(line.Upper().Find(_T("ERROR")) != wxNOT_FOUND){
             if( line.Upper().Find(_T("S63_PI")) != wxNOT_FOUND)  {
                 m_TestResult->SetLabel(line.Trim());
             }
             else {
                m_TestResult->SetLabel(_("Install permit invalid"));
             }
             berr = true;
             m_OKButton->Disable();
             break;
         }
     }
     if(!berr){
         m_TestResult->SetLabel(_("Install permit OK"));
         m_OKButton->Enable();
     }
 }
 
 void GetInstallpermitDialog::OnCancelClick( wxCommandEvent& event )
 {
    EndModal(2);
 }
 
 void GetInstallpermitDialog::OnOkClick( wxCommandEvent& event )
 {
     if( m_PermitCtl->GetValue().Length() == 0 ) 
         EndModal(1);
     else {
        g_installpermit = m_PermitCtl->GetValue();
        g_pi->SaveConfig();
     
        EndModal(0);
     }
 }
 
 void GetInstallpermitDialog::OnUpdated( wxCommandEvent& event )
 {
     if( m_PermitCtl->GetValue().Length() )
         m_testBtn->Enable();
     else
         m_testBtn->Disable();
 }
 
 
wxString GetInstallpermit(void)
{
    if(g_installpermit.Len())
        return g_installpermit;
    else {
        GetInstallpermitDialog dlg(NULL);
        dlg.SetSize(500,-1);
        dlg.Centre();
        int ret = dlg.ShowModal();
        if(ret == 0)
            return g_installpermit;
        else 
            return _T("Invalid");
    }
}