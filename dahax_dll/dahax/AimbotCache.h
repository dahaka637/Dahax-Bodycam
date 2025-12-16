#pragma once
#include <cstdint>
#include <atomic>
struct AimbotCacheData
{
    bool enabled;
    bool hotkeyDown;

    uint8_t targetMode;
    bool aimAtTeam;
    bool visibleOnly;

    float fov;
    float smooth;

    // ===============================
    // PREDIÇÃO
    // ===============================
    float prediction;
};


namespace AimbotCache
{
    bool Initialize();
    void Shutdown();

    void Update(); // lê do SharedMemory
    AimbotCacheData Get(); // snapshot seguro
}
