##---------------------------------------------------------------------------
## Author:      bdbcat aka. dsr (Dave Register)
## Copyright:
## License:     wxWidgets License
##---------------------------------------------------------------------------


# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)
# This should be 2.8.0 to have FindGTK2 module
IF (COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
  CMAKE_POLICY(SET CMP0005 NEW)
  CMAKE_POLICY(SET CMP0011 NEW)
  CMAKE_POLICY(SET CMP0037 NEW)
  if (POLICY CMP0053)
      CMAKE_POLICY(SET CMP0053 NEW)
  endif ()
ENDIF (COMMAND cmake_policy)

# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")

PROJECT(oesenc_pi)

SET(PACKAGE_NAME s63_pi)
SET(VERBOSE_NAME s63)
SET(PLUGIN_SOURCE_DIR .)
MESSAGE (STATUS "*** Building ${PACKAGE_NAME} ***")

include("VERSION.cmake")
SET(PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )

SET(PKG_RELEASE "0")
SET(PKG_API_VERSION "1.16")
SET(PKG_AUTHOR "David S Register")

# NVR: <Name-version>[-release]
SET(PKG_NVR ${PACKAGE_NAME}-${PACKAGE_VERSION}-${PKG_RELEASE})

# The @keyword@ references are patched to actual values by upload script.
SET(TARBALL_URL "https://dl.cloudsmith.io/public/@pkg_repo@/raw")
SET(TARBALL_URL ${TARBALL_URL}/names/@name@/versions/@version@/@filename@)


SET(API_DIR "${CMAKE_SOURCE_DIR}/api-16")
SET(OCPN_USE_API_16 ON)

SET(OCPN_MIN_VERSION "ov50")
#OPTION(OCPN_USE_API_16 "Use plugin api 16 (default: api 13)." OFF)
OPTION(OCPN_FLATPAK "Build flatpak plugin" OFF)
 
set(PLUGIN_NAME ${PKG_NVR}_${PKG_TARGET}-${PKG_TARGET_VERSION})

IF( NOT CMAKE_BUILD_TYPE )
  SET( CMAKE_BUILD_TYPE Release CACHE STRING
       "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
       FORCE )
ENDIF( NOT CMAKE_BUILD_TYPE )

MESSAGE (STATUS "Build type: ${CMAKE_BUILD_TYPE}")

SET(CMAKE_VERBOSE_MAKEFILE ON)

IF (PREFIX)
 SET(CMAKE_INSTALL_PREFIX ${PREFIX})
ENDIF (PREFIX)

SET(PREFIX_BIN bin)
SET(PREFIX_INCLUDE include)
SET(PREFIX_DATA share)
SET(PREFIX_PKGDATA ${PREFIX_DATA}/${PACKAGE_NAME})

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if (CMAKE_VERSION VERSION_LESS "3.1")
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    message(STATUS "Setting C++11 standard via CXX flags")
  elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    message(STATUS "Setting C++0x standard via CXX FLAGS")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
  endif()
else ()
  set (CMAKE_CXX_STANDARD 11)
  message(STATUS "Setting C++11 standard via cmake standard mecahnism")
endif ()

IF (NOT WIN32)
  # default
  SET (ARCH "i386")
  IF (UNIX AND NOT APPLE)

    MESSAGE (STATUS "*** Will install to ${CMAKE_INSTALL_PREFIX}  ***")

    IF (EXISTS /etc/debian_version)
      MESSAGE (STATUS "*** Debian detected  ***")
      SET (PACKAGE_FORMAT "DEB")
      SET (PACKAGE_DEPS "libc6, libwxgtk3.0-0, libglu1-mesa (>= 7.0.0) curl")
      SET (LIB_INSTALL_DIR "lib")
      IF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
        IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
          SET (ARCH "arm64")
          ADD_DEFINITIONS( -DOCPN_ARM64 )
        ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
          SET (ARCH "armhf")
          ADD_DEFINITIONS( -DOCPN_ARMHF )
        ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
      ELSE (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
        IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
          SET (ARCH "x86_64")
        ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
          SET (ARCH "i386")
        ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
      ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
    ENDIF (EXISTS /etc/debian_version)
    IF(NOT DEFINED PACKAGE_FORMAT)
      IF (EXISTS /app)
        MESSAGE (STATUS "*** Flatpak detected  ***")
        SET (PACKAGE_FORMAT "TGZ")
        SET (ARCH "x86_64")
        SET (LIB_INSTALL_DIR "lib")
      ENDIF (EXISTS /app)
    ENDIF(NOT DEFINED PACKAGE_FORMAT)
    
    IF(NOT DEFINED PACKAGE_FORMAT)
     IF (EXISTS /etc/redhat-release)
      MESSAGE (STATUS "*** Redhat detected  ***")
      SET (PACKAGE_FORMAT "RPM")
      SET (PACKAGE_DEPS  "opencpn")
      IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "x86_64")
        SET (LIB_INSTALL_DIR "lib64")
      ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "i386")
        SET (LIB_INSTALL_DIR "lib")
      ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
     ENDIF (EXISTS /etc/redhat-release)
    ENDIF(NOT DEFINED PACKAGE_FORMAT)
 
    IF(NOT DEFINED PACKAGE_FORMAT)
     IF (EXISTS /etc/os-release OR EXISTS /etc/sysconfig/SuSEfirewall2.d OR EXISTS /etc/suse-release OR EXISTS /etc/SuSE-release)
      MESSAGE (STATUS "*** OpenSUSE detected  ***")
      SET (PACKAGE_FORMAT "RPM")
      SET (PACKAGE_DEPS  "opencpn")
      IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "x86_64")
        SET (LIB_INSTALL_DIR "lib")
      ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "i386")
        SET (LIB_INSTALL_DIR "lib")
      ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
     ENDIF (EXISTS /etc/os-release OR EXISTS /etc/sysconfig/SuSEfirewall2.d OR EXISTS /etc/suse-release OR EXISTS /etc/SuSE-release)
    ENDIF(NOT DEFINED PACKAGE_FORMAT)

  ENDIF (UNIX AND NOT APPLE)
  
  IF(APPLE)
    SET (ARCH "x86_64")
  ENDIF(APPLE)

