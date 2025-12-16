#include "Aimbot.h"
#include "Menu.h"
#include "SharedMemory.h"
#include <Windows.h>

void Aimbot::Update()
{
    auto* block = SharedMemory::GetBlock();
    if (!block)
        return;

    // ===============================
    // 1) HOTKEY DOWN (tempo real)
    // ===============================
    Menu::Aimbot.HotkeyDown =
        (GetAsyncKeyState(Menu::Aimbot.Hotkey) & 0x8000) != 0;

    // ===============================
    // 2) ENVIO PARA SHARED MEMORY
    // ===============================
    SharedAimbotConfig& out = block->aimbot;

    out.enabled = Menu::Aimbot.Enabled ? 1 : 0;
    out.hotkeyDown = Menu::Aimbot.HotkeyDown ? 1 : 0;
    out.targetMode = static_cast<uint8_t>(Menu::Aimbot.Target);
    out.aimAtTeam = Menu::Aimbot.AimAtTeam ? 1 : 0;
    out.visibleOnly = Menu::Aimbot.VisibleOnly ? 1 : 0;

    out.fov = Menu::Aimbot.Fov;
    out.smooth = Menu::Aimbot.Smooth;

    // ===============================
    // PREDIÇÃO
    // ===============================
    out.prediction = Menu::Aimbot.Prediction; // 0 = OFF
}
