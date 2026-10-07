add_executable(carbon-engine
    ServerConfig.h
    main.cpp
)

# Force console subsystem so Qt Logger outputs print to the command prompt
set_target_properties(carbon-engine PROPERTIES WIN32_EXECUTABLE OFF)

target_link_libraries(carbon-engine PRIVATE carbon-engine-server-libs Qt6::Core)

add_custom_command(TARGET carbon-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${pdfium_SOURCE_DIR}/bin/pdfium.dll"
        $<TARGET_FILE_DIR:carbon-engine>/pdfium.dll
)

add_custom_command(TARGET carbon-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/apps/qml-client/template.html"
        $<TARGET_FILE_DIR:carbon-engine>/template.html
)

if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET carbon-engine POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-opengl-sw --compiler-runtime $<TARGET_FILE:carbon-engine>
        )
    endif()
endif()