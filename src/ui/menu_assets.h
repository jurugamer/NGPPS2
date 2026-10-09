/**
 * PS2 LAUNCHER ENGINE - LAYOUT & ASSETS HEADER
 * Gerado automaticamente por PS2 Menu UI Creator.
 */

#ifndef MENU_ASSETS_H
#define MENU_ASSETS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char title[64];
    char subtitle[64];
    char tag[16];
    char filepath[256];
} LauncherItem;

typedef struct {
    char button[16];
    char text[48];
} LauncherFooterPrompt;

typedef struct {
    int show_numbers;
    int show_subtitles;
    int show_tags;
    int show_scrollbar;
    int show_logo;
	int show_footer;
    const char* empty_msg;
    const char* empty_sub;
} PS2_LauncherConfig;

typedef struct {
    float screen_width;
    float screen_height;
    float atlas_width;
    float atlas_height;

    float font_char_width;
    float font_char_height;
    float font_letter_spacing;
    float font_glyph_advance;

    float item_width;
    float item_height;
    float item_spacing;
    float item_step_y;
    float item_start_x;
    float item_start_y;
    int   visible_item_count;

    float item_index_offset_x;
    float item_index_scale;
    float item_title_num_offset_x;
    float item_title_no_num_offset_x;
    float item_title_sub_offset_y;
    float item_title_sub_scale;
    float item_subtitle_offset_y;
    float item_subtitle_scale;
    float item_title_single_offset_y;
    float item_title_single_scale;

    float item_tag_badge_offset_x;
    float item_tag_badge_offset_y;
    float item_tag_badge_w;
    float item_tag_badge_h;

    float scrollbar_track_x;
    float scrollbar_track_y;
    float scrollbar_width;
    float scrollbar_track_height;

    int   title_enabled;
    float title_x;
    float title_y;
    float title_width;
    float title_height;

    /* Barra de Rodapé com Botões (Demonstrativa - Posição Hardcoded) */
    int   footer_enabled;
    int   footer_prompts_count;
    float footer_y;
    float footer_height;
    int   footer_show_dividers;
    LauncherFooterPrompt footer_prompts[8];

    unsigned int color_footer_bg;
    unsigned int color_footer_text;
    unsigned int color_text_normal;
    unsigned int color_text_selected;
    unsigned int color_glow_selected;
    unsigned int color_subtitle;
    unsigned int color_tag_text;
    unsigned int color_scrollbar_thumb;
    unsigned int color_scrollbar_track;
    unsigned int color_empty_state;

    /* Teclado Virtual (OSK) Cores Dinamicas do Tema */
    unsigned int color_keyboard_bg;
    unsigned int color_keyboard_border;
    unsigned int color_keyboard_key_bg;
    unsigned int color_keyboard_key_border;
    unsigned int color_keyboard_key_text;
    unsigned int color_keyboard_key_selected_bg;
    unsigned int color_keyboard_key_selected_border;
    unsigned int color_keyboard_key_selected_text;
    unsigned int color_keyboard_input_bg;
    unsigned int color_keyboard_input_border;

    float uv_title_u0, uv_title_v0, uv_title_u1, uv_title_v1;
    float uv_plate_normal_u0, uv_plate_normal_v0, uv_plate_normal_u1, uv_plate_normal_v1;
    float uv_plate_selected_u0, uv_plate_selected_v0, uv_plate_selected_u1, uv_plate_selected_v1;
    float uv_tag_badge_u0, uv_tag_badge_v0, uv_tag_badge_u1, uv_tag_badge_v1;
    float uv_scroll_thumb_u0, uv_scroll_thumb_v0, uv_scroll_thumb_u1, uv_scroll_thumb_v1;
    float uv_scroll_track_u0, uv_scroll_track_v0, uv_scroll_track_u1, uv_scroll_track_v1;
} PS2_ThemeConfig;

#define SCREEN_WIDTH               640
#define SCREEN_HEIGHT              448
#define ATLAS_WIDTH                512
#define ATLAS_HEIGHT               256

#define FONT_ATLAS_WIDTH           512
#define FONT_ATLAS_HEIGHT          256
#define FONT_CHAR_WIDTH            32
#define FONT_CHAR_HEIGHT           32
#define FONT_COLS                  16
#define FONT_ROWS                  6
#define FONT_START_ASCII           32
#define FONT_LETTER_SPACING        1.0f
#define FONT_GLYPH_ADVANCE         14.0f

