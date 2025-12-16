#include "Aimbot.h"
#include "SDK.hpp"
#include "AimbotCache.h"

#include <cmath>
#include <cfloat>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <chrono>

// ======================================================
// DEFINIÇÕES
// ======================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ======================================================
// VECTORMATH
// ======================================================

namespace VectorMath
{
    inline float Length(const SDK::FVector& v)
    {
        return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
    }

    inline float Dot(const SDK::FVector& a, const SDK::FVector& b)
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }
}

using namespace VectorMath;

// ======================================================
// HISTÓRICO DO ATOR (ESTÁVEL)
// ======================================================

struct ActorHistory
{
    struct Frame
    {
        SDK::FVector pos{};
        SDK::FVector vel{};
        std::chrono::steady_clock::time_point t;
    };

    std::vector<Frame> frames;
    std::chrono::steady_clock::time_point lastUpdate{};
    int lastTeam = -1;
    bool wasAlive = false;

    void Add(const SDK::FVector& p, int team, bool alive)
    {
        auto now = std::chrono::steady_clock::now();

        if (team != lastTeam || (!wasAlive && alive))
            frames.clear();

        Frame f{};
        f.pos = p;
        f.t = now;

        if (!frames.empty())
        {
            const Frame& last = frames.back();
            float dt = std::chrono::duration<float>(now - last.t).count();

            if (dt > 0.001f)
            {
                SDK::FVector inst{
                    (p.X - last.pos.X) / dt,
                    (p.Y - last.pos.Y) / dt,
                    (p.Z - last.pos.Z) / dt
                };

                f.vel.X = last.vel.X * 0.65f + inst.X * 0.35f;
                f.vel.Y = last.vel.Y * 0.65f + inst.Y * 0.35f;
                f.vel.Z = last.vel.Z * 0.65f + inst.Z * 0.35f;
            }
        }

        frames.push_back(f);
        if (frames.size() > 10)
            frames.erase(frames.begin());

        lastUpdate = now;
        lastTeam = team;
        wasAlive = alive;
    }

    bool Valid() const
    {
        if (frames.size() < 3)
            return false;

        float age = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - lastUpdate).count();

        return age < 0.8f;
    }

    SDK::FVector Predict(float t) const
    {
        const Frame& f = frames.back();
        return {
            f.pos.X + f.vel.X * t,
            f.pos.Y + f.vel.Y * t,
            f.pos.Z + f.vel.Z * t
        };
    }
};

static std::unordered_map<uintptr_t, ActorHistory> g_History;

// ======================================================
// ANGLE UTILS
// ======================================================

static float NormalizeAngle(float a)
{
    while (a > 180.f) a -= 360.f;
    while (a < -180.f) a += 360.f;
    return a;
}

static SDK::FRotator CalcLookAt(
    const SDK::FVector& from,
    const SDK::FVector& to)
{
    return SDK::UKismetMathLibrary::FindLookAtRotation(from, to);
}

// ======================================================
// FOV ANGULAR
// ======================================================

static float AngularDistance(
    const SDK::FRotator& view,
    const SDK::FVector& target,
    const SDK::FVector& cam)
{
    SDK::FVector dir{
        target.X - cam.X,
        target.Y - cam.Y,
        target.Z - cam.Z
    };

    float len = Length(dir);
    if (len < 0.001f)
        return 0.f;

    dir.X /= len;
    dir.Y /= len;
    dir.Z /= len;

    float pr = view.Pitch * (M_PI / 180.f);
    float yr = view.Yaw * (M_PI / 180.f);

    SDK::FVector viewDir{
        std::cos(yr) * std::cos(pr),
        std::sin(yr) * std::cos(pr),
        std::sin(pr)
    };

    float dot = Dot(viewDir, dir);
    dot = std::clamp(dot, -1.f, 1.f);

    return std::acos(dot) * (180.f / M_PI);
}

// ======================================================
// BONE
// ======================================================

static bool GetBone(
    SDK::USkeletalMeshComponent* mesh,
    const char* name,
    SDK::FVector& out)
{
    if (!mesh) return false;

    auto sockets = mesh->GetAllSocketNames();
    for (int i = 0; i < sockets.Num(); i++)
    {
        if (sockets[i].ToString() == name)
        {
            out = mesh->GetSocketLocation(sockets[i]);
            return true;
        }
    }
    return false;
}

// ======================================================
// PREDIÇÃO FINAL (DOMINANTE)
// ======================================================

