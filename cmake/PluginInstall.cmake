##---------------------------------------------------------------------------
## Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
## Copyright:   2014
## License:     GPLv3+
##---------------------------------------------------------------------------

IF(NOT APPLE)
  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${wxWidgets_LIBRARIES} ${PLUGINS_LIBS} )
ENDIF(NOT APPLE)

IF(WIN32)
  SET(PARENT "opencpn")

  IF(MSVC)
#    TARGET_LINK_LIBRARIES(${PACKAGE_NAME}
#	gdiplus.lib
#	glu32.lib)
    TARGET_LINK_LIBRARIES(${PACKAGE_NAME} ${OPENGL_LIBRARIES})

  ENDIF(MSVC)

  IF(MINGW)
# assuming wxwidgets is compiled with unicode, this is needed for mingw headers
    ADD_DEFINITIONS( " -DUNICODE" )
    TARGET_LINK_LIBRARIES(${PACKAGE_NAME} ${OPENGL_LIBRARIES})
    SET( CMAKE_SHARED_LINKER_FLAGS "-L../buildwin" )
  ENDIF(MINGW)

  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${OPENCPN_IMPORT_LIB} )
ENDIF(WIN32)

IF(UNIX)
 IF(PROFILING)
  find_library(GCOV_LIBRARY
    NAMES
    gcov
    PATHS
    /usr/lib/gcc/i686-pc-linux-gnu/4.7
    )

  SET(PLUGINS_LIBS ${PLUGINS_LIBS} ${GCOV_LIBRARY})
 ENDIF(PROFILING)
ENDIF(UNIX)

IF(APPLE)
  INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME LIBRARY DESTINATION OpenCPN.app/Contents/PlugIns)
  IF(EXISTS ${PROJECT_SOURCE_DIR}/data)
    INSTALL(DIRECTORY data DESTINATION OpenCPN.app/Contents/SharedSupport/plugins/${PACKAGE_NAME})
  ENDIF()
  
  FIND_PACKAGE(ZLIB REQUIRED)
  TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${ZLIB_LIBRARIES} )

ENDIF(APPLE)

IF(UNIX AND NOT APPLE)
#    FIND_PACKAGE(BZip2 REQUIRED)
#    INCLUDE_DIRECTORIES(${BZIP2_INCLUDE_DIR})
#    FIND_PACKAGE(ZLIB REQUIRED)
#    INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
#    TARGET_LINK_LIBRARIES( ${PACKAGE_NAME} ${BZIP2_LIBRARIES} ${ZLIB_LIBRARY} )
ENDIF(UNIX AND NOT APPLE)

SET(PARENT opencpn)

# Based on code from nohal
IF (NOT WIN32)
  # default
  SET (ARCH "i386")
  SET (LIB_INSTALL_DIR "lib")
  IF (EXISTS /etc/debian_version)
    SET (PACKAGE_FORMAT "DEB")
    SET (PACKAGE_DEPS "libc6, libwxgtk3.0-0, wx3.0-i18n, libglu1-mesa (>= 7.0.0), libgl1-mesa-glx (>= 7.0.0), zlib1g, bzip2, libtinyxml2.6.2, libportaudio2")
    SET (PACKAGE_RECS "xcalib,xdg-utils")
    SET (LIB_INSTALL_DIR "lib")
    IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "x86_64")
#      SET (LIB_INSTALL_DIR "lib64")
    ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "i386")
    ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET(TENTATIVE_PREFIX "/usr/local")
  ENDIF (EXISTS /etc/debian_version)
  IF (EXISTS /etc/redhat-release)
    SET (PACKAGE_FORMAT "RPM")
    #    SET (PACKAGE_DEPS  "wxGTK mesa-libGLU mesa-libGL gettext zlib bzip2 portaudio")
    IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "x86_64")
      SET (LIB_INSTALL_DIR "lib64")
    ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "i386")
      SET (LIB_INSTALL_DIR "lib")
    ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  ENDIF (EXISTS /etc/redhat-release)
  IF (EXISTS /etc/suse-release OR EXISTS /etc/SuSE-release)
    SET (PACKAGE_FORMAT "RPM")
    #    SET (PACKAGE_DEPS  "libwx_baseu-2_8-0-wxcontainer MesaGLw libbz2-1 portaudio")
    IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "x86_64")
      SET (LIB_INSTALL_DIR "lib64")
    ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "i386")
      SET (LIB_INSTALL_DIR "lib")
    ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  ENDIF (EXISTS /etc/suse-release OR EXISTS /etc/SuSE-release)
  IF (EXISTS /etc/gentoo-release)
    SET (LIB_INSTALL_DIR "lib${LIB_SUFFIX}")
  ENDIF (EXISTS /etc/gentoo-release)
  IF(APPLE)
    IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
