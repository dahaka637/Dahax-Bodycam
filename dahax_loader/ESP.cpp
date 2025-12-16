#include "ESP.h"
#include "Menu.h"

#include "imgui.h"
#include "ExternalCache.h"
#include "ESPHelper.h"
#include <algorithm>
#include <vector>
#include <cstdio>

// ======================================================
// CONFIG VISUAL
// ======================================================
static constexpr float LINE_THICKNESS = 1.4f;
static constexpr float BOX_THICKNESS = 1.6f;
static constexpr float NAME_OFFSET_Y = 14.0f;
static constexpr float DIST_OFFSET_Y = 6.0f;

// fallback em cm (Unreal)
static constexpr float FALLBACK_HEAD_OFFSET = 70.0f;

// ======================================================
// LIGAÇÕES DO ESQUELETO
// ======================================================
struct BoneLink { int a; int b; };

static constexpr BoneLink SkeletonLinks[] =
{
    {0,1},{1,2},
    {2,3},{3,4},{4,5},{5,6},
    {2,7},{7,8},{8,9},{9,10},
    {0,11},{11,12},{12,13},
    {0,14},{14,15},{15,16}
};

// ======================================================
// CAIXA 2D (DINÂMICA)
// ======================================================
static void DrawCornerBox(
    ImDrawList* draw,
    const ImVec2& top,
    const ImVec2& bottom,
    ImU32 color,
    float thickness
)
{
    float height = bottom.y - top.y;
    if (height < 20.f)
        return;

    float width = height * 0.45f;
    float corner = width * 0.25f;

    ImVec2 tl{ top.x - width * 0.5f, top.y };
    ImVec2 tr{ top.x + width * 0.5f, top.y };
    ImVec2 bl{ bottom.x - width * 0.5f, bottom.y };
    ImVec2 br{ bottom.x + width * 0.5f, bottom.y };

    draw->AddLine(tl, { tl.x + corner, tl.y }, color, thickness);
    draw->AddLine(tr, { tr.x - corner, tr.y }, color, thickness);

    draw->AddLine(bl, { bl.x + corner, bl.y }, color, thickness);
    draw->AddLine(br, { br.x - corner, br.y }, color, thickness);

    draw->AddLine(tl, { tl.x, tl.y + corner }, color, thickness);
    draw->AddLine(bl, { bl.x, bl.y - corner }, color, thickness);

    draw->AddLine(tr, { tr.x, tr.y + corner }, color, thickness);
    draw->AddLine(br, { br.x, br.y - corner }, color, thickness);
}

// ======================================================
// HEAD / FOOT (DINÂMICO)
// ======================================================
static bool GetBoxPoints(
    const ExternalActor& a,
    ImVec2& outTop,
    ImVec2& outBottom
)
{
    // HEAD
    ImVec2 head{};
    bool hasHead = false;

    if (a.boneValid[2])
        hasHead = ESPHelper::WorldToScreen(a.bones[2], head);

    if (!hasHead)
    {
        Vec3 fakeHead{
            a.location.x,
            a.location.y,
            a.location.z + FALLBACK_HEAD_OFFSET
        };
        hasHead = ESPHelper::WorldToScreen(fakeHead, head);
    }

    if (!hasHead)
        return false;

    // FOOT
    float minZ = a.location.z;
    bool hasFoot = false;

    for (int idx : { 13, 16 })
    {
        if (idx < BONE_COUNT && a.boneValid[idx])
        {
            minZ = (std::min)(minZ, a.bones[idx].z);

            hasFoot = true;
        }
    }

    if (!hasFoot)
        return false;

    Vec3 footWorld{
        a.location.x,
        a.location.y,
        minZ
    };

    ImVec2 foot{};
    if (!ESPHelper::WorldToScreen(footWorld, foot))
        return false;

    outTop = head;
    outBottom = foot;
    return true;
}

// ======================================================
// TEAM MODE
// ======================================================
static bool IsTeamMode(
    const ExternalLocalPlayer& local,
    const std::vector<ExternalActor>& players
)
{
    if (local.team != 0)
        return true;

    for (const auto& p : players)
        if (p.team != 0)
            return true;

    return false;
}

// ======================================================
// SHOULD DRAW
// ======================================================
static bool ShouldDrawActor(
    const ExternalLocalPlayer& local,
    const ExternalActor& a,
    bool teamMode
)
{
    if (!(a.flags & ACTOR_ALIVE))
        return false;

    if (teamMode && !Menu::ESP.ShowTeam && a.team == local.team)
        return false;

    return true;
}

