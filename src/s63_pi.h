/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  S63 Plugin
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

#ifndef _S63PI_H_
#define _S63PI_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include "wx/socket.h"
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include "TexFont.h"

#define     PLUGIN_VERSION_MAJOR    1
#define     PLUGIN_VERSION_MINOR    11

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    11

#include "ocpn_plugin.h"


enum {
    ID_BUTTONCELLIMPORT,
    ID_NOTEBOOK
};

//      Private logging functions
void ScreenLogMessage(wxString s);
void HideScreenLog(void);
void ClearScreenLog(void);
void ClearScreenLogSeq(void);

extern "C++" wxString GetUserpermit(void);
extern "C++" wxString GetInstallpermit(void);
extern "C++" wxArrayString exec_SENCutil_sync( wxString cmd, bool bshowlog );


class   s63_pi;
class   OCPNPermitList;
class   OCPNCertificateList;

// An Event handler class to catch events from S63 UI dialog
class s63_pi_event_handler : public wxEvtHandler
{
public:

    s63_pi_event_handler(s63_pi *parent);
    ~s63_pi_event_handler();

    void OnImportPermitClick( wxCommandEvent &event );
    void OnRemovePermitClick( wxCommandEvent &event );
    void OnImportCellsClick( wxCommandEvent &event );
    void OnSelectPermit( wxListEvent& event );    
    void OnNewUserpermitClick( wxCommandEvent& event );    
    void OnNewInstallpermitClick( wxCommandEvent& event );    
    void OnImportCertClick( wxCommandEvent &event );
    void OnNewFPRClick( wxCommandEvent &event );
    void OncbLogClick( wxCommandEvent &event );
    
    s63_pi  *m_parent;
};


class Catalog_Entry31
{
public:
    Catalog_Entry31(){};
    ~Catalog_Entry31(){};
    
    wxString m_filename;
    wxString m_comt;
};

WX_DECLARE_OBJARRAY(Catalog_Entry31,      Catalog31);


//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

class s63_pi : public opencpn_plugin_111
{
public:
      s63_pi(void *ppimgr);
      ~s63_pi();

//    The required PlugIn Methods
    int Init(void);
    bool DeInit(void);

    int GetAPIVersionMajor();
    int GetAPIVersionMinor();
    int GetPlugInVersionMajor();
    int GetPlugInVersionMinor();
    wxBitmap *GetPlugInBitmap();
    wxString GetCommonName();
    wxString GetShortDescription();
    wxString GetLongDescription();
    bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
    bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
    
    wxArrayString GetDynamicChartClassNameArray();

    void OnSetupOptions();
    void OnCloseToolboxPanel(int page_sel, int ok_apply_cancel);

    void SetPluginMessage(wxString &message_id, wxString &message_body);
    int ImportCellPermits( void );
    int RemoveCellPermit( void );
    int ImportCells( void );
    int ImportCert( void );
    void Set_FPR();
    
    void EnablePermitRemoveButton(bool benable){ m_buttonRemovePermit->Enable(benable); }
    void GetNewUserpermit(void);
    void GetNewInstallpermit(void);
    
    bool SaveConfig( void );
  
    wxString GetCertificateDir();
   
    wxStaticText        *m_up_text;
    wxStaticText        *m_ip_text;
    wxStaticText        *m_fpr_text;
    
    wxScrolledWindow    *m_s63chartPanelWinTop;
    wxPanel             *m_s63chartPanelWin;
    wxPanel             *m_s63chartPanelKeys;
    wxNotebook          *m_s63NB;
    
private:
    wxString GetPermitDir();
    
    Catalog31 *CreateCatalog31(const wxString &file31);
    
    int ProcessCellPermit( wxString &permit, bool b_confirm_existing );
    int AuthenticateCell( const wxString & cell_file );
    
    bool LoadConfig( void );
    
    int pi_error( wxString msg );
    
    wxArrayString     m_class_name_array;

    wxBitmap          *m_pplugin_icon;

    s63_pi_event_handler *m_event_handler;
    