static SDK::FVector PredictPosition(
    SDK::AActor* actor,
    const SDK::FVector& bonePos,
    const SDK::FVector& camPos,
    float prediction,
    float smooth,
    int team,
    bool alive)
{
    if (!actor || prediction <= 0.f)
        return bonePos;

    uintptr_t key = reinterpret_cast<uintptr_t>(actor);
    auto& hist = g_History[key];

    SDK::FVector actorPos = actor->K2_GetActorLocation();
    hist.Add(actorPos, team, alive);

    if (!hist.Valid())
        return bonePos;

    float dist = Length({
        actorPos.X - camPos.X,
        actorPos.Y - camPos.Y,
        actorPos.Z - camPos.Z
        });

    float t = (dist / 12000.f) * prediction;
    t = std::clamp(t, 0.f, 0.6f);

    SDK::FVector predictedActor = hist.Predict(t);

    SDK::FVector delta{
        predictedActor.X - actorPos.X,
        predictedActor.Y - actorPos.Y,
        0.f
    };

    float amplify = std::max(1.f, smooth);
    amplify *= prediction;

    return {
        bonePos.X + delta.X * amplify,
        bonePos.Y + delta.Y * amplify,
        bonePos.Z
    };
}

// ======================================================
// AIMBOT
// ======================================================

void Aimbot::Tick()
{
    auto cfg = AimbotCache::Get();
    if (!cfg.enabled || !cfg.hotkeyDown)
        return;

    SDK::UWorld* World = SDK::UWorld::GetWorld();
    if (!World || !World->OwningGameInstance)
        return;

    auto& LPs = World->OwningGameInstance->LocalPlayers;
    if (!LPs.Num())
        return;

    auto* PC = LPs[0]->PlayerController;
    if (!PC || !PC->PlayerCameraManager)
        return;

    auto* LocalPawn = PC->Character;
    if (!LocalPawn)
        return;

    auto& POV = PC->PlayerCameraManager->CameraCachePrivate.POV;
    SDK::FVector CamLoc = POV.Location;
    SDK::FRotator CamRot = POV.Rotation;

    int LocalTeam = -1;
    if (LocalPawn->IsA(SDK::AALS_AnimMan_CharacterBP_C::StaticClass()))
        LocalTeam = (int)static_cast<SDK::AALS_AnimMan_CharacterBP_C*>(LocalPawn)->Team;

    SDK::FVector BestPos{};
    float BestAng = FLT_MAX;
    SDK::AActor* BestActor = nullptr;

    auto& Actors = World->PersistentLevel->Actors;

    for (int i = 0; i < Actors.Num(); i++)
    {
        auto* A = Actors[i];
        if (!A || A == LocalPawn)
            continue;

        SDK::USkeletalMeshComponent* Mesh = nullptr;
        int Team = -1;
        bool Alive = false;

        if (A->IsA(SDK::AALS_AnimMan_CharacterBP_C::StaticClass()))
        {
            auto* C = static_cast<SDK::AALS_AnimMan_CharacterBP_C*>(A);
            Mesh = C->Mesh;
            Team = (int)C->Team;
            Alive = C->WW_SurvivorStatus && C->WW_SurvivorStatus->Health > 0;
        }
        else if (A->IsA(SDK::AAI_Humanoid_C::StaticClass()))
        {
            auto* B = static_cast<SDK::AAI_Humanoid_C*>(A);
            Mesh = B->Mesh;
            Team = (int)B->Team;
            Alive = B->WW_SurvivorStatus && B->WW_SurvivorStatus->Health > 0;
        }
        else continue;

        if (!Alive) continue;
        if (!cfg.aimAtTeam && Team == LocalTeam) continue;
        if (cfg.visibleOnly && !PC->LineOfSightTo(A, CamLoc, false)) continue;
        if (!Mesh) continue;

        SDK::FVector Bone{};
        if (!GetBone(Mesh, "head", Bone) &&
            !GetBone(Mesh, "spine_03", Bone))
            continue;

        Bone = PredictPosition(
            A,
            Bone,
            CamLoc,
            cfg.prediction,
            cfg.smooth,
            Team,
            Alive
        );

        float ang = AngularDistance(CamRot, Bone, CamLoc);
        if (ang < cfg.fov && ang < BestAng)
        {
            BestAng = ang;
            BestPos = Bone;
            BestActor = A;
        }
    }

    if (!BestActor)
        return;

    auto TargetRot = CalcLookAt(CamLoc, BestPos);
    auto CurRot = PC->GetControlRotation();

    SDK::FRotator Delta{
        NormalizeAngle(TargetRot.Pitch - CurRot.Pitch),
        NormalizeAngle(TargetRot.Yaw - CurRot.Yaw),
        0.f
    };

    float smooth = std::max(cfg.smooth, 1.f);

    SDK::FRotator Final{
        CurRot.Pitch + Delta.Pitch / smooth,
        CurRot.Yaw + Delta.Yaw / smooth,
        0.f
    };

    PC->SetControlRotation(Final);
}
