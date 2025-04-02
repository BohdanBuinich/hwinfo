cmake_minimum_required(VERSION 3.15)

project(SystemInfo
        VERSION 1.0.0
        DESCRIPTION "System Info Tool using lfreist-hwinfo and fmt"
        LANGUAGES CXX
)

# Set the C++ standard (modify if you need C++14, C++20, etc.)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Attempt to find libfmt
find_package(fmt QUIET)

# If not found, fetch fmt from GitHub
if(NOT fmt_FOUND)
    message(STATUS " -> libfmt not found. Fetching from GitHub...")

    include(FetchContent)
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG        11.1.4
    )
    FetchContent_MakeAvailable(fmt)
else()
    message(STATUS " -> Using installed libfmt")
endif()

# Define the system_info executable
add_executable(system_info
        system_infoMain.cpp
)

# Link the required libraries
target_link_libraries(system_info
        PRIVATE
        lfreist-hwinfo::hwinfo
        fmt::fmt
)

# Install the executable
install(TARGETS system_info
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
