#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <d3d11.h>

class Overlay
{
public:
    Overlay() = default;
    ~Overlay();

    HWND GetHwnd() const { return overlayHwnd; }

    bool Initialize(HWND targetWindow);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void SetClickThrough(bool enable);

private:
    // Window
    HWND overlayHwnd{ nullptr };
    HWND targetHwnd{ nullptr };
    DWORD targetPid{ 0 };

    // D3D11
    ID3D11Device* device{ nullptr };
    ID3D11DeviceContext* context{ nullptr };
    IDXGISwapChain* swapchain{ nullptr };
    ID3D11RenderTargetView* rtv{ nullptr };

    // State
    UINT width{ 0 };
    UINT height{ 0 };

private:
    bool CreateOverlayWindow();
    bool CreateDeviceAndSwapchain();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    void UpdateOverlayPosition();
    HWND ResolveTargetWindow();
};
