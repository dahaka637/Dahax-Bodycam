#include "ESPHelper.h"
#include <cstdio>
#include "Offsets.h"
// ======================================================
// Math
// ======================================================

float ESPHelper::DegToRad(float deg)
{
    return deg * 0.01745329251f;
}

float ESPHelper::Distance2D(const ImVec2& a, const ImVec2& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float ESPHelper::Distance3D(const Vec3& a, const Vec3& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ======================================================
// Flags
// ======================================================

bool ESPHelper::IsAlive(uint8_t flags)
{
    return (flags & ACTOR_ALIVE) != 0;
}

bool ESPHelper::IsBot(uint8_t flags)
{
    return (flags & ACTOR_BOT) != 0;
}

bool ESPHelper::IsValidFOV(float fov)
{
    return fov > 1.0f && fov < 179.0f;
}

bool ESPHelper::WorldToScreen(
    const Vec3& world,
    ImVec2& out
)
{
    auto& camMgr = Offsets::GetCameraManager();
    if (!camMgr.EnsureValid())
        return false;

    auto camLocD = camMgr.ReadLocation();
    auto camRotD = camMgr.ReadRotation();
    float fov = camMgr.ReadFOV();

    if (!IsValidFOV(fov))
        return false;

    Vec3 cam{
        (float)camLocD.X,
        (float)camLocD.Y,
        (float)camLocD.Z
    };

    float pitch = DegToRad((float)camRotD.Pitch);
    float yaw = DegToRad((float)camRotD.Yaw);

    Vec3 delta{
        world.x - cam.x,
        world.y - cam.y,
        world.z - cam.z
    };

    float cp = cosf(pitch);
    float sp = sinf(pitch);
    float cy = cosf(yaw);
    float sy = sinf(yaw);

    Vec3 forward{ cp * cy, cp * sy, sp };
    Vec3 right{ -sy, cy, 0.f };
    Vec3 up{
        -sp * cy,
        -sp * sy,
         cp
    };

    float tx = delta.x * forward.x + delta.y * forward.y + delta.z * forward.z;
    float ty = delta.x * right.x + delta.y * right.y + delta.z * right.z;
    float tz = delta.x * up.x + delta.y * up.y + delta.z * up.z;

    if (tx <= 0.01f)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    float cx = io.DisplaySize.x * 0.5f;
    float cyy = io.DisplaySize.y * 0.5f;

    float proj = cx / tanf(DegToRad(fov) * 0.5f);

    out.x = cx + (ty * proj / tx);
    out.y = cyy - (tz * proj / tx);

    return true;
}

// ======================================================
// WorldToScreen seguro
// ======================================================

ESPHelper::ScreenPoint ESPHelper::WorldToScreenSafe(
    const Vec3& world
)
{
    ScreenPoint r{};
    r.onScreen = WorldToScreen(world, r.pos);
    return r;
}

// ======================================================
// Helpers de posição
// ======================================================

Vec3 ESPHelper::GetActorHeadPosition(
    const ExternalActor& actor,
    int headBoneIndex
)
{
    if (headBoneIndex >= 0 &&
        headBoneIndex < BONE_COUNT &&
        actor.boneValid[headBoneIndex])
    {
        return actor.bones[headBoneIndex];
    }

    // fallback: posição + offset
    return {
        actor.location.x,
        actor.location.y,
        actor.location.z + 70.0f
    };
}

// ======================================================
// Screen utils
// ======================================================
// ======================================================
// Bone → Screen
// ======================================================

ImVec2 ESPHelper::GetBoneScreen(
    const ExternalActor& actor,
    int boneId,
    bool& valid
)
{
    valid = false;

    // Validação do bone index
    if (boneId < 0 || boneId >= BONE_COUNT)
        return {};

    if (!actor.boneValid[boneId])
        return {};

    ImVec2 screen{};
    if (!WorldToScreen(actor.bones[boneId], screen))
        return {};

    valid = true;
    return screen;
}


ImVec2 ESPHelper::GetScreenCenter()
{
    ImGuiIO& io = ImGui::GetIO();
    return {
        io.DisplaySize.x * 0.5f,
        io.DisplaySize.y * 0.5f
    };
}

bool ESPHelper::IsOnScreen(const ImVec2& pos, float margin)
{
    ImGuiIO& io = ImGui::GetIO();

    return (
        pos.x >= -margin &&
        pos.y >= -margin &&
        pos.x <= io.DisplaySize.x + margin &&
        pos.y <= io.DisplaySize.y + margin
        );
}
