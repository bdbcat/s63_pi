#
# Install the pre-compiled oexserverd DRM module + helper libraries.
#
macro (install_libs libs)
  if (APPLE)
    #set(_oe_libdir "OpenCPN.app/Contents/SharedSupport/plugins/${PACKAGE_NAME}")
    set(_ox_libdir "OpenCPN.app/Contents/PlugIns/")
  elseif (WIN32)
    set(_ox_libdir "plugins")
  elseif (UNIX)
    set(_ox_libdir "lib/opencpn/")
  endif ()
  foreach(lib ${ARGV})
    install(
      FILES ${lib}
      PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE
                  WORLD_READ WORLD_EXECUTE
      DESTINATION ${CMAKE_INSTALL_PREFIX}/${_ox_libdir}
    )
  endforeach ()
endmacro ()

if (QT_ANDROID)
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/android/OCPNsenc
    ${CMAKE_INSTALL_PREFIX}/bin/OCPNsenc COPYONLY
  )
elseif (APPLE)
  install_libs("osx/OCPNsenc")
elseif (UNIX AND ARCH  STREQUAL "x86_64")
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linux64/OCPNsenc
    ${CMAKE_INSTALL_PREFIX}/bin/OCPNsenc COPYONLY
  )
elseif (UNIX AND ARCH STREQUAL "armhf")
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linuxarm/OCPNsenc
    ${CMAKE_INSTALL_PREFIX}/bin/OCPNsenc COPYONLY
  )
elseif (UNIX AND ARCH STREQUAL "arm64")
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linuxarm64/OCPNsenc
    ${CMAKE_INSTALL_PREFIX}/bin/OCPNsenc COPYONLY
  )
elseif (UNIX)
  # 32-bit amd64: is this really useful?
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/linux/OCPNsenc
    ${CMAKE_INSTALL_PREFIX}/bin/OCPNsenc COPYONLY
  )
elseif (WIN32)
  file(GLOB_RECURSE vcdll_files "win/*.dll")
  message(STATUS "GLOB vcdll_files: ${vcdll_files}")
  install_libs(${vcdll_files} ${CMAKE_CURRENT_LIST_DIR}/win/OCPNsenc.exe)
endif ()
