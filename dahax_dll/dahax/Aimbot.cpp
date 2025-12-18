#include "Aimbot.h"
#include "SDK.hpp"
#include "AimbotCache.h"

#include <cmath>
#include <cfloat>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <string>

// ======================================================
// CONSTANTES
// ======================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ======================================================
// BONE DEFINITIONS
// ======================================================

struct BoneDef
{
    const char* Name;
};

static constexpr BoneDef BoneDefs[] =
{
    { "pelvis" },        // 0
    { "spine_03" },      // 1 - Chest
    { "head" },          // 2 - Head
    { "clavicle_l" },    // 3
    { "upperarm_l" },    // 4
    { "lowerarm_l" },    // 5
    { "hand_l" },        // 6
    { "clavicle_r" },    // 7
    { "upperarm_r" },    // 8
    { "lowerarm_r" },    // 9
    { "hand_r" },        // 10
    { "thigh_l" },       // 11
    { "calf_l" },        // 12
    { "foot_l" },        // 13
    { "thigh_r" },       // 14
    { "calf_r" },        // 15
    { "foot_r" }         // 16
};

constexpr size_t BONE_COUNT = sizeof(BoneDefs) / sizeof(BoneDefs[0]);

// ======================================================
// POSSIBLE HEAD NAMES
// ======================================================

static const char* PossibleHeadBones[] =
{
    "head","Head","HEAD",
    "Head_Socket","head_socket","HeadSocket","headSocket",
    "head_01","Head_01","head_1","Head_1",
    "skull","Skull","SKULL",
    "face","Face","FACE",
    "neck","neck_01","neck_02",
    "c_head","C_Head",
    nullptr
};

// ======================================================
// VECTOR MATH
// ======================================================

static float Length(const SDK::FVector& v)
{
    return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
}

static float Dot(const SDK::FVector& a, const SDK::FVector& b)
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

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

    float dot = std::clamp(Dot(viewDir, dir), -1.f, 1.f);
    return std::acos(dot) * (180.f / M_PI);
}


static float BonePreferenceBias(const std::string& lower)
{
    // Região central (nenhuma penalização)
    if (lower.find("pelvis") != std::string::npos ||
        lower.find("spine") != std::string::npos ||
        lower.find("chest") != std::string::npos ||
        lower.find("neck") != std::string::npos ||
        lower.find("head") != std::string::npos)
    {
        return 0.0f;
    }

    // Braços / mãos (penalização leve)
    if (lower.find("arm") != std::string::npos ||
        lower.find("hand") != std::string::npos)
    {
        return 0.9f; // leve
    }

    // Pernas / pés (penalização um pouco maior)
    if (lower.find("thigh") != std::string::npos ||
        lower.find("calf") != std::string::npos ||
        lower.find("foot") != std::string::npos)
    {
        return 1.3f;
    }

    // Outros sockets (neutro-leve)
    return 0.6f;
}


static bool ResolveNearestBone(
    SDK::USkeletalMeshComponent* Mesh,
    const SDK::FRotator& camRot,
    const SDK::FVector& camLoc,
    SDK::FVector& outPos
)
{
    if (!Mesh)
        return false;

    auto sockets = Mesh->GetAllSocketNames();

    float bestScore = FLT_MAX;
    bool found = false;

    for (int i = 0; i < sockets.Num(); i++)
    {
        const auto& socket = sockets[i];

        SDK::FVector pos = Mesh->GetSocketLocation(socket);

        float ang = AngularDistance(camRot, pos, camLoc);

        // Nome do osso
        std::string name = socket.ToString();
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Penalização leve por tipo de osso
        float bias = BonePreferenceBias(lower);

        // Score final
        float score = ang + bias;

        if (score < bestScore)
        {
            bestScore = score;
            outPos = pos;
            found = true;
        }
    }

    return found;
}


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
// HISTÓRICO DE MOVIMENTO BASEADO NO OSSO (PREDIÇÃO)
// ======================================================

// ======================================================
// HISTÓRICO DE MOVIMENTO DO ATOR (PREDIÇÃO)
// ======================================================