#define ITEM_START_X               100.0f
#define ITEM_START_Y               125.4f
#define ITEM_WIDTH                 440.0f
#define ITEM_HEIGHT                38.0f
#define ITEM_SPACING               6.0f
#define ITEM_STEP_Y                44.0f
#define VISIBLE_ITEM_COUNT         7

#define ITEM_INDEX_OFFSET_X        18.0f
#define ITEM_INDEX_SCALE           0.72f

#define ITEM_TITLE_NUM_OFFSET_X    52.0f
#define ITEM_TITLE_NO_NUM_OFFSET_X 18.0f

#define ITEM_TITLE_SUB_OFFSET_Y    3.0f
#define ITEM_TITLE_SUB_SCALE       0.80f

#define ITEM_SUBTITLE_OFFSET_Y     20.0f
#define ITEM_SUBTITLE_SCALE        0.54f

#define ITEM_TITLE_SINGLE_OFFSET_Y 9.0f
#define ITEM_TITLE_SINGLE_SCALE    0.92f

#define ITEM_TAG_BADGE_OFFSET_X    344.0f
#define ITEM_TAG_BADGE_OFFSET_Y    10.0f
#define ITEM_TAG_BADGE_W           78.0f
#define ITEM_TAG_BADGE_H           18.0f

#define SCROLLBAR_TRACK_X          550.0f
#define SCROLLBAR_TRACK_Y          125.4f
#define SCROLLBAR_WIDTH            7.0f
#define SCROLLBAR_TRACK_HEIGHT     302.0f

#define TITLE_ENABLED              1
#define TITLE_X                    204.5f
#define TITLE_Y                    35.8f
#define TITLE_WIDTH                231.0f
#define TITLE_HEIGHT               48.0f

#define FOOTER_HEIGHT              28.0f
#define FOOTER_Y                   (448.0f - 28.0f)
#define COLOR_FOOTER_BG            0xD9170602
#define COLOR_FOOTER_TEXT          0xFFE1D5CB

#define COLOR_TEXT_NORMAL          0xFFE1D5CB
#define COLOR_TEXT_SELECTED        0xFFFFFFFF
#define COLOR_GLOW_SELECTED        0xFFF6823B
#define COLOR_SUBTITLE             0xD7B8A394
#define COLOR_TAG_TEXT             0xFFE1D5CB
#define COLOR_SCROLLBAR_THUMB      0xFFF8BD38
#define COLOR_SCROLLBAR_TRACK      0xFFFFFFFF
#define COLOR_EMPTY_STATE          0xFF4444EF
#define COLOR_KEYBOARD_BG          0xF428140A
#define COLOR_KEYBOARD_BORDER      0xFFFAA560
#define COLOR_KEYBOARD_KEY_BG      0xE628140A
#define COLOR_KEYBOARD_KEY_BORDER  0xFF8A3A1E
#define COLOR_KEYBOARD_KEY_TEXT    0xFFE1D5CB
#define COLOR_KEYBOARD_KEY_SEL_BG  0xFFD84E1D
#define COLOR_KEYBOARD_KEY_SEL_BOR 0xFFFAA560
#define COLOR_KEYBOARD_KEY_SEL_TXT 0xFFFFFFFF
#define COLOR_KEYBOARD_INPUT_BG    0xFF170602
#define COLOR_KEYBOARD_INPUT_BOR   0xFFFAA560

#define UV_TITLE_U0                0.015625f
#define UV_TITLE_V0                0.015625f
#define UV_TITLE_U1                0.466797f
#define UV_TITLE_V1                0.203125f

#define UV_PLATE_NORMAL_U0         0.015625f
#define UV_PLATE_NORMAL_V0         0.250000f
#define UV_PLATE_NORMAL_U1         0.875000f
#define UV_PLATE_NORMAL_V1         0.414063f

#define UV_PLATE_SELECTED_U0       0.015625f
#define UV_PLATE_SELECTED_V0       0.453125f
#define UV_PLATE_SELECTED_U1       0.875000f
#define UV_PLATE_SELECTED_V1       0.617188f

#define UV_TAG_BADGE_U0            0.125000f
#define UV_TAG_BADGE_V0            0.664063f
#define UV_TAG_BADGE_U1            0.289063f
#define UV_TAG_BADGE_V1            0.750000f

