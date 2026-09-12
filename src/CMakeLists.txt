add_executable(carbon-engine
    main.cpp
    db/Database.h
    db/Database.cpp
    inference/LlamaEngine.h
    inference/LlamaEngine.cpp
    vector/VectorIndex.h
    vector/VectorIndex.cpp
    ingestion/DocumentProcessor.h
    ingestion/DocumentProcessor.cpp
    queue/JobQueue.h
    queue/JobQueue.cpp
    api/HttpServer.h
    api/HttpServer.cpp
)

target_include_directories(carbon-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PDFIUM_INCLUDE_DIRS}
)

target_link_libraries(carbon-engine PRIVATE
    Qt6::Core
    Qt6::HttpServer
    Qt6::Sql
    Qt6::Concurrent
    llama
    faiss
    ${PDFIUM_LIBRARIES}
    libtesseract
)

add_custom_command(TARGET carbon-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${pdfium_SOURCE_DIR}/bin/pdfium.dll"
        $<TARGET_FILE_DIR:carbon-engine>/pdfium.dll
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