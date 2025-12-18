#pragma once
#include <cstdint>

// ======================================================
// CONFIG (DEVE SER IGUAL EM TODOS OS PROJETOS)
// ======================================================

constexpr uint32_t SHARED_MAX_PLAYERS = 16;
constexpr uint32_t SHARED_MAX_BOTS = 16;
constexpr uint32_t SHARED_MAX_NAME = 32;
constexpr uint32_t SHARED_BONE_COUNT = 17;

// ======================================================
// TIPOS BÁSICOS (IPC-SAFE)
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
    // --------------------------------------------------
    // IDENTIDADE
    // --------------------------------------------------
    uint64_t actorAddress;     // <<< ENDEREÇO DO AActor (ID estável)

    // --------------------------------------------------
    // dados existentes
    // --------------------------------------------------
    char name[SHARED_MAX_NAME];   // sempre null-terminated pelo writer

    int32_t team;
    float   health;
    uint8_t flags;

    SharedVec3 location;          // fallback (DLL-side)

    SharedVec3 bones[SHARED_BONE_COUNT];
    uint8_t    boneValid[SHARED_BONE_COUNT];
};

// ======================================================
// LOCAL PLAYER
// ======================================================

struct SharedLocalPlayer
{
    // --------------------------------------------------
    // IDENTIDADE
    // --------------------------------------------------
    uint64_t actorAddress;     // <<< ENDEREÇO DO Pawn local

    // --------------------------------------------------
    // dados existentes
    // --------------------------------------------------
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
    uint32_t seq;          // seqlock (ímpar = escrita / par = estável)
    uint32_t frameId;      // incrementado SOMENTE após frame completo

    SharedLocalPlayer local;

    uint32_t playerCount;
    uint32_t botCount;

    SharedActor players[SHARED_MAX_PLAYERS];
    SharedActor bots[SHARED_MAX_BOTS];
};

#pragma pack(pop)

// ======================================================
// SANITY CHECKS (RECOMENDADO)
// ======================================================

static_assert(sizeof(uint8_t) == 1, "uint8_t deve ter 1 byte");
static_assert(sizeof(uint64_t) == 8, "uint64_t deve ter 8 bytes");
static_assert(sizeof(SharedVec3) == 12, "SharedVec3 layout inválido");
static_assert(sizeof(SharedRotator) == 12, "SharedRotator layout inválido");
