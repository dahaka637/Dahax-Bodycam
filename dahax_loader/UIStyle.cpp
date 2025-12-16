#include "UIStyle.h"
#include "imgui.h"

namespace UIStyle
{
    void Apply()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // ==================================================
        // ESPAÇAMENTO / FORMAS
        // ==================================================
        style.WindowRounding = 8.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 6.0f;

        style.WindowPadding = ImVec2(12, 12);
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(10, 8);
        style.ItemInnerSpacing = ImVec2(6, 4);

        // ==================================================
        // PALETA (PRETO + VERMELHO)
        // ==================================================
        ImVec4 red = ImVec4(0.90f, 0.10f, 0.10f, 1.00f);
        ImVec4 redHover = ImVec4(1.00f, 0.18f, 0.18f, 1.00f);
        ImVec4 redActive = ImVec4(0.75f, 0.08f, 0.08f, 1.00f);

        ImVec4 bgDark = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        ImVec4 bgMedium = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        ImVec4 bgLight = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

        ImVec4 text = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
        ImVec4 textMuted = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);

        // ==================================================
        // CORES GERAIS
        // ==================================================
        colors[ImGuiCol_Text] = text;
        colors[ImGuiCol_TextDisabled] = textMuted;

        colors[ImGuiCol_WindowBg] = bgDark;
        colors[ImGuiCol_ChildBg] = bgDark;
        colors[ImGuiCol_PopupBg] = bgMedium;

        colors[ImGuiCol_Border] = ImVec4(0.25f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        // ==================================================
        // FRAMES / BOTÕES
        // ==================================================
        colors[ImGuiCol_FrameBg] = bgMedium;
        colors[ImGuiCol_FrameBgHovered] = bgLight;
        colors[ImGuiCol_FrameBgActive] = bgLight;

        colors[ImGuiCol_Button] = bgMedium;
        colors[ImGuiCol_ButtonHovered] = redHover;
        colors[ImGuiCol_ButtonActive] = redActive;

        // ==================================================
        // CHECKBOX / SLIDER
        // ==================================================
        colors[ImGuiCol_CheckMark] = red;

        colors[ImGuiCol_SliderGrab] = red;
        colors[ImGuiCol_SliderGrabActive] = redHover;

        // ==================================================
        // TABS
        // ==================================================
        colors[ImGuiCol_Tab] = bgMedium;
        colors[ImGuiCol_TabHovered] = redHover;
        colors[ImGuiCol_TabActive] = red;
        colors[ImGuiCol_TabUnfocused] = bgMedium;
        colors[ImGuiCol_TabUnfocusedActive] = redActive;

        // ==================================================
        // HEADER (TREE / COLLAPSING)
        // ==================================================
        colors[ImGuiCol_Header] = bgMedium;
        colors[ImGuiCol_HeaderHovered] = redHover;
        colors[ImGuiCol_HeaderActive] = redActive;

        // ==================================================
        // SCROLLBAR
        // ==================================================
        colors[ImGuiCol_ScrollbarBg] = bgDark;
        colors[ImGuiCol_ScrollbarGrab] = bgLight;
        colors[ImGuiCol_ScrollbarGrabHovered] = redHover;
        colors[ImGuiCol_ScrollbarGrabActive] = redActive;

        // ==================================================
        // TITLE BAR (CABEÇALHO DA JANELA)
        // ==================================================
        colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = red;
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.02f, 0.02f, 1.00f);

        // ==================================================
        // RESIZE GRIP (CANTO DE REDIMENSIONAR)
        // ==================================================
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.40f, 0.05f, 0.05f, 0.40f);
        colors[ImGuiCol_ResizeGripHovered] = redHover;
        colors[ImGuiCol_ResizeGripActive] = redActive;


    }
}
