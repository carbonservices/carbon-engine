qt_add_executable(carbon-engine-client main.cpp)

# 1. LINK FIRST: qmltyperegistrar needs this to find logging/Logger.h during parsing
target_link_libraries(carbon-engine-client PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    carbon-engine-logging
)

# 2. SYNC OUTPUTS: Tell Qt to put the qmldir and .qmltypes alongside your custom __Builds/x64/bin .exe
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# 3. ALIAS QML FILES: Flattens the "qml/" folder so they register at the root namespace
set(QML_FILES
    qml/Main.qml
    qml/FilePickerBar.qml
    qml/PipelineStageIndicator.qml
    qml/ReportView.qml
)
foreach(qml_file IN LISTS QML_FILES)
    get_filename_component(filename ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${filename})
endforeach()

# 4. ADD MODULE: Now safe to run because dependencies and paths are established
qt_add_qml_module(carbon-engine-client
    URI CarbonEngineClient
    VERSION 1.0
    QML_FILES ${QML_FILES}
    SOURCES
        UploadClient.h UploadClient.cpp
        PipelineStatusPoller.h PipelineStatusPoller.cpp
        ReportClient.h ReportClient.cpp
)

if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET carbon-engine-client POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-opengl-sw --compiler-runtime --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml" $<TARGET_FILE:carbon-engine-client>
        )
    endif()
endif()