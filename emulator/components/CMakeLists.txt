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
        cpu.h
        display.h
        memory.h
        multimappedmemory.h
        system.h
        timer.h
)

if(BUILD_TESTS)
    # Generate unit tests for common components
    file(GLOB_RECURSE components_test_sources
        CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/emulators/components/tests/*.cpp
    )
    add_executable(components_test ${components_test_sources})
    target_link_libraries(components_test
        PRIVATE
            gtest
            gtest_main
            EmulatorComponentsInterface
            EmulatorComponents
    )
    target_include_directories(components_test
            PRIVATE
                ${CMAKE_SOURCE_DIR}/emulators
                ${CMAKE_SOURCE_DIR}/emulators/components
        )
    gtest_discover_tests(components_test)
endif()
