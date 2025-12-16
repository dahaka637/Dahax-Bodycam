#include "DLLInjector.h"
#include <TlHelp32.h>
#include <iostream>
#include "OverlayConfig.h"

// =============================================================
// Busca recursiva segura
// =============================================================
static bool FindDLLRecursiveSafe(
    const std::filesystem::path& dir,
    const std::string& name,
    int depth,
    int maxDepth,
    std::filesystem::path& out
) {
    if (depth > maxDepth)
        return false;

    try {
        for (const auto& e : std::filesystem::directory_iterator(dir)) {
            if (e.is_regular_file() && e.path().filename() == name) {
                out = e.path();
                return true;
            }
        }

        for (const auto& e : std::filesystem::directory_iterator(dir)) {
            if (e.is_directory()) {
                if (FindDLLRecursiveSafe(e.path(), name, depth + 1, maxDepth, out))
                    return true;
            }
        }
    }
    catch (...) {}

    return false;
}

// =============================================================
// FindDLL – busca inteligente subindo diretórios
// =============================================================
bool DLLInjector::FindDLL(const std::string& name,
    int maxDepth,
    std::string& outPath)
{
    std::filesystem::path base = std::filesystem::current_path();
    std::filesystem::path found;

    std::filesystem::path walker = base;
    for (int i = 0; i < 6 && walker.has_parent_path(); ++i) {
        if (FindDLLRecursiveSafe(walker, name, 0, maxDepth, found)) {
            outPath = found.string();
            std::cout << "[+] DLL encontrada: " << outPath << "\n";
            return true;
        }
        walker = walker.parent_path();
    }

    std::cout << "[-] DLL não encontrada: " << name << "\n";
    return false;
}

// =============================================================
// PID
// =============================================================
DWORD DLLInjector::GetProcessIdByName(const wchar_t* name)
{
    PROCESSENTRY32W pe{ sizeof(pe) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (!_wcsicmp(pe.szExeFile, name)) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return pid;
}

// =============================================================
// InjectIntoProcess (API limpa)
// =============================================================
bool DLLInjector::InjectIntoProcess(const wchar_t* processName,
    const std::string& dllName,
    int searchDepth)
{
    DWORD pid = GetProcessIdByName(processName);
    if (!pid) {
        std::cout << "[-] Processo não encontrado\n";
        return false;
    }

    std::cout << "[+] Processo encontrado (PID " << pid << ")\n";

    std::string dllPath;
    if (!FindDLL(dllName, searchDepth, dllPath))
        return false;

    return InjectDLL(pid, dllPath);
}

// =============================================================
// Injeção real
// =============================================================
bool DLLInjector::InjectDLL(DWORD pid, const std::string& fullPath)
{
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_VM_READ,
        FALSE,
        pid
    );

    if (!hProcess) {
        std::cout << "[-] OpenProcess falhou (" << GetLastError() << ")\n";
        return false;
    }

    SIZE_T size = fullPath.size() + 1;
    LPVOID remoteMem = VirtualAllocEx(
        hProcess, nullptr,
        size, MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!remoteMem) {
        CloseHandle(hProcess);
        return false;
    }

    WriteProcessMemory(hProcess, remoteMem, fullPath.c_str(), size, nullptr);

    auto loadLibrary = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(
        hProcess, nullptr, 0,
        loadLibrary,
        remoteMem, 0, nullptr
    );

    if (!hThread) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    DWORD base = 0;
    GetExitCodeThread(hThread, &base);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    if (!base) {
        std::cout << "[-] LoadLibrary falhou\n";
        return false;
    }

    std::cout << "[+] DLL injetada (base 0x" << std::hex << base << std::dec << ")\n";
    return true;
}
