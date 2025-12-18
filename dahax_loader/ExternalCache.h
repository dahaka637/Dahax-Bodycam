#pragma once
#include <vector>
#include <mutex>
#include <cstdint>

// ======================================================
// CONFIG (DEVE BATER COM O SHARED)
// ======================================================

constexpr int MAX_NAME_LEN = 32;
constexpr int BONE_COUNT = 17;

// ======================================================
// TIPOS BÁSICOS
// ======================================================

#pragma pack(push, 1)

struct Vec3
{
    float x, y, z;
};

struct Rotator
{
    float pitch, yaw, roll;
};

// ======================================================
// FLAGS
// ======================================================

enum ActorFlags : uint8_t
{
    ACTOR_ALIVE = 1 << 0,
    ACTOR_VISIBLE = 1 << 1,
    ACTOR_BOT = 1 << 2
};

// ======================================================
// ENTIDADE EXTERNA (PLAYER / BOT)
// ======================================================

struct ExternalActor
{
    // --------------------------------------------------
    // IDENTIDADE (NOVO)
    // --------------------------------------------------
    uint64_t actorAddress;   // <<< ENDEREÇO DO AActor (ID estável)

    // --------------------------------------------------
    // dados existentes
    // --------------------------------------------------
    char name[MAX_NAME_LEN];

    int32_t team;
    float   health;
    uint8_t flags;

    Vec3 location;           // fallback (DLL)

    Vec3    bones[BONE_COUNT];
    uint8_t boneValid[BONE_COUNT];
};

// ======================================================
// LOCAL PLAYER
// ======================================================

struct ExternalLocalPlayer
{
    // --------------------------------------------------
    // IDENTIDADE (NOVO)
    // --------------------------------------------------
    uint64_t actorAddress;   // <<< ENDEREÇO DO Pawn local

    // --------------------------------------------------
    // dados existentes
    // --------------------------------------------------
    Vec3     location;
    Vec3     cameraLocation;
    Rotator  cameraRotation;
    float    cameraFOV;
    int32_t  team;
};

#pragma pack(pop)

// ======================================================
// CACHE CENTRAL DO EXTERNO
// ======================================================

namespace ExternalCache
{
    void Initialize();
    void Shutdown();

    // Lê do SharedMemory usando SEQLOCK
    void Update();

    // Acessos thread-safe
    ExternalLocalPlayer        GetLocal();
    std::vector<ExternalActor> GetPlayers();
    std::vector<ExternalActor> GetBots();
}
