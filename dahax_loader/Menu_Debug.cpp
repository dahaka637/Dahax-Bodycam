#include "Menu_Debug.h"
#include "imgui.h"
#include "ExternalCache.h"
#include "Offsets.h"

#include <vector>
#include <string>
#include <cinttypes>
#include <Windows.h>

// ======================================================
// ESTADO INTERNO DO DEBUG
// ======================================================
namespace
{
    enum class SelectedType
    {
        None,
        Player,
        Bot
    };

    SelectedType g_selectedType = SelectedType::None;
    int g_selectedIndex = -1;
}

// ======================================================
// PROCESS HANDLE (ABERTO UMA VEZ)
// ======================================================
static HANDLE GetGameProcess()
{
    static HANDLE hProc = nullptr;

    if (!hProc)
    {
        HWND hwnd = FindWindowW(L"UnrealWindow", nullptr);
        if (!hwnd)
            return nullptr;

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid)
        {
            hProc = OpenProcess(
                PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                FALSE,
                pid
            );
        }
    }

    return hProc;
}

// ======================================================
// HELPERS VISUAIS
// ======================================================
static void DrawVec3(const char* label, const Vec3& v)
{
    ImGui::Text("%s:  X %.2f | Y %.2f | Z %.2f", label, v.x, v.y, v.z);
}

static void DrawVec3Unavailable(const char* label)
{
    ImGui::TextDisabled("%s:  <unavailable>", label);
}

static void DrawRotator(const char* label, const Rotator& r)
{
    ImGui::Text("%s:  P %.2f | Y %.2f | R %.2f", label, r.pitch, r.yaw, r.roll);
}

static void DrawAddress(const char* label, uint64_t addr)
{
    if (addr)
        ImGui::Text("%s:  0x%016" PRIX64, label, addr);
    else
        ImGui::TextDisabled("%s:  <null>", label);
}

// ======================================================
// DETALHES DO ATOR
// ======================================================
static void DrawActorDetails(const ExternalActor& a)
{
    HANDLE hProc = GetGameProcess();

    ImGui::Text("Name: %s", a.name);
    DrawAddress("Actor Address", a.actorAddress);
    ImGui::Text("Team: %d", a.team);
    ImGui::Text("Health: %.2f", a.health);
    ImGui::Text("Flags: 0x%02X", a.flags);

    ImGui::Separator();

    // ----------------------------
    // POSIÇÃO (CACHE)
    // ----------------------------
    DrawVec3("Location (cache)", a.location);

    // ----------------------------
    // POSIÇÃO (FRESH / RPM)
    // ----------------------------
    Offsets::FVector fresh{};
    if (hProc &&
        a.actorAddress &&
        Offsets::ReadActorLocation(hProc, a.actorAddress, fresh))
    {
        Vec3 freshPos{
            (float)fresh.X,
            (float)fresh.Y,
            (float)fresh.Z
        };

        DrawVec3("Location (fresh)", freshPos);
    }
    else
    {
        DrawVec3Unavailable("Location (fresh)");
    }

    ImGui::Separator();
    ImGui::Text("Bones");

    ImGui::BeginChild("dbg_bones", ImVec2(0, 160), true);
    for (int i = 0; i < BONE_COUNT; i++)
    {
        if (!a.boneValid[i])
            continue;

        ImGui::Text(
            "[%02d]  %.2f  %.2f  %.2f",
            i,
            a.bones[i].x,
            a.bones[i].y,
            a.bones[i].z
        );
    }
    ImGui::EndChild();
}

// ======================================================
// RENDER DEBUG
// ======================================================
void MenuDebug::Render()
{
    // ==================================================
    // LOCAL PLAYER
    // ==================================================
    if (ImGui::CollapsingHeader("Local Player", ImGuiTreeNodeFlags_DefaultOpen))
    {
        HANDLE hProc = GetGameProcess();
        ExternalLocalPlayer local = ExternalCache::GetLocal();

        DrawAddress("Actor Address", local.actorAddress);

        DrawVec3("Location (cache)", local.location);

        Offsets::FVector fresh{};
        if (hProc &&
            local.actorAddress &&
            Offsets::ReadActorLocation(hProc, local.actorAddress, fresh))
        {
            Vec3 freshPos{
                (float)fresh.X,
                (float)fresh.Y,
                (float)fresh.Z
            };

            DrawVec3("Location (fresh)", freshPos);
        }
        else
        {
            DrawVec3Unavailable("Location (fresh)");
        }

        DrawVec3("Camera", local.cameraLocation);
        DrawRotator("Camera Rot", local.cameraRotation);
        ImGui::Text("Camera FOV: %.2f", local.cameraFOV);
        ImGui::Text("Team: %d", local.team);
    }

    ImGui::Separator();

    // ==================================================
    // LISTA DE ATORES
    // ==================================================
    ImGui::Text("Actors");
    ImGui::BeginChild("dbg_actor_list", ImVec2(0, 160), true);

    auto players = ExternalCache::GetPlayers();
    auto bots = ExternalCache::GetBots();

    for (int i = 0; i < (int)players.size(); i++)
    {
        char label[128];
        snprintf(
            label,
            sizeof(label),
            "[PLY %02d] %s  (0x%016" PRIX64 ")",
            i,
            players[i].name,
            players[i].actorAddress
        );

        bool selected =
            (g_selectedType == SelectedType::Player && g_selectedIndex == i);

        if (ImGui::Selectable(label, selected))
        {
            g_selectedType = SelectedType::Player;
            g_selectedIndex = i;
        }
    }

    for (int i = 0; i < (int)bots.size(); i++)
    {
        char label[128];
        snprintf(
            label,
            sizeof(label),
            "[BOT %02d]  (0x%016" PRIX64 ")",
            i,
            bots[i].actorAddress
        );

        bool selected =
            (g_selectedType == SelectedType::Bot && g_selectedIndex == i);

        if (ImGui::Selectable(label, selected))
        {
            g_selectedType = SelectedType::Bot;
            g_selectedIndex = i;
        }
    }

    ImGui::EndChild();

    ImGui::Separator();

    // ==================================================
    // DETALHES DO ATOR SELECIONADO
    // ==================================================
    ImGui::Text("Actor Details");

    ImGui::BeginChild("dbg_actor_details", ImVec2(0, 0), true);

    if (g_selectedType == SelectedType::Player &&
        g_selectedIndex >= 0 &&
        g_selectedIndex < (int)players.size())
    {
        DrawActorDetails(players[g_selectedIndex]);
    }
    else if (g_selectedType == SelectedType::Bot &&
        g_selectedIndex >= 0 &&
        g_selectedIndex < (int)bots.size())
    {
        DrawActorDetails(bots[g_selectedIndex]);
    }
    else
    {
        ImGui::TextDisabled("Selecione um Player ou Bot na lista acima");
    }

    ImGui::EndChild();
}
