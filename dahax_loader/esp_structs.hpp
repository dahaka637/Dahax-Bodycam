#pragma once
#include <cstdint>
#include <cmath>

// ------------------------------------------------------------
// Estruturas fundamentais do Unreal Engine 5
// ------------------------------------------------------------

// Rotação padrão do UE5 (Pitch, Yaw, Roll)
struct FRotator
{
    float Pitch;   // +0x00
    float Yaw;     // +0x04
    float Roll;    // +0x08
};

// Vetor 3D padrão do UE5
struct FVector
{
    float X;       // +0x00
    float Y;       // +0x04
    float Z;       // +0x08
};

// Informações mínimas de câmera usadas pelo view matrix
struct FMinimalViewInfo
{
    FVector  Location;              // +0x00
    FRotator Rotation;              // +0x0C
    float    FOV;                   // +0x18

    float    OrthoWidth;            // +0x1C
    float    OrthoNearClipPlane;    // +0x20
    float    OrthoFarClipPlane;     // +0x24

    float    AspectRatio;           // +0x28
};

// Estrutura REAL do UE5 usada pelo PlayerCameraManager::CameraCachePrivate
// Tamanho total ~0x5D0 dependendo da versão.
// Para Bodycam (UE 5.3/5.4), esta versão está PERFEITA.
struct FCameraCacheEntry
{
    float TimeStamp;          // +0x00
    char  pad_0004[0x0C];     // +0x04 → +0x10 (padding interno do Unreal)
    FMinimalViewInfo POV;     // +0x10 → inclui Location, Rotation, FOV, etc.
};