// ======================================================
// SNAPLINE
// ======================================================
static void DrawSnapline(
    ImDrawList* draw,
    const ImVec2& from,
    const ImVec2& to,
    ImU32 color
)
{
    draw->AddLine(from, to, color, LINE_THICKNESS);
}

// ======================================================
// SKELETON
// ======================================================
static void DrawSkeleton(
    ImDrawList* draw,
    const ExternalActor& a,
    ImU32 color
)
{
    for (const auto& link : SkeletonLinks)
    {
        if (!a.boneValid[link.a] || !a.boneValid[link.b])
            continue;

        ImVec2 p1{}, p2{};
        if (!ESPHelper::WorldToScreen(a.bones[link.a], p1)) continue;
        if (!ESPHelper::WorldToScreen(a.bones[link.b], p2)) continue;

        draw->AddLine(p1, p2, color, LINE_THICKNESS);
    }
}

// ======================================================
// ESP PRINCIPAL
// ======================================================
void ESP::Render()
{
    // 🔴 ESP TOTALMENTE CONTROLADO PELO MENU
    if (!Menu::ESP.Enabled)
        return;


    ImDrawList* draw = ImGui::GetForegroundDrawList();
    // ======================================================
    // DESENHO DO FOV DO AIMBOT
    // ======================================================
    if (Menu::Aimbot.Enabled && Menu::Aimbot.DrawFov)
    {
        ImVec2 screenCenter{
            ImGui::GetIO().DisplaySize.x * 0.5f,
            ImGui::GetIO().DisplaySize.y * 0.5f
        };

        // Conversão simples: FOV -> pixels
        // Ajuste o multiplicador se quiser mais/menos sensibilidade visual
        float radius = Menu::Aimbot.Fov;

        draw->AddCircle(
            screenCenter,
            radius,
            IM_COL32(255, 255, 255, 160),
            128,            // segmentos (quanto maior, mais liso)
            1.5f            // espessura
        );
    }


    ExternalLocalPlayer local = ExternalCache::GetLocal();
    auto players = ExternalCache::GetPlayers();
    auto bots = ExternalCache::GetBots();

    bool teamMode = IsTeamMode(local, players);

    auto DrawEntity = [&](const ExternalActor& a, bool isBot)
        {
            if (!ShouldDrawActor(local, a, teamMode))
                return;

            ImVec2 top{}, bottom{};
            if (!GetBoxPoints(a, top, bottom))
                return;

            ImU32 mainColor =
                (a.flags & ACTOR_VISIBLE)
                ? IM_COL32(200, 255, 200, 255)
                : IM_COL32(255, 120, 120, 255);

            // BOX
            if (Menu::ESP.Box)
                DrawCornerBox(draw, top, bottom, mainColor, BOX_THICKNESS);

            // SNAPLINE
            if (Menu::ESP.Snapline)
            {
                ImVec2 screenCenter{
                    ImGui::GetIO().DisplaySize.x * 0.5f,
                    ImGui::GetIO().DisplaySize.y
                };
                DrawSnapline(draw, screenCenter, bottom, mainColor);
            }

            // NAME
            if (Menu::ESP.Name)
            {
                const char* label = isBot ? "BOT" : a.name;
                ImVec2 ts = ImGui::CalcTextSize(label);

                draw->AddText(
                    { top.x - ts.x * 0.5f, top.y - NAME_OFFSET_Y },
                    isBot ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255),
                    label
                );
            }

            // DISTANCE
            if (Menu::ESP.Distance)
            {
                float distM = ESPHelper::Distance3D(local.location, a.location) / 100.f;

                char buf[32];
                sprintf_s(buf, "%.1fm", distM);

                ImVec2 ts = ImGui::CalcTextSize(buf);
                draw->AddText(
                    { bottom.x - ts.x * 0.5f, bottom.y + DIST_OFFSET_Y },
                    IM_COL32(200, 200, 200, 255),
                    buf
                );
            }

            // SKELETON
            if (Menu::ESP.Skeleton)
                DrawSkeleton(draw, a, mainColor);
        };

    for (const auto& p : players)
        DrawEntity(p, false);

    for (const auto& b : bots)
        DrawEntity(b, true);
}
