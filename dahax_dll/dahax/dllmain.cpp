// dllmain.cpp
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <atomic>
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#include "AimbotDebug.h"
#include "SharedMemory.h"
#include "Cache.h"
#include "SharedSender.h"
#include "AimbotCache.h"
#include "Aimbot.h"

// ======================================================
// Globals
// ======================================================
static std::atomic<bool> g_Running{ false };
static HANDLE g_CacheThread = nullptr;
static HANDLE g_SenderThread = nullptr;

static void AttachConsoleIO()
{
    AllocConsole();

    FILE* f;

    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);

    SetConsoleTitleA("DLL DEBUG");

    // opcional: modo não-bufferizado
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    printf("[DLL] Console attached\n");
}


// ======================================================
// Cache Thread
// ======================================================
static DWORD WINAPI CacheThread(LPVOID)
{
    const DWORD frameTime = 1000 / 120;

    while (g_Running.load(std::memory_order_relaxed))
    {
        Cache::Update();
        AimbotCache::Update();
        Aimbot::Tick();

        //AimbotCacheDebug::Tick();

        Sleep(frameTime);
    }

    return 0;
}

// ======================================================
// Sender Thread
// ======================================================
static DWORD WINAPI SenderThread(LPVOID)
{
    const DWORD frameTime = 1000 / 120;

    while (g_Running.load(std::memory_order_relaxed))
    {
        SharedSender::Tick();
        Sleep(frameTime);
    }

    return 0;
}

// ======================================================
// DllMain
// ======================================================
BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID
)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);

        AttachConsoleIO();

        printf("[DLL] PROCESS_ATTACH\n");
        // ==================================================
        // Shared Memory
        // ==================================================
        if (!SharedMemory::Initialize())
            return FALSE;

        // ==================================================
        // Cache (dados do jogo)
        // ==================================================
        if (!Cache::Initialize())
        {
            SharedMemory::Shutdown();
            return FALSE;
        }

        // ==================================================
        // Aimbot Cache (configs vindas do externo)
        // ==================================================
        if (!AimbotCache::Initialize())
        {
            Cache::Shutdown();
            SharedMemory::Shutdown();
            return FALSE;
        }

        //AimbotCacheDebug::Initialize();


        // ==================================================
        // Threads
        // ==================================================
        g_Running.store(true, std::memory_order_relaxed);

        g_CacheThread = CreateThread(
            nullptr,
            0,
            CacheThread,
            nullptr,
            0,
            nullptr
        );

        g_SenderThread = CreateThread(
            nullptr,
            0,
            SenderThread,
            nullptr,
            0,
            nullptr
        );

        if (!g_CacheThread || !g_SenderThread)
        {
            g_Running.store(false, std::memory_order_relaxed);

            if (g_CacheThread)
            {
                CloseHandle(g_CacheThread);
                g_CacheThread = nullptr;
            }

            if (g_SenderThread)
            {
                CloseHandle(g_SenderThread);
                g_SenderThread = nullptr;
            }

            AimbotCache::Shutdown();
            Cache::Shutdown();
            //AimbotCacheDebug::Shutdown();
            SharedMemory::Shutdown();
            return FALSE;
        }

        break;
    }

    case DLL_PROCESS_DETACH:
    {
        g_Running.store(false, std::memory_order_relaxed);

        if (g_CacheThread)
        {
            WaitForSingleObject(g_CacheThread, INFINITE);
            CloseHandle(g_CacheThread);
            g_CacheThread = nullptr;
        }

        if (g_SenderThread)
        {
            WaitForSingleObject(g_SenderThread, INFINITE);
            CloseHandle(g_SenderThread);
            g_SenderThread = nullptr;
        }

        // Ordem correta de shutdown
        AimbotCache::Shutdown();
        Cache::Shutdown();
        SharedMemory::Shutdown();

        break;
    }

    default:
        break;
    }

    return TRUE;
}
