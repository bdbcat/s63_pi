##---------------------------------------------------------------------------
## Author:      bdbcat aka. dsr (Dave Register)
## Copyright:
## License:     wxWidgets License
##---------------------------------------------------------------------------


# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)
# This should be 2.8.0 to have FindGTK2 module
IF (COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 OLD)
  CMAKE_POLICY(SET CMP0005 OLD)
  CMAKE_POLICY(SET CMP0011 OLD)
ENDIF (COMMAND cmake_policy)

PROJECT(s63_pi)

SET(PACKAGE_NAME s63_pi)
SET(PLUGIN_SOURCE_DIR .)
MESSAGE (STATUS "*** Building ${PACKAGE_NAME} ***")

SET(VERSION_MAJOR "1")
SET(VERSION_MINOR "4")
SET(VERSION_PATCH "0")
SET(PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )


SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_VERBOSE_MAKEFILE ON)

IF (PREFIX)
 SET(CMAKE_INSTALL_PREFIX ${PREFIX})
ENDIF (PREFIX)

MESSAGE (STATUS "*** Will install to ${CMAKE_INSTALL_PREFIX}  ***")

SET(PREFIX_BIN bin)
SET(PREFIX_INCLUDE include)
SET(PREFIX_DATA share)

IF (NOT WIN32)
  # default
  SET (ARCH "i386")
  IF (UNIX AND NOT APPLE)
    IF (PREFIX)
        SET(CMAKE_INSTALL_PREFIX ${PREFIX})
    ELSE (PREFIX )
        SET(CMAKE_INSTALL_PREFIX "/usr/")
    ENDIF (PREFIX)

    IF (EXISTS /etc/debian_version)
      SET (PACKAGE_FORMAT "DEB")
      SET (PACKAGE_DEPS "libc6, libwxgtk2.8-0 (>= 2.8.7.1), libglu1-mesa (>= 7.0.0), libgl1-mesa-glx (>= 7.0.0), zlib1g, bzip2")
      SET (LIB_INSTALL_DIR "lib")
      IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "amd64")
      ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "i386")
      ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
    ENDIF (EXISTS /etc/debian_version)
    IF (EXISTS /etc/redhat-release)
      SET (PACKAGE_FORMAT "RPM")
      SET (PACKAGE_DEPS  "opencpn zlib bzip2")
      IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "x86_64")
        SET (LIB_INSTALL_DIR "lib64")
      ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
        SET (ARCH "i386")
        SET (LIB_INSTALL_DIR "lib")
      ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
    ENDIF (EXISTS /etc/redhat-release)
  ENDIF (UNIX AND NOT APPLE)
endif(NOT WIN32)

MESSAGE (STATUS "*** Build Architecture is ${ARCH}")


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/cpl)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/dsa)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})


IF(WIN32)
#      INCLUDE_DIRECTORIES("U:/Projects/opencpn_sf/opencpn/include")
ENDIF(WIN32)

IF(UNIX)
#     INCLUDE_DIRECTORIES("/home/dsr/Projects/opencpn_sf/opencpn/include")
ENDIF(UNIX)

# require proper c++
#ADD_DEFINITIONS( "-Wall -ansi -pedantic -Wno-variadic-macros" )
#TODO: Should we use  -fno-stack-protector
#  IF NOT DEBUGGING CFLAGS="-O2 -march=native"
IF(NOT WIN32)
 ADD_DEFINITIONS( "-Wall -g -fexceptions -fvisibility=hidden" )

 IF(NOT APPLE)
  SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
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

SET(wxWidgets_USE_LIBS base core net )

FIND_PACKAGE(OpenGL)
IF(OPENGL_GLU_FOUND)

    SET(wxWidgets_USE_LIBS ${wxWidgets_USE_LIBS} gl )
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})

    MESSAGE (STATUS "Found OpenGL..." )
    MESSAGE (STATUS "    Lib: " ${OPENGL_LIBRARIES})
    MESSAGE (STATUS "    Include: " ${OPENGL_INCLUDE_DIR})
    ADD_DEFINITIONS(-DocpnUSE_GL)

ELSE(OPENGL_GLU_FOUND)
    MESSAGE (STATUS "OpenGL not found..." )
ENDIF(OPENGL_GLU_FOUND)


SET(BUILD_SHARED_LIBS TRUE)
#SET( wxWidgets_USE_DEBUG ON)

FIND_PACKAGE(wxWidgets REQUIRED)

