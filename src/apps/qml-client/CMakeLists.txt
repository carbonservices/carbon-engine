set(CLIENT_SOURCES main.cpp)
if(WIN32)
    list(APPEND CLIENT_SOURCES app.rc)
endif()

qt_add_executable(carbon-engine-client ${CLIENT_SOURCES})

target_link_libraries(carbon-engine-client PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::WebEngineQuick
    carbon-engine-logging
    carbon-engine-common
    carbon-engine-system
)

if(WIN32)
    set_target_properties(carbon-engine-client PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(carbon-engine-client PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_command(TARGET carbon-engine-client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/logo.png"
        $<TARGET_FILE_DIR:carbon-engine-client>/logo.png
)

add_custom_command(TARGET carbon-engine-client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/app.ico"
        $<TARGET_FILE_DIR:carbon-engine-client>/app.ico
)

set(QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/LeftSidebar.qml
    qml/CentralWorkspace.qml
    qml/RightPanel.qml
    qml/DocumentsPanel.qml
    qml/SystemStatusPanel.qml
    qml/StatusLogPanel.qml
    qml/SettingsModal.qml
    qml/ReportReviewScreen.qml
    qml/FieldGroupCard.qml
    qml/LoopFieldEditor.qml
    qml/BRSRMetadata.js
)
foreach(qml_file IN LISTS QML_FILES)
    get_filename_component(filename ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${filename})
endforeach()

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(logo.png PROPERTIES QT_RESOURCE_ALIAS logo.png)

qt_add_qml_module(carbon-engine-client
    URI CarbonEngineClient
    VERSION 1.0
    QML_FILES ${QML_FILES}
    RESOURCES logo.png
    SOURCES
        ApiClientBase.h ApiClientBase.cpp
        UploadClient.h UploadClient.cpp
        PipelineStatusPoller.h PipelineStatusPoller.cpp
        ReportClient.h ReportClient.cpp
        SystemStatusPoller.h SystemStatusPoller.cpp
        ExtractionClient.h ExtractionClient.cpp
        ExtractionStatusPoller.h ExtractionStatusPoller.cpp
)

if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET carbon-engine-client POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-opengl-sw --compiler-runtime --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml" -network -widgets -httpserver -sql --add-plugin-types tls $<TARGET_FILE:carbon-engine-client> $<TARGET_FILE:carbon-engine> $<TARGET_FILE:carbon-tray>
        )
    endif()

    if(EXISTS "${_qt_bin_dir}/../plugins/tls")
        add_custom_command(TARGET carbon-engine-client POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_qt_bin_dir}/../plugins/tls"
                "$<TARGET_FILE_DIR:carbon-engine-client>/tls"
        )
    endif()
endif()
