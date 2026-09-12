cmake_minimum_required(VERSION 3.24)
project(carbon-engine LANGUAGES C CXX)

# --- Clean Output Directories ---
if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(BUILD_ROOT "${CMAKE_SOURCE_DIR}/__Builds/${ARCH_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_ROOT}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_ROOT}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_ROOT}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_ROOT}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BUILD_ROOT}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUILD_ROOT}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILD_ROOT}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BUILD_ROOT}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BUILD_ROOT}/lib")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.10 REQUIRED COMPONENTS Core HttpServer Sql Concurrent)
qt_standard_project_setup()

# --- Route FetchContent to project_root/third-party ---
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/third-party" CACHE PATH "Redirect FetchContent downloads" FORCE)
set_property(DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES "${FETCHCONTENT_BASE_DIR}")

include(FetchContent)

# --- Automate NASM Installation via Winget ---
find_program(NASM_EXECUTABLE nasm)
if(NOT NASM_EXECUTABLE)
    message(STATUS "NASM not found. Automating installation via winget...")
    execute_process(
        COMMAND winget install --id NASM.NASM --exact --silent --accept-package-agreements --accept-source-agreements
        RESULT_VARIABLE WINGET_RESULT
    )
    if(WINGET_RESULT EQUAL 0)
        message(STATUS "NASM installed successfully.")
        set(ENV{PATH} "$ENV{PATH};C:\\Program Files\\NASM;$ENV{LOCALAPPDATA}\\Programs\\NASM")
    else()
        message(WARNING "Winget failed to install NASM. SIMD extensions for JPEG will be disabled.")
    endif()
endif()

# 1. llama.cpp
message(STATUS "Fetching llama.cpp...")
FetchContent_Declare(
    llama
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG        master
)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(llama)

# 2. Intel MKL (Required by FAISS)
message(STATUS "Fetching Intel MKL via NuGet...")
FetchContent_Declare(
    mkl_static
    URL https://www.nuget.org/api/v2/package/intelmkl.static.win-x64/2026.1.0.226
)
FetchContent_Declare(
    mkl_devel
    URL https://www.nuget.org/api/v2/package/intelmkl.devel.win-x64/2026.1.0.226
)
FetchContent_MakeAvailable(mkl_static mkl_devel)

file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindMKL.cmake" "
set(MKL_FOUND TRUE)
set(MKL_INCLUDE_DIRS \"\${mkl_devel_SOURCE_DIR}/build/native/include\")
set(MKL_LIBRARIES 
    \"\${mkl_static_SOURCE_DIR}/build/native/lib/x64/mkl_intel_lp64.lib\"
    \"\${mkl_static_SOURCE_DIR}/build/native/lib/x64/mkl_sequential.lib\"
    \"\${mkl_static_SOURCE_DIR}/build/native/lib/x64/mkl_core.lib\"
)
if(NOT TARGET MKL::MKL)
    add_library(MKL::MKL INTERFACE IMPORTED)
    set_target_properties(MKL::MKL PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES \"\${MKL_INCLUDE_DIRS}\"
        INTERFACE_LINK_LIBRARIES \"\${MKL_LIBRARIES}\"
    )
endif()
")

FetchContent_Declare(
    faiss
    GIT_REPOSITORY https://github.com/facebookresearch/faiss.git
    GIT_TAG        v1.9.0
)
set(FAISS_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
set(FAISS_ENABLE_GPU OFF CACHE BOOL "" FORCE)
set(FAISS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(faiss)

# 3. PDFium
message(STATUS "Fetching Pre-compiled PDFium Binaries...")
FetchContent_Declare(
    pdfium
    URL      https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-x64.tgz
)
FetchContent_MakeAvailable(pdfium)
set(PDFIUM_INCLUDE_DIRS ${pdfium_SOURCE_DIR}/include)
set(PDFIUM_LIBRARIES ${pdfium_SOURCE_DIR}/lib/pdfium.dll.lib)

# 4. Image Dependencies for Leptonica
message(STATUS "Fetching ZLIB, JPEG, and PNG...")
set(SKIP_INSTALL_ALL ON CACHE BOOL "Disable ZLIB installation" FORCE)
set(PNG_INSTALL OFF CACHE BOOL "Disable PNG installation" FORCE)

FetchContent_Declare(
    ZLIB
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG        v1.3.1
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(ZLIB)

if(TARGET zlibstatic AND NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

FetchContent_Declare(
    JPEG
    GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
    GIT_TAG        2.1.5.1
    OVERRIDE_FIND_PACKAGE
)
set(ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(JPEG)

FetchContent_Declare(
    PNG
    GIT_REPOSITORY https://github.com/glennrp/libpng.git
    GIT_TAG        v1.6.43
    OVERRIDE_FIND_PACKAGE
)
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(PNG)

# 5. Leptonica & Tesseract
message(STATUS "Fetching Leptonica & Tesseract...")
FetchContent_Declare(
    Leptonica
    GIT_REPOSITORY https://github.com/DanBloomberg/leptonica.git
    GIT_TAG        1.83.1
    OVERRIDE_FIND_PACKAGE 
)
set(SW_BUILD OFF CACHE BOOL "" FORCE)
set(WITH_ZLIB ON CACHE BOOL "" FORCE)
set(WITH_JPEG ON CACHE BOOL "" FORCE)
set(WITH_PNG ON CACHE BOOL "" FORCE)
set(WITH_TIFF OFF CACHE BOOL "" FORCE)
set(WITH_GIF OFF CACHE BOOL "" FORCE)
set(WITH_WEBP OFF CACHE BOOL "" FORCE)
set(WITH_OPENJPEG OFF CACHE BOOL "" FORCE)
set(BUILD_PROG OFF CACHE BOOL "" FORCE)

set(Leptonica_DIR "${leptonica_SOURCE_DIR}" CACHE PATH "" FORCE)
FetchContent_MakeAvailable(Leptonica)
include_directories(${leptonica_SOURCE_DIR}/src)

FetchContent_Declare(
    tesseract
    GIT_REPOSITORY https://github.com/tesseract-ocr/tesseract.git
    GIT_TAG        5.3.3
)
set(BUILD_TRAINING_TOOLS OFF CACHE BOOL "" FORCE)
set(DISABLE_CURL ON CACHE BOOL "" FORCE)
set(DISABLE_ARCHIVE ON CACHE BOOL "" FORCE)
set(LEPT_TIFF_RESULT 0 CACHE INTERNAL "Bypass TIFF check")
FetchContent_MakeAvailable(tesseract)

add_subdirectory(src)