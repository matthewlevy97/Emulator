CreateNewEmulatorTarget(gameboy)

target_sources(gameboy
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cpu_decode.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cpu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gameboy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ppu.cpp
)
