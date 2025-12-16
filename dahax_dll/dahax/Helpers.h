#pragma once
#include <cstdint>

// ======================================================
// POINTER CHECK — PERMISSIVO E ESTÁVEL
// ======================================================
//
// Filosofia:
// - Bloquear apenas lixo óbvio
// - Nunca rejeitar ponteiro válido por layout/ASLR
// - Evitar crash, não “validar perfeição”
//

template<typename T>
inline bool IsBadPtr(T* ptr)
{
    if (!ptr)
        return true;

    uintptr_t p = reinterpret_cast<uintptr_t>(ptr);

    // Bloqueia null pages / lixo grosseiro
    if (p < 0x1000)
        return true;

    return false;
}

// ======================================================
// TARRAY CHECK — SANITY LEVE
// ======================================================
//
// Filosofia:
// - Aceitar quase tudo
// - Bloquear apenas corrupção evidente
// - Nunca quebrar streaming / mapas grandes
//

template<typename T>
inline bool IsSafeTArray(const SDK::TArray<T>& arr)
{
    int n = arr.Num();

    // corrupção real
    if (n < 0)
        return false;

    // limite alto apenas para evitar leitura absurda
    if (n > 100000)
        return false;

    return true;
}
