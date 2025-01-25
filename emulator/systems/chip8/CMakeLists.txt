CreateNewEmulatorTarget(chip8)

target_sources(chip8
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/chip8.cpp
)
