#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>

#include "SharedMemory.h"

// ======================================================
// Internos
// ======================================================

static HANDLE g_hMap = nullptr;
static SharedControlBlock* g_block = nullptr;

static LONG      g_lastHeartbeat = 0;
static ULONGLONG g_lastHeartbeatTime = 0;

// ======================================================
// Connect (RESILIENTE, NÃO INTRUSIVO)
// ======================================================

bool SharedMemory::Connect()
{
    if (g_block)
        return true;

    g_hMap = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,
        FALSE,
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

    // Apenas observa estado inicial
    g_lastHeartbeat = g_block->heartbeat;
    g_lastHeartbeatTime = GetTickCount64();

    printf("[EXT][SM] Conectado ao SharedMemory\n");
    return true;
}

// ======================================================
// Disconnect
// ======================================================

void SharedMemory::Disconnect()
{
    if (g_block)
    {
        // Solicita parada educadamente
        InterlockedExchange(&g_block->requestStop, 1);

        UnmapViewOfFile(g_block);
        g_block = nullptr;
    }

    if (g_hMap)
    {
        CloseHandle(g_hMap);
        g_hMap = nullptr;
    }

    printf("[EXT][SM] Desconectado\n");
}

// ======================================================
// Status
// ======================================================

bool SharedMemory::IsConnected()
{
    return g_block != nullptr;
}

SharedState SharedMemory::GetState()
{
    if (!g_block)
        return SharedState::NotInitialized;

    return static_cast<SharedState>(g_block->state);
}

SharedControlBlock* SharedMemory::GetBlock()
{
    return g_block;
}

// ======================================================
// Heartbeat (ROBUSTO)
// ======================================================

bool SharedMemory::IsAlive()
{
    if (!g_block)
        return false;

    LONG hb = g_block->heartbeat;

    if (hb != g_lastHeartbeat)
    {
        g_lastHeartbeat = hb;
        g_lastHeartbeatTime = GetTickCount64();
        return true;
    }

    // Timeout tolerante (2 segundos)
    return (GetTickCount64() - g_lastHeartbeatTime) < 2000;
}

// ======================================================
// Requests (THREAD-SAFE)
// ======================================================

void SharedMemory::RequestStart()
{
    if (!g_block)
        return;

    InterlockedExchange(&g_block->requestStart, 1);
}

void SharedMemory::RequestStop()
{
    if (!g_block)
        return;

    InterlockedExchange(&g_block->requestStop, 1);
}

// ======================================================
// Tick (MONITORAMENTO OPCIONAL)
// ======================================================

void SharedMemory::Tick()
{
    if (!g_block)
        return;

    // Aqui pode entrar lógica de auto-reconnect no futuro
    // Ex:
    // if (!IsAlive()) { Disconnect(); }
}

// ======================================================
// Verifica se já existe DLL injetada
// ======================================================

bool SharedMemory::IsAlreadyInjected()
{
    HANDLE hMap = OpenFileMappingW(
        FILE_MAP_READ,
        FALSE,
        SHARED_MEM_NAME
    );

    if (!hMap)
        return false;

    auto* block = reinterpret_cast<SharedControlBlock*>(
        MapViewOfFile(
            hMap,
            FILE_MAP_READ,
            0, 0,
            sizeof(SharedControlBlock)
        )
        );

    if (!block)
    {
        CloseHandle(hMap);
        return false;
    }

    LONG hb1 = block->heartbeat;
    Sleep(100);
    LONG hb2 = block->heartbeat;

    bool alive = (hb1 != hb2);

    UnmapViewOfFile(block);
    CloseHandle(hMap);

    return alive;
}
