#pragma once

#include <exception>

namespace emulator::component {

class AddressInUse : public std::exception {
public:
    const char* what() const noexcept override {
        return "Address already in use.";
    }
};

}; // namespace emulator::component
