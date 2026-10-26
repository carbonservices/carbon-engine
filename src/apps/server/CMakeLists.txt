set(SERVER_SOURCES
    ServerConfig.h
    main.cpp
    api/HTTPServer.h
    api/HTTPServer.cpp
    api/Hateoas.h
    api/Hateoas.cpp
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
