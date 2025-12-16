#include "Watermark.h"
#include "imgui.h"
#include <cstdio>

void Watermark::Draw(int fps)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    // --------------------------------------------------
    // Posição base (canto superior esquerdo)
    // --------------------------------------------------
    ImVec2 basePos(
        viewport->Pos.x + 12.0f,
        viewport->Pos.y + 10.0f
    );

    // --------------------------------------------------
    // Textos
    // --------------------------------------------------
    const char* titleText = "Dahax Menu 0.5";

    char fpsText[32];
    std::snprintf(fpsText, sizeof(fpsText), "FPS: %d", fps);

    // --------------------------------------------------
    // Cores
    // --------------------------------------------------
    ImColor shadowColor = ImColor(0, 0, 0, 200);

    ImColor titleColor = ImColor(220, 40, 40, 255);   // vermelho principal
    ImColor fpsColor = ImColor(200, 200, 200, 220); // cinza claro

    // --------------------------------------------------
    // Fonte
    // --------------------------------------------------
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();

    // --------------------------------------------------
    // Título (com sombra)
    // --------------------------------------------------
    draw->AddText(
        font,
        fontSize,
        ImVec2(basePos.x + 1.0f, basePos.y + 1.0f),
        shadowColor,
        titleText
    );

    draw->AddText(
        font,
        fontSize,
        basePos,
        titleColor,
        titleText
    );

    // --------------------------------------------------
    // FPS (logo abaixo, menor espaçamento)
    // --------------------------------------------------
    ImVec2 fpsPos(
        basePos.x,
        basePos.y + fontSize + 2.0f
    );

    draw->AddText(
        font,
        fontSize,
        ImVec2(fpsPos.x + 1.0f, fpsPos.y + 1.0f),
        shadowColor,
        fpsText
    );

    draw->AddText(
        font,
        fontSize,
        fpsPos,
        fpsColor,
        fpsText
    );
}
