#include "ExternalCache.h"
#include "SharedMemory.h"

#include <cstring>

// ======================================================
// STORAGE INTERNO
// ======================================================

namespace
{
    std::mutex g_mutex;

    ExternalLocalPlayer        g_local{};
    std::vector<ExternalActor> g_players;
    std::vector<ExternalActor> g_bots;

    uint32_t g_lastFrameId = 0;
}

// ======================================================
// INIT / SHUTDOWN
// ======================================================

void ExternalCache::Initialize()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_players.clear();
    g_bots.clear();
    g_lastFrameId = 0;
}

void ExternalCache::Shutdown()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_players.clear();
    g_bots.clear();
}

// ======================================================
// UPDATE (LEITURA COM SEQLOCK)
// ======================================================

void ExternalCache::Update()
{
    if (!SharedMemory::IsConnected())
        return;

    auto* block = SharedMemory::GetBlock();
    if (!block)
        return;

    if ((SharedState)block->state != SharedState::Streaming)
        return;

    // ==================================================
    // SEQLOCK READ (SNAPSHOT CONSISTENTE)
    // ==================================================

    SharedPayload snapshot{};
    uint32_t s1 = 0, s2 = 0;

    do
    {
        s1 = block->payload.seq;
        snapshot = block->payload;
        s2 = block->payload.seq;
    } while (s1 != s2 || (s1 & 1));

    // ==================================================
    // FRAME FILTER
    // ==================================================

    if (snapshot.frameId == g_lastFrameId)
        return;

    g_lastFrameId = snapshot.frameId;

    // ==================================================
    // BLINDAGEM DE CONTADORES
    // ==================================================

    uint32_t safePlayers = snapshot.playerCount;
    if (safePlayers > SHARED_MAX_PLAYERS)
        safePlayers = SHARED_MAX_PLAYERS;

    uint32_t safeBots = snapshot.botCount;
    if (safeBots > SHARED_MAX_BOTS)
        safeBots = SHARED_MAX_BOTS;

    // ==================================================
    // COMMIT THREAD-SAFE
    // ==================================================

    std::lock_guard<std::mutex> lock(g_mutex);

    // ------------------
    // LOCAL PLAYER
    // ------------------

    g_local.location = {
        snapshot.local.location.x,
        snapshot.local.location.y,
        snapshot.local.location.z
    };

    g_local.cameraLocation = {
        snapshot.local.cameraLocation.x,
        snapshot.local.cameraLocation.y,
        snapshot.local.cameraLocation.z
    };

    g_local.cameraRotation = {
        snapshot.local.cameraRotation.pitch,
        snapshot.local.cameraRotation.yaw,
        snapshot.local.cameraRotation.roll
    };

    g_local.cameraFOV = snapshot.local.cameraFOV;
    g_local.team = snapshot.local.team;

    // ------------------
    // PLAYERS
    // ------------------

    g_players.clear();
    g_players.reserve(safePlayers);

    for (uint32_t i = 0; i < safePlayers; i++)
    {
        const SharedActor& src = snapshot.players[i];
        ExternalActor dst{};

        std::memcpy(dst.name, src.name, MAX_NAME_LEN);
        dst.name[MAX_NAME_LEN - 1] = '\0';

        dst.team = src.team;
        dst.health = src.health;
        dst.flags = src.flags;

        dst.location = {
            src.location.x,
            src.location.y,
            src.location.z
        };

        for (size_t b = 0; b < BONE_COUNT; b++)
        {
            if (src.boneValid[b])
            {
                dst.boneValid[b] = 1;
                dst.bones[b] = {
                    src.bones[b].x,
                    src.bones[b].y,
                    src.bones[b].z
                };
            }
        }

        g_players.push_back(dst);
    }

    // ------------------
    // BOTS
    // ------------------

    g_bots.clear();
    g_bots.reserve(safeBots);

    for (uint32_t i = 0; i < safeBots; i++)
    {
        const SharedActor& src = snapshot.bots[i];
        ExternalActor dst{};

        std::memcpy(dst.name, src.name, MAX_NAME_LEN);
        dst.name[MAX_NAME_LEN - 1] = '\0';

        dst.team = src.team;
        dst.health = src.health;
        dst.flags = src.flags;

        dst.location = {
            src.location.x,
            src.location.y,
            src.location.z
        };

        for (size_t b = 0; b < BONE_COUNT; b++)
        {
            if (src.boneValid[b])
            {
                dst.boneValid[b] = 1;
                dst.bones[b] = {
                    src.bones[b].x,
                    src.bones[b].y,
                    src.bones[b].z
                };
            }
        }

        g_bots.push_back(dst);
    }
}

// ======================================================
// GETTERS (THREAD-SAFE)
// ======================================================

ExternalLocalPlayer ExternalCache::GetLocal()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_local;
}

std::vector<ExternalActor> ExternalCache::GetPlayers()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_players;
}

std::vector<ExternalActor> ExternalCache::GetBots()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_bots;
}
