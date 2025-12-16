#pragma once
#include "imgui.h"
#include "ExternalCache.h"

#include <cmath>
#include <string>

namespace ESPHelper
{
    // =============================
    // Tipos auxiliares
    // =============================

    struct ScreenPoint
    {
        ImVec2 pos{};
        bool   onScreen = false;
    };

    // =============================
    // Math utils
    // =============================

    float DegToRad(float deg);
    float Distance2D(const ImVec2& a, const ImVec2& b);
    float Distance3D(const Vec3& a, const Vec3& b);

    // =============================
    // Validações
    // =============================

    bool IsAlive(uint8_t flags);
    bool IsBot(uint8_t flags);
    bool IsValidFOV(float fov);

    // =============================
    // World → Screen
    // =============================

    bool WorldToScreen(
        const Vec3& world,
        ImVec2& out
    );

    ScreenPoint WorldToScreenSafe(
        const Vec3& world
    );

    // =============================
    // Helpers de posição
    // =============================

    Vec3 GetActorHeadPosition(
        const ExternalActor& actor,
        int headBoneIndex = 8
    );

    // =============================
    // Screen utils
    // =============================

    ImVec2 GetScreenCenter();
    bool IsOnScreen(const ImVec2& pos, float margin = 5.0f);

    ImVec2 GetBoneScreen(
        const ExternalActor& actor,
        int boneId,
        bool& valid
    );
}
