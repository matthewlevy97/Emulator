#include "emumanager.h"

#include <spdlog/spdlog.h>

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace emulator::core {

EmulatorManager::EmulatorManager(std::uint16_t debuggerPort) : debugger_(debuggerPort, true)
{}

EmulatorManager::~EmulatorManager() noexcept
{
    for (auto& it : loadedEmulators_) {
#ifdef _WIN32
        FreeLibrary(it.second);
#else
        dlclose(it.second);
#endif
    }
}

bool EmulatorManager::LoadEmulator(std::string name) noexcept
{
    // Don't reload the emulator if it's already loaded
    if (loadedEmulators_.find(name) != loadedEmulators_.end()) {
        spdlog::info("Already loaded emulator: {}", name);
        return true;
    }

    std::string sharedObjectPath;
#ifdef _WIN32
    char buffer[MAX_PATH+1];
#else
    char buffer[PATH_MAX+1];
#endif

#ifdef _WIN32
    if(!GetModuleFileName(NULL, buffer, sizeof(buffer))) {
        spdlog::debug("{}: GetModuleFileName failed", __FUNCTION__, name);
        return false;
    }
    std::string exePath = std::string(buffer);
    std::string exeDir = exePath.substr(0, exePath.find_last_of(std::filesystem::path::preferred_separator));
    sharedObjectPath = exeDir + std::filesystem::path::preferred_separator + sharedObjectPath;
#elif __APPLE__
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size)) {
        spdlog::debug("{}: _NSGetExecutablePath failed", __FUNCTION__, name);
        return false;
    }
    std::string exePath = std::string(buffer);
    std::string exeDir = exePath.substr(0, exePath.find_last_of(std::filesystem::path::preferred_separator));
    sharedObjectPath = exeDir + std::filesystem::path::preferred_separator + sharedObjectPath;
#else
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        return false;
    }
    buffer[len] = '\0';
    std::string exePath = std::string(buffer);
    std::string exeDir = exePath.substr(0, exePath.find_last_of(std::filesystem::path::preferred_separator));
    sharedObjectPath = exeDir + std::filesystem::path::preferred_separator + sharedObjectPath;
#endif
    
#ifdef _WIN32
    sharedObjectPath = sharedObjectPath +
        "systems" +
        std::filesystem::path::preferred_separator +
        name + ".dll";
#elif __APPLE__
    sharedObjectPath = sharedObjectPath +
        "systems" +
        std::filesystem::path::preferred_separator +
        "lib" + name + ".dylib";
#else
    sharedObjectPath = sharedObjectPath +
        "systems" +
        std::filesystem::path::preferred_separator +
        "lib" + name + ".so";
#endif

    SO_HANDLE handle;
#ifdef _WIN32
    handle = LoadLibrary(sharedObjectPath.c_str());
    if (!handle) {
        spdlog::debug("{}: Failed loading library: {}", __FUNCTION__, sharedObjectPath);
        return false;
    }
#else
    handle = dlopen(sharedObjectPath.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        spdlog::debug("{}: Failed loading library: {} {}", __FUNCTION__, sharedObjectPath, dlerror());
        return false;
    }
    dlerror();
#endif

    loadedEmulators_[name] = handle;
    return true;
}

component::CreateSystemFunc EmulatorManager::GetSystem(std::string name) noexcept
{
    static const char* symbolName = "CreateSystem";

    auto it = loadedEmulators_.find(name);
    if (it == loadedEmulators_.end()) {
        return nullptr;
    }
    SO_HANDLE handle = it->second;

#ifdef _WIN32
    FARPROC symbol = GetProcAddress(handle, symbolName);
#else
    void* symbol = dlsym(handle, symbolName);
#endif

    return reinterpret_cast<component::CreateSystemFunc>(symbol);
}

}; // namespace emulator::core
