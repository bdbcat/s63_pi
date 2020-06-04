#include <string>
#include <wx/string.h>

#if 0
/**
 * Return binary directory used when installing plugin, without
 * trailing separator.
 *
 *   - Parameter: name: Name of plugin.
 *   - Return: Binary dir used when installing plugin or "" if not found.
 */
wxString getInstallationBindir(const char* name);
#endif

/*
 * Look for binary with given basename (no extension) in PATH.
 *
 * @return path to binary if found, else 0.
 *
 */
std::string find_in_path(std::string binary);
