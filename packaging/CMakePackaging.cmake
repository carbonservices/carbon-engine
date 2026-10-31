# ─────────────────────────────────────────────────────────────
# Carbon Services - Carbon ESG & Carbon Engine Packaging Setup
# ─────────────────────────────────────────────────────────────

include(GNUInstallDirs)

# 1. Target Installation Rules
install(TARGETS carbon-engine-client
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

install(TARGETS carbon-engine
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

if(TARGET carbon-tray)
    install(TARGETS carbon-tray
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
    )
endif()

# 2. Asset & Shared Library Installation Rules
if(WIN32)
    # Windows dynamic dependencies & assets
    if(EXISTS "${pdfium_SOURCE_DIR}/bin/pdfium.dll")
        install(FILES "${pdfium_SOURCE_DIR}/bin/pdfium.dll"
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    # Explicitly install Qt TLS plugins into bin/tls for reliable HTTPS downloads
    # NOTE: Qt TLS plugins end in 'backend.dll' which matches generic '*d.dll' patterns.
    # We explicitly match '*backend.dll' (release) and '*backendd.dll' (debug) to prevent accidental exclusion.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    if(_qmake_executable)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        get_filename_component(_qt_root "${_qt_bin_dir}" DIRECTORY)
    endif()

    if(EXISTS "${_qt_root}/plugins/tls")
        set(_tls_plugins_src "${_qt_root}/plugins/tls")
    elseif(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tls")
        set(_tls_plugins_src "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tls")
    endif()

    if(_tls_plugins_src)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            install(DIRECTORY "${_tls_plugins_src}/"
                DESTINATION ${CMAKE_INSTALL_BINDIR}/tls
                FILES_MATCHING
                PATTERN "*backendd.dll"
            )
        else()
            install(DIRECTORY "${_tls_plugins_src}/"
                DESTINATION ${CMAKE_INSTALL_BINDIR}/tls
                FILES_MATCHING
                PATTERN "*backend.dll"
            )
        endif()
    endif()

    # Install deployed Qt runtime DLLs, plugins, resources, and complete QML tree into bin
    install(DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PATTERN "*d.dll" EXCLUDE
        PATTERN "*d.exe" EXCLUDE
        PATTERN "*debug*" EXCLUDE
        PATTERN "carbon-engine*" EXCLUDE
        PATTERN "carbon-tray*" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "*.exp" EXCLUDE
        PATTERN "*.lib" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.iobj" EXCLUDE
        PATTERN "*.ipdb" EXCLUDE
        PATTERN "*.idb" EXCLUDE
        PATTERN "*.log" EXCLUDE
        PATTERN "*.part" EXCLUDE
        PATTERN "*.sqlite*" EXCLUDE
        PATTERN "template.html" EXCLUDE
        PATTERN "qmltooling" EXCLUDE
        PATTERN "tls" EXCLUDE
    )

    install(FILES "${CMAKE_SOURCE_DIR}/src/libs/common/templates/template.html"
        DESTINATION templates
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/tessdata/eng.traineddata")
        install(FILES "${CMAKE_SOURCE_DIR}/tessdata/eng.traineddata"
            DESTINATION ${CMAKE_INSTALL_BINDIR}/tessdata
        )
    endif()
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../lib:$ORIGIN")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    # Linux dynamic dependencies & desktop integration
    if(EXISTS "${pdfium_SOURCE_DIR}/lib/libpdfium.so")
        install(FILES "${pdfium_SOURCE_DIR}/lib/libpdfium.so"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()

    install(FILES "${CMAKE_SOURCE_DIR}/src/libs/common/templates/template.html"
        DESTINATION share/carbon-esg/templates
    )
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/carbon-esg.desktop"
        DESTINATION share/applications
    )
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/linux/carbon-esg.png"
        DESTINATION share/icons/hicolor/256x256/apps
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/tessdata/eng.traineddata")
        install(FILES "${CMAKE_SOURCE_DIR}/tessdata/eng.traineddata"
            DESTINATION share/tessdata
        )
    endif()

    # Bundle Qt 6 runtime libraries, plugins, QML modules, and resources for self-contained Linux deployment
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    if(_qmake_executable)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        get_filename_component(_qt_root "${_qt_bin_dir}" DIRECTORY)

        install(DIRECTORY "${_qt_root}/lib/"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FILES_MATCHING
            PATTERN "libQt6*.so*"
            PATTERN "cmake" EXCLUDE
            PATTERN "pkgconfig" EXCLUDE
            PATTERN "*.a" EXCLUDE
            PATTERN "*.prl" EXCLUDE
            PATTERN "*.la" EXCLUDE
        )

        if(EXISTS "${_qt_root}/plugins")
            install(DIRECTORY "${_qt_root}/plugins/"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/plugins
            )
        endif()

        if(EXISTS "${_qt_root}/qml")
            install(DIRECTORY "${_qt_root}/qml/"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/qml
            )
        endif()

        if(EXISTS "${_qt_root}/resources")
            install(DIRECTORY "${_qt_root}/resources/"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/resources
            )
        endif()

        if(EXISTS "${_qt_root}/libexec/QtWebEngineProcess")
            install(PROGRAMS "${_qt_root}/libexec/QtWebEngineProcess"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
        endif()

        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
            "[Paths]\nPrefix = ..\nLibraries = lib\nPlugins = lib/plugins\nQmlImports = lib/qml\nData = lib/resources\n"
        )
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()


# 3. CPack Configuration
set(CPACK_PACKAGE_NAME "CarbonESG")
set(CPACK_PACKAGE_VENDOR "Carbon Services")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Carbon ESG - Enterprise Statutory BRSR Reporting Application")
set(CPACK_PACKAGE_VERSION "${CARBON_ENGINE_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Carbon ESG")
set(CPACK_PACKAGE_CONTACT "carbon.esgreports@gmail.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_LIST_DIR}/README.txt")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_FILE_NAME "CarbonESG-${CARBON_ENGINE_VERSION}-Windows-x64-${CARBON_WORKLOAD_TAG}")
    set(CPACK_NSIS_PACKAGE_NAME "Carbon ESG")
    set(CPACK_NSIS_DISPLAY_NAME "Carbon ESG")
    set(CPACK_NSIS_HELP_LINK "https://carbonesg.online")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://carbonesg.online")
    set(CPACK_NSIS_CONTACT "carbon.esgreports@gmail.com")
    string(REPLACE "/" "\\\\" CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/src/apps/server/app.ico")
    string(REPLACE "/" "\\\\" CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/src/apps/server/app.ico")
    string(REPLACE "/" "\\\\" CPACK_PACKAGE_ICON "${CMAKE_CURRENT_LIST_DIR}/windows/header.bmp")
    string(REPLACE "/" "\\\\" CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_CURRENT_LIST_DIR}/windows/header.bmp")
    string(REPLACE "/" "\\\\" CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_LIST_DIR}/windows/banner.bmp")
    string(REPLACE "/" "\\\\" CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_LIST_DIR}/windows/banner.bmp")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DEFINES "RequestExecutionLevel admin")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_BRANDING_TEXT "Carbon Services - ESG Statutory Reporting")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
      ExecWait '\\\"$INSTDIR\\\\bin\\\\carbon-engine.exe\\\" --stop-service'
      ExecWait '\\\"$INSTDIR\\\\bin\\\\carbon-engine.exe\\\" --uninstall-service'
      WriteRegStr HKLM 'Software\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\Run' 'CarbonEngineTray' '\\\"$INSTDIR\\\\bin\\\\carbon-tray.exe\\\" --minimized'
      Exec '\\\"$INSTDIR\\\\bin\\\\carbon-tray.exe\\\"'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
      ExecWait 'taskkill /IM carbon-tray.exe'
      ExecWait 'taskkill /IM carbon-engine-client.exe'
      ExecWait 'taskkill /IM carbon-engine.exe'
      ExecWait 'taskkill /F /IM carbon-tray.exe /T'
      ExecWait 'taskkill /F /IM carbon-engine-client.exe /T'
      ExecWait 'taskkill /F /IM carbon-engine.exe /T'
      ExecWait '\\\"$INSTDIR\\\\bin\\\\carbon-engine.exe\\\" --stop-service'
      ExecWait '\\\"$INSTDIR\\\\bin\\\\carbon-engine.exe\\\" --uninstall-service'
      DeleteRegValue HKLM 'Software\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\Run' 'CarbonEngineTray'
    ")
    set(CPACK_PACKAGE_EXECUTABLES
        "carbon-tray" "Carbon ESG"
    )
    set(CPACK_CREATE_DESKTOP_LINKS "carbon-tray")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_FILE_NAME "CarbonESG-${CARBON_ENGINE_VERSION}-Linux-x64-${CARBON_WORKLOAD_TAG}")
    set(CPACK_DEBIAN_PACKAGE_NAME "carbon-esg")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Carbon Services <carbon.esgreports@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Carbon ESG statutory BRSR reporting engine and desktop application by Carbon Services.")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://carbonesg.online")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1, libegl1, libxkbcommon0, libxkbcommon-x11-0, libxcb-cursor0, libxcb-xinerama0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-render-util0, libtesseract5, libopenblas0, libpulse0, libnss3, libasound2t64 | libasound2")
endif()

set(CPACK_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION FALSE)
set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)

include(CPack)
