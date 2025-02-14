#include "filedialog.h"

#include <ShObjIdl.h>
#include <windows.h>
#include <winnt.h>

namespace emulator::frontend
{

static LPWSTR StringToLPWSTR(const std::string& str) noexcept
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::vector<wchar_t> wstrTo(size_needed + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return _wcsdup(wstrTo.data());
}

std::string FileDialog::Open()
{
    std::string ret = "";

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
                                          COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return ret;
    }
    IFileOpenDialog* pFileOpen;
    COMDLG_FILTERSPEC* availableFileTypes = nullptr;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (FAILED(hr)) {
        goto cleanup_couninitialize;
    }

    availableFileTypes = new COMDLG_FILTERSPEC[fileExtensions_.size()];

    {
        int i = 0;
        for (auto& [name, extensions] : fileExtensions_) {
            availableFileTypes[i].pszName = StringToLPWSTR(name);

            std::string exts;
            for (int extIndex = 0; extIndex < extensions.size(); ++extIndex) {
                if (extIndex + 1 != extensions.size()) {
                    exts += "*." + extensions[extIndex] + ";";
                } else {
                    exts += "*." + extensions[extIndex];
                }
            }
            availableFileTypes[i].pszSpec = StringToLPWSTR(exts);

            ++i;
        }
    }

    hr = pFileOpen->SetFileTypes(fileExtensions_.size(), availableFileTypes);
    if (FAILED(hr)) {
        goto cleanup_couninitialize;
    }

    hr = pFileOpen->SetFileTypeIndex(2);
    if (FAILED(hr)) {
        goto cleanup_couninitialize;
    }

    // Show the Open dialog box.
    hr = pFileOpen->Show(NULL);

    // Get the file name from the dialog box.
    if (FAILED(hr)) {
        goto cleanup_pfileopen;
    }

    IShellItem* pItem;
    hr = pFileOpen->GetResult(&pItem);
    if (FAILED(hr)) {
        goto cleanup_pfileopen;
    }

    PWSTR pszFilePath;
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

    // Save the selected file path
    if (SUCCEEDED(hr)) {
        int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
        if (size > 0) {
            std::string utf8Str(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &utf8Str[0], size, NULL, NULL);
            ret = utf8Str;
        }

        CoTaskMemFree(pszFilePath);
    }
    pItem->Release();

cleanup_pfileopen:
    pFileOpen->Release();

cleanup_couninitialize:
    CoUninitialize();

    if (availableFileTypes != nullptr) {
        delete[] availableFileTypes;
    }

    return ret;
}

};
