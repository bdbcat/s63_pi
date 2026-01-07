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

#include<vector>

#include "wx/socket.h"
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include "TexFont.h"

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    16

#include "ocpn_plugin.h"

#ifdef __MSVC__
#include <windows.h>
#endif

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

//WX_DECLARE_OBJARRAY(Catalog_Entry31,      Catalog31);


//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

class s63_pi : public opencpn_plugin_116
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
    void SetColorScheme(PI_ColorScheme cs) { s63_pi::global_color_scheme = cs; }

    wxStaticText        *m_up_text;
    wxStaticText        *m_ip_text;
    wxStaticText        *m_fpr_text;

    wxScrolledWindow    *m_s63chartPanelWinTop;
    wxPanel             *m_s63chartPanelWin;
    wxPanel             *m_s63chartPanelKeys;
    wxNotebook          *m_s63NB;

    static PI_ColorScheme global_color_scheme;

private:
    wxString GetPermitDir();

    void CreateCatalog31(const wxString &file31);

    int ProcessCellPermit( wxString &permit, bool b_confirm_existing );
    int AuthenticateCell( const wxString & cell_file );

    bool LoadConfig( void );

    int pi_error( wxString msg );

    wxArrayString     m_class_name_array;

    wxBitmap          *m_pplugin_icon;
    wxBitmap           m_panelBitmap;

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

    std::vector<Catalog_Entry31 *> m_catalog;
    wxString            m_last_enc_root_dir;

    OCPNCertificateList *m_cert_list;
    wxButton            *m_buttonImportCert;

    bool                m_bSSE26_shown;
    TexFont             m_TexFontMessage;
};

// An Event handler class to catch events
class s63_pi_event_handler_timer : public wxEvtHandler
{
public:

    s63_pi_event_handler_timer(s63_pi *parent);
    ~s63_pi_event_handler_timer();

    void Reset();
    void onTimerEvent(wxTimerEvent &event);

    s63_pi  *m_parent;

    wxTimer     m_eventTimer;

    DECLARE_EVENT_TABLE()

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

#include <fcntl.h>
#include <atomic>
#include <sys/wait.h>
#include <sys/poll.h>

struct ProcessOptions
{
    std::vector<std::string> argv;   // argv[0] = executable
    std::string workingDir;
    bool captureStdout = true;
    bool captureStderr = false;
    int timeoutMs = -1;              // -1 = infinite
};

struct ProcessResult
{
    int exitCode = -1;
    std::string stdoutText;
    std::string stderrText;
    bool timedOut = false;
};
void SetNonBlocking(int fd);
std::vector<char*> BuildArgv(const std::vector<std::string>& args);

class IProcessBackend
{
public:
    virtual ~IProcessBackend() = default;

