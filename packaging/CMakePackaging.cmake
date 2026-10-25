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

    # Install deployed Qt runtime DLLs, plugins, resources, and complete QML tree into bin
    install(DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "*.exp" EXCLUDE
        PATTERN "*.lib" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "template.html" EXCLUDE
        PATTERN "qmltooling" EXCLUDE
    )

    install(FILES "${CMAKE_CURRENT_LIST_DIR}/windows/download-models.cmd"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES "${CMAKE_SOURCE_DIR}/src/libs/common/templates/template.html"
        DESTINATION templates
    )
    install(FILES "${CMAKE_SOURCE_DIR}/src/apps/qml-client/logo.png"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES "${CMAKE_SOURCE_DIR}/src/apps/server/app.ico"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES "${CMAKE_SOURCE_DIR}/src/apps/qml-client/logo.png"
        DESTINATION .
    )
    install(FILES "${CMAKE_SOURCE_DIR}/src/apps/server/app.ico"
        DESTINATION .
    )
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN")
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
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/eng.traineddata")
    install(FILES "${CMAKE_SOURCE_DIR}/eng.traineddata"
        DESTINATION share/tessdata
    )
    if(WIN32)
        install(FILES "${CMAKE_SOURCE_DIR}/eng.traineddata"
            DESTINATION ${CMAKE_INSTALL_BINDIR}/tessdata
        )
    endif()
endif()

option(CARBON_ENGINE_EMBED_MODELS "Embed AI models statically inside the installer" OFF)

if(CARBON_ENGINE_EMBED_MODELS AND EXISTS "${CMAKE_SOURCE_DIR}/models")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/models"
        DESTINATION .
        FILES_MATCHING
        PATTERN "*.gguf"
    )
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

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_PACKAGE_FILE_NAME "CarbonESG-${CARBON_ENGINE_VERSION}-Windows-x64-${CARBON_WORKLOAD_TAG}")
    set(CPACK_NSIS_PACKAGE_NAME "Carbon ESG")
    set(CPACK_NSIS_DISPLAY_NAME "Carbon ESG")
    set(CPACK_NSIS_HELP_LINK "https://carbonesg.online")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://carbonesg.online")
    set(CPACK_NSIS_CONTACT "carbon.esgreports@gmail.com")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/src/apps/server/app.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/src/apps/server/app.ico")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DEFINES "RequestExecutionLevel admin")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
      CreateDirectory '$INSTDIR\\\\models'
      DetailPrint 'Downloading AI Models (bge-small-en-v1.5, gemma-2-2b)...'
      ExecWait '\"$INSTDIR\\\\bin\\\\download-models.cmd\" \"$INSTDIR\"'
      ExecWait '\"$INSTDIR\\\\bin\\\\carbon-engine.exe\" --install-service'
      ExecWait '\"$INSTDIR\\\\bin\\\\carbon-engine.exe\" --start-service'
      WriteRegStr HKLM 'Software\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\Run' 'CarbonEngineTray' '\"$INSTDIR\\\\bin\\\\carbon-tray.exe\"'
      Exec '\"$INSTDIR\\\\bin\\\\carbon-tray.exe\"'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
      ExecWait 'taskkill /F /IM carbon-tray.exe /T'
      ExecWait 'taskkill /F /IM carbon-engine-client.exe /T'
      ExecWait '\"$INSTDIR\\\\bin\\\\carbon-engine.exe\" --stop-service'
      ExecWait '\"$INSTDIR\\\\bin\\\\carbon-engine.exe\" --uninstall-service'
      ExecWait 'taskkill /F /IM carbon-engine.exe /T'
      DeleteRegValue HKLM 'Software\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\Run' 'CarbonEngineTray'
    ")
    set(CPACK_PACKAGE_EXECUTABLES
        "carbon-tray" "Carbon ESG Workspace"
    )
    set(CPACK_CREATE_DESKTOP_LINKS "carbon-tray")
else()
    set(CPACK_GENERATOR "DEB;TGZ")
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

