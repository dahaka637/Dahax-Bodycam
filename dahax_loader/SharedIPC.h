#pragma once
#include <cstdint>

// ======================================================
// CONFIG (TEM QUE BATER EM DLL + EXTERNO)
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
// ACTOR (PLAYER / BOT)
// ======================================================

struct SharedActor
{
    char    name[SHARED_MAX_NAME];

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
// PAYLOAD (COM SEQLOCK)
// ======================================================

struct SharedPayload
{
    uint32_t seq;        // <<< SEQLOCK (ímpar = escrevendo / par = estável)
    uint32_t frameId;    // ID do frame completo

    SharedLocalPlayer local;

    uint32_t playerCount;
    uint32_t botCount;

    SharedActor players[SHARED_MAX_PLAYERS];
    SharedActor bots[SHARED_MAX_BOTS];
};

#pragma pack(pop)

#pragma pack(push, 1)

struct SharedAimbotConfig
{
    uint8_t enabled;        // 0/1
    uint8_t hotkeyDown;     // 0/1

    uint8_t targetMode;     // 0=Head,1=Chest,2=NearestBone
    uint8_t aimAtTeam;      // 0/1
    uint8_t visibleOnly;    // 0/1

    float   fov;
    float   smooth;
    float   prediction;
};

#pragma pack(pop)
