function(UseVendor_ImGUISDL target_name)
    option(ENABLE_IMGUI_DEMO "Enable ImGui Demo Window" OFF)

    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/emulator/frontends/vendor/imgui")
    set(SDL_DIR "${CMAKE_SOURCE_DIR}/emulator/frontends/vendor/SDL")

    set(BUILD_SHARED_LIBS OFF)

    # https://github.com/libsdl-org/SDL
    set(SDL_LIBC ON)
    set(SDL_TEST OFF)
    set(SDL_STATIC ON)

    add_subdirectory(${SDL_DIR} ${CMAKE_BINARY_DIR}/vendor/SDL)

    target_sources(${target_name}
        PRIVATE
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp

            ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
            ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    )

    if (ENABLE_IMGUI_DEMO)
        target_sources(${target_name}
            PRIVATE
                ${IMGUI_DIR}/imgui_demo.cpp
        )

        target_compile_definitions(${target_name} PRIVATE ENABLE_IMGUI_DEMO=1)
    endif()

    target_include_directories(${target_name}
        PRIVATE
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends

            ${SDL_DIR}/include
    )

    target_link_libraries(${target_name}
        PUBLIC
            SDL3::SDL3
    )
endfunction()

function(UseFileDialog target_name)
    if(WIN32)
        set(PLATFORM_SPECIFIC_SRCS "filedialog_win.cpp")
    elseif(APPLE)
        set(PLATFORM_SPECIFIC_SRCS "filedialog_macos.mm")
        target_link_options(${target_name} PRIVATE -framework AppKit)
    elseif(UNIX)
        set(PLATFORM_SPECIFIC_SRCS "filedialog_nix.cpp")
    else()
        message(FATAL_ERROR "Unsupported operating system")
    endif()

    set(FILEDIALOG_DIR "${CMAKE_SOURCE_DIR}/emulator/frontends/filedialog")

    target_sources(${target_name}
        PRIVATE
            ${FILEDIALOG_DIR}/${PLATFORM_SPECIFIC_SRCS}
    )
    target_include_directories(${target_name}
        PRIVATE
            ${FILEDIALOG_DIR}
    )
endfunction()

# Frontends
add_subdirectory(ImGUIOpenGL)
