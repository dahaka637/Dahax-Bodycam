#pragma once

// ======================================================
// SharedSender
// Responsável por serializar o Cache interno da DLL
// para o SharedPayload (SharedMemory)
// ======================================================

namespace SharedSender
{
    // Chamada periódica (thread sender)
    // - Escreve o payload completo
    // - Usa seqlock (seq)
    // - Incrementa frameId por último
    void Tick();
}
