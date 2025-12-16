#include "Menu_Debug.h"
#include "imgui.h"
#include "ExternalCache.h"
#include <vector>
#include <string>

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
// HELPERS VISUAIS
// ======================================================
static void DrawVec3(const char* label, const Vec3& v)
{
    ImGui::Text("%s:  X %.2f | Y %.2f | Z %.2f", label, v.x, v.y, v.z);
}

static void DrawRotator(const char* label, const Rotator& r)
{
    ImGui::Text("%s:  P %.2f | Y %.2f | R %.2f", label, r.pitch, r.yaw, r.roll);
}

static void DrawActorDetails(const ExternalActor& a)
{
    ImGui::Text("Name: %s", a.name);
    ImGui::Text("Team: %d", a.team);
    ImGui::Text("Health: %.2f", a.health);
    ImGui::Text("Flags: 0x%02X", a.flags);

    ImGui::Separator();
    DrawVec3("Location", a.location);

    ImGui::Separator();
    ImGui::Text("Bones");

    ImGui::BeginChild("dbg_bones", ImVec2(0, 160), true);
    for (int i = 0; i < BONE_COUNT; i++)
    {
        if (!a.boneValid[i])
            continue;

        ImGui::Text("[%02d]  %.2f  %.2f  %.2f",
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
    // LOCAL PLAYER (SEMPRE VISÍVEL)
    // ==================================================
    if (ImGui::CollapsingHeader("Local Player", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ExternalLocalPlayer local = ExternalCache::GetLocal();

        DrawVec3("Location", local.location);
        DrawVec3("Camera", local.cameraLocation);
        DrawRotator("Camera Rot", local.cameraRotation);
        ImGui::Text("Camera FOV: %.2f", local.cameraFOV);
    }

    ImGui::Separator();

    // ==================================================
    // LISTA ÚNICA DE ATORES
    // ==================================================
    ImGui::Text("Actors");
    ImGui::BeginChild("dbg_actor_list", ImVec2(0, 160), true);

    auto players = ExternalCache::GetPlayers();
    auto bots = ExternalCache::GetBots();

    // PLAYERS
    for (int i = 0; i < (int)players.size(); i++)
    {
        char label[64];
        snprintf(label, sizeof(label), "[PLY %02d] %s", i, players[i].name);

        bool selected =
            (g_selectedType == SelectedType::Player && g_selectedIndex == i);

        if (ImGui::Selectable(label, selected))
        {
            g_selectedType = SelectedType::Player;
            g_selectedIndex = i;
        }
    }

    // BOTS
    for (int i = 0; i < (int)bots.size(); i++)
    {
        char label[64];
        snprintf(label, sizeof(label), "[BOT %02d]", i);

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
