#include "loader.h"

namespace emulator::core {

EmulatorManager::~EmulatorManager() {
    for (auto handle : loadedEmulators) {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    }
}

bool EmulatorManager::LoadEmulator(std::string name) {
    const char* sharedObjectPath = "/path/to/shared/object.so";
    const char* symbolName = "CreateSystem";

    #ifdef _WIN32
        HMODULE handle = LoadLibrary(sharedObjectPath);
        if (!handle) {
            return false;
        }

        FARPROC symbol = GetProcAddress(handle, symbolName);
        if (!symbol) {
            FreeLibrary(handle);
            return false;
        }
    #else
        void* handle = dlopen(sharedObjectPath, RTLD_LAZY);
        if (!handle) {
            return false;
        }

        dlerror();

        void* symbol = dlsym(handle, symbolName);
        const char* dlsymError = dlerror();
        if (dlsymError) {
            dlclose(handle);
            return false;
        }
    #endif

    loadedEmulators_.push_back(handle);

    return true;
}

}; // namespace emulator::core
