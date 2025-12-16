#include "Watermark.h"
#include "ImGui/imgui.h"

// ============================================================================
// RENDER ULTRA SIMPLES — SEM JANELA, SEM ATLAS, SEM MENU, SEM ERRO
// ============================================================================
void Watermark::Render()
{
    // Garantir que o contexto existe
    if (!ImGui::GetCurrentContext())
        return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    if (!draw)
        return;

    // Texto simples no canto superior esquerdo
    draw->AddText(ImVec2(10, 10), IM_COL32(255, 255, 255, 255), "DAHAX RUNNING");
}
