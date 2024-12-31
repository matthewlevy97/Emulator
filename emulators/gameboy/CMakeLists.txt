
CreateNewEmulatorTarget(gameboy)

target_sources(gameboy
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cpu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gameboy.cpp
)