INCLUDE(${wxWidgets_USE_FILE})

FIND_PACKAGE(Gettext REQUIRED)

# For convenience we define the sources as a variable. You can add
# header files and cpp/c files and CMake will sort them out



SET(SRC_S63
            src/s63_pi.h
            src/s63_pi.cpp
            src/s63chart.cpp
            src/s63chart.h
            src/mygeom63.h
            src/mygeom63.cpp
            src/tri.c
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


ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_S63} ${SRC_CPL} ${SRC_ISO8211} ${SRC_DSA})
#ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_S63} )

IF(WIN32)
#  SET(OPENCPN_IMPORT_LIB "opencpn.lib")
#  SET(OPENCPN_IMPORT_LIB "E:/opencpn_cmake2010/${CMAKE_CFG_INTDIR}/opencpn.lib")
  SET(OPENCPN_IMPORT_LIB "C:/Projects/opencpn/build/${CMAKE_CFG_INTDIR}/opencpn.lib")
  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${OPENCPN_IMPORT_LIB} )
#  ADD_DEPENDENCIES(${PACKAGE_NAME} ${PARENT})
ENDIF(WIN32)

IF(NOT APPLE)
TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${wxWidgets_LIBRARIES} )
ENDIF(NOT APPLE)




IF(UNIX AND NOT APPLE)
SET(PARENT opencpn)
SET(PREFIX_PLUGINS ${LIB_INSTALL_DIR}/${PARENT})
SET(PREFIX_PARENTBIN bin)
INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME LIBRARY DESTINATION ${PREFIX_PLUGINS})

IF (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")
INSTALL(FILES "buildlinux64/OCPNsenc/OCPNsenc"
       PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
       DESTINATION ${PREFIX_PARENTBIN})
ELSE (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")
INSTALL(FILES "buildlinux/OCPNsenc/OCPNsenc"
       PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
       DESTINATION ${PREFIX_PARENTBIN})
ENDIF (ARCH MATCHES "amd64" OR ARCH MATCHES "x86_64")

ENDIF(UNIX AND NOT APPLE)

IF(WIN32)
#INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "c:/Program Files/OpenCPN/plugins")
#INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "E:/opencpn_cmake2010/plugins")
INSTALL(FILES "buildwin/OCPNsenc/OCPNsenc.exe"
       PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ WORLD_EXECUTE
       DESTINATION ".")

INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION ".")
ENDIF(WIN32)

IF(APPLE)
SET(CMAKE_INSTALL_PREFIX "/tmp")

INSTALL(FILES "buildosx/OCPNsenc/OCPNsenc"
       PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ WORLD_EXECUTE
       DESTINATION "s63_pi/s63_pi")

INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME LIBRARY DESTINATION "s63_pi/s63_pi")
ENDIF(APPLE)

# find src/ -name \*.cpp -or -name \*.c -or -name \*.h -or -name \*.hpp >po/POTFILES.in
FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
IF (GETTEXT_XGETTEXT_EXECUTABLE)
ADD_CUSTOM_COMMAND(
   OUTPUT po/${PACKAGE_NAME}.pot.dummy
   COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} --force-po --package-name=${PACKAGE_NAME} --package-version="${PACKAGE_VERSION}" --output=po/${PACKAGE_NAME}.pot  --keyword=_ --width=80 --files-from=${CMAKE_CURRENT_SOURCE_DIR}/po/POTFILES.in
   DEPENDS po/POTFILES.in po/${PACKAGE_NAME}.pot
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
   COMMENT "s63-pot-update [${PACKAGE_NAME}]: Generated pot file."
)
ADD_CUSTOM_TARGET(s63-pot-update COMMENT "s63-pot-update: Done." DEPENDS po/${PACKAGE_NAME}.pot.dummy)

ENDIF(GETTEXT_XGETTEXT_EXECUTABLE )

MACRO(GETTEXT_UPDATE_PO _potFile)
   SET(_poFiles ${_potFile})
   GET_FILENAME_COMPONENT(_absPotFile ${_potFile} ABSOLUTE)

   FOREACH (_currentPoFile ${ARGN})
      GET_FILENAME_COMPONENT(_absFile ${_currentPoFile} ABSOLUTE)
      GET_FILENAME_COMPONENT(_poBasename ${_absFile} NAME_WE)

      ADD_CUSTOM_COMMAND(
         OUTPUT ${_absFile}.dummy
         COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --width=80 --strict --quiet --update --backup=none --no-location -s ${_absFile} ${_absPotFile}
         DEPENDS ${_absPotFile} ${_absFile}
         COMMENT "s63-po-update [${_poBasename}]: Updated po file."
      )

      SET(_poFiles ${_poFiles} ${_absFile}.dummy)

   ENDFOREACH (_currentPoFile )

   ADD_CUSTOM_TARGET(s63-po-update COMMENT "s63-po-update: Done." DEPENDS ${_poFiles})