struct ActorHistory
{
    struct Frame
    {
        SDK::FVector pos;   // posição do ator
        SDK::FVector vel;   // velocidade do ator
        std::chrono::steady_clock::time_point t;
    };

    std::vector<Frame> frames;
    std::chrono::steady_clock::time_point lastUpdate{};
    int lastTeam = -1;
    bool wasAlive = false;

    // ==================================================
    // Alimenta histórico com POSIÇÃO DO ATOR
    // ==================================================
    void Add(const SDK::FVector& actorPos, int team, bool alive)
    {
        auto now = std::chrono::steady_clock::now();

        // Reset em troca de alvo, time ou respawn
        if (team != lastTeam || (!wasAlive && alive))
            frames.clear();

        Frame f{};
        f.pos = actorPos;
        f.t = now;

        if (!frames.empty())
        {
            const auto& last = frames.back();
            float dt = std::chrono::duration<float>(now - last.t).count();

            if (dt > 0.001f)
            {
                SDK::FVector inst{
                    (actorPos.X - last.pos.X) / dt,
                    (actorPos.Y - last.pos.Y) / dt,
                    (actorPos.Z - last.pos.Z) / dt
                };

                // EMA para suavizar jitter
                f.vel.X = last.vel.X * 0.65f + inst.X * 0.35f;
                f.vel.Y = last.vel.Y * 0.65f + inst.Y * 0.35f;
                f.vel.Z = last.vel.Z * 0.65f + inst.Z * 0.35f;
            }
        }

        frames.push_back(f);
        if (frames.size() > 12)
            frames.erase(frames.begin());

        lastUpdate = now;
        lastTeam = team;
        wasAlive = alive;
    }

    // ==================================================
    // Histórico válido?
    // ==================================================
    bool Valid() const
    {
        if (frames.size() < 3)
            return false;

        float age = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - lastUpdate
        ).count();

        return age < 0.6f;
    }

    // ==================================================
    // Predição futura do ATOR
    // ==================================================
    SDK::FVector Predict(float t) const
    {
        const auto& f = frames.back();
        return {
            f.pos.X + f.vel.X * t,
            f.pos.Y + f.vel.Y * t,
            f.pos.Z + f.vel.Z * t
        };
    }
};


// ======================================================
// HISTÓRICO GLOBAL (1 POR ATOR)
// ======================================================

static std::unordered_map<uintptr_t, ActorHistory> g_ActorHistory;


// ======================================================
// BONE RESOLVER
// ======================================================

static bool ResolveHeadBone(
    SDK::USkeletalMeshComponent* Mesh,
    SDK::FVector& out)
{
    if (!Mesh) return false;

    auto sockets = Mesh->GetAllSocketNames();

    for (int i = 0; i < sockets.Num(); i++)
    {
        std::string name = sockets[i].ToString();
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        for (const char** p = PossibleHeadBones; *p; ++p)
        {
            if (name == *p)
            {
                out = Mesh->GetSocketLocation(sockets[i]);
                return true;
            }
        }

        if (lower.find("head") != std::string::npos ||
            lower.find("skull") != std::string::npos ||
            lower.find("face") != std::string::npos ||
            lower.find("neck") != std::string::npos)
        {
            out = Mesh->GetSocketLocation(sockets[i]);
            return true;
        }
    }

    return false;
}

static bool ResolveChestBone(
    SDK::USkeletalMeshComponent* Mesh,
    SDK::FVector& out)
{
    if (!Mesh) return false;

    auto sockets = Mesh->GetAllSocketNames();
    for (int i = 0; i < sockets.Num(); i++)
    {
        if (sockets[i].ToString() == BoneDefs[1].Name)
        {
            out = Mesh->GetSocketLocation(sockets[i]);
            return true;
        }
    }

    return false;
}


// ======================================================
// PREDIÇÃO FINAL (BASEADA NA POSIÇÃO DO ATOR)
// ======================================================

