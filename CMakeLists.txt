cmake_minimum_required(VERSION 3.24)
project(carbon-engine LANGUAGES C CXX)

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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.10 REQUIRED COMPONENTS Core HttpServer Sql Concurrent)
qt_standard_project_setup()

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/third-party" CACHE PATH "" FORCE)
include(FetchContent)

find_program(NASM_EXECUTABLE nasm)
if(NOT NASM_EXECUTABLE)
    execute_process(COMMAND winget install --id NASM.NASM --exact --silent --accept-package-agreements --accept-source-agreements)
    set(ENV{PATH} "$ENV{PATH};C:\\Program Files\\NASM;$ENV{LOCALAPPDATA}\\Programs\\NASM")
endif()

# llama.cpp
FetchContent_Declare(llama GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git GIT_TAG master)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(llama)

# Intel MKL & FAISS
FetchContent_Declare(mkl_static URL https://www.nuget.org/api/v2/package/intelmkl.static.win-x64/2026.1.0.226 DOWNLOAD_NAME mkl_static.zip)
FetchContent_Declare(mkl_devel URL https://www.nuget.org/api/v2/package/intelmkl.devel.win-x64/2026.1.0.226 DOWNLOAD_NAME mkl_devel.zip)
FetchContent_MakeAvailable(mkl_static mkl_devel)

# nuget MKL packages ship headers at <pkg>/build/native/include and win-x64 .lib files at
# <pkg>/build/native/win-x64 - hardcoded since the package version above is pinned exactly.
set(MKL_INCLUDE_DIR "${mkl_devel_SOURCE_DIR}/build/native/include")
set(MKL_LIB_DIR "${mkl_static_SOURCE_DIR}/build/native/win-x64")

file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindMKL.cmake" "
set(MKL_FOUND TRUE)
find_path(MKL_INCLUDE_DIRS NAMES mkl.h PATHS \"${MKL_INCLUDE_DIR}\" NO_DEFAULT_PATH)
find_library(MKL_INTEL_LP64 NAMES mkl_intel_lp64 PATHS \"${MKL_LIB_DIR}\" NO_DEFAULT_PATH)
find_library(MKL_SEQUENTIAL NAMES mkl_sequential PATHS \"${MKL_LIB_DIR}\" NO_DEFAULT_PATH)
find_library(MKL_CORE NAMES mkl_core PATHS \"${MKL_LIB_DIR}\" NO_DEFAULT_PATH)

if (NOT MKL_INCLUDE_DIRS OR NOT MKL_INTEL_LP64 OR NOT MKL_SEQUENTIAL OR NOT MKL_CORE)
    message(FATAL_ERROR \"Intel MKL libraries or headers could not be found under ${MKL_LIB_DIR}.\")
endif()

set(MKL_LIBRARIES \${MKL_INTEL_LP64} \${MKL_SEQUENTIAL} \${MKL_CORE})
if(NOT TARGET MKL::MKL)
    add_library(MKL::MKL INTERFACE IMPORTED)
    set_target_properties(MKL::MKL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES \"\${MKL_INCLUDE_DIRS}\" INTERFACE_LINK_LIBRARIES \"\${MKL_LIBRARIES}\")
endif()
")

FetchContent_Declare(faiss GIT_REPOSITORY https://github.com/facebookresearch/faiss.git GIT_TAG v1.9.0)
set(FAISS_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
set(FAISS_ENABLE_GPU OFF CACHE BOOL "" FORCE)
set(FAISS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(faiss)

# PDFium
FetchContent_Declare(pdfium URL https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-win-x64.tgz)
FetchContent_MakeAvailable(pdfium)
set(PDFIUM_INCLUDE_DIRS ${pdfium_SOURCE_DIR}/include)
set(PDFIUM_LIBRARIES ${pdfium_SOURCE_DIR}/lib/pdfium.dll.lib)

# Leptonica Dependencies
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
set(PNG_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(ZLIB GIT_REPOSITORY https://github.com/madler/zlib.git GIT_TAG v1.3.1 OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(ZLIB)
if(TARGET zlibstatic AND NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

FetchContent_Declare(JPEG GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git GIT_TAG 2.1.5.1 OVERRIDE_FIND_PACKAGE)
set(ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(JPEG)

FetchContent_Declare(PNG GIT_REPOSITORY https://github.com/glennrp/libpng.git GIT_TAG v1.6.43 OVERRIDE_FIND_PACKAGE)
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(PNG)

# OVERRIDE_FIND_PACKAGE redirects find_package(ZLIB/JPEG/PNG) to these FetchContent targets,
# but doesn't populate the legacy *_INCLUDE_DIR(S)/*_LIBRARIES variables that Leptonica's
# CMakeLists.txt links against - set them explicitly.
set(ZLIB_INCLUDE_DIRS ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR} CACHE PATH "" FORCE)
set(ZLIB_LIBRARIES zlibstatic CACHE STRING "" FORCE)
set(JPEG_INCLUDE_DIR ${jpeg_SOURCE_DIR} ${jpeg_BINARY_DIR} CACHE PATH "" FORCE)
set(JPEG_LIBRARIES jpeg-static CACHE STRING "" FORCE)
set(PNG_INCLUDE_DIRS ${png_SOURCE_DIR} ${png_BINARY_DIR} CACHE PATH "" FORCE)
set(PNG_LIBRARIES png_static CACHE STRING "" FORCE)

# Leptonica's own export(TARGETS leptonica ...) call requires every target leptonica links
# against to already be in an export set - pre-export these so that check passes.
export(TARGETS zlibstatic jpeg-static png_static FILE "${CMAKE_BINARY_DIR}/DepsTargets.cmake")

# Leptonica & Tesseract
FetchContent_Declare(Leptonica GIT_REPOSITORY https://github.com/DanBloomberg/leptonica.git GIT_TAG 1.83.1 OVERRIDE_FIND_PACKAGE)
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

# leptonica's install(EXPORT ...) requires zlibstatic/jpeg-static/png_static to be in an
# export set too, which the fetched zlib/jpeg/png projects don't provide - skip install
# rules for this subproject entirely since carbon-engine only needs it at build time.
set(CMAKE_SKIP_INSTALL_RULES ON)
FetchContent_MakeAvailable(Leptonica)
set(CMAKE_SKIP_INSTALL_RULES OFF)
include_directories(${leptonica_SOURCE_DIR}/src ${leptonica_BINARY_DIR}/src)

# OVERRIDE_FIND_PACKAGE redirects find_package(Leptonica CONFIG) to this FetchContent target,
# but does not populate the legacy Leptonica_INCLUDE_DIRS/Leptonica_LIBRARIES variables that
# tesseract's CMakeLists.txt links against - set them explicitly.
set(Leptonica_FOUND TRUE CACHE BOOL "" FORCE)
set(Leptonica_INCLUDE_DIRS ${leptonica_SOURCE_DIR}/src ${leptonica_BINARY_DIR}/src CACHE PATH "" FORCE)
set(Leptonica_LIBRARIES leptonica CACHE STRING "" FORCE)

FetchContent_Declare(tesseract GIT_REPOSITORY https://github.com/tesseract-ocr/tesseract.git GIT_TAG 5.3.3)
set(BUILD_TRAINING_TOOLS OFF CACHE BOOL "" FORCE)
set(DISABLE_CURL ON CACHE BOOL "" FORCE)
set(DISABLE_ARCHIVE ON CACHE BOOL "" FORCE)
set(LEPT_TIFF_RESULT 0 CACHE INTERNAL "")
FetchContent_MakeAvailable(tesseract)

add_subdirectory(src)