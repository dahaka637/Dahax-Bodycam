#pragma once
#include <vector>
#include <array>
#include <mutex>
#include <cstdint>
#include "SDK.hpp"

// ======================================================
// CONFIGURAÇÃO
// ======================================================

constexpr int MAX_NAME_LEN = 32;

// ======================================================
// BONE IDS (FIXOS, SEM STRING)
// ======================================================


struct BoneDef
{
    const char* Name;
};

static constexpr BoneDef BoneDefs[] =
{
    { "pelvis" },
    { "spine_03" },
    { "head" },

    { "clavicle_l" },
    { "upperarm_l" },
    { "lowerarm_l" },
    { "hand_l" },

    { "clavicle_r" },
    { "upperarm_r" },
    { "lowerarm_r" },
    { "hand_r" },

    { "thigh_l" },
    { "calf_l" },
    { "foot_l" },

    { "thigh_r" },
    { "calf_r" },
    { "foot_r" }
};

constexpr size_t BONE_COUNT = sizeof(BoneDefs) / sizeof(BoneDefs[0]);

// ======================================================
// FLAGS DE ENTIDADE (BITMASK)
// ======================================================

enum ActorFlags : uint8_t
{
    ACTOR_ALIVE = 1 << 0,
    ACTOR_VISIBLE = 1 << 1,
    ACTOR_BOT = 1 << 2
};

// ======================================================
// CACHE DE ENTIDADE (INTERNO)
// ======================================================

struct CachedActor
{
    // ponteiros internos (NUNCA exportar)
    SDK::AActor* Actor = nullptr;
    SDK::APawn* Pawn = nullptr;
    SDK::APlayerState* PlayerState = nullptr;

    // dados exportáveis
    char Name[MAX_NAME_LEN]{};
    int Team = -1;
    float Health = 0.0f;
    SDK::FVector Location{};

    uint8_t Flags = 0;

    // ossos world-space (array fixo)
    std::array<SDK::FVector, BONE_COUNT> Bones{};
    std::array<bool, BONE_COUNT> BoneValid{};
};

// ======================================================
// CACHE DO LOCAL PLAYER
// ======================================================

struct LocalPlayerData
{
    // ponteiros internos
    SDK::APlayerController* Controller = nullptr;
    SDK::APawn* Pawn = nullptr;
    SDK::APlayerState* PlayerState = nullptr;

    // dados exportáveis
    SDK::FVector Location{};
    SDK::FVector CameraLocation{};
    SDK::FRotator CameraRotation{};
    float CameraFOV = 0.0f; // <<< FOV REAL DO POV
    int Team; // <<< ADICIONAR
};


// ======================================================
// CACHE GLOBAL
// ======================================================

namespace Cache
{
    extern std::mutex DataMutex;

    extern LocalPlayerData Local;
    extern std::vector<CachedActor> Players;
    extern std::vector<CachedActor> Bots;

    bool Initialize();
    void Update();
    void Shutdown();
}
