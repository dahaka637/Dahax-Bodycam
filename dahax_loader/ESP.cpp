#include "ESP.h"
#include "Menu.h"

#include "imgui.h"
#include "ExternalCache.h"
#include "ESPHelper.h"
#include "Offsets.h"

#include <algorithm>
#include <vector>
#include <cstdio>
#include <Windows.h>

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
// PROCESS HANDLE (ABERTO UMA VEZ)
// ======================================================
static HANDLE GetGameProcess()
{
    static HANDLE hProc = nullptr;

    if (!hProc)
    {
        hProc = OpenProcess(
            PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
            FALSE,
            GetProcessId(FindWindowW(L"UnrealWindow", nullptr))
        );
    }

    return hProc;
}

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
// HEAD / FOOT (AGORA COM POSIÇÃO RESOLVIDA)
// ======================================================
static bool GetBoxPoints(
    const ExternalActor& a,
    const Vec3& actorPos,
    ImVec2& outTop,
    ImVec2& outBottom
)
{
    ImVec2 head{};
    bool hasHead = false;

    if (a.boneValid[2])
        hasHead = ESPHelper::WorldToScreen(a.bones[2], head);

    if (!hasHead)
    {
        Vec3 fakeHead{
            actorPos.x,
            actorPos.y,
            actorPos.z + FALLBACK_HEAD_OFFSET
        };
        hasHead = ESPHelper::WorldToScreen(fakeHead, head);
    }

    if (!hasHead)
        return false;

    float minZ = actorPos.z;
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
        actorPos.x,
        actorPos.y,
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

static float BoneThickness(int a, int b)
{
    // tronco (pelvis, spine, chest, neck, head)
    if (a <= 2 && b <= 2)
        return 2.4f;

    // membros
    if (a >= 3)
        return 1.4f;

    return 1.8f;
}


// ======================================================
// SKELETON (CONECTADO / ORGÂNICO)
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

        float thickness = BoneThickness(link.a, link.b);

        // Segmento principal (osso)
        draw->AddLine(
            p1,
            p2,
            color,
            thickness
        );

        // Junta A
        draw->AddCircleFilled(
            p1,
            thickness * 0.6f,
            color,
            10
        );

        // Junta B
        draw->AddCircleFilled(
            p2,
            thickness * 0.6f,
            color,
            10
        );
    }
}



// ======================================================
// ESP PRINCIPAL (POSIÇÃO FRESH AQUI)
// ======================================================
void ESP::Render()
{
    if (!Menu::ESP.Enabled)
        return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    ExternalLocalPlayer local = ExternalCache::GetLocal();
    auto players = ExternalCache::GetPlayers();
    auto bots = ExternalCache::GetBots();

    bool teamMode = IsTeamMode(local, players);
    HANDLE hProc = GetGameProcess();

    auto DrawEntity = [&](const ExternalActor& a, bool isBot)
        {
            if (!ShouldDrawActor(local, a, teamMode))
                return;

            // ==================================================
            // RESOLVE POSIÇÃO EM TEMPO REAL
            // ==================================================
            Vec3 actorPos{};
            bool hasFresh = false;

            Offsets::FVector fresh{};
            if (hProc &&
                a.actorAddress &&
                Offsets::ReadActorLocation(hProc, a.actorAddress, fresh))
            {
                actorPos = {
                    (float)fresh.X,
                    (float)fresh.Y,
                    (float)fresh.Z
                };
                hasFresh = true;
            }

            // fallback SOMENTE se RPM falhar
            if (!hasFresh)
            {
                actorPos = a.location;
            }


            ImVec2 top{}, bottom{};
            if (!GetBoxPoints(a, actorPos, top, bottom))
                return;

            ImU32 mainColor =
                (a.flags & ACTOR_VISIBLE)
                ? IM_COL32(200, 255, 200, 255)
                : IM_COL32(255, 120, 120, 255);

            if (Menu::ESP.Box)
                DrawCornerBox(draw, top, bottom, mainColor, BOX_THICKNESS);

            if (Menu::ESP.Snapline)
            {
                ImVec2 screenCenter{
                    ImGui::GetIO().DisplaySize.x * 0.5f,
                    ImGui::GetIO().DisplaySize.y
                };
                DrawSnapline(draw, screenCenter, bottom, mainColor);
            }

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

            if (Menu::ESP.Distance)
            {
                float distM = ESPHelper::Distance3D(local.location, actorPos) / 100.f;

                char buf[32];
                sprintf_s(buf, "%.1fm", distM);

                ImVec2 ts = ImGui::CalcTextSize(buf);
                draw->AddText(
                    { bottom.x - ts.x * 0.5f, bottom.y + DIST_OFFSET_Y },
                    IM_COL32(200, 200, 200, 255),
                    buf
                );
            }

            if (Menu::ESP.Skeleton)
                DrawSkeleton(draw, a, mainColor);
        };

    for (const auto& p : players)
        DrawEntity(p, false);

    for (const auto& b : bots)
        DrawEntity(b, true);
}
