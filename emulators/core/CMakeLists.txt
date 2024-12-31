add_executable(emulator main.cpp)
target_sources(emulator
    PRIVATE
        emumanager.cpp
)

LinkCoreEmulatorDeps(emulator)
