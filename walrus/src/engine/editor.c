#include <engine/editor.h>

bool igVec3(char const *label, f32 *values, f32 reset_value, f32 column_width)
{
    bool     ret      = false;
    ImGuiIO *io       = igGetIO();
    ImFont  *boldfont = io->Fonts->Fonts.Data[0];

    igPushID_Str(label);

    igColumns(2, NULL, NULL);
    igSetColumnWidth(0, column_width);
    igTextUnformatted(label, NULL);
    igNextColumn();

    igPushMultiItemsWidths(3, igCalcItemWidth());
    igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){0, 0});

    float  line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 button_size = {line_height + 3.0f, line_height};

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.8f, 0.1f, 0.15f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.9f, 0.2f, 0.2f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.8f, 0.1f, 0.15f, 1.0f});
    igPushFont(boldfont);
    if (igButton("X", button_size)) {
        ret |= true;
        values[0] = reset_value;
    }
    igPopFont();
    igPopStyleColor(3);

    igSameLine(0.f, -1.0f);
    ret |= igDragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f", 0);
    igPopItemWidth();
    igSameLine(0.f, -1.0f);

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.2f, 0.7f, 0.2f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.3f, 0.8f, 0.3f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.2f, 0.7f, 0.2f, 1.0f});
    igPushFont(boldfont);
    if (igButton("Y", button_size)) {
        ret |= true;
        values[1] = reset_value;
    }
    igPopFont();
    igPopStyleColor(3);

    igSameLine(0.f, -1.0f);
    ret |= igDragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f", 0);
    igPopItemWidth();
    igSameLine(0.f, -1.0f);

    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.1f, 0.25f, 0.8f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.2f, 0.35f, 0.9f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.1f, 0.25f, 0.8f, 1.0f});
    igPushFont(boldfont);
    if (igButton("Z", button_size)) {
        ret |= true;
        values[2] = reset_value;
    }
    igPopFont();
    igPopStyleColor(3);

    igSameLine(0.f, -1.0f);
    ret |= igDragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f", 0);
    igPopItemWidth();

    igPopStyleVar(1);

    igColumns(1, NULL, true);

    igPopID();

    return ret;
}
