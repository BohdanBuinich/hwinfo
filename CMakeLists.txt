cmake_minimum_required(VERSION 3.21)

# ----------------------------------------------------------------------------
# Project Definition
# ----------------------------------------------------------------------------
project(hwinfo
        VERSION 1.0.0
        DESCRIPTION "Hardware information library"
        LANGUAGES CXX
)

# ----------------------------------------------------------------------------
# If no C++ standard is specified, pick the highest supported from [23, 20, 17, 14, 11].
# ----------------------------------------------------------------------------
if ("${CMAKE_CXX_STANDARD}" STREQUAL "")
    function(_set_language_standard output_var language)
        foreach(version IN LISTS ARGN)
            if (DEFINED "CMAKE_${language}${version}_STANDARD_COMPILE_OPTION"
                    OR DEFINED "CMAKE_${language}${version}_EXTENSION_COMPILE_OPTION")
                set("${output_var}" "${version}" PARENT_SCOPE)
                break()
            endif()
        endforeach()
    endfunction()
    _set_language_standard(CMAKE_CXX_STANDARD CXX 23 20 17 14 11)
endif()

# ----------------------------------------------------------------------------
# Detect if we are top-level
# ----------------------------------------------------------------------------
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)

# ----------------------------------------------------------------------------
# GNUInstallDirs: Provides variables like CMAKE_INSTALL_[BIN|LIB|INCLUDE]DIR
# ----------------------------------------------------------------------------
include(GNUInstallDirs)

# ----------------------------------------------------------------------------
# Build Directories
# ----------------------------------------------------------------------------
if(${PROJECT_IS_TOP_LEVEL})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Archive output dir.")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Library output dir.")
    set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/bin" CACHE PATH "PDB (MSVC debug symbol)output dir.")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Executable/dll output dir.")
endif()

# ----------------------------------------------------------------------------
# Library Type Options
# ----------------------------------------------------------------------------
option(HWINFO_SHARED "Build hwinfo as a shared library" ON)
option(HWINFO_STATIC "Build hwinfo as a static library" OFF)

if (HWINFO_STATIC AND HWINFO_SHARED)
    message(WARNING "Both HWINFO_STATIC and HWINFO_SHARED are ON; defaulting to SHARED.")
    set(HWINFO_BUILD "SHARED")
elseif(HWINFO_STATIC)
    set(HWINFO_BUILD "STATIC")
else()
    set(HWINFO_BUILD "SHARED")
endif()

# ----------------------------------------------------------------------------
# Component (Module) Options
# ----------------------------------------------------------------------------
option(HWINFO_OS         "Enable OS information module"            ON)
option(HWINFO_MAINBOARD  "Enable mainboard information module"     ON)
option(HWINFO_CPU        "Enable CPU information module"           ON)
option(HWINFO_DISK       "Enable disk information module"          ON)
option(HWINFO_RAM        "Enable RAM information module"           ON)
option(HWINFO_GPU        "Enable GPU information module"           ON)
option(HWINFO_GPU_OPENCL "Enable OpenCL support for GPU module"    OFF)
option(HWINFO_BATTERY    "Enable battery information module"       ON)
option(HWINFO_NETWORK    "Enable network information module"       ON)

# ----------------------------------------------------------------------------
# Examples & Testing
# ----------------------------------------------------------------------------
set(BUILD_EXAMPLES "${PROJECT_IS_TOP_LEVEL}" CACHE BOOL "Build example programs" FORCE)
set(BUILD_TESTING  "${PROJECT_IS_TOP_LEVEL}" CACHE BOOL "Build test programs"    FORCE)

# ----------------------------------------------------------------------------
# Windows-specific Settings
# ----------------------------------------------------------------------------
if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# ----------------------------------------------------------------------------
# Add Source Directory
# ----------------------------------------------------------------------------
add_subdirectory(src)

# ----------------------------------------------------------------------------
# Configure & Install CMake Package Config Files
# ----------------------------------------------------------------------------
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/hwinfoConfigVersion.cmake"
        VERSION       "${PROJECT_VERSION}"
        COMPATIBILITY AnyNewerVersion
)

configure_file(
        cmake/hwinfoConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/hwinfoConfig.cmake"
        @ONLY
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/hwinfoConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/hwinfoConfigVersion.cmake"
        DESTINATION
        "${CMAKE_INSTALL_LIBDIR}/cmake/hwinfo"
)

install(
        EXPORT hwinfoTargets
        FILE hwinfoTargets.cmake
        NAMESPACE lfreist-hwinfo::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/hwinfo"
)

# ----------------------------------------------------------------------------
# Examples & Tests
# ----------------------------------------------------------------------------
if (BUILD_EXAMPLES OR BUILD_TESTING)
    add_subdirectory(examples)
endif()

if (BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
    add_test(NAME system_info COMMAND system_info)
endif()
