add_executable(${EMULATOR_NAME} main.cpp)
target_sources(${EMULATOR_NAME}
    PRIVATE
        emumanager.cpp
)
target_include_directories(${EMULATOR_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

LinkCoreEmulatorDeps()
