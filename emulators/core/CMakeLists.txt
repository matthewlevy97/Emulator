add_executable(emulator main.cpp)
target_sources(emulator
    PRIVATE
        emumanager.cpp
)

LinkCoreEmulatorDeps(emulator)

if(COMPILE_DEBUGGER)
    target_link_libraries(emulator PRIVATE debugger)
endif()
