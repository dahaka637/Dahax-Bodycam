#pragma once
#include "imgui.h"

namespace UI
{
    // =========================================================
    // Paleta de cores — Dark / Black + Red (estilo premium)
    // =========================================================

    // Vermelho principal (ativo)
    inline ImVec4 AccentRed = ImVec4(0.90f, 0.12f, 0.12f, 1.00f);

    // Vermelho hover (escuro, vinho)
    inline ImVec4 AccentRedHover = ImVec4(0.45f, 0.08f, 0.08f, 1.00f);

    // Vermelho hover suave
    inline ImVec4 AccentRedSoft = ImVec4(0.30f, 0.06f, 0.06f, 1.00f);

    // Fundos
    inline ImVec4 BgDeep = ImVec4(0.04f, 0.04f, 0.04f, 0.70f);
    inline ImVec4 BgDark = ImVec4(0.07f, 0.07f, 0.07f, 0.74f);
    inline ImVec4 BgMid = ImVec4(0.11f, 0.11f, 0.11f, 0.78f);
    inline ImVec4 BgLight = ImVec4(0.16f, 0.16f, 0.16f, 0.82f);

    // =========================================================
    // Aplica o estilo global (chamar UMA VEZ após InitImGui)
    // =========================================================
    inline void ApplySketchStyle()
    {
        ImGuiStyle& s = ImGui::GetStyle();
        ImVec4* c = s.Colors;

        // ----------------- Layout / Shape -----------------
        s.WindowRounding = 12.f;
        s.ChildRounding = 10.f;
        s.FrameRounding = 6.f;
        s.PopupRounding = 10.f;
        s.ScrollbarRounding = 6.f;
        s.GrabRounding = 6.f;
        s.TabRounding = 6.f;

        s.WindowBorderSize = 1.0f;
        s.FrameBorderSize = 0.0f;
        s.PopupBorderSize = 0.0f;

        // ----------------- Espaçamento --------------------
        s.WindowPadding = ImVec2(14, 14);
        s.FramePadding = ImVec2(10, 6);
        s.ItemSpacing = ImVec2(10, 8);
        s.ItemInnerSpacing = ImVec2(6, 6);
        s.ScrollbarSize = 6.f;
        s.GrabMinSize = 14.f;

        // ----------------- Texto --------------------------
        c[ImGuiCol_Text] = ImVec4(1, 1, 1, 1);
        c[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1);

        // ----------------- Fundos -------------------------
        c[ImGuiCol_WindowBg] = BgDeep;
        c[ImGuiCol_ChildBg] = BgDark;
        c[ImGuiCol_PopupBg] = BgDark;

        // Borda da janela (vermelho escuro fino)
        c[ImGuiCol_Border] = ImVec4(
            AccentRedHover.x,
            AccentRedHover.y,
            AccentRedHover.z,
            0.35f
        );

        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        // ----------------- Frames / Slider Track -----------------
        c[ImGuiCol_FrameBg] = BgMid;

        c[ImGuiCol_FrameBgHovered] = ImVec4(
            AccentRedHover.x * 0.6f,
            AccentRedHover.y * 0.6f,
            AccentRedHover.z * 0.6f,
            0.85f
        );

        c[ImGuiCol_FrameBgActive] = ImVec4(
            AccentRedHover.x * 0.75f,
            AccentRedHover.y * 0.75f,
            AccentRedHover.z * 0.75f,
            1.00f
        );

        // ----------------- Checkbox -------------------------
        c[ImGuiCol_CheckMark] = AccentRed;

        // Fundo do checkbox
        c[ImGuiCol_FrameBg] = BgMid;

        // Hover do checkbox
        c[ImGuiCol_FrameBgHovered] = ImVec4(
            AccentRedSoft.x * 0.85f,
            AccentRedSoft.y * 0.85f,
            AccentRedSoft.z * 0.85f,
            0.80f
        );

        // Quando marcado / clicando
        c[ImGuiCol_FrameBgActive] = ImVec4(
            AccentRedHover.x,
            AccentRedHover.y,
            AccentRedHover.z,
            0.95f
        );


        // ----------------- Slider Grab (bolinha) -----------------
        c[ImGuiCol_SliderGrab] = AccentRed;
        c[ImGuiCol_SliderGrabActive] = AccentRed;

        // ----------------- Headers / Selectables ----------
        c[ImGuiCol_Header] = BgMid;
        c[ImGuiCol_HeaderHovered] = AccentRedSoft;
        c[ImGuiCol_HeaderActive] = AccentRedHover;

        // ----------------- Tabs ---------------------------
        c[ImGuiCol_Tab] = BgDark;
        c[ImGuiCol_TabHovered] = AccentRedSoft;
        c[ImGuiCol_TabActive] = AccentRedHover;
        c[ImGuiCol_TabUnfocused] = BgDark;
        c[ImGuiCol_TabUnfocusedActive] = BgMid;

        // ----------------- Botões -------------------------
        c[ImGuiCol_Button] = BgMid;
        c[ImGuiCol_ButtonHovered] = AccentRedSoft;
        c[ImGuiCol_ButtonActive] = AccentRed;

        // ----------------- Scrollbar ----------------------
        c[ImGuiCol_ScrollbarBg] = BgDark;
        c[ImGuiCol_ScrollbarGrab] = BgMid;
        c[ImGuiCol_ScrollbarGrabHovered] = AccentRedSoft;
        c[ImGuiCol_ScrollbarGrabActive] = AccentRed;

        // ----------------- Separadores --------------------
        c[ImGuiCol_Separator] = BgLight;
        c[ImGuiCol_SeparatorHovered] = AccentRedSoft;
        c[ImGuiCol_SeparatorActive] = AccentRedHover;

        // ----------------- Resize Grip --------------------
        c[ImGuiCol_ResizeGrip] = ImVec4(
            AccentRedHover.x,
            AccentRedHover.y,
            AccentRedHover.z,
            0.25f
        );

        c[ImGuiCol_ResizeGripHovered] = AccentRedSoft;
        c[ImGuiCol_ResizeGripActive] = AccentRed;
    }

    // =========================================================
    // Editor visual (opcional / debug)
    // =========================================================
    inline void DrawStyleEditor()
    {
        ImGui::Text("Style Editor");
        ImGui::Separator();

        ImGui::Spacing();
        ImGui::Text("Accent");
        ImGui::ColorEdit4("Accent", (float*)&AccentRed, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Accent Hover", (float*)&AccentRedHover, ImGuiColorEditFlags_NoInputs);

        ImGui::Spacing();
        ImGui::Text("Background");
        ImGui::ColorEdit4("Deep", (float*)&BgDeep, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Dark", (float*)&BgDark, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Mid", (float*)&BgMid, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Light", (float*)&BgLight, ImGuiColorEditFlags_NoInputs);

        ImGui::Spacing();
        ImGui::TextDisabled("All changes apply instantly");
    }
}
