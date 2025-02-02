# ImGUIOpenGL
if (TARGET_FRONTEND STREQUAL "Desktop")

    target_sources(${EMULATOR_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/frontend_init.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/frontend.cpp
    )
    UseVendor_ImGUISDL(${EMULATOR_NAME})

    find_package(OpenGL REQUIRED)
    target_link_libraries(${EMULATOR_NAME}
        PUBLIC
            ${OPENGL_LIBRARIES}
    )

endif()