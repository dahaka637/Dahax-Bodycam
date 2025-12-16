#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>

#include "SharedIPC.h" // <<< contém SharedPayload (COM seq)

// ======================================================
// NOME DO SHARED MEMORY (IDÊNTICO AO DA DLL)
// ======================================================

#define SHARED_MEM_NAME L"Local\\DAHAX_SHARED_MEM_V1"

// ======================================================
// ESTADOS (IDÊNTICOS AOS DA DLL)
// ======================================================

enum class SharedState : uint32_t
{
    NotInitialized = 0,
    Ready,
    ClientConnected,
    Streaming,
    Stopped
};

// ======================================================
// BLOCO COMPARTILHADO (IPC-SAFE)
// ======================================================

#pragma pack(push, 1)

struct SharedControlBlock
{
    uint32_t version;

    volatile LONG state;
    volatile LONG heartbeat;

    volatile LONG requestStart;
    volatile LONG requestStop;

    SharedAimbotConfig aimbot; // <<< NOVO (externo escreve, DLL lê)

    SharedPayload payload;     // streaming (DLL escreve)
};


#pragma pack(pop)

// ======================================================
// API DO SOFTWARE EXTERNO
// ======================================================

namespace SharedMemory
{
    // Conexão resiliente (não intrusiva)
    bool Connect();
    void Disconnect();

    // Estado
    bool IsConnected();
    bool IsAlive();

    SharedState GetState();

    // Detecta se já existe DLL injetada
    bool IsAlreadyInjected();

    // Requests (THREAD-SAFE)
    void RequestStart();
    void RequestStop();

    // Tick opcional (monitoramento / reconnect)
    void Tick();

    // Acesso direto ao bloco (somente leitura pelo externo)
    SharedControlBlock* GetBlock();
}
