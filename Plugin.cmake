# ~~~
# Author:      bdcat <Dave Register>
# Copyright:
# License:     GPLv2+
# ~~~

# -------- Cmake setup ---------
#

set(OCPN_TEST_REPO
    "david-register/ocpn-plugins-unstable"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "opencpn/s63-beta"
    CACHE STRING
    "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "opencpn/s63-prod"
    CACHE STRING
    "Default repository for tagged builds not matching 'beta'"
)

set(OCPN_TARGET_TUPLE "" CACHE STRING
  "Target spec: \"platform;version;arch\""
)

# -------  Plugin setup --------
#
include("VERSION.cmake")
set(PKG_NAME s63_pi)
set(PKG_VERSION ${OCPN_VERSION})


set(PKG_PRERELEASE "")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME S63)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME S63) # As of GetCommonName() in plugin API
set(CPACK_PACKAGE_CONTACT "Dave Register")
set(PKG_SUMMARY
  "OpenCPN support for S63 Vector Charts"
)
set(PKG_DESCRIPTION [=[
OpenCPN support for S63 Vector Charts
]=])

set(PKG_HOMEPAGE https://github.com/bdbcat/s63_pi)
set(PKG_INFO_URL https://opencpn.org/)

set(PKG_AUTHOR "Dave register")


SET(SRC_S63
            src/s63_pi.h
            src/s63_pi.cpp
            src/s63chart.cpp
            src/s63chart.h
            src/mygeom63.h
            src/mygeom63.cpp
            src/TexFont.cpp
            src/TexFont.h
            src/InstallDirs.cpp
     )

SET(SRC_CPL
                src/cpl/cpl_config.h
                src/cpl/cpl_conv.h
                src/cpl/cpl_csv.h
                src/cpl/cpl_error.h
                src/cpl/cpl_port.h
                src/cpl/cpl_string.h
                src/cpl/cpl_vsi.h
                src/cpl/cpl_conv.cpp
                src/cpl/cpl_csv.cpp
                src/cpl/cpl_error.cpp
                src/cpl/cpl_findfile.cpp
                src/cpl/cpl_path.cpp
                src/cpl/cpl_string.cpp
                src/cpl/cpl_vsisimple.cpp
        )

SET(SRC_ISO8211
                src/myiso8211/ddffielddefn.cpp
                src/myiso8211/ddfmodule.cpp
                src/myiso8211/ddfrecord.cpp
                src/myiso8211/ddfsubfielddefn.cpp
                src/myiso8211/ddffield.cpp
                src/myiso8211/ddfutils.cpp
        )

SET(SRC_DSA
                src/dsa/dsa_utils.cpp
                src/dsa/mp_math.c
                src/dsa/sha1.c
    )

SET(SRC_JSON
            src/wxJSON/json_defs.h
            src/wxJSON/jsonreader.h
            src/wxJSON/jsonval.h
            src/wxJSON/jsonwriter.h
            src/wxJSON/jsonreader.cpp
            src/wxJSON/jsonval.cpp
            src/wxJSON/jsonwriter.cpp
        )

set(SRC ${SRC_S63} ${SRC_CPL} ${SRC_ISO8211} ${SRC_DSA} )


if(QT_ANDROID)
  set(SRC ${SRC} src/androidSupport.cpp)
endif(QT_ANDROID)

set(PKG_API_LIB api-16)  #  A directory in libs/ e. g., api-17 or api-16

macro(late_init)
  # Perform initialization after the PACKAGE_NAME library, compilers
  # and ocpn::api is available.
  target_include_directories(${PACKAGE_NAME} PRIVATE libs/gdal/src src)

  # A specific target requires special handling
  # When OCPN core is a ubuntu-bionic build, and the debian-10 plugin is loaded,
  #   then there is conflict between wxCurl in core, vs plugin
  # So, in this case, disable the plugin's use of wxCurl directly, and revert to
  #   plugin API for network access.
  # And, Android always uses plugin API for network access
  string(TOLOWER "${OCPN_TARGET_TUPLE}" _lc_target)
  message(STATUS "late_init: ${OCPN_TARGET_TUPLE}.")

  if ( (NOT "${_lc_target}" MATCHES "debian;10;x86_64") AND
       (NOT "${_lc_target}" MATCHES "android*") )
    add_definitions(-D__OCPN_USE_CURL__)
  endif()

endmacro ()

add_definitions(-DocpnUSE_GL)

macro(add_plugin_libraries)
  add_subdirectory("libs/cpl")
  target_link_libraries(${PACKAGE_NAME} ocpn::cpl)

  add_subdirectory("libs/dsa")
  target_link_libraries(${PACKAGE_NAME} ocpn::dsa)

  add_subdirectory("opencpn-libs/wxJSON")
  target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

  add_subdirectory("opencpn-libs/iso8211")
  target_link_libraries(${PACKAGE_NAME} ocpn::iso8211)

  add_subdirectory("opencpn-libs/tinyxml")
  target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)

  add_subdirectory("opencpn-libs/zlib")
  target_link_libraries(${PACKAGE_NAME} ocpn::zlib)

  #add_subdirectory(libs/s52plib)
  #target_link_libraries(${PACKAGE_NAME} ocpn::s52plib)

  #add_subdirectory(libs/geoprim)
  #target_link_libraries(${PACKAGE_NAME} ocpn::geoprim)

  #add_subdirectory(libs/pugixml)
  #target_link_libraries(${PACKAGE_NAME} ocpn::pugixml)


#if (MSVC)
#  add_subdirectory("libs/WindowsHeaders")
#  target_link_libraries(${PACKAGE_NAME} _windows_headers)
#endif ()


#   add_subdirectory("libs/opencpn-glu")
#   target_link_libraries(${PACKAGE_NAME} opencpn::glu)

#  add_subdirectory("libs/wxcurl")
#  target_link_libraries(${PACKAGE_NAME} ocpn::wxcurl)

  add_subdirectory("libs/OCPNsenc")

endmacro ()
