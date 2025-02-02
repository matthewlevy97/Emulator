add_executable(${EMULATOR_NAME} main.cpp)
target_sources(${EMULATOR_NAME}
    PRIVATE
        emumanager.cpp
)

LinkCoreEmulatorDeps()
