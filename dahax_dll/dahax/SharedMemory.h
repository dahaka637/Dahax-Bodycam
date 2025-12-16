#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>

// ======================================================
// SHARED MEMORY NAME
// ======================================================

#define SHARED_MEM_NAME L"Local\\DAHAX_SHARED_MEM_V1"

// ======================================================
// LIMITES (DEVEM SER IGUAIS EM TODOS OS PROJETOS)
// ======================================================

constexpr uint32_t SHARED_MAX_PLAYERS = 16;
constexpr uint32_t SHARED_MAX_BOTS = 16;
constexpr uint32_t SHARED_MAX_NAME = 32;
constexpr uint32_t SHARED_BONE_COUNT = 17;

// ======================================================
// ESTADOS
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
// TIPOS IPC-SAFE
// ======================================================

#pragma pack(push, 1)

struct SharedVec3
{
    float x;
    float y;
    float z;
};

struct SharedRotator
{
    float pitch;
    float yaw;
    float roll;
};

// ======================================================
// FLAGS
// ======================================================

enum SharedActorFlags : uint8_t
{
    SH_ACTOR_ALIVE = 1 << 0,
    SH_ACTOR_VISIBLE = 1 << 1,
    SH_ACTOR_BOT = 1 << 2
};

// ======================================================
// ACTOR
// ======================================================

struct SharedActor
{
    char name[SHARED_MAX_NAME];   // sempre null-terminated

    int32_t team;
    float   health;
    uint8_t flags;

    SharedVec3 location;

    SharedVec3 bones[SHARED_BONE_COUNT];
    uint8_t    boneValid[SHARED_BONE_COUNT];
};

// ======================================================
// LOCAL PLAYER
// ======================================================

struct SharedLocalPlayer
{
    SharedVec3     location;
    SharedVec3     cameraLocation;
    SharedRotator  cameraRotation;
    float          cameraFOV;
    int32_t        team;
};

// ======================================================
// PAYLOAD (SEQLOCK + FRAME SYNC)
// ======================================================

struct SharedPayload
{
    volatile uint32_t seq;        // seqlock (ímpar = escrita / par = estável)
    uint32_t          frameId;    // incrementado SOMENTE após frame completo

    SharedLocalPlayer local;

    uint32_t playerCount;
    uint32_t botCount;

    SharedActor players[SHARED_MAX_PLAYERS];
    SharedActor bots[SHARED_MAX_BOTS];
};

// ======================================================
// AIMBOT CONFIG (EXTERNO → DLL)
// ======================================================

struct SharedAimbotConfig
{
    uint8_t enabled;        // 0 / 1
    uint8_t hotkeyDown;     // 0 / 1
    uint8_t targetMode;     // enum
    uint8_t aimAtTeam;      // 0 / 1
    uint8_t visibleOnly;    // 0 / 1

    float fov;
    float smooth;

    // ===============================
    // PREDIÇÃO (0 = OFF)
    // ===============================
    float prediction;
};



// ======================================================
// BLOCO DE CONTROLE COMPARTILHADO
// ======================================================

struct SharedControlBlock
{
    uint32_t version;

    volatile LONG state;
    volatile LONG heartbeat;

    volatile LONG requestStart;
    volatile LONG requestStop;

    SharedAimbotConfig aimbot; // <<< NOVO BLOCO (EXTERNO → DLL)

    SharedPayload payload;     // <<< STREAMING (DLL → EXTERNO)
};

#pragma pack(pop)

// ======================================================
// API SHARED MEMORY
// ======================================================

namespace SharedMemory
{
    bool Initialize();
    void Shutdown();

    SharedState GetState();

    // Retorna ponteiro para o bloco compartilhado
    SharedControlBlock* GetBlock();
}

// ======================================================
// SANITY CHECKS (DEBUG / COMPILE-TIME)
// ======================================================

static_assert(sizeof(uint8_t) == 1, "uint8_t deve ter 1 byte");
static_assert(sizeof(SharedVec3) == 12, "SharedVec3 layout inválido");
static_assert(sizeof(SharedRotator) == 12, "SharedRotator layout inválido");
static_assert(sizeof(SharedAimbotConfig) == 17,
    "SharedAimbotConfig layout inesperado");
