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



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    AttachConsole();
    printf("[Overlay ESP] Iniciando...\n");

    printf("[INFO] Procurando Bodycam...\n");

    DWORD pid = GetProcessId(L"Bodycam-Win64-Shipping.exe");
    bool gameAlreadyRunning = false;

    if (pid)
    {
        gameAlreadyRunning = true;
        printf("[INFO] Jogo já estava em execução (PID %lu)\n", pid);
    }
    else
    {
        printf("[INFO] Jogo não encontrado, aguardando iniciar...\n");

        while (!(pid = GetProcessId(L"Bodycam-Win64-Shipping.exe")))
        {
            Sleep(100);

            if (GetAsyncKeyState(VK_F10) & 1)
            {
                printf("[USER] Encerrando manualmente\n");
                return 0;
            }
        }

        printf("[INFO] Jogo iniciado (PID %lu)\n", pid);
    }

    // ==================================================
    // Espera inicial SOMENTE se o jogo acabou de abrir
    // ==================================================
    if (!gameAlreadyRunning)
    {
        printf("[INFO] Aguardando inicializacao completa do jogo (5s)...\n");
        Sleep(5000);
    }
    else
    {
        printf("[INFO] Pulando espera inicial (jogo já estava aberto)\n");
    }

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
    // Injeção
    // ==================================================
    printf("[INFO] Verificando DLL...\n");

    if (!SharedMemory::IsAlreadyInjected())
    {
        printf("[INFO] Injetando DLL...\n");

        InjectResult injectResult = Injector::InjectWhenReady(
            L"Bodycam-Win64-Shipping.exe",
            L"dahax.dll",
            3000
        );

        if (injectResult != InjectResult::Success)
        {
            printf("[ERRO] Falha na injeção (%d)\n", (int)injectResult);
            CloseHandle(hProcess);
            return 0;
        }

        printf("[INFO] DLL injetada com sucesso\n");
    }
    else
    {
        printf("[INFO] DLL já estava injetada\n");
    }

    // ==================================================
    // Aguarda DLL SOMENTE se foi injetada agora
    // ==================================================
    if (!gameAlreadyRunning)
    {
        printf("[INFO] Aguardando inicializacao da DLL (5s)...\n");
        Sleep(5000);
    }
    else
    {
        printf("[INFO] Pulando espera de DLL\n");
    }

    // ==================================================
    // SharedMemory
    // ==================================================
    printf("[INFO] Conectando SharedMemory...\n");

    DWORD smStart = GetTickCount();
    while (!SharedMemory::Connect())
    {
        Sleep(250);

        if (GetTickCount() - smStart > 10000)
        {
            printf("[ERRO] Timeout ao conectar SharedMemory\n");
            CloseHandle(hProcess);
            return 0;
        }
    }

    printf("[INFO] SharedMemory conectada\n");

    // ==================================================
    // Offsets / Camera
    // ==================================================
    if (!gameAlreadyRunning)
    {
        printf("[INFO] Aguardando estabilizacao de offsets e camera (4s)...\n");
        Sleep(4000);
    }
    else
    {
        printf("[INFO] Pulando espera de offsets/camera\n");
    }

    printf("[INFO] Inicializando CameraManager...\n");

    if (!Offsets::InitializeCameraSystem(hProcess, baseAddress))
    {
        printf("[ERRO] Falha ao inicializar CameraManager\n");
        CloseHandle(hProcess);
        return 0;
    }

    printf("[INFO] CameraManager inicializado com sucesso\n");

    // ==================================================
    // Cache externo
    // ==================================================
    SharedMemory::RequestStart();
    ExternalCache::Initialize();

    // ==================================================
    // Overlay
    // ==================================================
    HWND gameWindow = nullptr;
    while (!gameWindow)
    {
        gameWindow = FindMainWindowByPID(pid);
        Sleep(50);
    }

    printf("[INFO] Janela do jogo encontrada: 0x%p\n", gameWindow);

    if (!g_Overlay.Initialize(gameWindow))
    {
        printf("[ERRO] Falha ao inicializar overlay\n");
        CloseHandle(hProcess);
        return 0;
    }

    g_Overlay.SetClickThrough(true);
    printf("[INFO] Overlay inicializado\n");

    // ==================================================
    // Timer
    // ==================================================
    LARGE_INTEGER freq{};
    LARGE_INTEGER lastUpdate{};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastUpdate);

    constexpr double UPDATE_INTERVAL_MS = 1000.0 / 160.0;

    // ==================================================
    // Loop principal
    // ==================================================
    MSG msg{};
    while (true)
    {
        if (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0)
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
    // Shutdown
    // ==================================================
    printf("[SHUTDOWN] Finalizando...\n");

    SharedMemory::RequestStop();
    SharedMemory::Disconnect();
    ExternalCache::Shutdown();

    g_Overlay.Shutdown();
    CloseHandle(hProcess);

    printf("[SHUTDOWN] Finalizado com sucesso\n");
    getchar();
    return 0;
}
