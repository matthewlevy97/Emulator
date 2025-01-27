CreateNewFrontendTarget(ImGUIOpenGL)
target_sources(ImGUIOpenGL
    PRIVATE
        frontend.cpp
)
UseVendor_ImGUISDL(ImGUIOpenGL)

find_package(OpenGL REQUIRED)
target_link_libraries(ImGUIOpenGL
    PUBLIC
        ${OPENGL_LIBRARIES}
)