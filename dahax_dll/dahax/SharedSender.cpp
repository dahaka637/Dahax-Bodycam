#include "SharedSender.h"
#include "SharedMemory.h"
#include "Cache.h"

#include <cstring>

// ======================================================
// Serializa Cache → SharedPayload (SEQLOCK SAFE)
// ======================================================
void SharedSender::Tick()
{
    auto* block = SharedMemory::GetBlock();
    if (!block)
        return;

    auto state = SharedMemory::GetState();
    if (state == SharedState::Stopped)
        return;

    // ==================================================
    // Snapshot thread-safe do cache
    // ==================================================
    LocalPlayerData local;
    std::vector<CachedActor> players;
    std::vector<CachedActor> bots;

    {
        std::lock_guard<std::mutex> lock(Cache::DataMutex);
        local = Cache::Local;
        players = Cache::Players;
        bots = Cache::Bots;
    }

    SharedPayload& out = block->payload;

    // ==================================================
    // ENTER WRITE (SEQLOCK)
    // ==================================================
    out.seq++;

    // ==================================================
    // LOCAL PLAYER
    // ==================================================

    // >>> ENDEREÇO DO ATOR LOCAL
    out.local.actorAddress = local.ActorAddress;

    out.local.location = {
        (float)local.Location.X,
        (float)local.Location.Y,
        (float)local.Location.Z
    };

    out.local.cameraLocation = {
        (float)local.CameraLocation.X,
        (float)local.CameraLocation.Y,
        (float)local.CameraLocation.Z
    };

    out.local.cameraRotation = {
        (float)local.CameraRotation.Pitch,
        (float)local.CameraRotation.Yaw,
        (float)local.CameraRotation.Roll
    };

    out.local.cameraFOV = local.CameraFOV;
    out.local.team = local.Team;

    // ==================================================
    // PLAYERS
    // ==================================================
    uint32_t playerCount = (uint32_t)players.size();
    if (playerCount > SHARED_MAX_PLAYERS)
        playerCount = SHARED_MAX_PLAYERS;

    out.playerCount = playerCount;

    for (uint32_t i = 0; i < playerCount; i++)
    {
        const CachedActor& src = players[i];
        SharedActor& dst = out.players[i];

        std::memset(&dst, 0, sizeof(dst));

        // >>> ENDEREÇO DO ATOR
        dst.actorAddress = src.ActorAddress;

        strncpy_s(
            dst.name,
            SHARED_MAX_NAME,
            src.Name,
            _TRUNCATE
        );

        dst.team = src.Team;
        dst.health = src.Health;
        dst.flags = 0;

        if (src.Flags & ACTOR_ALIVE)   dst.flags |= SH_ACTOR_ALIVE;
        if (src.Flags & ACTOR_VISIBLE) dst.flags |= SH_ACTOR_VISIBLE;

        dst.location = {
            (float)src.Location.X,
            (float)src.Location.Y,
            (float)src.Location.Z
        };

        for (size_t b = 0; b < SHARED_BONE_COUNT && b < BONE_COUNT; b++)
        {
            if (!src.BoneValid[b])
                continue;

            dst.bones[b] = {
                (float)src.Bones[b].X,
                (float)src.Bones[b].Y,
                (float)src.Bones[b].Z
            };

            dst.boneValid[b] = 1;
        }
    }

    // ==================================================
    // BOTS
    // ==================================================
    uint32_t botCount = (uint32_t)bots.size();
    if (botCount > SHARED_MAX_BOTS)
        botCount = SHARED_MAX_BOTS;

    out.botCount = botCount;

    for (uint32_t i = 0; i < botCount; i++)
    {
        const CachedActor& src = bots[i];
        SharedActor& dst = out.bots[i];

        std::memset(&dst, 0, sizeof(dst));

        // >>> ENDEREÇO DO ATOR (BOT)
        dst.actorAddress = src.ActorAddress;

        strncpy_s(
            dst.name,
            SHARED_MAX_NAME,
            src.Name,
            _TRUNCATE
        );

        dst.team = src.Team;
        dst.health = src.Health;
        dst.flags = SH_ACTOR_BOT;

        if (src.Flags & ACTOR_ALIVE)   dst.flags |= SH_ACTOR_ALIVE;
        if (src.Flags & ACTOR_VISIBLE) dst.flags |= SH_ACTOR_VISIBLE;

        dst.location = {
            (float)src.Location.X,
            (float)src.Location.Y,
            (float)src.Location.Z
        };

        for (size_t b = 0; b < SHARED_BONE_COUNT && b < BONE_COUNT; b++)
        {
            if (!src.BoneValid[b])
                continue;

            dst.bones[b] = {
                (float)src.Bones[b].X,
                (float)src.Bones[b].Y,
                (float)src.Bones[b].Z
            };

            dst.boneValid[b] = 1;
        }
    }

    // ==================================================
    // FRAME FINAL (SÓ AGORA)
    // ==================================================
    static uint32_t frameId = 0;
    out.frameId = ++frameId;

    // ==================================================
    // EXIT WRITE (SEQLOCK)
    // ==================================================
    out.seq++;
}
