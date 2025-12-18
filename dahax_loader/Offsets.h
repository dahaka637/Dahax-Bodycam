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
    inline constexpr uint64_t POV_Offset = 0x10;

    // ======================================================
    // POV
    // ======================================================
    inline constexpr uint64_t POV_Location = 0x00; // double x3
    inline constexpr uint64_t POV_Rotation = 0x18; // double x3
    inline constexpr uint64_t POV_FOV = 0x30; // float

    // ======================================================
    // ACTOR / COMPONENT
    // ======================================================
    inline constexpr uint64_t RootComponent = 0x01B8;
    inline constexpr uint64_t RelativeLocation = 0x0128;

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
    // HELPERS BÁSICOS
    // ======================================================
    inline bool IsValidPtr(uintptr_t p)
    {
        return p && p > 0x10000;
    }

    // ======================================================
    // CAMERA MANAGER (LEITURA)
    // ======================================================
    class CameraManager
    {
    private:
        HANDLE    m_hProcess = nullptr;
        uintptr_t m_base = 0;
        uintptr_t m_pov = 0;
        uintptr_t m_lastWorld = 0; // <<< NOVO


    public:
        bool Initialize(HANDLE hProcess, uintptr_t base)
        {
            m_hProcess = hProcess;
            m_base = base;
            m_pov = 0;

            uintptr_t world = 0;
            if (!ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(m_base + GWorld),
                &world,
                sizeof(world),
                nullptr))
                return false;

            if (!IsValidPtr(world))
                return false;

            // >>> DETECTA TROCA DE PARTIDA
            if (world != m_lastWorld)
            {
                m_lastWorld = world;
            }


            uintptr_t gameInstance = 0;
            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(world + OwningGameInstance),
                &gameInstance,
                sizeof(gameInstance),
                nullptr);

            if (!IsValidPtr(gameInstance))
                return false;

            TArray localPlayers{};
            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(gameInstance + LocalPlayers),
                &localPlayers,
                sizeof(localPlayers),
                nullptr);

            if (localPlayers.Count <= 0 || !IsValidPtr(localPlayers.Data))
                return false;

            uintptr_t localPlayer = 0;
            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(localPlayers.Data),
                &localPlayer,
                sizeof(localPlayer),
                nullptr);

            if (!IsValidPtr(localPlayer))
                return false;

            uintptr_t controller = 0;
            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(localPlayer + PlayerController),
                &controller,
                sizeof(controller),
                nullptr);

            if (!IsValidPtr(controller))
                return false;

            uintptr_t camMgr = 0;
            ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(controller + PlayerCameraManager),
                &camMgr,
                sizeof(camMgr),
                nullptr);

            if (!IsValidPtr(camMgr))
                return false;

            m_pov = camMgr + CameraCachePrivate + POV_Offset;
            return IsValidPtr(m_pov);
        }

        bool IsValid() const
        {
            return m_hProcess && m_base && IsValidPtr(m_pov);
        }

        bool EnsureValid()
        {
            if (!m_hProcess || !m_base)
                return false;

            uintptr_t world = 0;
            if (!ReadProcessMemory(
                m_hProcess,
                (LPCVOID)(m_base + GWorld),
                &world,
                sizeof(world),
                nullptr))
                return false;

            if (!IsValidPtr(world))
                return false;

            if (world != m_lastWorld)
            {
                m_lastWorld = world;
                return Initialize(m_hProcess, m_base);
            }

            float fov = ReadFOV();
            if (fov < 10.f || fov > 170.f)
                return Initialize(m_hProcess, m_base);

            return true;
        }



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
    };

    // ======================================================
    // ACTOR HELPERS (LEITURA EM TEMPO REAL)
    // ======================================================
    inline bool ReadActorLocation(
        HANDLE hProcess,
        uint64_t actorAddress,
        FVector& outLocation)
    {
        outLocation = {};

        if (!IsValidPtr(actorAddress))
            return false;

        uintptr_t rootComponent = 0;
        if (!ReadProcessMemory(
            hProcess,
            (LPCVOID)(actorAddress + RootComponent),
            &rootComponent,
            sizeof(rootComponent),
            nullptr))
            return false;

        if (!IsValidPtr(rootComponent))
            return false;

        return ReadProcessMemory(
            hProcess,
            (LPCVOID)(rootComponent + RelativeLocation),
            &outLocation,
            sizeof(outLocation),
            nullptr);
    }

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
