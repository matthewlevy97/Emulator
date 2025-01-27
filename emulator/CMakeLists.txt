function(CreateNewEmulatorTarget target_name)
    if(BUILD_TESTS AND WIN32)
        add_library(${target_name} STATIC "")
        message(STATUS "Windows is annoying and needs .lib files built instead of .dlls here")
        message(STATUS "Rebuild without BUILD_TESTS enabled to actually run")
    else()
        add_library(${target_name} SHARED "")
    endif()

    target_link_libraries(${target_name}
        PRIVATE
            EmulatorComponentsInterface
            EmulatorComponents
            spdlog::spdlog_header_only
    )

    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulator
            ${CMAKE_SOURCE_DIR}/emulator/debugger
    )

    if(NOT (BUILD_TESTS AND WIN32))
        install(TARGETS ${target_name} DESTINATION bin/systems/)
    endif()

    if(BUILD_TESTS)
        file(GLOB_RECURSE emulator_sources
            CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/emulator/systems/${target_name}/tests/*.cpp
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
                ${CMAKE_SOURCE_DIR}/emulator
                ${CMAKE_SOURCE_DIR}/emulator/systems/${target_name}
        )
        gtest_discover_tests(${target_name}_test)
    endif()
endfunction()

function(CreateNewFrontendTarget target_name)
    add_library(${target_name} SHARED "")

    target_link_libraries(${target_name}
        PRIVATE
            EmulatorComponentsInterface
            EmulatorComponents

            debugger
            
            spdlog::spdlog_header_only
    )
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulator
            ${CMAKE_SOURCE_DIR}/emulator/debugger
    )

    install(TARGETS ${target_name} DESTINATION bin/lib/)
endfunction()

function(LinkCoreEmulatorDeps target_name target_frontend)
    target_link_libraries(${target_name}
        PRIVATE
            EmulatorComponentsInterface
            EmulatorComponents

            debugger
            ${target_frontend}

            spdlog::spdlog_header_only
    )
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulator
            ${CMAKE_SOURCE_DIR}/emulator/debugger
            ${CMAKE_SOURCE_DIR}/emulator/frontends
    )

    install(TARGETS ${target_name} DESTINATION bin)
endfunction()

add_subdirectory(components)

# Emulators to build
add_subdirectory(systems)

# Frontends / UIs
add_subdirectory(frontends)

# Core Features
add_subdirectory(debugger)
add_subdirectory(core)
