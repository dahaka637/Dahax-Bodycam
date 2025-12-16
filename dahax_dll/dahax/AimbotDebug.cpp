#include "AimbotDebug.h"
#include "AimbotCache.h"

#include <cstdio>
#include <cstring>

static bool g_Initialized = false;
static bool g_FirstDump = true;

static AimbotCacheData g_Last{};

static void PrintFull(const AimbotCacheData& d)
{
    printf("========== AIMBOT CACHE (FULL DUMP) ==========\n");
    printf("enabled       : %d\n", d.enabled);
    printf("hotkeyDown    : %d\n", d.hotkeyDown);
    printf("targetMode    : %u\n", d.targetMode);
    printf("aimAtTeam     : %d\n", d.aimAtTeam);
    printf("visibleOnly   : %d\n", d.visibleOnly);
    printf("fov           : %.3f\n", d.fov);
    printf("smooth        : %.3f\n", d.smooth);
    printf("prediction    : %.3f\n", d.prediction);
    printf("==============================================\n");
}

static void PrintDiff(const char* name, bool oldV, bool newV)
{
    if (oldV != newV)
        printf("[AIMBOT] %s: %d -> %d\n", name, oldV, newV);
}

static void PrintDiff(const char* name, uint8_t oldV, uint8_t newV)
{
    if (oldV != newV)
        printf("[AIMBOT] %s: %u -> %u\n", name, oldV, newV);
}

static void PrintDiff(const char* name, float oldV, float newV)
{
    if (oldV != newV)
        printf("[AIMBOT] %s: %.3f -> %.3f\n", name, oldV, newV);
}

void AimbotCacheDebug::Initialize()
{
    std::memset(&g_Last, 0, sizeof(g_Last));
    g_FirstDump = true;
    g_Initialized = true;

    printf("[AIMBOT DEBUG] Initialized\n");
}

void AimbotCacheDebug::Shutdown()
{
    g_Initialized = false;
    printf("[AIMBOT DEBUG] Shutdown\n");
}

void AimbotCacheDebug::Tick()
{
    if (!g_Initialized)
        return;

    AimbotCacheData cur = AimbotCache::Get();

    if (g_FirstDump)
    {
        PrintFull(cur);
        g_Last = cur;
        g_FirstDump = false;
        return;
    }

    // ===============================
    // DIFF
    // ===============================
    PrintDiff("enabled", g_Last.enabled, cur.enabled);
    PrintDiff("hotkeyDown", g_Last.hotkeyDown, cur.hotkeyDown);
    PrintDiff("targetMode", g_Last.targetMode, cur.targetMode);
    PrintDiff("aimAtTeam", g_Last.aimAtTeam, cur.aimAtTeam);
    PrintDiff("visibleOnly", g_Last.visibleOnly, cur.visibleOnly);
    PrintDiff("fov", g_Last.fov, cur.fov);
    PrintDiff("smooth", g_Last.smooth, cur.smooth);
    PrintDiff("prediction", g_Last.prediction, cur.prediction);

    g_Last = cur;
}
