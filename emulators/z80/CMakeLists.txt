
CreateNewEmulatorTarget(z80)

target_sources(z80
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/z80.cpp
)

# http://z80.info/zip/z80cpu_um.pdf
