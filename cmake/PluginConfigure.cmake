##---------------------------------------------------------------------------
## Author:      Sean D'Epagnier
## Copyright:
## License:     GPLv3+
##---------------------------------------------------------------------------

# This should be 2.8.0 to have FindGTK2 module

MESSAGE (STATUS "*** Staging to build ${PACKAGE_NAME} ***")

include  ("VERSION.cmake")

#  Do the version.h configuration into the build output directory,
#  thereby allowing building from a read-only source tree.
IF(NOT SKIP_VERSION_CONFIG)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/version.h.in ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/include/version.h)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/include)
ENDIF(NOT SKIP_VERSION_CONFIG)

SET(PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )


#SET(CMAKE_BUILD_TYPE Debug)
#SET(CMAKE_VERBOSE_MAKEFILE ON)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)

# SET(PROFILING 1)

#  IF NOT DEBUGGING CFLAGS="-O2 -march=native"
IF(NOT MSVC)
 IF(PROFILING)
  ADD_DEFINITIONS( "-Wall -g -fprofile-arcs -ftest-coverage -fexceptions" )
 ELSE(PROFILING)
#  ADD_DEFINITIONS( "-Wall -g -fexceptions" )
 ADD_DEFINITIONS( "-Wall -Wno-unused-result -g -O2 -fexceptions" )
 ENDIF(PROFILING)

 IF(NOT APPLE)
  SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
 ELSE(NOT APPLE)
  SET(CMAKE_SHARED_LINKER_FLAGS "-Wl -undefined dynamic_lookup")
 ENDIF(NOT APPLE)

ENDIF(NOT MSVC)

# Add some definitions to satisfy MS
IF(MSVC)
    ADD_DEFINITIONS(-D__MSVC__)
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE)
ENDIF(MSVC)

IF(NOT DEFINED wxWidgets_USE_FILE)
  SET(wxWidgets_USE_LIBS base core net xml html adv aui)
ENDIF(NOT DEFINED wxWidgets_USE_FILE)


#  QT_ANDROID is a cross-build, so the native FIND_PACKAGE(wxWidgets...) and wxWidgets_USE_FILE is not useful.
#  We add the dependencies manually.
IF(QT_ANDROID)
  ADD_DEFINITIONS(-D__WXQT__)
  ADD_DEFINITIONS(-D__OCPN__ANDROID__)
  ADD_DEFINITIONS(-DOCPN_USE_WRAPPER)
  ADD_DEFINITIONS(-DANDROID)

  SET(CMAKE_CXX_FLAGS "-pthread -fPIC -O2")

  ## Compiler flags
 #   if(CMAKE_COMPILER_IS_GNUCXX)
 #       set(CMAKE_CXX_FLAGS "-O2")        ## Optimize
        set(CMAKE_EXE_LINKER_FLAGS "-s")  ## Strip binary
 #   endif()

  INCLUDE_DIRECTORIES("${Qt_Base}/${Qt_Build}/include/QtCore")
  INCLUDE_DIRECTORIES("${Qt_Base}/${Qt_Build}/include")
  INCLUDE_DIRECTORIES("${Qt_Base}/${Qt_Build}/include/QtWidgets")
  INCLUDE_DIRECTORIES("${Qt_Base}/${Qt_Build}/include/QtGui")
  INCLUDE_DIRECTORIES("${Qt_Base}/${Qt_Build}/include/QtOpenGL")
  INCLUDE_DIRECTORIES("${Qt_Base}/${Qt_Build}/include/QtTest")

  INCLUDE_DIRECTORIES( "${wxQt_Base}/${wxQt_Build}/lib/wx/include/arm-linux-androideabi-qt-unicode-static-3.1")
  INCLUDE_DIRECTORIES("${wxQt_Base}/include")

  ADD_DEFINITIONS(-DQT_WIDGETS_LIB)

ENDIF(QT_ANDROID)


IF(MSYS)
# this is just a hack. I think the bug is in FindwxWidgets.cmake
STRING( REGEX REPLACE "/usr/local" "\\\\;C:/MinGW/msys/1.0/usr/local" wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} )
ENDIF(MSYS)

#  QT_ANDROID is a cross-build, so the native FIND_PACKAGE(OpenGL) is not useful.
#
IF (NOT QT_ANDROID )
FIND_PACKAGE(OpenGL)
IF(OPENGL_GLU_FOUND)

    SET(wxWidgets_USE_LIBS ${wxWidgets_USE_LIBS} gl)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})

    MESSAGE (STATUS "Found OpenGL..." )
    #MESSAGE (STATUS "    Lib: " ${OPENGL_LIBRARIES})
    #MESSAGE (STATUS "    Include: " ${OPENGL_INCLUDE_DIR})
    ADD_DEFINITIONS(-DocpnUSE_GL)
ELSE(OPENGL_GLU_FOUND)
    MESSAGE (STATUS "OpenGL not found..." )
ENDIF(OPENGL_GLU_FOUND)
ENDIF(NOT QT_ANDROID)

#  Building for QT_ANDROID involves a cross-building environment,
#  So the OpenGL include directories, flags, etc must be stated explicitly
#  without trying to locate them on the host build system.
IF(QT_ANDROID)
    ADD_DEFINITIONS(-DocpnUSE_GLES)
    ADD_DEFINITIONS(-DocpnUSE_GL)
#    ADD_DEFINITIONS(-DUSE_GLU_TESS)
    ADD_DEFINITIONS(-DARMHF)

    SET(OPENGLES_FOUND "YES")
    SET(OPENGL_FOUND "YES")
    
      
  SET(USE_GLES2 ON )

  IF(USE_GLES2)
    MESSAGE (STATUS "Using GLESv2 for Android")
    ADD_DEFINITIONS(-DUSE_ANDROID_GLES2)
    ADD_DEFINITIONS(-DUSE_GLSL)
  ENDIF(USE_GLES2)



ENDIF(QT_ANDROID)

IF (NOT QT_ANDROID )
    if(WXWIDGETS_FORCE_VERSION)
        set (wxWidgets_CONFIG_OPTIONS --version=${WXWIDGETS_FORCE_VERSION})
    endif()
    find_package(wxWidgets COMPONENTS ${wxWidgets_USE_LIBS})
    INCLUDE(${wxWidgets_USE_FILE})
ENDIF (NOT QT_ANDROID )

# On Android, PlugIns need a specific linkage set....
IF (QT_ANDROID )
  # These libraries are needed to create PlugIns on Android.

  SET(OCPN_Core_LIBRARIES
        # Presently, Android Plugins are built in the core tree, so the variables {wxQT_BASE}, etc.
        # flow to this module from above.  If we want to build Android plugins out-of-core, this will need improvement.

  
    ${Qt_Base}/${Qt_Build}/lib/libQt5Core.so
    ${Qt_Base}/${Qt_Build}/lib/libQt5OpenGL.so
    ${Qt_Base}/${Qt_Build}/lib/libQt5Widgets.so
    ${Qt_Base}/${Qt_Build}/lib/libQt5Gui.so
    ${Qt_Base}/${Qt_Build}/lib/libQt5AndroidExtras.so

        libGLESv2.so
        libEGL.so
        )

ENDIF(QT_ANDROID)


ADD_DEFINITIONS(-DBUILDING_PLUGIN)


SET(BUILD_SHARED_LIBS TRUE)

FIND_PACKAGE(Gettext REQUIRED)

