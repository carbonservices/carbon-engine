set(TRAY_SOURCES
    main.cpp
    TrayManager.h
    TrayManager.cpp
)
if(WIN32)
    list(APPEND TRAY_SOURCES app.rc)
endif()

add_executable(carbon-tray ${TRAY_SOURCES})

set_target_properties(carbon-tray PROPERTIES WIN32_EXECUTABLE TRUE)

target_include_directories(carbon-tray PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(carbon-tray PRIVATE
    carbon-engine-common
    carbon-engine-system
    Qt6::Core
    Qt6::Widgets
)
