function(CreateNewEmulatorTarget target_name)
    add_library(${target_name} SHARED "")

    target_link_libraries(${target_name} PRIVATE EmulatorComponentsInterface EmulatorComponents)

    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulators
    )

    install(TARGETS ${target_name} DESTINATION bin/emulators)
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
add_subdirectory(z80)

# Main emulator
add_subdirectory(core)