ENDMACRO(GETTEXT_UPDATE_PO)

IF (GETTEXT_MSGMERGE_EXECUTABLE)
FILE(GLOB PACKAGE_PO_FILES ${CMAKE_SOURCE_DIR}/po/*.po)
GETTEXT_UPDATE_PO(po/${PACKAGE_NAME}.pot ${PACKAGE_PO_FILES})
ENDIF(GETTEXT_MSGMERGE_EXECUTABLE)

SET(_gmoFiles)
MACRO(GETTEXT_BUILD_MO  _poFile _lang)
      GET_FILENAME_COMPONENT(_absFile ${_poFile} ABSOLUTE)
      GET_FILENAME_COMPONENT(_poBasename ${_absFile} NAME_WE)
      SET(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_poBasename}.mo)

IF(APPLE)
      ADD_CUSTOM_COMMAND(
         OUTPUT ${_gmoFile}
         COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --check -o ${_gmoFile} ${_absFile}
         COMMAND ${CMAKE_COMMAND} -E copy ${_gmoFile} "Resources/${_lang}.lproj/opencpn-${PACKAGE_NAME}.mo"
         DEPENDS ${_absFile}
         COMMENT "s63-i18n [${_poBasename}]: Created mo file."
      )
ELSE(APPLE)        
      ADD_CUSTOM_COMMAND(
         OUTPUT ${_gmoFile}
         COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --check -o ${_gmoFile} ${_absFile}
         COMMAND ${CMAKE_COMMAND} -E copy ${_gmoFile} "Resources/${_lang}.lproj/${PACKAGE_NAME}.mo"
         DEPENDS ${_absFile}
         COMMENT "s63-i18n [${_poBasename}]: Created mo file."
      )
ENDIF(APPLE)         


IF(APPLE)
      INSTALL(FILES ${_gmoFile} DESTINATION ${CMAKE_INSTALL_PREFIX}/s63_pi/s63_pi/Resources/${_lang}.lproj RENAME opencpn-${PACKAGE_NAME}.mo )
ELSE(APPLE)
      INSTALL(FILES ${_gmoFile} DESTINATION ${PREFIX_DATA}/locale/${_lang}/LC_MESSAGES RENAME opencpn-${PACKAGE_NAME}.mo PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
ENDIF(APPLE)

      SET(_gmoFiles ${_gmoFiles} ${_gmoFile})
ENDMACRO(GETTEXT_BUILD_MO)

if(GETTEXT_MSGFMT_EXECUTABLE)
# Could use a glob here..
GETTEXT_BUILD_MO(${CMAKE_SOURCE_DIR}/po/${PACKAGE_NAME}_fr_FR.po fr)
GETTEXT_BUILD_MO(${CMAKE_SOURCE_DIR}/po/${PACKAGE_NAME}_es_ES.po es)
GETTEXT_BUILD_MO(${CMAKE_SOURCE_DIR}/po/${PACKAGE_NAME}_de_DE.po de)
GETTEXT_BUILD_MO(${CMAKE_SOURCE_DIR}/po/${PACKAGE_NAME}_et_ET.po et)
GETTEXT_BUILD_MO(${CMAKE_SOURCE_DIR}/po/${PACKAGE_NAME}_pl_PL.po pl)
GETTEXT_BUILD_MO(${CMAKE_SOURCE_DIR}/po/${PACKAGE_NAME}_ru_RU.po ru)


#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_cs_CZ.po cs)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_da_DK.po da)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_it_IT.po it)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_nl_NL.po nl)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_sv_SE.po sv)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_pt_PT.po pt_PT)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_pt_BR.po pt_BR)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_nb_NO.po nb_NO)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_tr_TR.po tr_TR)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_fi_FI.po fi_FI)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_el_GR.po el_GR)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_zh_TW.po zh_TW)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_hu_HU.po hu_HU)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_gl_ES.po gl_ES)
#    GETTEXT_BUILD_MO(po/${PACKAGE_NAME}_ca_ES.po ca_ES)

   ADD_CUSTOM_TARGET(s63-i18n COMMENT "s63-i18n: Done." DEPENDS ${_gmoFiles})
   ADD_DEPENDENCIES(${PACKAGE_NAME} s63-i18n)
ENDIF(GETTEXT_MSGFMT_EXECUTABLE)



# build a CPack driven installer package
#include (InstallRequiredSystemLibraries)


SET(CPACK_PACKAGE_NAME "S63_PI")
SET(CPACK_PACKAGE_VENDOR "opencpn.org")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_NAME} ${PACKAGE_VERSION})
SET(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
SET(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${PACKAGE_NAME};ALL;/")
SET(CPACK_PACKAGE_EXECUTABLES ${PACKAGE_NAME} "S63_pi")



IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.

  SET(CPACK_NSIS_INSTALLED_ICON_NAME "s63_pi.dll")
  SET(CPACK_NSIS_PACKAGE_NAME_LC "s63_pi")

#  These lines set the name of the Windows Start Menu shortcut and the icon that goes with it
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${PACKAGE_VERSION}")
  SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\opencpn.ico")

  SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}_setup" )

  SET(CPACK_NSIS_DIR "${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode")  #Gunther
  SET(CPACK_BUILDWIN_DIR "${CMAKE_SOURCE_DIR}/buildwin")  #Gunther

ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY ${PACKAGE_NAME})
ENDIF(WIN32 AND NOT UNIX)

SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/data/license.txt")
IF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README")
#    MESSAGE(STATUS "Using generic cpack package description file.")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
    SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
ENDIF ()
SET(CPACK_SOURCE_GENERATOR "TGZ")

# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
set(CPACK_SOURCE_IGNORE_FILES
"\\\\.cvsignore$"
"^${CMAKE_CURRENT_SOURCE_DIR}.*/CVS/"
"^${CMAKE_CURRENT_SOURCE_DIR}/build*"
"^${CPACK_PACKAGE_INSTALL_DIRECTORY}/*"
)

