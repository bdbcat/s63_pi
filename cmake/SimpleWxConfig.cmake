macro(SimpleWxConfig)
 if(NOT QT_ANDROID)
    set( wxWidgets_USE_DEBUG OFF)
    set( wxWidgets_USE_UNICODE ON)
    set( wxWidgets_USE_UNIVERSAL OFF)
    set( wxWidgets_USE_STATIC OFF)
    if(OCPN_WXWIDGETS_FORCE_VERSION)
      set (wxWidgets_CONFIG_OPTIONS --version=${OCPN_WXWIDGETS_FORCE_VERSION})
    endif()
    if(MSVC)
        # Exclude wxexpat.lib, since we use our own version.
        # Other things are excluded as well, but we don't need them
        SET(wxWidgets_EXCLUDE_COMMON_LIBRARIES TRUE)
    endif(MSVC)
    set(wxWidgets_USE_LIBS base core xml html)

    if(FORCE_GTK3)
      set (wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} --toolkit=gtk3)
    else(FORCE_GTK3)
      find_package(GTK2)
      if(GTK2_FOUND)
        set(wxWidgets_CONFIG_OPTIONS
            ${wxWidgets_CONFIG_OPTIONS} --toolkit=gtk2)
      else ()
        find_package(GTK3)
        if(GTK3_FOUND)
            set(wxWidgets_CONFIG_OPTIONS
                ${wxWidgets_CONFIG_OPTIONS} --toolkit=gtk3)
        endif ()
      endif ()
    endif(FORCE_GTK3)



      find_package(wxWidgets REQUIRED)
    INCLUDE(${wxWidgets_USE_FILE})
 endif(NOT QT_ANDROID)
endmacro(SimpleWxConfig)
