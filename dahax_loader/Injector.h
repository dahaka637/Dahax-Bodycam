#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <filesystem>

// ============================================================
// Resultado da injeção
// ============================================================
enum class InjectResult
{
    Success,
    ProcessNotFound,
    DllNotFound,
    OpenProcessFailed,
    AllocFailed,
    WriteFailed,
    ThreadFailed
};

// ============================================================
// API pública
// ============================================================
namespace Injector
{
    InjectResult InjectWhenReady(
        const wchar_t* processName,
        const wchar_t* dllName,
        DWORD waitAfterFoundMs = 5000
    );

    const wchar_t* ResultToString(InjectResult r);
}
