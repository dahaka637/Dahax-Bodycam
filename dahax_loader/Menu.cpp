#include "Menu.h"
#include "imgui.h"
#include <cstdio>
#include <Windows.h>
#include "Overlay.h"
#include "Menu_Debug.h"
#include "UIStyle.h"

extern Overlay g_Overlay;
static HWND g_LastGameHwnd = nullptr;

// ======================================================
// Helpers
// ======================================================
struct HotkeyCaptureState
{
    bool Active = false;
    int* TargetKey = nullptr;
    DWORD StartTime = 0;
    bool MouseReleased = false;
};

static HotkeyCaptureState g_HotkeyCapture;


static const char* GetKeyName(int key)
{
    switch (key)
    {
    case VK_LBUTTON: return "Mouse 1";
    case VK_RBUTTON: return "Mouse 2";
    case VK_MBUTTON: return "Mouse 3";
    case VK_XBUTTON1: return "Mouse 4";
    case VK_XBUTTON2: return "Mouse 5";
    }

    static char name[32]{};

    UINT scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
    LONG lParam = (scan << 16);
    if (GetKeyNameTextA(lParam, name, sizeof(name)) == 0)
        strcpy_s(name, "Unknown");

    return name;
}


static bool HotkeyButton(const char* label, int& key)
{
    ImGui::Text("%s:", label);
    ImGui::SameLine();

    char buf[64];

    if (g_HotkeyCapture.Active && g_HotkeyCapture.TargetKey == &key)
        strcpy_s(buf, "Pressione uma tecla...");
    else
        sprintf_s(buf, "[ %s ]", GetKeyName(key));

    if (ImGui::Button(buf, ImVec2(160, 0)))
    {
        g_HotkeyCapture.Active = true;
        g_HotkeyCapture.TargetKey = &key;
        g_HotkeyCapture.StartTime = GetTickCount();
        g_HotkeyCapture.MouseReleased = false;
    }

    // ===============================
    // CAPTURA
    // ===============================
    if (g_HotkeyCapture.Active && g_HotkeyCapture.TargetKey == &key)
    {
        // Espera soltar TODOS os botões do mouse
        if (!g_HotkeyCapture.MouseReleased)
        {
            if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) &&
                !(GetAsyncKeyState(VK_RBUTTON) & 0x8000) &&
                !(GetAsyncKeyState(VK_MBUTTON) & 0x8000))
            {
                g_HotkeyCapture.MouseReleased = true;
            }
            return false;
        }

        // Cancela com ESC
        if (GetAsyncKeyState(VK_ESCAPE) & 1)
        {
            g_HotkeyCapture.Active = false;
            g_HotkeyCapture.TargetKey = nullptr;
            return false;
        }

        // Limpa com BACKSPACE
        if (GetAsyncKeyState(VK_BACK) & 1)
        {
            key = 0;
            g_HotkeyCapture.Active = false;
            g_HotkeyCapture.TargetKey = nullptr;
            return true;
        }

        // Teclado
        for (int i = 0x08; i <= 0xFE; i++)
        {
            if (GetAsyncKeyState(i) & 1)
            {
                key = i;
                g_HotkeyCapture.Active = false;
                g_HotkeyCapture.TargetKey = nullptr;
                return true;
            }
        }

        // Mouse (depois do release)
        if (GetAsyncKeyState(VK_LBUTTON) & 1) { key = VK_LBUTTON; goto done; }
        if (GetAsyncKeyState(VK_RBUTTON) & 1) { key = VK_RBUTTON; goto done; }
        if (GetAsyncKeyState(VK_MBUTTON) & 1) { key = VK_MBUTTON; goto done; }
        if (GetAsyncKeyState(VK_XBUTTON1) & 1) { key = VK_XBUTTON1; goto done; }
        if (GetAsyncKeyState(VK_XBUTTON2) & 1) { key = VK_XBUTTON2; goto done; }

        return false;

    done:
        g_HotkeyCapture.Active = false;
        g_HotkeyCapture.TargetKey = nullptr;
        return true;
    }

    return false;
}


