function(CreateNewEmulatorTarget target_name)
    add_library(${target_name} SHARED "")

    target_link_libraries(${target_name} PRIVATE EmulatorComponentsInterface EmulatorComponents)

    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulators
    )

    install(TARGETS ${target_name} DESTINATION bin/systems/)

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
endfunction()

function(LinkCoreEmulatorDeps target_name)
    target_link_libraries(${target_name} PRIVATE EmulatorComponentsInterface EmulatorComponents)
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
add_subdirectory(core)