    virtual ProcessResult Run(const ProcessOptions& opts,
                              std::atomic<bool>& cancelFlag) = 0;
};

std::unique_ptr<IProcessBackend> CreateBackend();


#ifndef __MSVC__
class PosixProcessBackend : public IProcessBackend
{
public:
    ProcessResult Run(const ProcessOptions& opts,
                      std::atomic<bool>& cancelFlag) override
    {
        ProcessResult result;
        auto argvt = BuildArgv(opts.argv);

        int stdoutPipe[2] = {-1, -1};
        int stderrPipe[2] = {-1, -1};

        if (opts.captureStdout && pipe(stdoutPipe) != 0)
            return result;

        if (opts.captureStderr && pipe(stderrPipe) != 0)
            return result;

        pid_t pid = fork();
        if (pid == 0)
        {
            // ---------- Child ----------
            if (!opts.workingDir.empty())
                chdir(opts.workingDir.c_str());

            if (opts.captureStdout)
            {
                dup2(stdoutPipe[1], STDOUT_FILENO);
            }
            if (opts.captureStderr)
            {
                dup2(stderrPipe[1], STDERR_FILENO);
            }

            // Close all pipe FDs
            if (stdoutPipe[0] != -1) close(stdoutPipe[0]);
            if (stdoutPipe[1] != -1) close(stdoutPipe[1]);
            if (stderrPipe[0] != -1) close(stderrPipe[0]);
            if (stderrPipe[1] != -1) close(stderrPipe[1]);

            auto argv = BuildArgv(opts.argv);
            execvp(argv[0], argv.data());

            _exit(127); // exec failed
        }

        // ---------- Parent (worker thread) ----------
        if (opts.captureStdout)
        {
            close(stdoutPipe[1]);
            SetNonBlocking(stdoutPipe[0]);
        }
        if (opts.captureStderr)
        {
            close(stderrPipe[1]);
            SetNonBlocking(stderrPipe[0]);
        }

        const int startTimeMs = NowMs();
        bool stdoutOpen = opts.captureStdout;
        bool stderrOpen = opts.captureStderr;

        while (stdoutOpen || stderrOpen)
        {
            if (cancelFlag.load())
            {
                kill(pid, SIGTERM);
                result.timedOut = true;
                break;
            }

            if (opts.timeoutMs >= 0 &&
                (NowMs() - startTimeMs) > opts.timeoutMs)
            {
                kill(pid, SIGKILL);
                result.timedOut = true;
                break;
            }

            struct pollfd fds[2];
            int nfds = 0;

            if (stdoutOpen)
            {
                fds[nfds++] = { stdoutPipe[0], POLLIN, 0 };
            }
            if (stderrOpen)
            {
                fds[nfds++] = { stderrPipe[0], POLLIN, 0 };
            }

            int rc = poll(fds, nfds, 100);
            if (rc <= 0)
                continue;

            for (int i = 0; i < nfds; ++i)
            {
                if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
                {
                  char buf[4096];
                  ssize_t n = read(fds[i].fd, buf, sizeof(buf));

                  if (n > 0)
                  {
                    if (fds[i].fd == stdoutPipe[0])
                      result.stdoutText.append(buf, n);
                    else
                      result.stderrText.append(buf, n);
                  }
                  else
                  {
                    // EOF or error → close FD
                    if (fds[i].fd == stdoutPipe[0])
                    {
                      close(stdoutPipe[0]);
                      stdoutOpen = false;
                    }
                    else
                    {
                      close(stderrPipe[0]);
                      stderrOpen = false;
                    }
                  }
                }
            }
        }

        int status = 0;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
            result.exitCode = WEXITSTATUS(status);

        return result;
    }

private:
    static int NowMs()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
};

#else

class Win32ProcessBackend : public IProcessBackend
{
public:
    ProcessResult Run(const ProcessOptions& opts,
                      std::atomic<bool>& cancelFlag) override
    {
        ProcessResult result;

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE hStdOutRd = nullptr, hStdOutWr = nullptr;
        HANDLE hStdErrRd = nullptr, hStdErrWr = nullptr;

        if (opts.captureStdout)
        {
            CreatePipe(&hStdOutRd, &hStdOutWr, &sa, 0);
            SetHandleInformation(hStdOutRd, HANDLE_FLAG_INHERIT, 0);
        }

        if (opts.captureStderr)
        {
            CreatePipe(&hStdErrRd, &hStdErrWr, &sa, 0);
            SetHandleInformation(hStdErrRd, HANDLE_FLAG_INHERIT, 0);
        }

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = opts.captureStdout ? hStdOutWr : GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError  = opts.captureStderr ? hStdErrWr : GetStdHandle(STD_ERROR_HANDLE);
        si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

        PROCESS_INFORMATION pi{};

        std::wstring cmdLine = BuildCommandLine(opts.argv);
        std::wstring cwd = Utf8ToWide(opts.workingDir);

        BOOL ok = CreateProcessW(
                nullptr,
                cmdLine.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                cwd.empty() ? nullptr : cwd.c_str(),
                &si,
                &pi);

        if (!ok)
            return result;

        // Parent: close write ends
        if (hStdOutWr) CloseHandle(hStdOutWr);
        if (hStdErrWr) CloseHandle(hStdErrWr);

        const DWORD startTick = GetTickCount();
        bool stdoutOpen = opts.captureStdout;
        bool stderrOpen = opts.captureStderr;

        while (stdoutOpen || stderrOpen)
        {
            if (cancelFlag.load())
            {
                TerminateProcess(pi.hProcess, 1);
                result.timedOut = true;
                break;
            }

            if (opts.timeoutMs >= 0 &&
                (GetTickCount() - startTick) > (DWORD)opts.timeoutMs)
            {
                TerminateProcess(pi.hProcess, 1);
                result.timedOut = true;
                break;
            }

            DWORD avail = 0;
            char buffer[4096];

            if (stdoutOpen &&
                PeekNamedPipe(hStdOutRd, nullptr, 0, nullptr, &avail, nullptr))
            {
                if (avail == 0)
                {
                    DWORD rc = WaitForSingleObject(pi.hProcess, 0);
                    if (rc == WAIT_OBJECT_0)
                    {
                        CloseHandle(hStdOutRd);
                        stdoutOpen = false;
                    }
                }
                else
                {
                    DWORD read = 0;
                    if (ReadFile(hStdOutRd, buffer, sizeof(buffer), &read, nullptr) && read > 0)
                        result.stdoutText.append(buffer, read);
                }
            }

            if (stderrOpen &&
                PeekNamedPipe(hStdErrRd, nullptr, 0, nullptr, &avail, nullptr))
            {
                if (avail == 0)
                {
                    DWORD rc = WaitForSingleObject(pi.hProcess, 0);
                    if (rc == WAIT_OBJECT_0)
                    {
                        CloseHandle(hStdErrRd);
                        stderrOpen = false;
                    }
                }
                else
                {
                    DWORD read = 0;
                    if (ReadFile(hStdErrRd, buffer, sizeof(buffer), &read, nullptr) && read > 0)
                        result.stderrText.append(buffer, read);
                }
            }

            Sleep(10);
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        result.exitCode = (int)exitCode;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return result;
    }
};
#endif


class ProcessRunner
{
public:
    ProcessRunner()
            : m_backend(CreateBackend()) {}

    ProcessResult Run(const ProcessOptions& opts)
    {
        m_cancel.store(false);
        return m_backend->Run(opts, m_cancel);
    }

    void Cancel()
    {
        m_cancel.store(true);
    }

private:
    std::atomic<bool> m_cancel{false};
    std::unique_ptr<IProcessBackend> m_backend;
};

#endif


