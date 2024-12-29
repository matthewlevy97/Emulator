add_library(EmulatorComponents OBJECT)

# Add the headers and sources
target_sources(EmulatorComponents
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/bus.cpp
)

# Include directories
target_include_directories(EmulatorComponents
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(EmulatorComponentsInterface INTERFACE)
target_include_directories(EmulatorComponentsInterface INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/EmulatorComponents>
)
target_precompile_headers(EmulatorComponentsInterface
    INTERFACE
        bus.h
        component.h
        system.h
)
