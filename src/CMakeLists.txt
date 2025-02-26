include(GNUInstallDirs)

# Set include dir
SET(HWINFO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

# Create INTERFACE library for hwinfo
add_library(hwinfo INTERFACE)
target_include_directories(hwinfo INTERFACE
        $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
add_library(lfreist-hwinfo::hwinfo ALIAS hwinfo)
set_target_properties(hwinfo PROPERTIES OUTPUT_NAME "hwinfo")

# Function to simplify component creation
function(add_hwinfo_component NAME)
    set(OPTIONS "")
    set(ONE_VALUE_ARGS "")
    set(MULTI_VALUE_ARGS SOURCES LINK_LIBS COMPILE_DEFS)
    cmake_parse_arguments(COMP "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    add_library(hwinfo_${NAME} ${HWINFO_BUILD})

    # Add sources dynamically
    target_sources(hwinfo_${NAME} PRIVATE ${COMP_SOURCES})

    # Include directories
    target_include_directories(hwinfo_${NAME} PUBLIC
            $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    # Compile definitions
    target_compile_definitions(hwinfo_${NAME} PUBLIC
            $<$<BOOL:${HWINFO_SHARED}>:${CMAKE_PROJECT_NAME}_EXPORTS>
            ${COMP_COMPILE_DEFS}
    )

    # Link libraries
    if (COMP_LINK_LIBS)
        target_link_libraries(hwinfo_${NAME} PRIVATE ${COMP_LINK_LIBS})
    endif()

    target_link_libraries(hwinfo INTERFACE hwinfo_${NAME})

    set_target_properties(hwinfo_${NAME} PROPERTIES OUTPUT_NAME "hwinfo_${NAME}")

    install(TARGETS hwinfo_${NAME}
            EXPORT lfreist-hwinfoTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/${NAME}.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
endfunction()

# === Components =======================================================================================================

if (HWINFO_BATTERY)
    add_hwinfo_component(battery SOURCES
            battery.cpp
            apple/battery.cpp
            linux/battery.cpp
            windows/battery.cpp

            windows/utils/wmi_wrapper.cpp
    )

    if (APPLE)
        target_link_libraries(hwinfo_battery PRIVATE "-framework IOKit" "-framework CoreFoundation")
    endif()
endif()

if (HWINFO_CPU)
    add_hwinfo_component(cpu SOURCES
            cpu.cpp
            apple/cpu.cpp
            linux/cpu.cpp
            windows/cpu.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )
endif()

if (HWINFO_DISK)
    add_hwinfo_component(disk SOURCES
            disk.cpp
            apple/disk.cpp
            linux/disk.cpp
            windows/disk.cpp

            windows/utils/wmi_wrapper.cpp
            linux/utils/filesystem.cpp
    )

    if (APPLE)
        target_link_libraries(hwinfo_disk PRIVATE "-framework IOKit" "-framework CoreFoundation")
    endif()
endif()

if (HWINFO_GPU)
    add_hwinfo_component(gpu SOURCES
            gpu.cpp
            apple/gpu.cpp
            linux/gpu.cpp
            windows/gpu.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
            PCIMapper.cpp
    )

    if (HWINFO_GPU_OPENCL)
        add_subdirectory(opencl)
        target_compile_definitions(hwinfo_gpu PUBLIC USE_OCL)
        target_link_libraries(hwinfo_gpu PRIVATE opencl_device)
        install(DIRECTORY ${HWINFO_INCLUDE_DIR}/hwinfo/opencl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
    endif()
endif()

if (HWINFO_MAINBOARD)
    add_hwinfo_component(mainboard SOURCES
            mainboard.cpp
            apple/mainboard.cpp
            linux/mainboard.cpp
            windows/mainboard.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )
endif()

if (HWINFO_OS)
    add_hwinfo_component(os SOURCES
            os.cpp
            apple/os.cpp
            linux/os.cpp
            windows/os.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )
endif()

if (HWINFO_RAM)
    add_hwinfo_component(ram SOURCES
            ram.cpp
            apple/ram.cpp
            linux/ram.cpp
            windows/ram.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )
endif()

if (HWINFO_NETWORK)
    add_hwinfo_component(network SOURCES
            network.cpp
            apple/network.cpp
            linux/network.cpp
            windows/network.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )
endif()

if (HWINFO_MONITOR)
    add_hwinfo_component(monitor SOURCES
            monitor.cpp
            apple/monitor.cpp
            linux/monitor.cpp
            windows/monitor.cpp

            linux/utils/filesystem.cpp
    )

    if (APPLE)
        target_link_libraries(hwinfo_monitor PRIVATE "-framework IOKit" "-framework CoreFoundation" "-framework CoreGraphics")
    endif()
endif()

# Install Headers and Interface Library
install(FILES
        ${HWINFO_INCLUDE_DIR}/hwinfo/platform.h
        ${HWINFO_INCLUDE_DIR}/hwinfo/hwinfo.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo
)

install(DIRECTORY ${HWINFO_INCLUDE_DIR}/hwinfo/utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)

install(TARGETS hwinfo
        EXPORT lfreist-hwinfoTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