else(NOT WIN32)  
    SET (ARCH "x86_64")
endif(NOT WIN32)

IF (CMAKE_SYSTEM_PROCESSOR MATCHES "armhf")
  SET (ARCH "armhf")
  ADD_DEFINITIONS( -DARMHF )
ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "armhf")

IF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
  SET (ARCH "arm64")
  ADD_DEFINITIONS( -DARMHF )
ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")

MESSAGE (STATUS "*** Build Architecture is ${ARCH}")
MESSAGE (STATUS "*** CMAKE_SYSTEM_PROCESSOR is ${CMAKE_SYSTEM_PROCESSOR}")
MESSAGE (STATUS "*** PACKAGE_FORMAT is ${PACKAGE_FORMAT}")

if (OCPN_FLATPAK)
    # Lie to PluginSetup that we have found wxWidgets so it don't barfs.
    set(wxWidgets_LIBRARIES FOOBAR)
    include("cmake/PluginSetup.cmake")
    configure_file(
      # Required in flatpak, done early.
      ${CMAKE_SOURCE_DIR}/pkg_version.sh.in
      ${CMAKE_CURRENT_BINARY_DIR}/pkg_version.sh
    )
    set(PLUGIN_NAME ${VERBOSE_NAME}-plugin-${PKG_TARGET}-${PKG_TARGET_VERSION})

    find_program(TAR NAMES gtar tar)
    if (NOT TAR)
        message(FATAL_ERROR "tar not found, required for OCPN_FLATPAK")
    endif ()
    configure_file(
        ${CMAKE_SOURCE_DIR}/oesenc-plugin.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.xml
    )
    add_custom_target(flatpak-build ALL
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/flatpak
        COMMAND flatpak-builder --force-clean
            ${CMAKE_CURRENT_BINARY_DIR}/app
            org.opencpn.OpenCPN.Plugin.oesenc.yaml
    )
    add_custom_target("package")
    add_custom_command(
        TARGET package
        COMMAND ${TAR}
            -czf ${PKG_NVR}_${PKG_TARGET_NVR}.tar.gz
            --transform 's|.*/files/|oesenc_pi-flatpak-${PACKAGE_VERSION}/|'
            ${CMAKE_CURRENT_BINARY_DIR}/app/files
    )
    return ()