IF(APPLE)
#  SET(CPACK_GENERATOR "Bundle")
#  SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
#  SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/buildosx/opencpn.icns")
#  SET(CPACK_BUNDLE_NAME "${PACKAGE_NAME}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
#  SET(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/buildosx/opencpn.icns")
#  SET(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/buildosx/Info.plist")

#  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/buildosx/README.txt")
#  SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/buildosx/README.txt")

#  SET(CPACK_PACKAGING_INSTALL_PREFIX "/")

ENDIF(APPLE)


IF(UNIX AND NOT APPLE)
#    INCLUDE(UseRPMTools)
#    IF(RPMTools_FOUND)
#        RPMTools_ADD_RPM_TARGETS(packagename ${CMAKE_SOURCE_DIR}/package.spec)
#    ENDIF(RPMTools_FOUND)


    SET(CPACK_GENERATOR ${PACKAGE_FORMAT})
    SET(CPACK_PACKAGE_CONTACT "David S. Register ")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS ${PACKAGE_DEPS})
    SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS ${PACKAGE_RECS})
    SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCH})
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
    SET(CPACK_DEBIAN_PACKAGE_SECTION "misc")
#    SET(CPACK_RPM_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
#    SET(CPACK_RPM_PACKAGE_ARCHITECTURE  ${ARCH})
#    SET(CPACK_RPM_PACKAGE_REQUIRES  ${PACKAGE_DEPS})
#    SET(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_SOURCE_DIR}/opencpn.spec.in")
#    SET(CPACK_OPENCPN_RPM_BINDIR "${CMAKE_INSTALL_PREFIX}/${PREFIX_BIN}")
#    SET(CPACK_OPENCPN_RPM_LIBDIR "${PREFIX_LIB}")
#    SET(CPACK_OPENCPN_RPM_DATADIR "${CMAKE_INSTALL_PREFIX}/${PREFIX_DATA}")
    SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "S63 Chart PlugIn for OpenCPN")
    SET(CPACK_PACKAGE_DESCRIPTION "S63 Chart PlugIn for OpenCPN")
    SET(CPACK_SET_DESTDIR ON)
    SET(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}_${PACKAGE_VERSION}-${PACKAGE_RELEASE}_${ARCH}" )


ENDIF(UNIX AND NOT APPLE)

