// dahax_loader.cpp
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include "imgui.h"
#include "Overlay.h"
#include "Watermark.h"
#include "Injector.h"
#include "SharedMemory.h"
#include "ExternalCache.h"
#include "ESP.h"
#include <thread>
#include <chrono>
#include "Aimbot.h"  // <-- ADICIONE ESTA LINHA
#include "Menu.h"
#include "UIStyle.h"
#include "Offsets.h"

// ======================================================
// Overlay
// ======================================================
Overlay g_Overlay;
bool running = true;
// ======================================================
// Console
// ======================================================
static void AttachConsole()
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Bodycam ESP");
}

static uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName)
{
    MODULEENTRY32W me{ sizeof(me) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    uintptr_t base = 0;
    if (Module32FirstW(snap, &me))
    {
        do
        {
            if (_wcsicmp(me.szModule, moduleName) == 0)
            {
                base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                break;
            }
        } while (Module32NextW(snap, &me));
    }

    CloseHandle(snap);
    return base;
}


// ======================================================
// Utils
// ======================================================
static DWORD GetProcessId(const wchar_t* name)
{
    PROCESSENTRY32W pe{ sizeof(pe) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    DWORD pid = 0;
    if (Process32FirstW(snap, &pe))
    {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0)
            {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static HWND FindMainWindowByPID(DWORD pid)
{
    struct EnumData
    {
        DWORD pid;
        HWND  hwnd;
    } data{ pid, nullptr };

    EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
        {
            auto& d = *reinterpret_cast<EnumData*>(lParam);

            DWORD windowPid = 0;
            GetWindowThreadProcessId(hWnd, &windowPid);

            if (windowPid != d.pid)
                return TRUE;

            // Ignora janelas invisíveis
            if (!IsWindowVisible(hWnd))
                return TRUE;

            // Ignora janelas filhas / owned
            if (GetWindow(hWnd, GW_OWNER) != nullptr)
                return TRUE;

            // Ignora janelas tool / helper
            LONG ex = GetWindowLongW(hWnd, GWL_EXSTYLE);
            if (ex & WS_EX_TOOLWINDOW)
                return TRUE;

            d.hwnd = hWnd;
            return FALSE; // achou a principal
        }, reinterpret_cast<LPARAM>(&data));

    return data.hwnd;

}



// ======================================================
// WinMain
// ======================================================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    AttachConsole();
    printf("[Overlay ESP] Iniciando...\n");

    // ==================================================
    // Aguarda jogo
    // ==================================================
    printf("Aguardando Bodycam...\n");

    DWORD pid = 0;
    while (!(pid = GetProcessId(L"Bodycam-Win64-Shipping.exe")))
    {
        Sleep(100);
        if (GetAsyncKeyState(VK_F10) & 1)
        {
            printf("[USER] Encerrando manualmente\n");
            return 0;
        }
    }

    printf("Jogo encontrado (PID %lu)\n", pid);

    // ==================================================
    // Abre processo
    // ==================================================
    HANDLE hProcess = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE,
        pid
    );

    if (!hProcess)
    {
        printf("[ERRO] OpenProcess falhou\n");
        return 0;
    }

    // ==================================================
    // Base address
    // ==================================================
    uintptr_t baseAddress = GetModuleBaseAddress(
        pid,
        L"Bodycam-Win64-Shipping.exe"
    );

    if (!baseAddress)
    {
        printf("[ERRO] BaseAddress não encontrado\n");
        CloseHandle(hProcess);
        return 0;
    }

    printf("[INFO] BaseAddress = 0x%p\n", (void*)baseAddress);

    // ==================================================
    // Inicializa leitura direta da câmera (POV)
    // ==================================================
    printf("[INFO] Inicializando CameraManager...\n");

    if (!Offsets::InitializeCameraSystem(hProcess, baseAddress))
    {
        printf("[ERRO] Falha ao inicializar CameraManager\n");
        CloseHandle(hProcess);
        return 0;
    }

    printf("[INFO] CameraManager inicializado com sucesso\n");

    // ==================================================
    // Injeção
    // ==================================================
    printf("Verificando se DLL já está ativa...\n");

    if (!SharedMemory::IsAlreadyInjected())
    {
        printf("Injetando DLL...\n");
        InjectResult injectResult = Injector::InjectWhenReady(
            L"Bodycam-Win64-Shipping.exe",
            L"dahax.dll",
            2000
        );

        if (injectResult != InjectResult::Success)
        {
            printf("[ERRO] Falha na injeção (%d)\n", (int)injectResult);
            CloseHandle(hProcess);
            return 0;
        }

        printf("DLL injetada com sucesso!\n");
        Sleep(1000);
    }
    else
    {
        printf("[INFO] DLL já está injetada.\n");
    }

    // ==================================================
    // SharedMemory e Cache
    // ==================================================
    printf("Conectando SharedMemory...\n");
    while (!SharedMemory::Connect())
    {
        printf("[EXT][SM] aguardando DLL...\n");
        Sleep(500);
    }

    SharedMemory::RequestStart();
    ExternalCache::Initialize();

    printf("SharedMemory conectada\n");



    // ==================================================
    // Overlay
    // ==================================================
    HWND gameWindow = nullptr;
    while (!gameWindow)
    {
        gameWindow = FindMainWindowByPID(pid);
        Sleep(50);
    }

    printf("Janela do jogo encontrada: 0x%p\n", gameWindow);

    if (!g_Overlay.Initialize(gameWindow))
    {
        printf("[ERRO] Falha ao inicializar overlay\n");
        CloseHandle(hProcess);
        return 0;
    }

    g_Overlay.SetClickThrough(true);
    printf("Overlay inicializado com sucesso\n");

    // ==================================================
    // TIMER DE UPDATE
    // ==================================================
    LARGE_INTEGER freq{};
    LARGE_INTEGER lastUpdate{};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastUpdate);
    constexpr double UPDATE_INTERVAL_MS = 1000.0 / 160.0;

    // ==================================================
    // LOOP PRINCIPAL
    // ==================================================
    MSG msg{};
    while (true)
    {
        // ================= GAME ALIVE CHECK =================
        DWORD gameState = WaitForSingleObject(hProcess, 0);
        if (gameState == WAIT_OBJECT_0)
        {
            printf("[INFO] Jogo foi fechado\n");
            break;
        }

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (GetAsyncKeyState(VK_F10) & 1)
        {
            printf("[USER] Encerrando manualmente\n");
            break;
        }

        // ================= UPDATE =================
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double deltaMs =
            (double)(now.QuadPart - lastUpdate.QuadPart) * 1000.0 /
            (double)freq.QuadPart;

        if (deltaMs >= UPDATE_INTERVAL_MS)
        {
            if (!SharedMemory::IsAlive())
            {
                printf("[ERRO] SharedMemory morreu\n");
                break;
            }

            ExternalCache::Update();
            lastUpdate = now;
        }

        // ================= RENDER =================
        g_Overlay.BeginFrame();

        static int fps = 0;
        static int frameCount = 0;
        static DWORD lastFpsTime = GetTickCount();

        frameCount++;
        DWORD nowMs = GetTickCount();
        if (nowMs - lastFpsTime >= 1000)
        {
            fps = frameCount;
            frameCount = 0;
            lastFpsTime = nowMs;
        }

        Watermark::Draw(fps);
        Menu::ProcessHotkeys();
        Menu::Render();
        ESP::Render();
        Aimbot::Update();

        g_Overlay.EndFrame();
        Sleep(0);
    }

    // ==================================================
    // SHUTDOWN
    // ==================================================
    printf("\n[SHUTDOWN] Finalizando...\n");

    SharedMemory::RequestStop();
    SharedMemory::Disconnect();
    ExternalCache::Shutdown();

    g_Overlay.Shutdown();
    CloseHandle(hProcess);

    printf("[SHUTDOWN] Finalizado com sucesso\n");
    getchar();
    return 0;
}
