#include "Cache.h"
#include "Helpers.h"
#include <cstring>
#include <unordered_map>

namespace Cache
{
    std::mutex DataMutex;

    LocalPlayerData Local{};
    std::vector<CachedActor> Players;
    std::vector<CachedActor> Bots;

    static uintptr_t g_LastWorld = 0;
}

// ======================================================
// HELPERS SEGUROS
// ======================================================

static bool SafeGetBone(
    SDK::USkeletalMeshComponent* Mesh,
    const char* BoneName,
    SDK::FVector& Out
)
{
    if (!Mesh || !BoneName)
        return false;

    SDK::TArray<SDK::FName> sockets = Mesh->GetAllSocketNames();
    if (!IsSafeTArray(sockets))
        return false;

    for (int i = 0; i < sockets.Num(); i++)
    {
        if (sockets[i].ToString() == BoneName)
        {
            Out = Mesh->GetSocketLocation(sockets[i]);
            return true;
        }
    }

    return false;
}

static SDK::FVector SafeActorLocation(SDK::AActor* Actor)
{
    if (!Actor)
        return SDK::FVector{};

    if (Actor->RootComponent)
        return Actor->RootComponent->RelativeLocation;

    return SDK::FVector{};
}

// ======================================================
// INIT / SHUTDOWN
// ======================================================

bool Cache::Initialize()
{
    return true;
}

void Cache::Shutdown()
{
    std::lock_guard<std::mutex> lock(DataMutex);
    Players.clear();
    Bots.clear();
}

// ======================================================
// UPDATE CACHE (PERMISSIVO)
// ======================================================

void Cache::Update()
{
    SDK::UWorld* World = SDK::UWorld::GetWorld();
    if (IsBadPtr(World))
        return;

    // ================================================
    // WORLD CHANGE → RESET TOTAL
    // ================================================
    uintptr_t worldKey = reinterpret_cast<uintptr_t>(World);
    if (worldKey != g_LastWorld)
    {
        std::lock_guard<std::mutex> lock(DataMutex);
        Players.clear();
        Bots.clear();
        Local = {};
        g_LastWorld = worldKey;
    }

    SDK::UGameInstance* GI = World->OwningGameInstance;
    if (IsBadPtr(GI))
        return;

    SDK::ULevel* Level = World->PersistentLevel;
    if (IsBadPtr(Level))
        return;

    auto& Actors = Level->Actors;
    if (!IsSafeTArray(Actors))
        return;

    LocalPlayerData tmpLocal{};
    std::vector<CachedActor> tmpPlayers;
    std::vector<CachedActor> tmpBots;

    // ==================================================
    // LOCAL PLAYER (BEST EFFORT)
    // ==================================================

    auto& LPArray = GI->LocalPlayers;
    if (IsSafeTArray(LPArray) && LPArray.Num() > 0)
    {
        auto* LP = LPArray[0];
        if (!IsBadPtr(LP) && !IsBadPtr(LP->PlayerController))
        {
            auto* PC = LP->PlayerController;

            tmpLocal.Controller = PC;
            tmpLocal.Pawn = PC->Character;
            tmpLocal.PlayerState = PC->PlayerState;

            if (!IsBadPtr(PC->PlayerCameraManager))
            {
                auto& POV = PC->PlayerCameraManager->CameraCachePrivate.POV;
                tmpLocal.CameraLocation = POV.Location;
                tmpLocal.CameraRotation = POV.Rotation;
                tmpLocal.CameraFOV = POV.FOV;
            }

            if (!IsBadPtr(tmpLocal.Pawn))
            {
                tmpLocal.Location = SafeActorLocation(tmpLocal.Pawn);

                if (tmpLocal.Pawn->IsA(SDK::AALS_AnimMan_CharacterBP_C::StaticClass()))
                {
                    auto* C = static_cast<SDK::AALS_AnimMan_CharacterBP_C*>(tmpLocal.Pawn);
                    tmpLocal.Team = static_cast<int>(C->Team);
                }
            }
        }
    }

    auto* LocalController = tmpLocal.Controller;
    SDK::FVector CamLoc = tmpLocal.CameraLocation;

    // ==================================================
    // ACTORS LOOP (NUNCA QUEBRA)
    // ==================================================

    for (int i = 0; i < Actors.Num(); i++)
    {
        auto* Actor = Actors[i];
        if (IsBadPtr(Actor))
            continue;

        CachedActor CA{};
        CA.Actor = Actor;
        CA.Location = SafeActorLocation(Actor);

        // ==============================================
        // PLAYER
        // ==============================================
        if (Actor->IsA(SDK::AALS_AnimMan_CharacterBP_C::StaticClass()))
        {
            auto* C = static_cast<SDK::AALS_AnimMan_CharacterBP_C*>(Actor);
            CA.Pawn = C;
            CA.PlayerState = C->PlayerState;
            CA.Team = static_cast<int>(C->Team);

            if (!IsBadPtr(C->WW_SurvivorStatus))
            {
                CA.Health = (float)C->WW_SurvivorStatus->Health;
                if (CA.Health > 0.f)
                    CA.Flags |= ACTOR_ALIVE;
            }

            if (!IsBadPtr(LocalController) &&
                LocalController->LineOfSightTo(Actor, CamLoc, false))
            {
                CA.Flags |= ACTOR_VISIBLE;
            }

            if (!IsBadPtr(CA.PlayerState))
            {
                strncpy_s(
                    CA.Name,
                    CA.PlayerState->PlayerNamePrivate.ToString().c_str(),
                    MAX_NAME_LEN - 1
                );
            }

            // BONES (OPCIONAL, NÃO CRÍTICO)
            if (!IsBadPtr(C->Mesh))
            {
                for (size_t b = 0; b < BONE_COUNT; b++)
                {
                    SDK::FVector pos;
                    if (SafeGetBone(C->Mesh, BoneDefs[b].Name, pos))
                    {
                        CA.Bones[b] = pos;
                        CA.BoneValid[b] = true;
                    }
                }
            }

            tmpPlayers.push_back(CA);
            continue;
        }

        // ==============================================
        // BOT
        // ==============================================
        if (Actor->IsA(SDK::AAI_Humanoid_C::StaticClass()))
        {
            auto* B = static_cast<SDK::AAI_Humanoid_C*>(Actor);
            CA.Pawn = B;
            CA.Flags |= ACTOR_BOT;
            CA.Team = static_cast<int>(B->Team);
            strncpy_s(CA.Name, "<BOT>", MAX_NAME_LEN - 1);

            if (!IsBadPtr(B->WW_SurvivorStatus))
            {
                CA.Health = (float)B->WW_SurvivorStatus->Health;
                if (CA.Health > 0.f)
                    CA.Flags |= ACTOR_ALIVE;
            }

            if (!IsBadPtr(LocalController) &&
                LocalController->LineOfSightTo(Actor, CamLoc, false))
            {
                CA.Flags |= ACTOR_VISIBLE;
            }

            if (!IsBadPtr(B->Mesh))
            {
                for (size_t b = 0; b < BONE_COUNT; b++)
                {
                    SDK::FVector pos;
                    if (SafeGetBone(B->Mesh, BoneDefs[b].Name, pos))
                    {
                        CA.Bones[b] = pos;
                        CA.BoneValid[b] = true;
                    }
                }
            }

            tmpBots.push_back(CA);
        }
    }

    // ==================================================
    // COMMIT FINAL
    // ==================================================

    {
        std::lock_guard<std::mutex> lock(DataMutex);
        Local = tmpLocal;
        Players = std::move(tmpPlayers);
        Bots = std::move(tmpBots);
    }
}
