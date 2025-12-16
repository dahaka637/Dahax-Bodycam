#pragma once
#include <Windows.h>

namespace Menu
{
    // ===============================
    // CONTROLE DO MENU
    // ===============================
    inline bool Open = false;
    inline int ToggleKey = VK_INSERT;

    void Toggle();
    void Render();
    void ProcessHotkeys();
    // ===============================
    // AIMBOT
    // ===============================
// ===============================
// AIMBOT
// ===============================
    struct AimbotConfig
    {

        bool Enabled = false;              
        int  Hotkey = VK_RBUTTON;         
        bool HotkeyDown = false;

        enum class TargetMode
        {
            Head = 0,                     
            Chest = 1,                     
            NearestBone = 2                
        };

        TargetMode Target = TargetMode::Head; 

        bool AimAtTeam = false;           
        bool VisibleOnly = true;           

        float Fov = 120.0f;               
        float Smooth = 6.0f;              
        float Prediction = 0.0f;

        bool DrawFov = true;               
    };

    inline AimbotConfig Aimbot;

    // ===============================
    // ESP
    // ===============================
    struct EspConfig
    {
        bool Enabled = true;
        int ToggleKey = 'V';

        bool Box = true;
        bool Name = true;
        bool Distance = true;
        bool Snapline = false;
        bool Skeleton = false;
        bool ShowTeam = false;
    };

    inline EspConfig ESP;

    // ===============================
    // DEBUG (futuro)
    // ===============================
    struct DebugConfig
    {
        bool Placeholder = false;
    };

    inline DebugConfig Debug;
}
