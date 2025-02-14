#include "frontend.h"

#include "filedialog/filedialog.h"

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

ImGuiFrontend::ImGuiFrontend(emulator::core::EmulatorManager* manager) : emulator::frontend::IFrontend(manager)
{
    frontendInterface_.OpenFileDialog = [this]() -> std::string {
        auto fd = FileDialog("", {{"Chip8", {"c8", "ch8"}},
                                  {"GameBoy", {"gb"}},
                                  {"GameBoy Color", {"gb", "gbc"}},
                                  {"All Files", {"*"}}});
        return fd.Open();
    };
}

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

void ImGuiFrontend::ScaleSystemDisplay(std::size_t scale) noexcept
{
    SDL_Window* window = (SDL_Window*)window_;

    if (scale == 0 || display_ == nullptr) {
        return;
    }

    // Update window size
    display_->SetScale(scale);
    SDL_SetWindowSize(window, (int)display_->GetWidth() * scale, (int)display_->GetHeight() * scale + menuBarHeight_);
}

void ImGuiFrontend::Run()
{
    SDL_Window* window = (SDL_Window*)window_;
    SDL_Renderer* renderer = SDL_GetRenderer(window);

    ImGuiIO& io = ImGui::GetIO();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool running = true;
    while (running) {
        auto startTick = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
            case SDL_EVENT_QUIT:
                systemStatus_ = emulator::component::SystemStatus::STOPPING;
                running = false;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    systemStatus_ = emulator::component::SystemStatus::STOPPING;
                    running = false;
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                for (auto& input : inputs_) {
                    input->PressKey(event.key.key);
                }
                break;
            case SDL_EVENT_KEY_UP:
                for (auto& input : inputs_) {
                    input->ReleaseKey(event.key.key);
                }
                break;
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

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Emulator")) {
                for (const auto& emulatorName : manager_->GetLoadedEmulators()) {
                    if (ImGui::MenuItem(emulatorName.c_str())) {
                        LoadSystem(emulatorName);
                        ScaleSystemDisplay(5);
                    }
                }
                ImGui::EndMenu();
            }

            if (display_ != nullptr) {
                if (ImGui::BeginMenu("Display")) {
                    if (ImGui::MenuItem("Scale +")) {
                        ScaleSystemDisplay(display_->GetScale() + 1);
                    }
                    if (ImGui::MenuItem("Scale -")) {
                        ScaleSystemDisplay(display_->GetScale() - 1);
                    }
                    ImGui::EndMenu();
                }
            }

            if (system_ != nullptr) {
                if (ImGui::BeginMenu(system_->Name().c_str())) {
                    // Emulator power settings
                    if (ImGui::MenuItem("Power Off")) {
                        StopSystem();
                    }
                    if (ImGui::MenuItem("Power On")) {
                        RunSystem();
                    }

                    // Custom system functions
                    for (const auto& [name, function] : system_->GetFrontendFunctions()) {
                        if (ImGui::MenuItem(name.c_str())) {
                            function(frontendInterface_);
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            if (menuBarHeight_ == 0) {
                menuBarHeight_ = ImGui::GetWindowHeight();
            }

            ImGui::EndMainMenuBar();
        }

        // Render system display into current window
        static ImVec2 topLeft = ImVec2(0.0f, menuBarHeight_);

        SDL_Texture* texture = nullptr;
        if (display_ != nullptr) {
            // Display system screen
            std::size_t displayWidth, displayHeight;
            auto pixelData = display_->GetPixelData(displayWidth, displayHeight);

            // Create surface from pixel data
            SDL_Surface* surface = SDL_CreateSurfaceFrom(displayWidth, displayHeight,
                                                         SDL_PIXELFORMAT_RGBA8888, pixelData,
                                                         displayWidth * sizeof(Uint32));
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
            delete pixelData;

            ImVec2 bottomRight = ImVec2(float(displayWidth), float(displayHeight) + menuBarHeight_);
            ImGui::GetBackgroundDrawList()->AddImage((unsigned long long)texture, topLeft, bottomRight);
        }

        // Rendering
        ImGui::Render();
        // SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        // Cleanup texture
        if (texture) {
            SDL_DestroyTexture(texture);
        }

        // Sleep until next tick
        auto endTick = SDL_GetTicks();
        if (endTick - startTick < 1000 / targetFPS_) {
            SDL_Delay((1000 / targetFPS_) - (endTick - startTick));
        }
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
