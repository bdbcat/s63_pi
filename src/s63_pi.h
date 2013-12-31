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

#define     PLUGIN_VERSION_MAJOR    0
#define     PLUGIN_VERSION_MINOR    1

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    11

#include "ocpn_plugin.h"


enum {
    ID_BUTTONCELLIMPORT
};

//      Private logging functions
void ScreenLogMessage(wxString s);
void HideScreenLog(void);
void ClearScreenLog(void);

extern "C++" wxString GetUserpermit(void);
extern "C++" wxArrayString exec_SENCutil_sync( wxString cmd, bool bshowlog );


class   s63_pi;
class   OCPNPermitList;

// An Event handler class to catch events from S63 UI dialog
class s63_pi_event_handler : public wxEvtHandler
{
public:

    s63_pi_event_handler(s63_pi *parent);
    ~s63_pi_event_handler();

    void OnImportPermitClick( wxCommandEvent &event );
    void OnRemovePermitClick( wxCommandEvent &event );
    void OnSelectPermit( wxListEvent& event );    
    s63_pi  *m_parent;
};




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

    wxArrayString GetDynamicChartClassNameArray();

    void OnSetupOptions();
    void OnCloseToolboxPanel(int page_sel, int ok_apply_cancel);
    
    int ImportCellPermits( void );
    int RemoveCellPermit( void );
    void EnablePermitRemoveButton(bool benable){ m_buttonRemovePermit->Enable(benable); }

    bool SaveConfig( void );
    
private:
    int ProcessCellPermit( wxString &permit, wxString &enc_root_dir );

    bool LoadConfig( void );
    
    int pi_error( wxString msg );
    
    wxArrayString     m_class_name_array;

    wxBitmap          *m_pplugin_icon;

    wxScrolledWindow  *m_s63chartPanelWin;
    s63_pi_event_handler *m_event_handler;
    
    OCPNPermitList      *m_permit_list;
    wxButton            *m_buttonImportPermit;
    wxButton            *m_buttonRemovePermit;
    
    wxFileConfig        *m_pconfig;
    wxString            m_SelectPermit_dir;

    wxString            m_userpermit;
};



class S63ScreenLog : public wxWindow
{
public:
    S63ScreenLog(wxWindow *parent);
    ~S63ScreenLog();
    
    void LogMessage(wxString &s);
    void ClearLog(void);
 
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
    
private:    
    S63ScreenLog        *m_slog;
};





class OCPNPermitList : public wxListCtrl
{
public:
    OCPNPermitList(wxWindow *parent);
    ~OCPNPermitList();
    
    void BuildList( const wxString &permit_dir );
    wxArrayString       m_permit_file_array;
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


#endif


