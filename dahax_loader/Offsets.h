// Offsets.h
#pragma once

#include <Windows.h>
#include <cstdint>

namespace Offsets
{
    // ======================================================
    // PROCESSO
    // ======================================================
    inline constexpr const wchar_t* PROCESS_NAME =
        L"Bodycam-Win64-Shipping.exe";

    // ======================================================
    // OFFSETS BASE
    // ======================================================
    inline constexpr uint64_t GWorld = 0x93CFB50;

    // ======================================================
    // UWorld
    // ======================================================
    inline constexpr uint64_t OwningGameInstance = 0x1D8;

    // ======================================================
    // UGameInstance
    // ======================================================
    inline constexpr uint64_t LocalPlayers = 0x38; // TArray<ULocalPlayer*>

    // ======================================================
    // ULocalPlayer
    // ======================================================
    inline constexpr uint64_t PlayerController = 0x30;

    // ======================================================
    // APlayerController
    // ======================================================
    inline constexpr uint64_t PlayerCameraManager = 0x360;

    // ======================================================
    // APlayerCameraManager
    // ======================================================
    inline constexpr uint64_t CameraCachePrivate = 0x1410;
    inline constexpr uint64_t POV_Offset = 0x10; // CameraCachePrivate.POV

    // ======================================================
    // POV OFFSETS (IGUAL AO PYTHON)
    // ======================================================
    inline constexpr uint64_t POV_Location = 0x00; // double x3
    inline constexpr uint64_t POV_Rotation = 0x18; // double x3
    inline constexpr uint64_t POV_FOV = 0x30; // float

    // ======================================================
    // ESTRUTURAS (IGUAIS AO PYTHON)
    // ======================================================
#pragma pack(push, 1)

    struct FVector
    {
        double X;
        double Y;
        double Z;
    };

    struct FRotator
    {
        double Pitch;
        double Yaw;
        double Roll;
    };

#pragma pack(pop)

    static_assert(sizeof(FVector) == 24, "FVector errado");
    static_assert(sizeof(FRotator) == 24, "FRotator errado");

    struct TArray
    {
        uintptr_t Data;
        int32_t   Count;
        int32_t   Max;
    };

    // ======================================================
    // CAMERA MANAGER (EXTERNAL – PYTHON-STYLE)
    // ======================================================
    class CameraManager
    {
    private:
        HANDLE    m_hProcess = nullptr;
        uintptr_t m_base = 0;
        uintptr_t m_pov = 0;

        bool IsValidPtr(uintptr_t p) const
        {
            return p && p > 0x10000;
        }

    public:
        bool Initialize(HANDLE hProcess, uintptr_t base)
        {
            m_hProcess = hProcess;
            m_base = base;

            uintptr_t world = 0;
            if (!ReadProcessMemory(m_hProcess, (LPCVOID)(m_base + GWorld),
                &world, sizeof(world), nullptr))
                return false;

            if (!IsValidPtr(world))
                return false;

            uintptr_t gameInstance = 0;
            ReadProcessMemory(m_hProcess,
                (LPCVOID)(world + OwningGameInstance),
                &gameInstance, sizeof(gameInstance), nullptr);

            if (!IsValidPtr(gameInstance))
                return false;

            TArray localPlayers{};
            ReadProcessMemory(m_hProcess,
                (LPCVOID)(gameInstance + LocalPlayers),
                &localPlayers, sizeof(localPlayers), nullptr);

            if (localPlayers.Count <= 0 || !IsValidPtr(localPlayers.Data))
                return false;

            uintptr_t localPlayer = 0;
            ReadProcessMemory(m_hProcess,
                (LPCVOID)(localPlayers.Data),
                &localPlayer, sizeof(localPlayer), nullptr);

            if (!IsValidPtr(localPlayer))
                return false;

            uintptr_t controller = 0;
            ReadProcessMemory(m_hProcess,
                (LPCVOID)(localPlayer + PlayerController),
                &controller, sizeof(controller), nullptr);

            if (!IsValidPtr(controller))
                return false;

            uintptr_t camMgr = 0;
            ReadProcessMemory(m_hProcess,
                (LPCVOID)(controller + PlayerCameraManager),
                &camMgr, sizeof(camMgr), nullptr);

            if (!IsValidPtr(camMgr))
                return false;

            m_pov = camMgr + CameraCachePrivate + POV_Offset;
            return IsValidPtr(m_pov);
        }

        bool IsValid() const
        {
            return m_hProcess && IsValidPtr(m_pov);
        }

        // ===================== LEITURA =====================
        FVector ReadLocation() const
        {
            FVector v{};
            if (!IsValid()) return v;

            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(m_pov + POV_Location),
                &v,
                sizeof(v),
                nullptr);

            return v;
        }

        FRotator ReadRotation() const
        {
            FRotator r{};
            if (!IsValid()) return r;

            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(m_pov + POV_Rotation),
                &r,
                sizeof(r),
                nullptr);

            return r;
        }

        float ReadFOV() const
        {
            float fov = 90.f;
            if (!IsValid()) return fov;

            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(m_pov + POV_FOV),
                &fov,
                sizeof(fov),
                nullptr);

            return fov;
        }

        // ===================== ESCRITA =====================
        bool WriteRotation(const FRotator& rot) const
        {
            if (!IsValid()) return false;

            SIZE_T bw = 0;
            return WriteProcessMemory(
                m_hProcess,
                (LPVOID)(m_pov + POV_Rotation),
                &rot,
                sizeof(rot),
                &bw) && bw == sizeof(rot);
        }

        bool WriteAngles(double pitch, double yaw, double roll = 0.0) const
        {
            FRotator r{ pitch, yaw, roll };
            return WriteRotation(r);
        }
    };

    // ======================================================
    // SINGLETON
    // ======================================================
    inline CameraManager& GetCameraManager()
    {
        static CameraManager inst;
        return inst;
    }

    inline bool InitializeCameraSystem(HANDLE hProcess, uintptr_t base)
    {
        return GetCameraManager().Initialize(hProcess, base);
    }
}
