add_subdirectory(logging)

add_library(carbon-engine-server-libs STATIC
    db/Database.h db/Database.cpp
    inference/LlamaEngine.h inference/LlamaEngine.cpp
    vector/VectorIndex.h vector/VectorIndex.cpp
    ingestion/DocumentProcessor.h ingestion/DocumentProcessor.cpp
    queue/JobQueue.h queue/JobQueue.cpp
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
    Qt6::HttpServer Qt6::Sql Qt6::Concurrent
    llama faiss ${PDFIUM_LIBRARIES} libtesseract
)
