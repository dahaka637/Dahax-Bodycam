#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string>
#include <filesystem>

class DLLInjector {
public:
    DLLInjector() = default;
    ~DLLInjector() = default;

    bool InjectIntoProcess(const wchar_t* processName,
        const std::string& dllName,
        int searchDepth = 6);

private:
    bool FindDLL(const std::string& name,
        int maxDepth,
        std::string& outPath);

    DWORD GetProcessIdByName(const wchar_t* name);
    bool InjectDLL(DWORD pid, const std::string& fullPath);
};