static SDK::FVector ApplyPrediction(
    SDK::AActor* actor,
    const SDK::FVector& bone,
    const SDK::FVector& cam,
    float prediction,
    int team,
    bool alive)
{
    if (!actor || prediction <= 0.f)
        return bone;

    uintptr_t key = reinterpret_cast<uintptr_t>(actor);
    auto& hist = g_ActorHistory[key];

    // Usa posição do ATOR
    SDK::FVector actorPos = actor->K2_GetActorLocation();
    hist.Add(actorPos, team, alive);

    if (!hist.Valid())
        return bone;

    // Distância câmera → ator
    float dist = Length({
        actorPos.X - cam.X,
        actorPos.Y - cam.Y,
        actorPos.Z - cam.Z
        });

    // Tempo de predição
    float t = std::clamp(
        (dist / 12000.f) * prediction,
        0.f,
        0.6f
    );

    // Posição futura prevista do ator
    SDK::FVector predictedActor = hist.Predict(t);

    // Delta de movimento do ator
    SDK::FVector delta{
        predictedActor.X - actorPos.X,
        predictedActor.Y - actorPos.Y,
        predictedActor.Z - actorPos.Z
    };

    // Amortecimento vertical adaptativo
    float verticalSpeed = std::abs(hist.frames.back().vel.Z);
    float verticalFactor = std::clamp(
        verticalSpeed / 250.f,
        0.2f,
        0.85f
    );

    return {
        bone.X + delta.X,
        bone.Y + delta.Y,
        bone.Z + delta.Z * verticalFactor
    };
}



// ======================================================
// AIMBOT
// ======================================================

static bool IsLockValid(
    SDK::AActor* A,
    SDK::USkeletalMeshComponent* Mesh,
    SDK::APlayerController* PC,
    const SDK::FVector& CamLoc,
    const SDK::FRotator& CamRot,
    float fov)
{
    if (!A || !Mesh)
        return false;

    if (!PC->LineOfSightTo(A, CamLoc, false))
        return false;

    SDK::FVector test{};
    if (!ResolveHeadBone(Mesh, test))
        return false;

    float ang = AngularDistance(CamRot, test, CamLoc);

    // histerese: permite sair um pouco do FOV sem quebrar
    if (ang > fov * 1.25f)
        return false;

    return true;
}


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

    SDK::FVector BestBone{};
    float BestAng = FLT_MAX;
    SDK::AActor* BestActor = nullptr;
    int BestTeam = -1;
    bool BestAlive = false;

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

        SDK::FVector target{};
        bool valid = false;

        if (cfg.targetMode == 0)
            valid = ResolveHeadBone(Mesh, target);
        else if (cfg.targetMode == 1)
            valid = ResolveChestBone(Mesh, target);
        else if (cfg.targetMode == 2)
            valid = ResolveNearestBone(Mesh, CamRot, CamLoc, target);

        if (!valid)
            continue;

        constexpr float FOV_SCALE = 0.1f; // 10%

        float ang = AngularDistance(CamRot, target, CamLoc);
        float effectiveFov = cfg.fov * FOV_SCALE;

        if (ang < effectiveFov && ang < BestAng)

        {
            BestAng = ang;
            BestBone = target;
            BestActor = A;
            BestTeam = Team;
            BestAlive = Alive;
        }
    }

    if (!BestActor)
        return;

    // ===============================
    // CONTROLE DE TROCA DE ALVO
    // ===============================
    static uintptr_t lastTarget = 0;
    uintptr_t currentTarget = reinterpret_cast<uintptr_t>(BestActor);

    if (currentTarget != lastTarget || !BestAlive)
    {
        g_ActorHistory[currentTarget].frames.clear();
        lastTarget = currentTarget;
    }

    // ===============================
    // APLICA PREDIÇÃO UMA ÚNICA VEZ
    // ===============================
    BestBone = ApplyPrediction(
        BestActor,
        BestBone,
        CamLoc,
        cfg.prediction,
        BestTeam,
        BestAlive
    );

    auto TargetRot = CalcLookAt(CamLoc, BestBone);
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

