set(SERVER_SOURCES
    ServerConfig.h
    main.cpp
    api/HTTPServer.h
    api/HTTPServer.cpp
    queue/JobQueue.h
    queue/JobQueue.cpp
    report/ReportExtractionService.h
    report/ReportExtractionService.cpp
)
if(WIN32)
    list(APPEND SERVER_SOURCES app.rc)
endif()

add_executable(carbon-engine ${SERVER_SOURCES})

set_target_properties(carbon-engine PROPERTIES WIN32_EXECUTABLE OFF)

target_include_directories(carbon-engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(carbon-engine PRIVATE
    carbon-engine-common
    carbon-engine-offline
    carbon-engine-logging
    carbon-engine-system
    Qt6::Core
    Qt6::HttpServer
    Qt6::Network
)

if(WIN32)
    add_custom_command(TARGET carbon-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${pdfium_SOURCE_DIR}/bin/pdfium.dll"
            $<TARGET_FILE_DIR:carbon-engine>/pdfium.dll
    )
else()
    add_custom_command(TARGET carbon-engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${pdfium_SOURCE_DIR}/lib/libpdfium.so"
            $<TARGET_FILE_DIR:carbon-engine>/libpdfium.so
    )
    set_target_properties(carbon-engine PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

add_custom_command(TARGET carbon-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/libs/common/templates/template.html"
        $<TARGET_FILE_DIR:carbon-engine>/template.html
)

add_custom_command(TARGET carbon-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/apps/qml-client/logo.png"
        $<TARGET_FILE_DIR:carbon-engine>/logo.png
)

add_custom_command(TARGET carbon-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/app.ico"
        $<TARGET_FILE_DIR:carbon-engine>/app.ico
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
