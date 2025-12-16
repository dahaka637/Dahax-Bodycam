#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "SharedMemory.h"

// ======================================================
// INTERNOS
// ======================================================

static HANDLE g_hMap = nullptr;
static SharedControlBlock* g_block = nullptr;

static HANDLE g_thread = nullptr;
static volatile LONG g_running = 0;

// ======================================================
// THREAD INTERNA (HEARTBEAT + CONTROLE)
// ======================================================

static DWORD WINAPI SharedMemoryThread(LPVOID)
{
    while (InterlockedCompareExchange(&g_running, 1, 1))
    {
        if (g_block)
        {
            InterlockedIncrement(&g_block->heartbeat);

            // START REQUEST
            if (InterlockedCompareExchange(&g_block->requestStart, 0, 1) == 1)
            {
                g_block->state = (LONG)SharedState::Streaming;
            }

            // STOP REQUEST
            if (InterlockedCompareExchange(&g_block->requestStop, 0, 1) == 1)
            {
                g_block->state = (LONG)SharedState::ClientConnected;
            }
        }

        Sleep(16); // ~60 Hz
    }

    return 0;
}

// ======================================================
// INITIALIZE
// ======================================================

bool SharedMemory::Initialize()
{
    g_hMap = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedControlBlock),
        SHARED_MEM_NAME
    );

    if (!g_hMap)
        return false;

    g_block = reinterpret_cast<SharedControlBlock*>(
        MapViewOfFile(
            g_hMap,
            FILE_MAP_ALL_ACCESS,
            0, 0,
            sizeof(SharedControlBlock)
        )
        );

    if (!g_block)
    {
        CloseHandle(g_hMap);
        g_hMap = nullptr;
        return false;
    }

    // Inicializa somente se for criação nova
    if (GetLastError() != ERROR_ALREADY_EXISTS)
    {
        ZeroMemory(g_block, sizeof(SharedControlBlock));
        g_block->version = 1;
        g_block->state = (LONG)SharedState::Ready;
    }

    InterlockedExchange(&g_running, 1);

    g_thread = CreateThread(
        nullptr,
        0,
        SharedMemoryThread,
        nullptr,
        0,
        nullptr
    );

    if (!g_thread)
    {
        Shutdown();
        return false;
    }

    return true;
}

// ======================================================
// SHUTDOWN
// ======================================================

void SharedMemory::Shutdown()
{
    InterlockedExchange(&g_running, 0);

    if (g_thread)
    {
        WaitForSingleObject(g_thread, INFINITE);
        CloseHandle(g_thread);
        g_thread = nullptr;
    }

    if (g_block)
    {
        g_block->state = (LONG)SharedState::Stopped;
        UnmapViewOfFile(g_block);
        g_block = nullptr;
    }

    if (g_hMap)
    {
        CloseHandle(g_hMap);
        g_hMap = nullptr;
    }
}

// ======================================================
// ESTADO
// ======================================================

SharedState SharedMemory::GetState()
{
    if (!g_block)
        return SharedState::NotInitialized;

    return (SharedState)g_block->state;
}

// ======================================================
// ACESSO AO BLOCO (USADO PELO SENDER)
// ======================================================

SharedControlBlock* SharedMemory::GetBlock()
{
    return g_block;
}
