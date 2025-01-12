function(CreateNewEmulatorTarget target_name)
    if(BUILD_TESTS AND WIN32)
        add_library(${target_name} STATIC "")
        message(STATUS "Windows is annoying and needs .lib files built instead of .dlls here")
        message(STATUS "Rebuild without BUILD_TESTS enabled to actually run")
    else()
        add_library(${target_name} SHARED "")

        if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set_target_properties(${target_name} PROPERTIES COMPILE_FLAGS "-fPIC")
        endif()
    endif()

    target_link_libraries(${target_name}
        PRIVATE
            EmulatorComponentsInterface
            EmulatorComponents
            spdlog::spdlog_header_only
    )

    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulators
            ${CMAKE_SOURCE_DIR}/emulators/debugger
    )

    if(NOT (BUILD_TESTS AND WIN32))
        install(TARGETS ${target_name} DESTINATION bin/systems/)
    endif()

    if(BUILD_TESTS)
        file(GLOB_RECURSE emulator_sources
            CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/emulators/${target_name}/tests/*.cpp
        )
        add_executable(${target_name}_test ${emulator_sources})
        target_link_libraries(${target_name}_test
            PRIVATE
                gtest
                gtest_main
                ${target_name}
        )
        target_include_directories(${target_name}_test
            PRIVATE
                ${CMAKE_SOURCE_DIR}/emulators
                ${CMAKE_SOURCE_DIR}/emulators/${target_name}
        )
        gtest_discover_tests(${target_name}_test)
    endif()
endfunction()

function(LinkCoreEmulatorDeps target_name)
    target_link_libraries(${target_name}
        PRIVATE
            EmulatorComponentsInterface
            EmulatorComponents
            spdlog::spdlog_header_only
    )
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulators
    )

    install(TARGETS ${target_name} DESTINATION bin)
endfunction()

add_subdirectory(components)

# Emulators to build
add_subdirectory(gameboy)

# Main emulator
add_subdirectory(debugger)
add_subdirectory(core)
