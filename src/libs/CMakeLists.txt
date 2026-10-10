add_subdirectory(logging)

# ── system telemetry lib (server-side only) ───────────────────
add_library(carbon-engine-system STATIC
    system/SystemMonitor.h system/SystemMonitor.cpp
)
target_compile_features(carbon-engine-system PUBLIC cxx_std_20)
target_compile_definitions(carbon-engine-system PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(carbon-engine-system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(carbon-engine-system PUBLIC
    carbon-engine-logging
    Qt6::Core
    dxgi dxguid psapi    # GPU detection (DXGI) + process memory (PSAPI)
)

add_library(carbon-engine-server-libs STATIC
    db/Database.h db/Database.cpp
    inference/LlamaEngine.h inference/LlamaEngine.cpp
    vector/VectorIndex.h vector/VectorIndex.cpp
    vector/VectorIndexRegistry.h vector/VectorIndexRegistry.cpp
    ingestion/DocumentProcessor.h ingestion/DocumentProcessor.cpp
    queue/JobQueue.h queue/JobQueue.cpp
    report/BrsrSchema.h report/BrsrSchema.cpp
    report/FactStore.h report/FactStore.cpp
    report/ReportExtractionService.h report/ReportExtractionService.cpp
    report/ReportValidator.h report/ReportValidator.cpp
    report/ReportExporter.h report/ReportExporter.cpp
    api/HTTPServer.h api/HTTPServer.cpp
)

target_compile_features(carbon-engine-server-libs PUBLIC cxx_std_20)

target_compile_definitions(carbon-engine-server-libs PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)

target_include_directories(carbon-engine-server-libs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PDFIUM_INCLUDE_DIRS}
    ${tesseract_BINARY_DIR}/include
)

target_link_libraries(carbon-engine-server-libs PUBLIC
    carbon-engine-logging
    carbon-engine-system
    Qt6::HttpServer Qt6::Sql Qt6::Concurrent
    llama faiss ${PDFIUM_LIBRARIES} libtesseract
    inja
)