    OCPNPermitList      *m_permit_list;
    wxButton            *m_buttonImportPermit;
    wxButton            *m_buttonRemovePermit;
    wxButton            *m_buttonNewUP;
    wxButton            *m_buttonImportCells;
    wxButton            *m_buttonNewIP;
    wxButton            *m_buttonNewFPR;
    wxCheckBox          *m_cbLog;
    
    wxFileConfig        *m_pconfig;
    wxString            m_SelectPermit_dir;

    wxString            m_userpermit;
    
    Catalog31           *m_catalog;
    wxString            m_last_enc_root_dir;
    
    OCPNCertificateList *m_cert_list;
    wxButton            *m_buttonImportCert;
    
    bool                m_bSSE26_shown;
    TexFont             m_TexFontMessage;
    
    
    
};



class S63ScreenLog : public wxWindow
{
public:
    S63ScreenLog(wxWindow *parent);
    ~S63ScreenLog();
    
    void LogMessage(wxString &s);
    void ClearLog(void);
    void ClearLogSeq(void){ m_nseq = 0; }
    
    void OnServerEvent(wxSocketEvent& event);
    void OnSocketEvent(wxSocketEvent& event);
    void OnSize( wxSizeEvent& event);
    
    
private:    
    wxTextCtrl          *m_plogtc;
    unsigned int        m_nseq;
    
    wxSocketServer      *m_server;
    
    DECLARE_EVENT_TABLE()
    
};

class S63ScreenLogContainer : public wxDialog
{
public:
    S63ScreenLogContainer(wxWindow *parent);
    ~S63ScreenLogContainer();
 
    void LogMessage(wxString &s);
    void ClearLog(void);
    S63ScreenLog        *m_slog;
    
private:    
};





class OCPNPermitList : public wxListCtrl
{
public:
    OCPNPermitList(wxWindow *parent);
    ~OCPNPermitList();
    
    void BuildList( const wxString &permit_dir );
    wxArrayString       m_permit_file_array;
};

class OCPNCertificateList : public wxListCtrl
{
public:
    OCPNCertificateList(wxWindow *parent);
    ~OCPNCertificateList();
    
    void BuildList( const wxString &cert_dir );
//    wxArrayString       m_permit_file_array;
};


/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_GETUP 8100
#define SYMBOL_GETUP_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_GETUP_TITLE _("S63_pi Userpermit Required")
#define SYMBOL_GETUP_IDNAME ID_GETUP
#define SYMBOL_GETUP_SIZE wxSize(500, 200)
#define SYMBOL_GETUP_POSITION wxDefaultPosition
#define ID_GETUP_CANCEL 8101
#define ID_GETUP_OK 8102
#define ID_GETUP_UP 8103
#define ID_GETUP_TEST 8104


////@end control identifiers

/*!
 * GetUserpermitDialog class declaration
 */
class GetUserpermitDialog: public wxDialog
{
    DECLARE_DYNAMIC_CLASS( GetUserpermitDialog )
    DECLARE_EVENT_TABLE()
    
public:
    /// Constructors
    GetUserpermitDialog( );
    GetUserpermitDialog( wxWindow* parent, wxWindowID id = SYMBOL_GETUP_IDNAME,
                        const wxString& caption = SYMBOL_GETUP_TITLE,
                        const wxPoint& pos = SYMBOL_GETUP_POSITION,
                        const wxSize& size = SYMBOL_GETUP_SIZE,
                        long style = SYMBOL_GETUP_STYLE );
    
    ~GetUserpermitDialog();
    
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_GETUP_IDNAME,
                 const wxString& caption = SYMBOL_GETUP_TITLE,
                 const wxPoint& pos = SYMBOL_GETUP_POSITION,
                 const wxSize& size = SYMBOL_GETUP_SIZE, long style = SYMBOL_GETUP_STYLE );
    
   
    void CreateControls();
    
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnUpdated( wxCommandEvent& event );
    void OnTestClick( wxCommandEvent& event );
    
    /// Should we show tooltips?
    static bool ShowToolTips();
    
    wxTextCtrl*   m_PermitCtl;
    wxButton*     m_CancelButton;
    wxButton*     m_OKButton;
    wxButton*     m_testBtn;
    wxStaticText* m_TestResult;
    
    
};

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_GETIP 8200
#define SYMBOL_GETIP_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_GETIP_TITLE _("S63_pi Install Permit Required")
#define SYMBOL_GETIP_IDNAME ID_GETIP
#define SYMBOL_GETIP_SIZE wxSize(500, 200)
#define SYMBOL_GETIP_POSITION wxDefaultPosition
#define ID_GETIP_CANCEL 8201
#define ID_GETIP_OK 8202
#define ID_GETIP_IP 8203
#define ID_GETIP_TEST 8204