IF(WIN32 AND NOT UNIX)
# configure_file(${CMAKE_SOURCE_DIR}/src/opencpn.rc.in ${CMAKE_SOURCE_DIR}/src/opencpn.rc)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_GERMAN.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_GERMAN.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_FRENCH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_FRENCH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_CZECH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_CZECH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_DANISH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_DANISH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_SPANISH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_SPANISH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_ITALIAN.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_ITALIAN.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_DUTCH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_DUTCH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_POLISH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_POLISH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_PORTUGUESEBR.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_PORTUGUESEBR.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_PORTUGUESE.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_PORTUGUESE.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_RUSSIAN.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_RUSSIAN.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_SWEDISH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_SWEDISH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_FINNISH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_FINNISH.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_NORWEGIAN.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_NORWEGIAN.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_CHINESETW.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_CHINESETW.nsh" @ONLY)
 configure_file("${CMAKE_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_TURKISH.nsh.in" "${CMAKE_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_TURKISH.nsh" @ONLY)
ENDIF(WIN32 AND NOT UNIX)


# this dummy target is necessary to make sure the ADDITIONAL_MAKE_CLEAN_FILES directive is executed.
# apparently, the base CMakeLists.txt file must have "some" target to activate all the clean steps.
#ADD_CUSTOM_TARGET(dummy COMMENT "dummy: Done." DEPENDS ${PACKAGE_NAME})


INCLUDE(CPack)

IF(APPLE)

 ADD_CUSTOM_COMMAND(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.dmg
   COMMAND ${CMAKE_SOURCE_DIR}/buildosx/create-dmg ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.dmg ${CMAKE_INSTALL_PREFIX}/s63_pi/
   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
   COMMENT "create-dmg [${PACKAGE_NAME}]: Generating dmg file."
)

 ADD_CUSTOM_TARGET(create-dmg COMMENT "create-dmg: Done."
 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.dmg)

 #  Copy a bunch of files so the Packages installer builder can find them
 #  relative to ${CMAKE_CURRENT_BINARY_DIR}
 #  This avoids absolute paths in the S63.pkgproj file

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/OCPNsenc/OCPNsenc
            ${CMAKE_CURRENT_BINARY_DIR}/OCPNsenc COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S-63_plug-in_explained-cz.rtf
            ${CMAKE_CURRENT_BINARY_DIR}/S-63_plug-in_explained-cz.rtf COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S-63_plug-in_erklaert-0_4.rtf
            ${CMAKE_CURRENT_BINARY_DIR}/S-63_plug-in_erklaert-0_4.rtf COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S-63_plug-in_explained-0_4en.rtf
            ${CMAKE_CURRENT_BINARY_DIR}/S-63_plug-in_explained-0_4en.rtf COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S-63_plug-in_explained-es.rtf
            ${CMAKE_CURRENT_BINARY_DIR}/S-63_plug-in_explained-es.rtf COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S-63_plug-in_explained-fr.rtf
            ${CMAKE_CURRENT_BINARY_DIR}/S-63_plug-in_explained-fr.rtf COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S-63_plug-in_explained-it.rtf
            ${CMAKE_CURRENT_BINARY_DIR}/S-63_plug-in_explained-it.rtf COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S-63_plug-in_explained-nl.rtf
            ${CMAKE_CURRENT_BINARY_DIR}/S-63_plug-in_explained-nl.rtf COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/license.txt
            ${CMAKE_CURRENT_BINARY_DIR}/license.txt COPYONLY)

 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/Readme.txt
            ${CMAKE_CURRENT_BINARY_DIR}/Readme.txt COPYONLY)

 # Patch the pkgproj.in file to make the output package name conform to S63-Plugin_x.x.x.pkg format
 #  Key is:
 #  <key>NAME</key>
 #  <string>S63-Plugin_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}</string>

 #  This is the apackaging app that is used
 # http://www.macupdate.com/app/mac/34613/packages
 
 configure_file(${CMAKE_SOURCE_DIR}/buildosx/S63-InstallOSX/S63.pkgproj.in
            ${CMAKE_CURRENT_BINARY_DIR}/S63.pkgproj)

 ADD_CUSTOM_COMMAND(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/S63-Plugin.pkg
   COMMAND /usr/local/bin/packagesbuild -F ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/S63.pkgproj
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   DEPENDS s63_pi
   COMMENT "create-pkg [${PACKAGE_NAME}]: Generating pkg file."
)

 ADD_CUSTOM_TARGET(create-pkg COMMENT "create-pkg: Done."
 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/S63-Plugin.pkg )


ENDIF(APPLE)