endif()



#  To build for android, use this cmake invocation..
#$cmake -DUSE_GARMINHOST=OFF -D_wx_selected_config=androideabi-qt
#-DCMAKE_TOOLCHAIN_FILE=../buildandroid/build_android.cmake
#-DwxQt_Build=build_android_55_intl -DwxQt_Base=/home/dsr/Projects/wxqt/wxWidgets
#-DQt_Base=/home/dsr/Projects/qt5bare -DQt_Build=build_release/qtbase ..

IF(DEFINED _wx_selected_config)

#  May be overridden
IF(NOT DEFINED Qt_Build)
  SET(Qt_Build "android_armv7")
ENDIF(NOT DEFINED Qt_Build)

MESSAGE (STATUS "selected config ${_wx_selected_config}")

IF(_wx_selected_config MATCHES "androideabi-qt")
  MESSAGE (STATUS "Building for wxQt-Android")
  MESSAGE (STATUS "Qt_Base/Build: " ${Qt_Base} "/" ${Qt_Build})
  MESSAGE (STATUS "wxQt_Base/Build: " ${wxQt_Base} "/" ${wxQt_Build})
  SET(QT_ANDROID "ON")
  SET(QT_LINUX "OFF")
  SET(QT "ON")
  ADD_DEFINITIONS("-Wno-inconsistent-missing-override -Wno-potentially-evaluated-expression")
  SET(CMAKE_SKIP_BUILD_RPATH  TRUE)
ENDIF(_wx_selected_config MATCHES "androideabi-qt")

ENDIF(DEFINED _wx_selected_config)


INCLUDE("cmake/PluginConfigure.cmake")


INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src/wxJSON)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src/cpl)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/src/dsa)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR})

if (OCPN_USE_API_16) 
  INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-16)
else ()
  INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/api-13)
endif ()

IF(NOT WIN32)
 ADD_DEFINITIONS( "-Wall -g -fexceptions -O2 -Wno-unused -fno-strict-aliasing ")

 IF(CMAKE_BUILD_TYPE MATCHES "Debug")
    ADD_DEFINITIONS( " -O0")
 ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

 IF(NOT APPLE)
  SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic ")
  IF(NOT QT_ANDROID)
   ADD_DEFINITIONS( "-rdynamic" )
  ENDIF(NOT QT_ANDROID)
 ELSE(NOT APPLE)
  SET(CMAKE_SHARED_LINKER_FLAGS "-Wl -undefined dynamic_lookup")
  ADD_DEFINITIONS( "-Wno-overloaded-virtual" )
 ENDIF(NOT APPLE)

ENDIF(NOT WIN32)


# Add some definitions to satisfy MS
IF(WIN32)
    ADD_DEFINITIONS(-D__MSVC__)
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE)
ENDIF(WIN32)

# Let cmake find additional modules private
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})


ADD_DEFINITIONS(-DTIXML_USE_STL)


# For convenience we define the sources as a variable. You can add
# header files and cpp/c files and CMake will sort them out


SET(SRC_S63
            src/s63_pi.h
            src/s63_pi.cpp
            src/s63chart.cpp
            src/s63chart.h
            src/mygeom63.h
            src/mygeom63.cpp
            #src/tri.c
            src/TexFont.cpp
            src/TexFont.h
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


IF(UNIX AND NOT APPLE)
 INCLUDE("cmake/FindTinyXML.cmake")
 FIND_PACKAGE(TinyXML QUIET)
ENDIF(UNIX AND NOT APPLE)

IF(TINYXML_FOUND)
 message (STATUS "s63_pi building with system tinyxml")
 INCLUDE_DIRECTORIES(${TINYXML_INCLUDE_DIR})
ELSE(TINYXML_FOUND)
 message (STATUS "s63_pi building with embedded tinyxml")
 INCLUDE_DIRECTORIES(src/tinyxml/)
ENDIF(TINYXML_FOUND)
ADD_DEFINITIONS(-DTIXML_USE_STL)


ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_S63} ${SRC_CPL} ${SRC_ISO8211} ${SRC_DSA} ${SRC_JSON})

