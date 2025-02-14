#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace emulator::frontend
{

class FileDialog
{
private:
    std::string baseDirectory_;
    std::unordered_map<std::string, std::vector<std::string>> fileExtensions_;

public:
    FileDialog(std::string baseDirectory, std::unordered_map<std::string, std::vector<std::string>> fileExtensions = {})
        : baseDirectory_(baseDirectory), fileExtensions_(fileExtensions)
    {
    }

    ~FileDialog() = default;

    std::string Open();
};

}; // namespace emulator::frontend
