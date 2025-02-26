cmake_minimum_required(VERSION 3.21)

project(hwinfo VERSION 1.0.0 LANGUAGES CXX)

# if standard not provided, use the latest supported by the compiler
if("${CMAKE_CXX_STANDARD}" STREQUAL "")
  function(_set_language_standard output language)
    foreach(version IN LISTS ARGN)
      if(DEFINED "CMAKE_${language}${version}_STANDARD_COMPILE_OPTION"
        OR DEFINED "CMAKE_${language}${version}_EXTENSION_COMPILE_OPTION"
      )
        set("${output}" "${version}" PARENT_SCOPE)
        break()
      endif()
    endforeach()
  endfunction()
  _set_language_standard(CMAKE_CXX_STANDARD CXX 20 17 14 11)
endif()

include(GNUInstallDirs)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Define Build Options
option(HWINFO_SHARED "Build as shared library" ON)
option(HWINFO_STATIC "Build as static library" OFF)

if(HWINFO_STATIC AND HWINFO_SHARED)
    message(WARNING "Both HWINFO_STATIC and HWINFO_SHARED are enabled. Defaulting to SHARED.")
    set(HWINFO_BUILD SHARED)
elseif(HWINFO_STATIC)
    set(HWINFO_BUILD STATIC)
else()
    set(HWINFO_BUILD SHARED)
endif()

# Enable Components
option(HWINFO_OS "Enable OS information module" ON)
option(HWINFO_MAINBOARD "Enable mainboard information module" ON)
option(HWINFO_CPU "Enable CPU information module" ON)
option(HWINFO_DISK "Enable disk information module" ON)
option(HWINFO_RAM "Enable RAM information module" ON)
option(HWINFO_GPU "Enable GPU information module" ON)
option(HWINFO_GPU_OPENCL "Enable OpenCL support for GPU" OFF)
option(HWINFO_BATTERY "Enable battery information module" ON)
option(HWINFO_NETWORK "Enable network information module" ON)
option(HWINFO_MONITOR "Enable monitor information module" ON)

# Define Examples & Testing Options
option(BUILD_EXAMPLES "Build example program" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_TESTING  "Build test program" ${PROJECT_IS_TOP_LEVEL})

# Windows-specific settings
if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Include source directory
add_subdirectory(src)

# Configure CMake Package Config Files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/lfreist-hwinfoConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfig.cmake"
        @ONLY
)

# Install CMake package configuration
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/hwinfo
)

install(
        EXPORT lfreist-hwinfoTargets
        FILE lfreist-hwinfoTargets.cmake
        NAMESPACE lfreist-hwinfo::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/hwinfo
)

if(BUILD_EXAMPLES OR BUILD_TESTING)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
    add_test(NAME system_info COMMAND system_info)
endif()