IF (QT_ANDROID )
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${OCPN_Core_LIBRARIES} )
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} "-lc++_shared")
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} "-lz" )
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME}  ${Qt_Base}/${Qt_Build}/lib/libQt5Core.so )
    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} "${OCPN_Base}/${OCPN_Build}/libgorp.so" )
ENDIF(QT_ANDROID)


if (MINGW)
  SET(OPENCPN_IMPORT_LIB "${API_DIR}/mingw/libopencpn.dll.a")
elseif (WIN32)
  SET(OPENCPN_IMPORT_LIB "${API_DIR}/msvc/opencpn.lib")
endif ()

IF(WIN32)
  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${CURL_LIBRARIES})
  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${OPENCPN_IMPORT_LIB} )

ENDIF(WIN32)

IF(NOT APPLE)
TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${wxWidgets_LIBRARIES} )
ENDIF(NOT APPLE)



# Install any special data files or binary helpers here....
# The PlugIn library itself and any "data" directory (with contents) gets installed in generic PluginInstall.cmake later

IF(UNIX AND NOT APPLE)
SET(PARENT opencpn)
SET(PREFIX_PLUGINS ${LIB_INSTALL_DIR}/${PARENT})
SET(PREFIX_PARENTBIN bin)
SET(PREFIX_PARENTDATA ${PREFIX_DATA}/${PARENT})


IF (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")
    INSTALL(FILES "buildlinux64/OCPNsenc/OCPNsenc"
       PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
       DESTINATION ${PREFIX_PARENTBIN})
ELSE (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")

    IF (ARCH MATCHES "arm*" )

        IF (ARCH MATCHES "armhf" )
            INSTALL(FILES "buildlinuxarm/OCPNsenc/OCPNsenc"
                PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
                DESTINATION ${PREFIX_PARENTBIN})
        ELSE (ARCH MATCHES "armhf" )
            INSTALL(FILES "buildlinuxarm64/OCPNsenc/OCPNsenc"
                PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
                DESTINATION ${PREFIX_PARENTBIN})
        ENDIF (ARCH MATCHES "armhf" )
    
    ELSE (ARCH MATCHES "arm*" )
        INSTALL(FILES "buildlinux/OCPNsenc/OCPNsenc"
          PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
          DESTINATION ${PREFIX_PARENTBIN})
    ENDIF (ARCH MATCHES "arm*" )

ENDIF (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")
ENDIF(UNIX AND NOT APPLE)


IF(WIN32)
    INSTALL(FILES "buildwin/OCPNsenc/OCPNsenc.exe"
       PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ WORLD_EXECUTE
       DESTINATION "plugins\\\\${PACKAGE_NAME}")

    INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
    SET(INSTALL_DIRECTORY "plugins\\\\${PACKAGE_NAME}")

ENDIF(WIN32)

IF(APPLE)

INSTALL(FILES "buildosx/OCPNsenc/OCPNsenc"
       PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ WORLD_EXECUTE
       DESTINATION "OpenCPN.app/Contents/PlugIns/")

  # We also need to copy the helper utility and required dylibs to the binary build directory so that the PACKAGES scripts will find it.
configure_file(${PROJECT_SOURCE_DIR}/buildosx/OCPNsenc/OCPNsenc
            ${CMAKE_CURRENT_BINARY_DIR}/OCPNsenc COPYONLY)

ENDIF(APPLE)


IF(NOT WIN32)
SET(CPACK_SET_DESTDIR ON)
ENDIF(NOT WIN32)

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "dsr")

INCLUDE("cmake/PluginSetup.cmake")

configure_file(
  ${CMAKE_SOURCE_DIR}/pkg_version.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkg_version.sh
)

set(PLUGIN_NAME ${VERBOSE_NAME}-plugin-${PKG_TARGET}-${PKG_TARGET_VERSION})

configure_file(
  ${CMAKE_SOURCE_DIR}/s63-plugin.xml.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.xml
)

INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginPackage.cmake")