////@end control identifiers

/*!
 * GetInstallpermitDialog class declaration
 */
class GetInstallpermitDialog: public wxDialog
{
    DECLARE_DYNAMIC_CLASS( GetInstallpermitDialog )
    DECLARE_EVENT_TABLE()
    
public:
    /// Constructors
    GetInstallpermitDialog( );
    GetInstallpermitDialog( wxWindow* parent, wxWindowID id = SYMBOL_GETIP_IDNAME,
                         const wxString& caption = SYMBOL_GETIP_TITLE,
                         const wxPoint& pos = SYMBOL_GETIP_POSITION,
                         const wxSize& size = SYMBOL_GETIP_SIZE,
                         long style = SYMBOL_GETIP_STYLE );
    
    ~GetInstallpermitDialog();
    
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_GETIP_IDNAME,
                 const wxString& caption = SYMBOL_GETIP_TITLE,
                 const wxPoint& pos = SYMBOL_GETIP_POSITION,
                 const wxSize& size = SYMBOL_GETIP_SIZE, long style = SYMBOL_GETIP_STYLE );
    
    
    void CreateControls();
    
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnUpdated( wxCommandEvent& event );
    void OnTestClick( wxCommandEvent& event );
    
    /// Should we show tooltips?
    static bool ShowToolTips();
    
    wxTextCtrl*   m_PermitCtl;
    wxButton*     m_CancelButton;
    wxButton*     m_OKButton;
    wxButton*     m_testBtn;
    wxStaticText* m_TestResult;
    
    
};

class InfoWin: public wxWindow
{
public:
    InfoWin( wxWindow *parent, const wxString&s = _T(""), bool show_gauge = true );
    ~InfoWin();
    
    void SetString(const wxString &s);
    const wxString& GetString(void) { return m_string; }
    
    void SetPosition( wxPoint pt ){ m_position = pt; }
    void SetWinSize( wxSize sz ){ m_size = sz; }
    void Realize( void );
    wxSize GetWinSize( void ){ return m_size; }
    void OnPaint( wxPaintEvent& event );
    void OnEraseBackground( wxEraseEvent& event );
    void OnTimer( wxTimerEvent& event );
    
    wxStaticText *m_pInfoTextCtl;
    wxGauge   *m_pGauge;
    wxTimer     m_timer;
    
private:
    
    wxString m_string;
    wxSize m_size;
    wxPoint m_position;
    bool m_bGauge;
    
    DECLARE_EVENT_TABLE()
};

class InfoWinDialog: public wxDialog
{
public:
    InfoWinDialog( wxWindow *parent, const wxString&s = _T(""), bool show_gauge = true );
    ~InfoWinDialog();
    
    void SetString(const wxString &s);
    const wxString& GetString(void) { return m_string; }
    
    void SetPosition( wxPoint pt ){ m_position = pt; }
    void SetWinSize( wxSize sz ){ m_size = sz; }
    void Realize( void );
    wxSize GetWinSize( void ){ return m_size; }
    void OnPaint( wxPaintEvent& event );
    void OnEraseBackground( wxEraseEvent& event );
    void OnTimer( wxTimerEvent& event );
    
    wxStaticText *m_pInfoTextCtl;
    wxGauge   *m_pGauge;
    wxTimer     m_timer;
    
private:
    
    wxString m_string;
    wxSize m_size;
    wxPoint m_position;
    bool m_bGauge;
    
    DECLARE_EVENT_TABLE()
};

#endif


