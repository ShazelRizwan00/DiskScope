//Shazel

#include "platform/NativeFolderDialog.h"

#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#endif

namespace diskscope::platform {

std::optional<std::filesystem::path> pickFolder(const std::filesystem::path& initialDirectory) {
#if defined(_WIN32)
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool shouldUninit = SUCCEEDED(hrInit);
    if (!SUCCEEDED(hrInit) && hrInit != RPC_E_CHANGED_MODE) {
        return std::nullopt;
    }

    std::optional<std::filesystem::path> result = std::nullopt;

    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr) && pfd != nullptr) {
        DWORD dwOptions = 0;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }

        std::error_code ec;
        if (!initialDirectory.empty() && std::filesystem::exists(initialDirectory, ec)) {
            IShellItem* psiInitial = nullptr;
            std::wstring initialW = initialDirectory.wstring();
            if (SUCCEEDED(SHCreateItemFromParsingName(initialW.c_str(), nullptr,
                                                     IID_PPV_ARGS(&psiInitial))) && psiInitial != nullptr) {
                pfd->SetFolder(psiInitial);
                psiInitial->Release();
            }
        }

        pfd->SetTitle(L"Select Folder to Scan");

        if (SUCCEEDED(pfd->Show(nullptr))) {
            IShellItem* psiResult = nullptr;
            if (SUCCEEDED(pfd->GetResult(&psiResult)) && psiResult != nullptr) {
                PWSTR pszPath = nullptr;
                if (SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath != nullptr) {
                    result = std::filesystem::path(pszPath);
                    CoTaskMemFree(pszPath);
                }
                psiResult->Release();
            }
        }
        pfd->Release();
    }

    if (shouldUninit) {
        CoUninitialize();
    }

    return result;
#else
    (void)initialDirectory;
    return std::nullopt;
#endif
}

}  // namespace diskscope::platform
