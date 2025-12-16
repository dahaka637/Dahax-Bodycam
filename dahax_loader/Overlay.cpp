#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>
#include <cstdio>

#include "Overlay.h"

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
);


// =====================================================
// Helpers
// =====================================================
static HWND FindMainWindowByPID(DWORD pid)
{
    struct EnumData { DWORD pid; HWND hwnd; } data{ pid, nullptr };

    EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
        {
            auto& d = *(EnumData*)lParam;

            DWORD windowPid = 0;
            GetWindowThreadProcessId(hWnd, &windowPid);

            if (windowPid == d.pid &&
                IsWindowVisible(hWnd) &&
                GetWindow(hWnd, GW_OWNER) == nullptr)
            {
                d.hwnd = hWnd;
                return FALSE;
            }
            return TRUE;
        }, (LPARAM)&data);

    return data.hwnd;
}

// =====================================================
// WinProc
// =====================================================
static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return TRUE;

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// =====================================================
// Lifecycle
// =====================================================
Overlay::~Overlay()
{
    Shutdown();
}

bool Overlay::Initialize(HWND targetWindow)
{
    if (!targetWindow)
        return false;

    targetHwnd = targetWindow;
    GetWindowThreadProcessId(targetHwnd, &targetPid);

    if (!CreateOverlayWindow())
        return false;

    if (!CreateDeviceAndSwapchain())
        return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(overlayHwnd);
    ImGui_ImplDX11_Init(device, context);

    return true;
}

void Overlay::Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupRenderTarget();

    if (swapchain) swapchain->Release();
    if (context) context->Release();
    if (device) device->Release();

    if (overlayHwnd)
        DestroyWindow(overlayHwnd);

    overlayHwnd = nullptr;
}

// =====================================================
// Window
// =====================================================
bool Overlay::CreateOverlayWindow()
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"CleanOverlay";

    RegisterClassExW(&wc);

    overlayHwnd = CreateWindowExW(
        WS_EX_TOPMOST |
        WS_EX_LAYERED |
        WS_EX_TRANSPARENT |
        WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"",
        WS_POPUP,
        0, 0, 1, 1,
        nullptr, nullptr,
        wc.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(overlayHwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    MARGINS m{ -1 };
    DwmExtendFrameIntoClientArea(overlayHwnd, &m);

    ShowWindow(overlayHwnd, SW_SHOW);
    return true;
}

// =====================================================
// D3D11
// =====================================================
bool Overlay::CreateDeviceAndSwapchain()
{
    RECT r{};
    DwmGetWindowAttribute(
        targetHwnd,
        DWMWA_EXTENDED_FRAME_BOUNDS,
        &r,
        sizeof(r)
    );

    width = r.right - r.left;
    height = r.bottom - r.top;

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = overlayHwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        levels,
        1,
        D3D11_SDK_VERSION,
        &sd,
        &swapchain,
        &device,
        &fl,
        &context)))
        return false;

    CreateRenderTarget();
    return true;
}

void Overlay::CreateRenderTarget()
{
    ID3D11Texture2D* bb{};
    swapchain->GetBuffer(0, IID_PPV_ARGS(&bb));
    device->CreateRenderTargetView(bb, nullptr, &rtv);
    bb->Release();
}

void Overlay::CleanupRenderTarget()
{
    if (rtv)
    {
        rtv->Release();
        rtv = nullptr;
    }
}

// =====================================================
// Frame
// =====================================================
void Overlay::BeginFrame()
{
    UpdateOverlayPosition();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Overlay::EndFrame()
{
    ImGui::Render();

    const float clear[4] = { 0,0,0,0 };
    context->OMSetRenderTargets(1, &rtv, nullptr);
    context->ClearRenderTargetView(rtv, clear);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swapchain->Present(0, 0); // sem vsync

}

// =====================================================
// Sync
// =====================================================
void Overlay::UpdateOverlayPosition()
{
    HWND newTarget = ResolveTargetWindow();
    if (!newTarget)
        return;

    targetHwnd = newTarget;

    RECT r{};
    DwmGetWindowAttribute(
        targetHwnd,
        DWMWA_EXTENDED_FRAME_BOUNDS,
        &r,
        sizeof(r)
    );

    UINT newW = r.right - r.left;
    UINT newH = r.bottom - r.top;

    SetWindowPos(
        overlayHwnd,
        HWND_TOPMOST,
        r.left,
        r.top,
        newW,
        newH,
        SWP_NOACTIVATE
    );

    if (newW != width || newH != height)
    {
        width = newW;
        height = newH;

        CleanupRenderTarget();
        swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
    }
}

HWND Overlay::ResolveTargetWindow()
{
    return FindMainWindowByPID(targetPid);
}

// =====================================================
// Click-through
// =====================================================
void Overlay::SetClickThrough(bool enable)
{
    LONG ex = GetWindowLongW(overlayHwnd, GWL_EXSTYLE);

    if (enable)
        ex |= WS_EX_TRANSPARENT;
    else
        ex &= ~WS_EX_TRANSPARENT;

    SetWindowLongW(overlayHwnd, GWL_EXSTYLE, ex);

    // 🔴 LINHA CRÍTICA — força o Windows a reaplicar o estilo
    SetWindowPos(
        overlayHwnd,
        nullptr,
        0, 0, 0, 0,
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_NOZORDER |
        SWP_FRAMECHANGED
    );
}