#      IF (CMAKE_OSX_64)
        SET(ARCH "x86_64")
#      ENDIF (CMAKE_OSX_64)
    ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  ENDIF()
ELSE (NOT WIN32)
  # On WIN32 probably CMAKE_SIZEOF_VOID_P EQUAL 8, but we don't use it at all now...
  SET (ARCH "i386")
ENDIF (NOT WIN32)

IF (NOT CMAKE_INSTALL_PREFIX)
    SET(CMAKE_INSTALL_PREFIX ${TENTATIVE_PREFIX})
ENDIF (NOT CMAKE_INSTALL_PREFIX)

MESSAGE (STATUS "*** Will install to ${CMAKE_INSTALL_PREFIX}  ***")
SET(PREFIX_DATA share)
SET(PREFIX_PKGDATA ${PREFIX_DATA}/${PACKAGE_NAME})
#SET(PREFIX_LIB "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}")
SET(PREFIX_LIB "${LIB_INSTALL_DIR}")

IF(WIN32)
    MESSAGE (STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
    SET(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/../OpenCPN)
  IF(CMAKE_CROSSCOMPILING)
    INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
    SET(INSTALL_DIRECTORY "plugins/${PACKAGE_NAME}")
  ELSE(CMAKE_CROSSCOMPILING)
    INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION "plugins")
    SET(INSTALL_DIRECTORY "plugins\\\\${PACKAGE_NAME}")
  ENDIF(CMAKE_CROSSCOMPILING)

  IF(EXISTS ${PROJECT_SOURCE_DIR}/data)
    INSTALL(DIRECTORY data DESTINATION "${INSTALL_DIRECTORY}")
    MESSAGE (STATUS "Install Data: ${INSTALL_DIRECTORY}")
  ENDIF(EXISTS ${PROJECT_SOURCE_DIR}/data)
  
  #INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.xml DESTINATION "\\\\"  RENAME "metadata.xml") 

  
  #fix for missing dll's 
  #FILE(GLOB gtkdll_files "${CMAKE_CURRENT_SOURCE_DIR}/buildwin/gtk/*.dll")
  #    INSTALL(FILES ${gtkdll_files} DESTINATION ".")
  #    FILE(GLOB expatdll_files "${CMAKE_CURRENT_SOURCE_DIR}/buildwin/expat-2.1.0/*.dll")
  #    INSTALL(FILES ${expatdll_files} DESTINATION ".")

ENDIF(WIN32)

IF(UNIX AND NOT APPLE)

  SET(PREFIX_PLUGINS ${LIB_INSTALL_DIR}/${PARENT})
  SET(PREFIX_PARENTDATA ${PREFIX_DATA}/${PARENT})

  INSTALL(TARGETS ${PACKAGE_NAME} RUNTIME LIBRARY DESTINATION ${PREFIX_PLUGINS})

  IF(EXISTS ${PROJECT_SOURCE_DIR}/data)
    INSTALL(DIRECTORY data DESTINATION ${PREFIX_PARENTDATA}/plugins/${PACKAGE_NAME})
  ENDIF()
  
  #INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.xml DESTINATION "/" RENAME "metadata.xml") 
ENDIF(UNIX AND NOT APPLE)

IF(APPLE)
    # For Apple build, we need to copy the "data" directory contents to the build directory, so that the packager can pick them up.
    if (NOT EXISTS "${PROJECT_BINARY_DIR}/data/")
        file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/data/")
        message ("Generating data directory")
        endif ()

   FILE(GLOB PACKAGE_DATA_FILES ${CMAKE_SOURCE_DIR}/data/*)

   FOREACH (_currentDataFile ${PACKAGE_DATA_FILES})
        MESSAGE (STATUS "copying: ${_currentDataFile}" )
        configure_file(${_currentDataFile} ${CMAKE_CURRENT_BINARY_DIR}/data COPYONLY)
   ENDFOREACH (_currentDataFile )
   
 
ENDIF(APPLE)
