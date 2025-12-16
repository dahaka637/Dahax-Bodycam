#include "AimbotCache.h"
#include "SharedMemory.h"

static AimbotCacheData g_Data{};
static std::atomic<bool> g_Initialized{ false };

bool AimbotCache::Initialize()
{
    g_Data = {};
    g_Initialized.store(true);
    return true;
}

void AimbotCache::Shutdown()
{
    g_Initialized.store(false);
}

void AimbotCache::Update()
{
    if (!g_Initialized.load())
        return;

    auto* block = SharedMemory::GetBlock();
    if (!block)
        return;

    const auto& in = block->aimbot;

    g_Data.enabled = in.enabled != 0;
    g_Data.hotkeyDown = in.hotkeyDown != 0;
    g_Data.targetMode = in.targetMode;
    g_Data.aimAtTeam = in.aimAtTeam != 0;
    g_Data.visibleOnly = in.visibleOnly != 0;

    g_Data.fov = in.fov;
    g_Data.smooth = in.smooth;

    // ===============================
    // PREDIÇÃO
    // ===============================
    g_Data.prediction = in.prediction;
}


AimbotCacheData AimbotCache::Get()
{
    return g_Data; // cópia segura
}
