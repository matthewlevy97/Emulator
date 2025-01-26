#pragma once

namespace emulator {

char *Strsep(char** str, char sep) noexcept
{
    if (!*str) {
        return nullptr;
    }

    char* start = *str;
    char* end = start;
    while (*end != '\0') {
        if (*end == sep) {
            *end = '\0';
            *str = end + 1;
            return start;
        }
        ++end;
    }

    *str = nullptr;
    return start;
}

template <typename T>
T GenerateRandom() noexcept
{
    return static_cast<T>(rand());
}

}; // namespace emulator
