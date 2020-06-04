#include "InstallDirs.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <wx/filename.h>
#include <wx/log.h>

// FIXME: Missing includes in ocpn_plugin.h
#include <wx/dcmemory.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/menuitem.h>
#include <wx/gdicmn.h>
#include <wx/platinfo.h>

#include  "ocpn_plugin.h"


typedef std::unordered_map<std::string, std::string> pathmap_t;


std::vector<std::string> split(std::string s, char delimiter)
{
    using namespace std;

    vector<string> tokens;
    size_t start = s.find_first_not_of(delimiter);
    size_t end = start;
    while (start != string::npos) {
        end = s.find(delimiter, start);
        tokens.push_back(s.substr(start, end - start));
        start = s.find_first_not_of(delimiter, end);
    }
    return tokens;
}


inline bool exists(const std::string& name) {
    wxFileName fn(name.c_str());
    if (!fn.IsOk()) {
        return false;
    }
    return fn.FileExists();
}


std::string find_in_path(std::string binary)
{
    using namespace std;

    wxString wxPath;
    wxGetEnv("PATH", &wxPath);
    string path(wxPath.c_str()); 

    auto const osSystemId = wxPlatformInfo::Get().GetOperatingSystemId();
    char delimiter = ':';
    if (osSystemId & wxOS_WINDOWS) {
        delimiter =  ';';
        binary += ".exe";
    }
    vector<string> elements = split(path, delimiter);
    for (auto element: elements) {
       string filename = element + "/" + binary;
       if (exists(filename)) {
	   return filename;
       }
    }
    return "";
}