// ======================================================
// Toggle do menu (FOCO / CLICK)
// ======================================================
void Menu::Toggle()
{
    Open = !Open;

    g_Overlay.SetClickThrough(!Open);
    ImGui::GetIO().MouseDrawCursor = Open;

    if (Open)
    {
        HWND overlay = g_Overlay.GetHwnd();

        // 🔴 GUARDA O HWND DO JOGO ANTES DE ROUBAR O FOCO
        g_LastGameHwnd = GetForegroundWindow();

        if (overlay && g_LastGameHwnd)
        {
            DWORD overlayThread = GetWindowThreadProcessId(overlay, nullptr);
            DWORD gameThread = GetWindowThreadProcessId(g_LastGameHwnd, nullptr);

            AttachThreadInput(gameThread, overlayThread, TRUE);

            SetForegroundWindow(overlay);
            SetActiveWindow(overlay);
            SetFocus(overlay);

            AttachThreadInput(gameThread, overlayThread, FALSE);
        }
    }
    else
    {
        // 🔁 DEVOLVE FOCO AO JOGO ORIGINAL
        if (g_LastGameHwnd && IsWindow(g_LastGameHwnd))
        {
            DWORD overlayThread = GetWindowThreadProcessId(g_Overlay.GetHwnd(), nullptr);
            DWORD gameThread = GetWindowThreadProcessId(g_LastGameHwnd, nullptr);

            AttachThreadInput(overlayThread, gameThread, TRUE);

            SetForegroundWindow(g_LastGameHwnd);
            SetActiveWindow(g_LastGameHwnd);
            SetFocus(g_LastGameHwnd);

            AttachThreadInput(overlayThread, gameThread, FALSE);
        }
    }
}




void Menu::ProcessHotkeys()
{
    static bool lastEspKey = false;

    bool espKeyNow = (GetAsyncKeyState(ESP.ToggleKey) & 0x8000) != 0;

    if (espKeyNow && !lastEspKey)
    {
        ESP.Enabled = !ESP.Enabled;
    }

    lastEspKey = espKeyNow;
}

// ======================================================
// Render
// ======================================================
void Menu::Render()
{
    UIStyle::Apply();
    static bool lastToggle = false;
    bool toggleNow = (GetAsyncKeyState(ToggleKey) & 1) != 0;

    if (toggleNow && !lastToggle)
        Toggle();

    lastToggle = toggleNow;

    if (!Open)
        return;

    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_FirstUseEver);


    if (!ImGui::Begin("Bodycam Config", &Open,
        ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }


    if (ImGui::BeginTabBar("##tabs"))
    {
        // ==================================================
        // AIMBOT
        // ==================================================
        if (ImGui::BeginTabItem("AIM"))
        {
            // ----------------------------
            // ATIVAÇÃO
            // ----------------------------
            ImGui::Checkbox("Ativar Aimbot", &Aimbot.Enabled);
            HotkeyButton("Hotkey", Aimbot.Hotkey);

            ImGui::Separator();

            // ----------------------------
            // TARGET MODE
            // ----------------------------
            const char* targets[] = { "Head", "Chest", "Nearest Bone" };
            ImGui::Combo(
                "Target",
                reinterpret_cast<int*>(&Aimbot.Target),
                targets,
                IM_ARRAYSIZE(targets)
            );

            // ----------------------------
            // REGRAS DE SELEÇÃO
            // ----------------------------
            ImGui::Checkbox("Mirar na propria equipe", &Aimbot.AimAtTeam);
            ImGui::Checkbox("Apenas alvos visiveis", &Aimbot.VisibleOnly);

            ImGui::Separator();

            // ----------------------------
            // PARAMETROS DE MIRA
            // ----------------------------
            ImGui::SliderFloat("FOV", &Aimbot.Fov, 10.f, 400.f, "%.0f");
            ImGui::SliderFloat("Suavizacao", &Aimbot.Smooth, 1.f, 20.f, "%.1f");

            ImGui::Separator();

            // ----------------------------
            // PREDIÇÃO (0 = OFF)
            // ----------------------------
            ImGui::Text("Predicao");
            ImGui::SliderFloat(
                "Forca da Predicao",
                &Aimbot.Prediction,
                0.0f,
                5.0f,
                "%.2f"
            );

            ImGui::Separator();

            // ----------------------------
            // VISUAL (LOCAL)
            // ----------------------------
            ImGui::Checkbox("Desenhar FOV", &Aimbot.DrawFov);

            ImGui::EndTabItem();
        }


        // ==================================================
        // ESP
        // ==================================================
        if (ImGui::BeginTabItem("ESP"))
        {
            ImGui::Checkbox("Ativar ESP", &ESP.Enabled);
            HotkeyButton("Toggle ESP", ESP.ToggleKey);

            ImGui::Separator();
            ImGui::Checkbox("Box", &ESP.Box);
            ImGui::Checkbox("Nome", &ESP.Name);
            ImGui::Checkbox("Distancia", &ESP.Distance);
            ImGui::Checkbox("Snapline", &ESP.Snapline);
            ImGui::Checkbox("Esqueleto", &ESP.Skeleton);
            ImGui::Checkbox("Mostrar equipe", &ESP.ShowTeam);

            ImGui::EndTabItem();
        }

        // ==================================================
        // DEBUG
        // ==================================================
        if (ImGui::BeginTabItem("DBG"))
        {
            MenuDebug::Render();
            ImGui::EndTabItem();
        }


        ImGui::EndTabBar();
    }

    ImGui::End();
}