#define UV_SCROLL_THUMB_U0         0.015625f
#define UV_SCROLL_THUMB_V0         0.664063f
#define UV_SCROLL_THUMB_U1         0.046875f
#define UV_SCROLL_THUMB_V1         0.851563f

#define UV_SCROLL_TRACK_U0         0.070313f
#define UV_SCROLL_TRACK_V0         0.664063f
#define UV_SCROLL_TRACK_U1         0.101563f
#define UV_SCROLL_TRACK_V1         0.851563f


#define KEYBOARD_ATLAS_WIDTH       512
#define KEYBOARD_ATLAS_HEIGHT      256

#define UV_KBD_DIALOG_U0           0.015625f
#define UV_KBD_DIALOG_V0           0.031250f
#define UV_KBD_DIALOG_U1           0.984375f
#define UV_KBD_DIALOG_V1           0.539063f

#define UV_KBD_INPUT_U0            0.015625f
#define UV_KBD_INPUT_V0            0.570313f
#define UV_KBD_INPUT_U1            0.640625f
#define UV_KBD_INPUT_V1            0.703125f

#define UV_KBD_KEY_NORMAL_U0       0.015625f
#define UV_KBD_KEY_NORMAL_V0       0.742188f
#define UV_KBD_KEY_NORMAL_U1       0.109375f
#define UV_KBD_KEY_NORMAL_V1       0.882813f

#define UV_KBD_KEY_SELECTED_U0     0.125000f
#define UV_KBD_KEY_SELECTED_V0     0.742188f
#define UV_KBD_KEY_SELECTED_U1     0.218750f
#define UV_KBD_KEY_SELECTED_V1     0.882813f

#define UV_KBD_KEY_ACTIVE_U0       0.234375f
#define UV_KBD_KEY_ACTIVE_V0       0.742188f
#define UV_KBD_KEY_ACTIVE_U1       0.328125f
#define UV_KBD_KEY_ACTIVE_V1       0.882813f

#define UV_KBD_SPACE_NORMAL_U0     0.343750f
#define UV_KBD_SPACE_NORMAL_V0     0.742188f
#define UV_KBD_SPACE_NORMAL_U1     0.640625f
#define UV_KBD_SPACE_NORMAL_V1     0.882813f

#define UV_KBD_SPACE_SELECTED_U0   0.656250f
#define UV_KBD_SPACE_SELECTED_V0   0.742188f
#define UV_KBD_SPACE_SELECTED_U1   0.953125f
#define UV_KBD_SPACE_SELECTED_V1   0.882813f

void PS2_Button_GetUV(const char* button_name, float* out_u0, float* out_v0, float* out_u1, float* out_v1);

/* Coordenadas UV dos 16 botoes embutidos diretamente no font_atlas.png (Row 6) */
#define FONT_ATLAS_HAS_BUTTONS     1
#define FONT_BTN_ROW_V0            0.750000f
#define FONT_BTN_ROW_V1            0.875000f
void PS2_Font_GetButtonUV(const char* button_name, float* out_u0, float* out_v0, float* out_u1, float* out_v1);

/* Funcoes do Tema Dinamico via .cfg */
int  PS2_Launcher_LoadThemeConfig(const char* filepath);
const PS2_ThemeConfig* PS2_Launcher_GetThemeConfig(void);
void PS2_Launcher_SetThemeConfig(const PS2_ThemeConfig* theme);

void PS2_Launcher_ScrollInit(LauncherItem* items, int total_items);
void PS2_Launcher_SetItems(LauncherItem* items, int total_items);
void PS2_Launcher_SetConfig(const PS2_LauncherConfig* config);
PS2_LauncherConfig PS2_Launcher_GetDefaultConfig(void);
LauncherItem* PS2_Launcher_GetSelectedItem(void);
void PS2_Launcher_ScrollInput(int up, int down, int page_up, int page_down);
void PS2_Launcher_GetScrollbarThumb(float* out_thumb_y, float* out_thumb_h);
void PS2_Font_GetCharUV(char c, float* out_u0, float* out_v0, float* out_u1, float* out_v1);

void PS2_Launcher_Render(
    void (*draw_quad)(float x, float y, float w, float h, float u0, float v0, float u1, float v1, unsigned int color),
    void (*draw_text)(float x, float y, const char* str, unsigned int color, float scale)
);

#ifdef __cplusplus
}
#endif

#endif /* MENU_ASSETS_H */
