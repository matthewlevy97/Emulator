#include "frontend.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_surface.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

namespace emulator::frontend::imgui_opengl
{

bool ImGuiFrontend::Initialize() noexcept
{
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        return false;
    }

    Uint32 window_flags = SDL_WINDOW_HIDDEN;
    SDL_Window* window = SDL_CreateWindow(kWindowTitle, width_, height_, window_flags);
    if (window == nullptr) {
        return false;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        return false;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetRenderVSync(renderer, 1);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.IniFilename = nullptr;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    window_ = window;

    return true;
}

void ImGuiFrontend::Run()
{
    // Run the emulator in new thread
    RunSystem();

    SDL_Window* window = (SDL_Window*)window_;
    SDL_Renderer* renderer = SDL_GetRenderer(window);

    ImGuiIO& io = ImGui::GetIO();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    auto displays = system_->GetComponentsByType(emulator::component::IComponent::ComponentType::Display);
    if (displays.size() != 1) {
        spdlog::error("Expected 1 display component, found {}", displays.size());
        return;
    }
    auto display = dynamic_cast<emulator::component::Display*>(displays[0]);

    // Update window size
    SDL_SetWindowSize(window, display->GetWidth(), display->GetHeight());

    while (systemStatus_ == emulator::component::SystemStatus::RUNNING) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                systemStatus_ = emulator::component::SystemStatus::STOPPING;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
                systemStatus_ = emulator::component::SystemStatus::STOPPING;
            }
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

#if defined(ENABLE_IMGUI_DEMO)
        ImGui::ShowDemoWindow();
#endif

        // Display system screen
        std::size_t displayWidth, displayHeight;
        auto pixelData = display->GetPixelData(displayWidth, displayHeight);

        // Create surface from pixel data
        SDL_Surface* surface = SDL_CreateSurfaceFrom(displayWidth, displayHeight,
                                                     SDL_PIXELFORMAT_RGBA8888, pixelData,
                                                     displayWidth * sizeof(Uint32));
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        delete pixelData;

        // Render system display into current window
        static ImVec2 topLeft = ImVec2(0, 0);
        static ImVec2 bottomRight = ImVec2(displayWidth, displayHeight);
        ImGui::GetBackgroundDrawList()->AddImage((unsigned long long)texture, topLeft, bottomRight);

        // Rendering
        ImGui::Render();
        // SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        // Cleanup texture
        SDL_DestroyTexture(texture);
    }

    // Stop the emulator if it's still running
    StopSystem();
}

void ImGuiFrontend::Shutdown() noexcept
{
    SDL_Window* window = (SDL_Window*)window_;
    SDL_Renderer* renderer = SDL_GetRenderer(window);

    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

}; // namespace emulator::frontend::imgui_opengl