include(GNUInstallDirs)

# Set include dir
set(HWINFO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

# Create an INTERFACE library to gather all hwinfo components
add_library(hwinfo INTERFACE)
add_library(lfreist-hwinfo::hwinfo ALIAS hwinfo)

target_include_directories(hwinfo INTERFACE
        $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(hwinfo PROPERTIES OUTPUT_NAME "hwinfo")

# ------------------------------------------------------------------------------------------------
# add_hwinfo_component(NAME [SOURCES ...] [LINK_LIBS ...] [COMPILE_DEFS ...] [INCLUDE_DIRS ...])
#
# Creates:
#   1) A real library target called hwinfo_<NAME>.
#   2) An ALIAS lfreist-hwinfo::<NAME> pointing to hwinfo_<NAME>.
#   3) Adds hwinfo_<NAME> into the aggregator library "hwinfo".
#   4) Installs hwinfo_<NAME> into the same export set (hwinfoTargets).
# ------------------------------------------------------------------------------------------------
function(add_hwinfo_component NAME)
    cmake_parse_arguments(COMP "" "" "SOURCES;LINK_LIBS;COMPILE_DEFS;INCLUDE_DIRS" ${ARGN})

    # Check that at least one source is provided
    if(NOT COMP_SOURCES)
        message(FATAL_ERROR
                "No source files provided for component '${NAME}'. "
                "Please pass at least one source via SOURCES parameter."
        )
    endif()

    # Replace '-' with '_' and reassign to CMAKE_PROJECT_NAME
    string(REPLACE "-" "_" CMAKE_PROJECT_NAME "${CMAKE_PROJECT_NAME}")

    # Create the library target for this component (e.g., hwinfo_cpu).
    add_library(hwinfo_${NAME} ${COMP_SOURCES})
    set_target_properties(hwinfo_${NAME} PROPERTIES
            OUTPUT_NAME "hwinfo_${NAME}"
    )

    # Create an ALIAS for easy usage (lfreist-hwinfo::hwinfo_<NAME>).
    add_library(lfreist-hwinfo::hwinfo_${NAME} ALIAS hwinfo_${NAME})

    # Include directories
    target_include_directories(hwinfo_${NAME} PUBLIC
            $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

            # Optionally include user-specified directories
            ${COMP_INCLUDE_DIRS}
    )

    # Compile definitions
    target_compile_definitions(hwinfo_${NAME} PUBLIC
            $<$<BOOL:${HWINFO_SHARED}>:${CMAKE_PROJECT_NAME}_EXPORTS>
            ${COMP_COMPILE_DEFS}
    )

    # Link libraries, if any were provided
    if(COMP_LINK_LIBS)
        target_link_libraries(hwinfo_${NAME} PRIVATE ${COMP_LINK_LIBS})
    endif()

    # Make the INTERFACE library "hwinfo" depend on this component.
    target_link_libraries(hwinfo INTERFACE hwinfo_${NAME})

    # Install the component library (into the same export set).
    install(TARGETS hwinfo_${NAME}
            EXPORT hwinfoTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install the component's public header(s).
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/${NAME}.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo
            OPTIONAL  # If the header doesn't exist, won't fail the install
    )
endfunction()

# === Components =======================================================================================================

if (HWINFO_BATTERY)
    set(BATTERY_SOURCES
            battery.cpp
            apple/battery.cpp
            linux/battery.cpp
            windows/battery.cpp

            windows/utils/wmi_wrapper.cpp
    )

    set(BATTERY_LINK_LIBS "")
    if (APPLE)
        list(APPEND BATTERY_LINK_LIBS
                "-framework IOKit"
                "-framework CoreFoundation"
        )
    endif()

    # Create the battery component
    add_hwinfo_component(battery
            SOURCES     ${BATTERY_SOURCES}
            LINK_LIBS   ${BATTERY_LINK_LIBS}
    )
endif()

if (HWINFO_CPU)
    set(CPU_SOURCES
            cpu.cpp
            apple/cpu.cpp
            linux/cpu.cpp
            windows/cpu.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_hwinfo_component(cpu
            SOURCES ${CPU_SOURCES}
    )
endif()

if (HWINFO_DISK)
    set(DISK_SOURCES
            disk.cpp
            apple/disk.cpp
            linux/disk.cpp
            windows/disk.cpp

            windows/utils/wmi_wrapper.cpp
            linux/utils/filesystem.cpp
    )

    set(DISK_LINK_LIBS "")
    if (APPLE)
        list(APPEND DISK_LINK_LIBS
                "-framework IOKit"
                "-framework CoreFoundation"
        )
    endif()

    add_hwinfo_component(disk
            SOURCES   ${DISK_SOURCES}
            LINK_LIBS ${DISK_LINK_LIBS}
    )
endif()

if (HWINFO_GPU)
    set(GPU_SOURCES
            gpu.cpp
            apple/gpu.cpp
            linux/gpu.cpp
            windows/gpu.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp

            PCIMapper.cpp
    )

    # Start with no special link libs or defs
    set(GPU_LINK_LIBS "")
    set(GPU_COMPILE_DEFS "")  # Only needed if we set compile definitions

    if (HWINFO_GPU_OPENCL)
        add_subdirectory(opencl)       # Builds "opencl_device" or similar
        list(APPEND GPU_COMPILE_DEFS "USE_OCL")
        list(APPEND GPU_LINK_LIBS opencl_device)
        install(DIRECTORY ${HWINFO_INCLUDE_DIR}/hwinfo/opencl
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo
        )
    endif()

    add_hwinfo_component(gpu
            SOURCES       ${GPU_SOURCES}
            LINK_LIBS     ${GPU_LINK_LIBS}
            COMPILE_DEFS  ${GPU_COMPILE_DEFS}
    )
endif()

if (HWINFO_MAINBOARD)
    set(MAINBOARD_SOURCES
            mainboard.cpp
            apple/mainboard.cpp
            linux/mainboard.cpp
            windows/mainboard.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_hwinfo_component(mainboard
            SOURCES ${MAINBOARD_SOURCES}
    )
endif()

if (HWINFO_OS)
    set(OS_SOURCES
            os.cpp
            apple/os.cpp
            linux/os.cpp
            windows/os.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_hwinfo_component(os
            SOURCES ${OS_SOURCES}
    )
endif()

if (HWINFO_RAM)
    set(RAM_SOURCES
            ram.cpp
            apple/ram.cpp
            linux/ram.cpp
            windows/ram.cpp
            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_hwinfo_component(ram
            SOURCES ${RAM_SOURCES}
    )
endif()

if (HWINFO_NETWORK)
    set(NETWORK_SOURCES
            network.cpp
            apple/network.cpp
            linux/network.cpp
            windows/network.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_hwinfo_component(network
            SOURCES ${NETWORK_SOURCES}
    )
endif()

if (HWINFO_MONITOR)
    set(MONITOR_SOURCES
            monitor.cpp
            apple/monitor.cpp
            linux/monitor.cpp
            windows/monitor.cpp
    )

    set(MONITOR_LINK_LIBS "")
    set(MONITOR_INCLUDE_DIRS "")

    if (APPLE)
        list(APPEND MONITOR_LINK_LIBS
                "-framework IOKit"
                "-framework CoreFoundation"
                "-framework CoreGraphics"
        )
    endif()

    add_hwinfo_component(monitor
            SOURCES     ${MONITOR_SOURCES}
            LINK_LIBS   ${MONITOR_LINK_LIBS}
            INCLUDE_DIRS ${MONITOR_INCLUDE_DIRS}
    )
endif()

# === Install Headers & Interface Library =============================================================================

install(FILES
        ${HWINFO_INCLUDE_DIR}/hwinfo/platform.h
        ${HWINFO_INCLUDE_DIR}/hwinfo/hwinfo.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo
)

install(DIRECTORY ${HWINFO_INCLUDE_DIR}/hwinfo/utils
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo
)

# Install the aggregator library (INTERFACE) to the same export set
install(TARGETS hwinfo
        EXPORT hwinfoTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
