#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <ShlObj.h>

#include <iostream>
#include <filesystem>

#include "Injector.h"

#pragma comment(lib, "Shell32.lib")

// ============================================================
// Utils — Processo
// ============================================================
static DWORD GetProcessIdByName(const wchar_t* name)
{
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    DWORD pid = 0;

    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, name) == 0)
            {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return pid;
}

// ============================================================
// Diretório do exe chamador
// ============================================================
static std::filesystem::path GetExeDirectory()
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

// ============================================================
// Busca recursiva da DLL
// ============================================================
static bool FindDllRecursive(
    const std::filesystem::path& base,
    const wchar_t* dllName,
    std::filesystem::path& out,
    int depth = 0
)
{
    if (!std::filesystem::exists(base) || depth > 3)
        return false;

    for (auto& entry : std::filesystem::directory_iterator(base))
    {
        try
        {
            if (entry.is_regular_file())
            {
                if (_wcsicmp(entry.path().filename().c_str(), dllName) == 0)
                {
                    out = entry.path();
                    return true;
                }
            }
            else if (entry.is_directory())
            {
                if (FindDllRecursive(entry.path(), dllName, out, depth + 1))
                    return true;
            }
        }
        catch (...) {}
    }

    return false;
}

// ============================================================
// Injeção interna
// ============================================================
static InjectResult InjectDll(DWORD pid, const std::filesystem::path& dllPath)
{
    HANDLE hProc = OpenProcess(
        PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_VM_READ,
        FALSE,
        pid
    );

    if (!hProc)
        return InjectResult::OpenProcessFailed;

    size_t size = (dllPath.wstring().length() + 1) * sizeof(wchar_t);

    void* remote = VirtualAllocEx(
        hProc,
        nullptr,
        size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!remote)
    {
        CloseHandle(hProc);
        return InjectResult::AllocFailed;
    }

    if (!WriteProcessMemory(
        hProc,
        remote,
        dllPath.c_str(),
        size,
        nullptr))
    {
        VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return InjectResult::WriteFailed;
    }

    HANDLE hThread = CreateRemoteThread(
        hProc,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)LoadLibraryW,
        remote,
        0,
        nullptr
    );

    if (!hThread)
    {
        VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return InjectResult::ThreadFailed;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProc);

    return InjectResult::Success;
}

// ============================================================
// API pública
// ============================================================
InjectResult Injector::InjectWhenReady(
    const wchar_t* processName,
    const wchar_t* dllName,
    DWORD waitAfterFoundMs
)
{
    DWORD pid = 0;

    // 1) Aguarda processo
    while ((pid = GetProcessIdByName(processName)) == 0)
        Sleep(500);

    // 2) Aguarda estabilização
    Sleep(waitAfterFoundMs);

    // 3) Procura DLL
    std::filesystem::path dllPath;

    if (!FindDllRecursive(GetExeDirectory(), dllName, dllPath))
    {
        wchar_t desktop[MAX_PATH]{};
        SHGetFolderPathW(nullptr, CSIDL_DESKTOP, nullptr, 0, desktop);

        std::filesystem::path fallback =
            std::filesystem::path(desktop) / L"dahax";

        if (!FindDllRecursive(fallback, dllName, dllPath))
            return InjectResult::DllNotFound;
    }

    // 4) Injeta
    return InjectDll(pid, dllPath);
}

const wchar_t* Injector::ResultToString(InjectResult r)
{
    switch (r)
    {
    case InjectResult::Success:             return L"Success";
    case InjectResult::ProcessNotFound:     return L"ProcessNotFound";
    case InjectResult::DllNotFound:         return L"DllNotFound";
    case InjectResult::OpenProcessFailed:   return L"OpenProcessFailed";
    case InjectResult::AllocFailed:         return L"AllocFailed";
    case InjectResult::WriteFailed:         return L"WriteFailed";
    case InjectResult::ThreadFailed:        return L"ThreadFailed";
    default:                                return L"Unknown";
    }
}
