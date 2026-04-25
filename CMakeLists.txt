cmake_minimum_required(VERSION 3.22)

project(Picotado VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# Shim FindZLIB module so libmysofa's find_package(ZLIB) picks up our CPM-built zlib
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/cpm.cmake)

CPMAddPackage(
    NAME JUCE
    GIT_TAG 8.0.12
    VERSION 8.0.12
    GITHUB_REPOSITORY juce-framework/JUCE
    SOURCE_DIR ${LIB_DIR}/juce
)

# zlib is a transitive dependency of libmysofa (SOFA file parser)
CPMAddPackage(
    NAME zlib
    GITHUB_REPOSITORY madler/zlib
    GIT_TAG v1.3.1
    SOURCE_DIR ${LIB_DIR}/zlib
    OPTIONS
        "ZLIB_BUILD_EXAMPLES OFF"
        "BUILD_SHARED_LIBS OFF"
)

# Expose zlib's headers transitively to anything linking against zlibstatic
# (madler/zlib doesn't declare PUBLIC include dirs on its targets).
if (TARGET zlibstatic)
    target_include_directories(zlibstatic PUBLIC
        $<BUILD_INTERFACE:${zlib_SOURCE_DIR}>
        $<BUILD_INTERFACE:${zlib_BINARY_DIR}>
    )
endif()

# Also export the paths via a cached variable so libmysofa's
# `include(FindZLIB) ; include_directories(${ZLIB_INCLUDE_DIRS})` can pick
# them up even from a child scope where zlib_SOURCE_DIR is not visible.
set(ZLIB_PICOTADO_INCLUDE_DIRS "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}"
    CACHE STRING "Paths to zlib headers (set by picotado)" FORCE)

# Download libmysofa sources only — we compile them directly into a
# private static lib instead of using their CMakeLists.txt, which has
# Windows/MSVC-specific NuGet hooks that fight find_package(ZLIB).
CPMAddPackage(
    NAME libmysofa
    GITHUB_REPOSITORY hoene/libmysofa
    GIT_TAG v1.3.4
    SOURCE_DIR ${LIB_DIR}/libmysofa
    DOWNLOAD_ONLY YES
)

if (libmysofa_ADDED)
    set(MYSOFA_GEN_DIR "${CMAKE_BINARY_DIR}/mysofa_gen")
    file(MAKE_DIRECTORY "${MYSOFA_GEN_DIR}")

    file(WRITE "${MYSOFA_GEN_DIR}/config.h"
"#if !defined _CONFIG_H
#define _CONFIG_H
#define CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\"
#define CPACK_PACKAGE_VERSION_MAJOR 1
#define CPACK_PACKAGE_VERSION_MINOR 3
#define CPACK_PACKAGE_VERSION_PATCH 4
#endif
")

    # Static-friendly export header (mimics CMake's generate_export_header
    # output with STATIC_DEFINE in effect).
    file(WRITE "${MYSOFA_GEN_DIR}/mysofa_export.h"
"#ifndef MYSOFA_EXPORT_H
#define MYSOFA_EXPORT_H
#define MYSOFA_EXPORT
#define MYSOFA_NO_EXPORT
#ifndef MYSOFA_DEPRECATED
#  if defined(_MSC_VER)
#    define MYSOFA_DEPRECATED __declspec(deprecated)
#  else
#    define MYSOFA_DEPRECATED __attribute__((__deprecated__))
#  endif
#endif
#define MYSOFA_DEPRECATED_EXPORT MYSOFA_EXPORT MYSOFA_DEPRECATED
#define MYSOFA_DEPRECATED_NO_EXPORT MYSOFA_NO_EXPORT MYSOFA_DEPRECATED
#endif
")

    file(GLOB MYSOFA_SOURCES
        "${libmysofa_SOURCE_DIR}/src/hrtf/*.c"
        "${libmysofa_SOURCE_DIR}/src/hdf/*.c"
        "${libmysofa_SOURCE_DIR}/src/resampler/*.c"
    )

    add_library(picotado_mysofa STATIC ${MYSOFA_SOURCES})
    target_include_directories(picotado_mysofa PUBLIC
        "${libmysofa_SOURCE_DIR}/src/hrtf"
        "${MYSOFA_GEN_DIR}"
    )
    target_include_directories(picotado_mysofa PRIVATE
        "${libmysofa_SOURCE_DIR}/src/hdf"
        "${libmysofa_SOURCE_DIR}/src/resampler"
    )
    target_link_libraries(picotado_mysofa PRIVATE zlibstatic)
    target_compile_definitions(picotado_mysofa PUBLIC MYSOFA_STATIC)
    if (MSVC)
        # libmysofa is C and triggers many MSVC warnings — silence them
        # without affecting our own /WX strict-warning policy.
        target_compile_options(picotado_mysofa PRIVATE /W0 /wd4244 /wd4267 /wd4101)
    endif()
endif()

if (MSVC)
  message(STATUS "Setting up WebView dependencies")
  find_program(POWERSHELL_EXE NAMES pwsh powershell REQUIRED)
  execute_process(COMMAND ${POWERSHELL_EXE} -NoProfile -ExecutionPolicy Bypass -File scripts/DownloadWebView2.ps1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE DOWNLOAD_WEBVIEW2_RESULT)

  if (NOT DOWNLOAD_WEBVIEW2_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to download Microsoft.Web.WebView2 NuGet package. Result: ${DOWNLOAD_WEBVIEW2_RESULT}")
  endif()
endif()

if (MSVC)
  set(CXX_PROJECT_WARNINGS "/W4;/WX;/wd4820;/wd4514;/wd4625;/wd4626;/wd5026;/wd5027")
else()
  set(CXX_PROJECT_WARNINGS "-Wall;-Wextra;-Wpedantic")
endif()

add_subdirectory(plugin)
