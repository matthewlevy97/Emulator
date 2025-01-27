add_executable(emulator main.cpp)
target_sources(emulator
    PRIVATE
        emumanager.cpp
)

LinkCoreEmulatorDeps(emulator ImGUIOpenGL)

set_target_properties(emulator PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/bin/lib") 
