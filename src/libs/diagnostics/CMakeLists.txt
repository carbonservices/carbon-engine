add_library(carbon-engine-diagnostics STATIC
    GitHubConstants.h
    DiagnosticCollector.h
    DiagnosticCollector.cpp
    GitHubClient.h
    GitHubClient.cpp
)

add_library(carbon-engine-github ALIAS carbon-engine-diagnostics)

target_compile_features(carbon-engine-diagnostics PUBLIC cxx_std_20)
target_compile_definitions(carbon-engine-diagnostics PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)

target_include_directories(carbon-engine-diagnostics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../system
)

target_link_libraries(carbon-engine-diagnostics PUBLIC
    carbon-engine-logging
    carbon-engine-common
    carbon-engine-system
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Network
)

if(WIN32)
    target_link_libraries(carbon-engine-diagnostics PRIVATE dxgi)
endif()
