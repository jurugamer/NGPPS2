/**
 * PS2 LAUNCHER ENGINE - MOTOR DE ROLAGEM, RODAPÉ & TECLADO VIRTUAL (C)
 * Compatível com gsKit e totalmente desacoplado por camadas de textura.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/file_util.h"
#include "menu_scroll.h"
#include "menu_assets.h"

static int g_custom_background = 1;
static int g_settings_mode = 0;



typedef struct {
    LauncherItem* items;
    int total_items;
    int selected_index;
    int scroll_offset;
    int visible_count;
} PS2_ScrollEngine;

static PS2_ScrollEngine g_launcher_scroll;

static char s_empty_msg_buf[128] = "NENHUM JOGO ENCONTRADO";
static char s_empty_sub_buf[128] = "PRESSIONE START PARA RECARREGAR";



static PS2_LauncherConfig g_launcher_config = {
    .show_numbers   = 1,
    .show_subtitles = 1,
    .show_tags      = 1,
    .show_scrollbar = 1,
    .show_logo      = TITLE_ENABLED,
    .show_footer    = 1,
    .empty_msg      = s_empty_msg_buf,
    .empty_sub      = s_empty_sub_buf
};

static char s_exit_path[128] = "";

const char* PS2_Launcher_GetExitPath(void) {
    return s_exit_path;
}

void PS2_Launcher_SetExitPath(const char* path) {
    if (path) {
        strncpy(s_exit_path, path, sizeof(s_exit_path) - 1);
        s_exit_path[sizeof(s_exit_path) - 1] = '\0';
    } else {
        s_exit_path[0] = '\0';
    }
}

static PS2_ThemeConfig g_theme_config = {
    .screen_width               = SCREEN_WIDTH,
    .screen_height              = SCREEN_HEIGHT,
    .atlas_width                = ATLAS_WIDTH,
    .atlas_height               = ATLAS_HEIGHT,
    .font_char_width            = FONT_CHAR_WIDTH,
    .font_char_height           = FONT_CHAR_HEIGHT,
    .font_letter_spacing        = FONT_LETTER_SPACING,
    .font_glyph_advance         = FONT_GLYPH_ADVANCE,
    .item_width                 = ITEM_WIDTH,
    .item_height                = ITEM_HEIGHT,
    .item_spacing               = ITEM_SPACING,
    .item_step_y                = ITEM_STEP_Y,
    .item_start_x               = ITEM_START_X,
    .item_start_y               = ITEM_START_Y,
    .visible_item_count         = VISIBLE_ITEM_COUNT,
    .item_index_offset_x        = ITEM_INDEX_OFFSET_X,
    .item_index_scale           = ITEM_INDEX_SCALE,
    .item_title_num_offset_x    = ITEM_TITLE_NUM_OFFSET_X,
    .item_title_no_num_offset_x = ITEM_TITLE_NO_NUM_OFFSET_X,
    .item_title_sub_offset_y    = ITEM_TITLE_SUB_OFFSET_Y,
    .item_title_sub_scale       = ITEM_TITLE_SUB_SCALE,
    .item_subtitle_offset_y     = ITEM_SUBTITLE_OFFSET_Y,
    .item_subtitle_scale        = ITEM_SUBTITLE_SCALE,
    .item_title_single_offset_y = ITEM_TITLE_SINGLE_OFFSET_Y,
    .item_title_single_scale    = ITEM_TITLE_SINGLE_SCALE,
    .item_tag_badge_offset_x    = ITEM_TAG_BADGE_OFFSET_X,
    .item_tag_badge_offset_y    = ITEM_TAG_BADGE_OFFSET_Y,
    .item_tag_badge_w           = ITEM_TAG_BADGE_W,
    .item_tag_badge_h           = ITEM_TAG_BADGE_H,
    .scrollbar_track_x          = SCROLLBAR_TRACK_X,
    .scrollbar_track_y          = SCROLLBAR_TRACK_Y,
    .scrollbar_width            = SCROLLBAR_WIDTH,
    .scrollbar_track_height     = SCROLLBAR_TRACK_HEIGHT,
    .title_enabled              = TITLE_ENABLED,
    .title_x                    = TITLE_X,
    .title_y                    = TITLE_Y,
    .title_width                = TITLE_WIDTH,
    .title_height               = TITLE_HEIGHT,
    .footer_enabled             = 1,
    .footer_prompts_count       = 3,
    .footer_y                   = FOOTER_Y,
    .footer_height              = FOOTER_HEIGHT,
    .footer_show_dividers       = 1,
    .footer_prompts             = {
        { .button = "cross",    .text = "Executar" },
        { .button = "triangle", .text = "Opcoes" },
        { .button = "start",    .text = "Configuracao" },
    },
    .color_footer_bg            = COLOR_FOOTER_BG,
    .color_footer_text          = COLOR_FOOTER_TEXT,
    .color_text_normal          = COLOR_TEXT_NORMAL,
    .color_text_selected        = COLOR_TEXT_SELECTED,
    .color_glow_selected        = COLOR_GLOW_SELECTED,
    .color_subtitle             = COLOR_SUBTITLE,
    .color_tag_text             = COLOR_TAG_TEXT,
    .color_scrollbar_thumb      = COLOR_SCROLLBAR_THUMB,
    .color_scrollbar_track      = COLOR_SCROLLBAR_TRACK,
    .color_empty_state          = COLOR_EMPTY_STATE,
    .uv_title_u0                = UV_TITLE_U0,
    .uv_title_v0                = UV_TITLE_V0,
    .uv_title_u1                = UV_TITLE_U1,
    .uv_title_v1                = UV_TITLE_V1,
    .uv_plate_normal_u0         = UV_PLATE_NORMAL_U0,
    .uv_plate_normal_v0         = UV_PLATE_NORMAL_V0,
    .uv_plate_normal_u1         = UV_PLATE_NORMAL_U1,
    .uv_plate_normal_v1         = UV_PLATE_NORMAL_V1,
    .uv_plate_selected_u0       = UV_PLATE_SELECTED_U0,
    .uv_plate_selected_v0       = UV_PLATE_SELECTED_V0,
    .uv_plate_selected_u1       = UV_PLATE_SELECTED_U1,
    .uv_plate_selected_v1       = UV_PLATE_SELECTED_V1,
    .uv_tag_badge_u0            = UV_TAG_BADGE_U0,
    .uv_tag_badge_v0            = UV_TAG_BADGE_V0,
    .uv_tag_badge_u1            = UV_TAG_BADGE_U1,
    .uv_tag_badge_v1            = UV_TAG_BADGE_V1,
    .uv_scroll_thumb_u0         = UV_SCROLL_THUMB_U0,
    .uv_scroll_thumb_v0         = UV_SCROLL_THUMB_V0,
    .uv_scroll_thumb_u1         = UV_SCROLL_THUMB_U1,
    .uv_scroll_thumb_v1         = UV_SCROLL_THUMB_V1,
    .uv_scroll_track_u0         = UV_SCROLL_TRACK_U0,
    .uv_scroll_track_v0         = UV_SCROLL_TRACK_V0,
    .uv_scroll_track_u1         = UV_SCROLL_TRACK_U1,
    .uv_scroll_track_v1         = UV_SCROLL_TRACK_V1
};

const PS2_ThemeConfig* PS2_Launcher_GetThemeConfig(void) {
    return &g_theme_config;
}

void PS2_Launcher_SetThemeConfig(const PS2_ThemeConfig* theme) {
    if (theme) g_theme_config = *theme;
}

static char* trim_whitespace(char* str) {
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    end[1] = '\0';
    return str;
}

int PS2_Launcher_GetSelectedIndex(void) {
    return g_launcher_scroll.selected_index;
}

int PS2_Launcher_GetCustomBackground(void) {
    return g_custom_background;
}

void PS2_Launcher_SetCustomBackground(int enabled) {
    g_custom_background = enabled ? 1 : 0;
}

void PS2_Launcher_SetSettingsMode(int enabled) {
    g_settings_mode = enabled ? 1 : 0;
}

int PS2_Launcher_GetFooterEnabled(void) {
    return g_theme_config.footer_enabled;
}

void PS2_Launcher_SetFooterEnabled(int enabled) {
    g_theme_config.footer_enabled = enabled ? 1 : 0;
}

int PS2_Launcher_GetFooterPromptsCount(void) {
    return g_theme_config.footer_prompts_count;
}

void PS2_Launcher_SetFooterPrompt(int index, const char* button, const char* text) {
    if (index < 0 || index >= 8) return;
    if (button) {
        strncpy(g_theme_config.footer_prompts[index].button, button, sizeof(g_theme_config.footer_prompts[index].button) - 1);
        g_theme_config.footer_prompts[index].button[sizeof(g_theme_config.footer_prompts[index].button) - 1] = '\0';
    }
    if (text) {
        strncpy(g_theme_config.footer_prompts[index].text, text, sizeof(g_theme_config.footer_prompts[index].text) - 1);
        g_theme_config.footer_prompts[index].text[sizeof(g_theme_config.footer_prompts[index].text) - 1] = '\0';
    }
    if (index >= g_theme_config.footer_prompts_count) {
        g_theme_config.footer_prompts_count = index + 1;
    }
}

int PS2_Launcher_LoadThemeConfig(const char* filepath) {
    u8 *buf = NULL;
    u32 size = 0;
    if (ReadFileToBuffer(filepath, &buf, &size) <= 0 || !buf) return 0;

    g_theme_config.footer_height = FOOTER_HEIGHT;
    g_theme_config.footer_y = FOOTER_Y;

    char *line = strtok((char*)buf, "\r\n");
    while (line != NULL) {
        char *l = trim_whitespace(line);
        if (l[0] != '#' && l[0] != ';' && l[0] != '[' && l[0] != '\0') {
            char *eq = strchr(l, '=');
            if (eq != NULL) {
                *eq = '\0';
                char *key = trim_whitespace(l);
                char *val = trim_whitespace(eq + 1);

                if (strcmp(key, "screen_width") == 0) g_theme_config.screen_width = (float)atof(val);
                else if (strcmp(key, "screen_height") == 0) g_theme_config.screen_height = (float)atof(val);
                else if (strcmp(key, "font_glyph_advance") == 0) g_theme_config.font_glyph_advance = (float)atof(val);
                else if (strcmp(key, "item_width") == 0) g_theme_config.item_width = (float)atof(val);
                else if (strcmp(key, "item_height") == 0) {
                    g_theme_config.item_height = (float)atof(val);
                    g_theme_config.item_step_y = g_theme_config.item_height + g_theme_config.item_spacing;
                }
                else if (strcmp(key, "item_spacing") == 0) {
                    g_theme_config.item_spacing = (float)atof(val);
                    g_theme_config.item_step_y = g_theme_config.item_height + g_theme_config.item_spacing;
                }
                else if (strcmp(key, "item_start_x") == 0) g_theme_config.item_start_x = (float)atof(val);
                else if (strcmp(key, "item_start_y") == 0) g_theme_config.item_start_y = (float)atof(val);
                else if (strcmp(key, "visible_item_count") == 0) {
                    g_theme_config.visible_item_count = atoi(val);
                    g_launcher_scroll.visible_count = g_theme_config.visible_item_count;
                }
                else if (strcmp(key, "item_index_offset_x") == 0) g_theme_config.item_index_offset_x = (float)atof(val);
                else if (strcmp(key, "item_title_num_offset_x") == 0) g_theme_config.item_title_num_offset_x = (float)atof(val);
                else if (strcmp(key, "item_title_no_num_offset_x") == 0) g_theme_config.item_title_no_num_offset_x = (float)atof(val);
                else if (strcmp(key, "item_title_sub_offset_y") == 0) g_theme_config.item_title_sub_offset_y = (float)atof(val);
                else if (strcmp(key, "item_subtitle_offset_y") == 0) g_theme_config.item_subtitle_offset_y = (float)atof(val);
                else if (strcmp(key, "item_title_single_offset_y") == 0) g_theme_config.item_title_single_offset_y = (float)atof(val);
                else if (strcmp(key, "item_tag_badge_offset_x") == 0) g_theme_config.item_tag_badge_offset_x = (float)atof(val);
                else if (strcmp(key, "item_tag_badge_offset_y") == 0) g_theme_config.item_tag_badge_offset_y = (float)atof(val);
                else if (strcmp(key, "item_tag_badge_w") == 0) g_theme_config.item_tag_badge_w = (float)atof(val);
                else if (strcmp(key, "item_tag_badge_h") == 0) g_theme_config.item_tag_badge_h = (float)atof(val);
                else if (strcmp(key, "scrollbar_track_x") == 0) g_theme_config.scrollbar_track_x = (float)atof(val);
                else if (strcmp(key, "scrollbar_track_y") == 0) g_theme_config.scrollbar_track_y = (float)atof(val);
                else if (strcmp(key, "scrollbar_width") == 0) g_theme_config.scrollbar_width = (float)atof(val);
                else if (strcmp(key, "scrollbar_track_height") == 0) g_theme_config.scrollbar_track_height = (float)atof(val);
                else if (strcmp(key, "title_enabled") == 0) g_theme_config.title_enabled = atoi(val);
                else if (strcmp(key, "title_x") == 0) g_theme_config.title_x = (float)atof(val);
                else if (strcmp(key, "title_y") == 0) g_theme_config.title_y = (float)atof(val);
                else if (strcmp(key, "title_width") == 0) g_theme_config.title_width = (float)atof(val);
                else if (strcmp(key, "title_height") == 0) g_theme_config.title_height = (float)atof(val);
                else if (strcmp(key, "footer_enabled") == 0) g_theme_config.footer_enabled = atoi(val);
                else if (strcmp(key, "footer_prompts_count") == 0) g_theme_config.footer_prompts_count = atoi(val);
                else if (strcmp(key, "footer_show_dividers") == 0) g_theme_config.footer_show_dividers = atoi(val);
                else if (strcmp(key, "color_footer_bg") == 0) g_theme_config.color_footer_bg = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_footer_text") == 0) g_theme_config.color_footer_text = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_text_normal") == 0) g_theme_config.color_text_normal = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_text_selected") == 0) g_theme_config.color_text_selected = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_glow_selected") == 0) g_theme_config.color_glow_selected = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_subtitle") == 0) g_theme_config.color_subtitle = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_tag_text") == 0) g_theme_config.color_tag_text = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_scrollbar_thumb") == 0) g_theme_config.color_scrollbar_thumb = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_scrollbar_track") == 0) g_theme_config.color_scrollbar_track = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "color_empty_state") == 0) g_theme_config.color_empty_state = (unsigned int)strtoul(val, NULL, 0);
                else if (strcmp(key, "uv_title_u0") == 0) g_theme_config.uv_title_u0 = (float)atof(val);
                else if (strcmp(key, "uv_title_v0") == 0) g_theme_config.uv_title_v0 = (float)atof(val);
                else if (strcmp(key, "uv_title_u1") == 0) g_theme_config.uv_title_u1 = (float)atof(val);
                else if (strcmp(key, "uv_title_v1") == 0) g_theme_config.uv_title_v1 = (float)atof(val);
                else if (strcmp(key, "uv_plate_normal_u0") == 0) g_theme_config.uv_plate_normal_u0 = (float)atof(val);
                else if (strcmp(key, "uv_plate_normal_v0") == 0) g_theme_config.uv_plate_normal_v0 = (float)atof(val);
                else if (strcmp(key, "uv_plate_normal_u1") == 0) g_theme_config.uv_plate_normal_u1 = (float)atof(val);
                else if (strcmp(key, "uv_plate_normal_v1") == 0) g_theme_config.uv_plate_normal_v1 = (float)atof(val);
                else if (strcmp(key, "uv_plate_selected_u0") == 0) g_theme_config.uv_plate_selected_u0 = (float)atof(val);
                else if (strcmp(key, "uv_plate_selected_v0") == 0) g_theme_config.uv_plate_selected_v0 = (float)atof(val);
                else if (strcmp(key, "uv_plate_selected_u1") == 0) g_theme_config.uv_plate_selected_u1 = (float)atof(val);
                else if (strcmp(key, "uv_plate_selected_v1") == 0) g_theme_config.uv_plate_selected_v1 = (float)atof(val);
                else if (strcmp(key, "uv_tag_badge_u0") == 0) g_theme_config.uv_tag_badge_u0 = (float)atof(val);
                else if (strcmp(key, "uv_tag_badge_v0") == 0) g_theme_config.uv_tag_badge_v0 = (float)atof(val);
                else if (strcmp(key, "uv_tag_badge_u1") == 0) g_theme_config.uv_tag_badge_u1 = (float)atof(val);
                else if (strcmp(key, "uv_tag_badge_v1") == 0) g_theme_config.uv_tag_badge_v1 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_thumb_u0") == 0) g_theme_config.uv_scroll_thumb_u0 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_thumb_v0") == 0) g_theme_config.uv_scroll_thumb_v0 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_thumb_u1") == 0) g_theme_config.uv_scroll_thumb_u1 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_thumb_v1") == 0) g_theme_config.uv_scroll_thumb_v1 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_track_u0") == 0) g_theme_config.uv_scroll_track_u0 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_track_v0") == 0) g_theme_config.uv_scroll_track_v0 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_track_u1") == 0) g_theme_config.uv_scroll_track_u1 = (float)atof(val);
                else if (strcmp(key, "uv_scroll_track_v1") == 0) g_theme_config.uv_scroll_track_v1 = (float)atof(val);
				else if (strcmp(key, "font_letter_spacing") == 0) g_theme_config.font_letter_spacing = (float)atof(val);
				else if (strcmp(key, "font_glyph_advance") == 0) g_theme_config.font_glyph_advance = (float)atof(val);
            }
        }
        line = strtok(NULL, "\r\n");
    }

    free(buf);
    return 1;
}

int PS2_Launcher_LoadLauncherConfig(const char* filepath) {
    u8 *buf = NULL;
    u32 size = 0;
    if (ReadFileToBuffer(filepath, &buf, &size) <= 0 || !buf) return 0;

    char *line = strtok((char*)buf, "\r\n");
    while (line != NULL) {
        char *l = trim_whitespace(line);
        if (l[0] != '#' && l[0] != ';' && l[0] != '[' && l[0] != '\0') {
            char *eq = strchr(l, '=');
            if (eq != NULL) {
                *eq = '\0';
                char *key = trim_whitespace(l);
                char *val = trim_whitespace(eq + 1);

                if (strcmp(key, "show_logo") == 0) g_launcher_config.show_logo = atoi(val);
                else if (strcmp(key, "show_numbers") == 0) g_launcher_config.show_numbers = atoi(val);
                else if (strcmp(key, "show_subtitles") == 0) g_launcher_config.show_subtitles = atoi(val);
                else if (strcmp(key, "show_tags") == 0) g_launcher_config.show_tags = atoi(val);
                else if (strcmp(key, "show_scrollbar") == 0) g_launcher_config.show_scrollbar = atoi(val);
                else if (strcmp(key, "custom_background") == 0) g_custom_background = atoi(val);
                else if (strcmp(key, "empty_message") == 0) {
                    strncpy(s_empty_msg_buf, val, sizeof(s_empty_msg_buf) - 1);
                    s_empty_msg_buf[sizeof(s_empty_msg_buf) - 1] = '\0';
                    g_launcher_config.empty_msg = s_empty_msg_buf;
                }
                else if (strcmp(key, "empty_subtitle") == 0) {
                    strncpy(s_empty_sub_buf, val, sizeof(s_empty_sub_buf) - 1);
                    s_empty_sub_buf[sizeof(s_empty_sub_buf) - 1] = '\0';
                    g_launcher_config.empty_sub = s_empty_sub_buf;
                }
				else if (strcmp(key, "exit_path") == 0) {
					strncpy(s_exit_path, val, sizeof(s_exit_path) - 1);
					s_exit_path[sizeof(s_exit_path) - 1] = '\0';
				}
            }
        }
        line = strtok(NULL, "\r\n");
    }

    free(buf);
    return 1;
}

int PS2_Launcher_SaveLauncherConfig(const char* filepath) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "[features]\n"
        "show_logo = %d\n"
        "show_numbers = %d\n"
        "show_subtitles = %d\n"
        "show_tags = %d\n"
        "show_scrollbar = %d\n"
        "custom_background = %d\n"
        "exit_path = %s\n\n"
        "[messages]\n"
        "empty_message = %s\n"
        "empty_subtitle = %s\n",
        g_launcher_config.show_logo,
        g_launcher_config.show_numbers,
        g_launcher_config.show_subtitles,
        g_launcher_config.show_tags,
        g_launcher_config.show_scrollbar,
        g_custom_background,
        s_exit_path,
        g_launcher_config.empty_msg ? g_launcher_config.empty_msg : "NENHUM JOGO ENCONTRADO",
        g_launcher_config.empty_sub ? g_launcher_config.empty_sub : "PRESSIONE START PARA RECARREGAR"
    );

    int fd = fileXioOpen(filepath, 0x0002 | 0x0200 | 0x0400);
    if (fd >= 0) {
        fileXioWrite(fd, buf, strlen(buf));
        fileXioClose(fd);
        return 1;
    }

    FILE *f = fopen(filepath, "w");
    if (f) {
        fputs(buf, f);
        fclose(f);
        return 1;
    }
    return 0;
}

PS2_LauncherConfig PS2_Launcher_GetDefaultConfig(void) {
    return g_launcher_config;
}

void PS2_Launcher_SetConfig(const PS2_LauncherConfig* config) {
    if (config) g_launcher_config = *config;
}

void PS2_Launcher_ScrollInit(LauncherItem* items, int total_items) {
    g_launcher_scroll.items = items;
    g_launcher_scroll.total_items = (items != NULL && total_items > 0) ? total_items : 0;
    g_launcher_scroll.selected_index = 0;
    g_launcher_scroll.scroll_offset = 0;
    g_launcher_scroll.visible_count = g_theme_config.visible_item_count > 0 ? g_theme_config.visible_item_count : VISIBLE_ITEM_COUNT;
}

void PS2_Launcher_SetItems(LauncherItem* items, int total_items) {
    g_launcher_scroll.items = items;
    g_launcher_scroll.total_items = (items != NULL && total_items > 0) ? total_items : 0;
    if (g_launcher_scroll.selected_index >= g_launcher_scroll.total_items) {
        g_launcher_scroll.selected_index = (g_launcher_scroll.total_items > 0) ? (g_launcher_scroll.total_items - 1) : 0;
    }
    if (g_launcher_scroll.scroll_offset > g_launcher_scroll.selected_index) {
        g_launcher_scroll.scroll_offset = g_launcher_scroll.selected_index;
    }
}

LauncherItem* PS2_Launcher_GetSelectedItem(void) {
    if (g_launcher_scroll.items == NULL || g_launcher_scroll.total_items <= 0) return NULL;
    if (g_launcher_scroll.selected_index < 0 || g_launcher_scroll.selected_index >= g_launcher_scroll.total_items) return NULL;
    return &g_launcher_scroll.items[g_launcher_scroll.selected_index];
}

void PS2_Launcher_ScrollInput(int up, int down, int page_up, int page_down) {
    if (g_launcher_scroll.total_items <= 0) return;

    if (up) {
        if (g_launcher_scroll.selected_index > 0) g_launcher_scroll.selected_index--;
        else g_launcher_scroll.selected_index = g_launcher_scroll.total_items - 1;
    } else if (down) {
        if (g_launcher_scroll.selected_index < g_launcher_scroll.total_items - 1) g_launcher_scroll.selected_index++;
        else g_launcher_scroll.selected_index = 0;
    } else if (page_up) {
        g_launcher_scroll.selected_index -= g_launcher_scroll.visible_count;
        if (g_launcher_scroll.selected_index < 0) g_launcher_scroll.selected_index = 0;
    } else if (page_down) {
        g_launcher_scroll.selected_index += g_launcher_scroll.visible_count;
        if (g_launcher_scroll.selected_index >= g_launcher_scroll.total_items) {
            g_launcher_scroll.selected_index = g_launcher_scroll.total_items - 1;
        }
    }

    if (g_launcher_scroll.selected_index < g_launcher_scroll.scroll_offset) {
        g_launcher_scroll.scroll_offset = g_launcher_scroll.selected_index;
    } else if (g_launcher_scroll.selected_index >= g_launcher_scroll.scroll_offset + g_launcher_scroll.visible_count) {
        g_launcher_scroll.scroll_offset = g_launcher_scroll.selected_index - g_launcher_scroll.visible_count + 1;
    }
}

void PS2_Launcher_GetScrollbarThumb(float* out_thumb_y, float* out_thumb_h) {
    if (g_launcher_scroll.total_items <= g_launcher_scroll.visible_count || g_launcher_scroll.total_items <= 0) {
        *out_thumb_y = g_theme_config.scrollbar_track_y;
        *out_thumb_h = g_theme_config.scrollbar_track_height;
        return;
    }

    float ratio = (float)g_launcher_scroll.visible_count / (float)g_launcher_scroll.total_items;
    float thumb_h = g_theme_config.scrollbar_track_height * ratio;
    if (thumb_h < 16.0f) thumb_h = 16.0f;

    int max_scroll = g_launcher_scroll.total_items - g_launcher_scroll.visible_count;
    float scroll_pct = (float)g_launcher_scroll.scroll_offset / (float)max_scroll;
    float thumb_y = g_theme_config.scrollbar_track_y + scroll_pct * (g_theme_config.scrollbar_track_height - thumb_h);

    *out_thumb_y = thumb_y;
    *out_thumb_h = thumb_h;
}

void PS2_Font_GetCharUV(char c, float* out_u0, float* out_v0, float* out_u1, float* out_v1) {
    if (c < FONT_START_ASCII || c > 126) c = ' ';
    int idx = (int)c - FONT_START_ASCII;
    int col = idx % FONT_COLS;
    int row = idx / FONT_COLS;
    float cell_w = (float)FONT_CHAR_WIDTH / (float)FONT_ATLAS_WIDTH;
    float cell_h = (float)FONT_CHAR_HEIGHT / (float)FONT_ATLAS_HEIGHT;
    *out_u0 = (float)col * cell_w;
    *out_v0 = (float)row * cell_h;
    *out_u1 = *out_u0 + cell_w;
    *out_v1 = *out_v0 + cell_h;
}

// -----------------------------------------------------------------------------
// UVs dos Ícones dos Botões (Row 6 do font_atlas.png)
// -----------------------------------------------------------------------------
void PS2_Button_GetUV(const char* button_name, float* out_u0, float* out_v0, float* out_u1, float* out_v1) {
    if (!button_name) {
        *out_u0 = 0.0f; *out_v0 = 0.750000f; *out_u1 = 0.062500f; *out_v1 = 0.875000f;
        return;
    }
    int col = 0;
    if (strcmp(button_name, "cross") == 0) col = 0;
    else if (strcmp(button_name, "circle") == 0) col = 1;
    else if (strcmp(button_name, "triangle") == 0) col = 2;
    else if (strcmp(button_name, "square") == 0 || strcmp(button_name, "backspace") == 0) col = 3;
    else if (strcmp(button_name, "dpad_up") == 0 || strcmp(button_name, "up") == 0) col = 4;
    else if (strcmp(button_name, "dpad_down") == 0 || strcmp(button_name, "down") == 0) col = 5;
    else if (strcmp(button_name, "dpad_left") == 0 || strcmp(button_name, "left") == 0) col = 6;
    else if (strcmp(button_name, "dpad_right") == 0 || strcmp(button_name, "right") == 0) col = 7;
    else if (strcmp(button_name, "l1") == 0) col = 8;
    else if (strcmp(button_name, "r1") == 0) col = 9;
    else if (strcmp(button_name, "l2") == 0) col = 10;
    else if (strcmp(button_name, "r2") == 0) col = 11;
    else if (strcmp(button_name, "l3") == 0) col = 12;
    else if (strcmp(button_name, "r3") == 0) col = 13;
    else if (strcmp(button_name, "start") == 0) col = 14;
    else if (strcmp(button_name, "select") == 0) col = 15;
    else col = 0;

    *out_u0 = (float)col / 16.0f;
    *out_v0 = 0.750000f;
    *out_u1 = (float)(col + 1) / 16.0f;
    *out_v1 = 0.875000f;
}

// =============================================================================
// MOTOR DO TECLADO VIRTUAL DO PS2 (OSK)
// =============================================================================
#define OSK_MAX_TEXT_LEN 128
#define OSK_ROWS 5
#define OSK_COLS 12

typedef struct {
    int is_open;
    char text[OSK_MAX_TEXT_LEN];
    int max_len;
    int cursor_row;
    int cursor_col;
    int text_cursor;
    int is_shift;
    int is_caps;
    int cursor_blink;
} PS2_KeyboardEngine;

static PS2_KeyboardEngine g_osk = {
    .is_open = 0,
    .text = "",
    .max_len = 32,
    .cursor_row = 1,
    .cursor_col = 0,
    .is_shift = 0,
    .is_caps = 0,
    .cursor_blink = 0
};

static char s_osk_backup[OSK_MAX_TEXT_LEN] = "";
static int s_osk_cancelled = 0;

static const char g_osk_layout_normal[OSK_ROWS][OSK_COLS] = {
    { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=' },
    { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']' },
    { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\\' },
    { '\x01', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\b' },
    { '\x02', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\x03', '\n' }
};

static const char g_osk_layout_shift[OSK_ROWS][OSK_COLS] = {
    { '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+' },
    { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}' },
    { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '|' },
    { '\x01', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '\b' },
    { '\x02', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\x03', '\n' }
};

void PS2_Keyboard_Open(const char* initial_text, int max_len) {
    g_osk.is_open = 1;
    s_osk_cancelled = 0;
    g_osk.max_len = (max_len > 0 && max_len < OSK_MAX_TEXT_LEN) ? max_len : 32;

    if (initial_text) {
        strncpy(g_osk.text, initial_text, sizeof(g_osk.text) - 1);
        g_osk.text[sizeof(g_osk.text) - 1] = '\0';
        strncpy(s_osk_backup, initial_text, sizeof(s_osk_backup) - 1);
        s_osk_backup[sizeof(s_osk_backup) - 1] = '\0';
    } else {
        g_osk.text[0] = '\0';
        s_osk_backup[0] = '\0';
    }

    g_osk.cursor_row = 1;
    g_osk.cursor_col = 0;
    g_osk.is_shift = 0;
    g_osk.is_caps = 0;
    g_osk.cursor_blink = 0;
    g_osk.text_cursor = (int)strlen(g_osk.text);
}

void PS2_Keyboard_Close(void) {
    g_osk.is_open = 0;
}

int PS2_Keyboard_IsOpen(void) {
    return g_osk.is_open;
}

int PS2_Keyboard_IsCancelled(void) {
    return s_osk_cancelled;
}

const char* PS2_Keyboard_GetText(void) {
    return g_osk.text;
}

void PS2_Keyboard_SetText(const char* text) {
    if (text) {
        strncpy(g_osk.text, text, sizeof(g_osk.text) - 1);
        g_osk.text[sizeof(g_osk.text) - 1] = '\0';
    } else {
        g_osk.text[0] = '\0';
    }
}

void PS2_Keyboard_Input(int up, int down, int left, int right, int btn_cross, int btn_square, int btn_triangle, int btn_start, int btn_circle, int btn_select, int btn_l1, int btn_r1) {
    if (!g_osk.is_open) return;

    if (up) g_osk.cursor_row = (g_osk.cursor_row - 1 + OSK_ROWS) % OSK_ROWS;
    else if (down) g_osk.cursor_row = (g_osk.cursor_row + 1) % OSK_ROWS;

    if (left) g_osk.cursor_col = (g_osk.cursor_col - 1 + OSK_COLS) % OSK_COLS;
    else if (right) g_osk.cursor_col = (g_osk.cursor_col + 1) % OSK_COLS;

    // L1 e R1: Movem o cursor de inserção do texto
    if (btn_l1 && g_osk.text_cursor > 0) {
        g_osk.text_cursor--;
        g_osk.cursor_blink = 0;
        return;
    }
    if (btn_r1 && g_osk.text_cursor < (int)strlen(g_osk.text)) {
        g_osk.text_cursor++;
        g_osk.cursor_blink = 0;
        return;
    }

    // Triângulo: Alterna o Caps Lock (Shift desativado)
    if (btn_triangle) {
        g_osk.is_caps = !g_osk.is_caps;
        return;
    }

    // Select: Limpa todo o texto (Clear)
    if (btn_select) {
        g_osk.text[0] = '\0';
        g_osk.text_cursor = 0;
        return;
    }

    // Bolinha: Cancela e fecha restaurando o texto original
    if (btn_circle) {
        s_osk_cancelled = 1;
        strncpy(g_osk.text, s_osk_backup, sizeof(g_osk.text) - 1);
        g_osk.text[sizeof(g_osk.text) - 1] = '\0';
        g_osk.text_cursor = (int)strlen(g_osk.text);
        PS2_Keyboard_Close();
        return;
    }

    // Start: Salva e fecha o teclado
    if (btn_start) {
        s_osk_cancelled = 0;
        PS2_Keyboard_Close();
        return;
    }

    // Quadrado: Backspace na posição do cursor
    if (btn_square) {
        int len = (int)strlen(g_osk.text);
        if (g_osk.text_cursor > 0 && len > 0) {
            for (int i = g_osk.text_cursor - 1; i < len; i++) {
                g_osk.text[i] = g_osk.text[i + 1];
            }
            g_osk.text_cursor--;
        }
        return;
    }

    // X: Seleciona a tecla atual
    if (btn_cross) {
        int use_shift = g_osk.is_caps;
        char key = use_shift ? g_osk_layout_shift[g_osk.cursor_row][g_osk.cursor_col]
                             : g_osk_layout_normal[g_osk.cursor_row][g_osk.cursor_col];

        if (key == '\x01') {
            // Tecla Shift na grade: sem função
        } else if (key == '\x02') {
            // Tecla Caps na grade
            g_osk.is_caps = !g_osk.is_caps;
        } else if (key == '\b') {
            // Backspace
            int len = (int)strlen(g_osk.text);
            if (g_osk.text_cursor > 0 && len > 0) {
                for (int i = g_osk.text_cursor - 1; i < len; i++) {
                    g_osk.text[i] = g_osk.text[i + 1];
                }
                g_osk.text_cursor--;
            }
        } else if (key == '\x03') {
            // Clear na grade
            g_osk.text[0] = '\0';
            g_osk.text_cursor = 0;
        } else if (key == '\n') {
            // Done na grade: salva e fecha
            s_osk_cancelled = 0;
            PS2_Keyboard_Close();
        } else if (key != '\0') {
            int len = (int)strlen(g_osk.text);
            if (len < g_osk.max_len) {
                for (int i = len; i >= g_osk.text_cursor; i--) {
                    g_osk.text[i + 1] = g_osk.text[i];
                }
                g_osk.text[g_osk.text_cursor] = key;
                g_osk.text_cursor++;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// RENDERIZAÇÃO DO TECLADO COM NOVA LEGENDA DE RODAPÉ
// -----------------------------------------------------------------------------
void PS2_Keyboard_Render(
    void (*draw_quad_layer)(int layer, float x, float y, float w, float h, float u0, float v0, float u1, float v1, unsigned int color),
    void (*draw_text)(float x, float y, const char* str, unsigned int color, float scale)
) {
    if (!g_osk.is_open) return;

    const PS2_ThemeConfig* cfg = &g_theme_config;
    float screen_w = cfg->screen_width;
    float screen_h = cfg->screen_height;

    // 1. Escurecimento do fundo
    if (draw_quad_layer) {
        draw_quad_layer(-1, 0.0f, 0.0f, screen_w, screen_h, 0.0f, 0.0f, 0.0f, 0.0f, 0xF0000000);
    }

    // 2. Janela
    float dialog_w = 530.0f;
    float dialog_h = 250.0f;
    float dialog_x = (screen_w - dialog_w) / 2.0f;
    float dialog_y = (screen_h - dialog_h) / 2.0f;

    if (draw_quad_layer) {
        draw_quad_layer(2, dialog_x, dialog_y, dialog_w, dialog_h,
                        UV_KBD_DIALOG_U0, UV_KBD_DIALOG_V0, UV_KBD_DIALOG_U1, UV_KBD_DIALOG_V1, 0xFFFFFFFF);
    }

    // 3. Barra de Entrada do Texto
    float bar_x = dialog_x + 14.0f;
    float bar_y = dialog_y + 12.0f;
    float bar_w = dialog_w - 28.0f;
    float bar_h = 32.0f;

    if (draw_quad_layer) {
        draw_quad_layer(2, bar_x, bar_y, bar_w, bar_h,
                        UV_KBD_INPUT_U0, UV_KBD_INPUT_V0, UV_KBD_INPUT_U1, UV_KBD_INPUT_V1, 0xFFFFFFFF);
    }

    if (draw_text) {
        g_osk.cursor_blink = (g_osk.cursor_blink + 1) % 40;
        char full_buf[160];
        int len = (int)strlen(g_osk.text);
        int cur_p = g_osk.text_cursor;
        if (cur_p > len) cur_p = len;

        int di = 0;
        for (int i = 0; i <= len; i++) {
            if (i == cur_p) {
                full_buf[di++] = (g_osk.cursor_blink < 20) ? '|' : ' ';
            }
            if (i < len) full_buf[di++] = g_osk.text[i];
        }
        full_buf[di] = '\0';

        int tlen = (int)strlen(full_buf);
        const char *disp_ptr = (tlen > 34) ? (full_buf + tlen - 34) : full_buf;
        draw_text(bar_x + 10.0f, bar_y + 7.0f, disp_ptr, 0xFFFFFFFF, 0.70f);

        // =====================================================================
        // NOVA LEGENDA DO RODAPÉ DA JANELA DO TECLADO
        // =====================================================================
        draw_text(dialog_x + 16.0f, dialog_y + dialog_h - 16.0f,
                  "L1/R1: Cursor | X: Selecionar | O: Cancelar | START: Salvar",
                  0xC0FFFFFF, 0.36f);
    }

    // 4. Grade de Teclas
    float grid_x = dialog_x + 14.0f;
    float grid_y = dialog_y + 52.0f;
    float key_w = 38.0f;
    float key_h = 30.0f;
    float key_pad = 4.0f;

    int use_shift = g_osk.is_caps;

    for (int r = 0; r < OSK_ROWS; r++) {
        for (int c = 0; c < OSK_COLS; c++) {
            float kx = grid_x + (float)c * (key_w + key_pad);
            float ky = grid_y + (float)r * (key_h + key_pad);
            int is_cursor = (r == g_osk.cursor_row && c == g_osk.cursor_col);

            char key = use_shift ? g_osk_layout_shift[r][c] : g_osk_layout_normal[r][c];
            unsigned int key_text_col = is_cursor ? 0xFFFFFFFF : 0xFFF1F5F9;

            // Barra de Espaço
            if (r == 4 && key == ' ') {
                if (c == 1) {
                    float space_w = (key_w * 9.0f) + (key_pad * 8.0f);
                    int is_space_sel = (g_osk.cursor_row == 4 && g_osk.cursor_col >= 1 && g_osk.cursor_col <= 9);
                    float ku0 = is_space_sel ? UV_KBD_SPACE_SELECTED_U0 : UV_KBD_SPACE_NORMAL_U0;
                    float kv0 = is_space_sel ? UV_KBD_SPACE_SELECTED_V0 : UV_KBD_SPACE_NORMAL_V0;
                    float ku1 = is_space_sel ? UV_KBD_SPACE_SELECTED_U1 : UV_KBD_SPACE_NORMAL_U1;
                    float kv1 = is_space_sel ? UV_KBD_SPACE_SELECTED_V1 : UV_KBD_SPACE_NORMAL_V1;

                    if (draw_quad_layer) {
                        draw_quad_layer(2, kx, ky, space_w, key_h, ku0, kv0, ku1, kv1, 0xFFFFFFFF);
                    }
                    if (draw_text) {
                        draw_text(kx + (space_w - 48.0f) / 2.0f, ky + 8.0f, "ESPACO", is_space_sel ? 0xFFFFFFFF : 0xFFF1F5F9, 0.44f);
                    }
                }
                continue;
            }

            if (draw_quad_layer) {
                float ku0 = UV_KBD_KEY_NORMAL_U0, kv0 = UV_KBD_KEY_NORMAL_V0;
                float ku1 = UV_KBD_KEY_NORMAL_U1, kv1 = UV_KBD_KEY_NORMAL_V1;

                if (is_cursor) {
                    ku0 = UV_KBD_KEY_SELECTED_U0; kv0 = UV_KBD_KEY_SELECTED_V0;
                    ku1 = UV_KBD_KEY_SELECTED_U1; kv1 = UV_KBD_KEY_SELECTED_V1;
                } else if (key == '\x02' && g_osk.is_caps) {
                    ku0 = UV_KBD_KEY_ACTIVE_U0; kv0 = UV_KBD_KEY_ACTIVE_V0;
                    ku1 = UV_KBD_KEY_ACTIVE_U1; kv1 = UV_KBD_KEY_ACTIVE_V1;
                }

                draw_quad_layer(2, kx, ky, key_w, key_h, ku0, kv0, ku1, kv1, 0xFFFFFFFF);

                float u0, v0, u1, v1;
                if (key == '\x02') { // Caps Lock -> Ícone de Triângulo
                    PS2_Button_GetUV("triangle", &u0, &v0, &u1, &v1);
                    draw_quad_layer(1, kx + (key_w - 11.0f) / 2.0f, ky + 2.0f, 11.0f, 11.0f, u0, v0, u1, v1, 0xFFFFFFFF);
                } else if (key == '\b') {
                    PS2_Button_GetUV("square", &u0, &v0, &u1, &v1);
                    draw_quad_layer(1, kx + (key_w - 11.0f) / 2.0f, ky + 2.0f, 11.0f, 11.0f, u0, v0, u1, v1, 0xFFFFFFFF);
                } else if (key == '\x03') { // Clear -> Ícone de Select
                    PS2_Button_GetUV("select", &u0, &v0, &u1, &v1);
                    draw_quad_layer(1, kx + (key_w - 14.0f) / 2.0f, ky + 3.0f, 14.0f, 9.0f, u0, v0, u1, v1, 0xFFFFFFFF);
                } else if (key == '\n') {
                    PS2_Button_GetUV("start", &u0, &v0, &u1, &v1);
                    draw_quad_layer(1, kx + (key_w - 14.0f) / 2.0f, ky + 3.0f, 14.0f, 9.0f, u0, v0, u1, v1, 0xFFFFFFFF);
                }
            }

            if (draw_text) {
                if (key == '\x01') draw_text(kx + (key_w - 18.0f) / 2.0f, ky + 10.0f, "---", key_text_col, 0.32f);
                else if (key == '\x02') draw_text(kx + (key_w - 18.0f) / 2.0f, ky + 15.0f, "CAPS", key_text_col, 0.32f);
                else if (key == '\b') draw_text(kx + (key_w - 18.0f) / 2.0f, ky + 15.0f, "BKSP", key_text_col, 0.32f);
                else if (key == '\x03') draw_text(kx + (key_w - 22.0f) / 2.0f, ky + 15.0f, "CLEAR", key_text_col, 0.30f);
                else if (key == '\n') draw_text(kx + (key_w - 18.0f) / 2.0f, ky + 15.0f, "DONE", key_text_col, 0.32f);
                else {
                    char str_buf[2] = { key, '\0' };
                    draw_text(kx + (key_w - 10.0f) / 2.0f, ky + 7.0f, str_buf, key_text_col, 0.52f);
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
// RENDERIZADOR PRINCIPAL DO LAUNCHER
// -----------------------------------------------------------------------------
void PS2_Launcher_RenderEx(
    void (*draw_quad_layer)(int layer, float x, float y, float w, float h, float u0, float v0, float u1, float v1, unsigned int color),
    void (*draw_text)(float x, float y, const char* str, unsigned int color, float scale)
) {
    const PS2_ThemeConfig* cfg = &g_theme_config;

    // 1. Logo (Camada 0 = SPRITE_ATLAS)
    if (g_launcher_config.show_logo && cfg->title_enabled && draw_quad_layer) {
        draw_quad_layer(0, cfg->title_x, cfg->title_y, cfg->title_width, cfg->title_height,
                        cfg->uv_title_u0, cfg->uv_title_v0, cfg->uv_title_u1, cfg->uv_title_v1, 0xFFFFFFFF);
    }

    // 2. Estado de Lista Vazia
    if (g_launcher_scroll.total_items <= 0 || g_launcher_scroll.items == NULL) {
        if (draw_text) {
            const char* msg = g_launcher_config.empty_msg ? g_launcher_config.empty_msg : "NENHUM JOGO ENCONTRADO";
            float scale_title = 0.95f;
            float title_w = (float)strlen(msg) * (cfg->font_glyph_advance * scale_title);
            float title_x = (cfg->screen_width - title_w) / 2.0f;
            float center_y = (cfg->screen_height / 2.0f) - 16.0f;
            draw_text(title_x, center_y, msg, cfg->color_empty_state, scale_title);

            const char* sub = g_launcher_config.empty_sub ? g_launcher_config.empty_sub : "PRESSIONE START PARA RECARREGAR";
            if (sub && sub[0] != '\0') {
                float scale_sub = 0.58f;
                float sub_w = (float)strlen(sub) * (cfg->font_glyph_advance * scale_sub);
                float sub_x = (cfg->screen_width - sub_w) / 2.0f;
                draw_text(sub_x, center_y + 28.0f, sub, cfg->color_subtitle, scale_sub);
            }
        }
    } else {
        // 3. Barra de Rolagem (Camada 0 = SPRITE_ATLAS)
        if (g_launcher_config.show_scrollbar && (g_launcher_scroll.total_items > g_launcher_scroll.visible_count)) {
            if (draw_quad_layer) {
                draw_quad_layer(0, cfg->scrollbar_track_x, cfg->scrollbar_track_y, cfg->scrollbar_width, cfg->scrollbar_track_height,
                                cfg->uv_scroll_track_u0, cfg->uv_scroll_track_v0, cfg->uv_scroll_track_u1, cfg->uv_scroll_track_v1, cfg->color_scrollbar_track);

                float thumb_y, thumb_h;
                PS2_Launcher_GetScrollbarThumb(&thumb_y, &thumb_h);
                draw_quad_layer(0, cfg->scrollbar_track_x, thumb_y, cfg->scrollbar_width, thumb_h,
                                cfg->uv_scroll_thumb_u0, cfg->uv_scroll_thumb_v0, cfg->uv_scroll_thumb_u1, cfg->uv_scroll_thumb_v1, cfg->color_scrollbar_thumb);
            }
        }

        // 4. Renderiza Itens Visíveis
        int start = g_launcher_scroll.scroll_offset;
        int slots = g_launcher_scroll.visible_count;
        if (slots > g_launcher_scroll.total_items - start) {
            slots = g_launcher_scroll.total_items - start;
        }

        int show_num = g_settings_mode ? 0 : g_launcher_config.show_numbers;
        int show_sub = g_settings_mode ? 1 : g_launcher_config.show_subtitles;
        int show_tag = g_settings_mode ? 1 : g_launcher_config.show_tags;

        for (int slot = 0; slot < slots; slot++) {
            int item_idx = start + slot;
            if (item_idx >= g_launcher_scroll.total_items) break;

            LauncherItem* item = &g_launcher_scroll.items[item_idx];
            int is_selected = (item_idx == g_launcher_scroll.selected_index);
            float item_x = cfg->item_start_x;
            float item_y = cfg->item_start_y + ((float)slot * cfg->item_step_y);

            // 4.1. Placa do Botão (Camada 0 = SPRITE_ATLAS)
            if (draw_quad_layer) {
                float u0, v0, u1, v1;
                if (is_selected) {
                    u0 = cfg->uv_plate_selected_u0;
                    v0 = cfg->uv_plate_selected_v0;
                    u1 = cfg->uv_plate_selected_u1;
                    v1 = cfg->uv_plate_selected_v1;
                } else {
                    v0 = (cfg->uv_plate_normal_v0 >= 0.22f) ? cfg->uv_plate_normal_v0 : 0.250000f;
                    v1 = (cfg->uv_plate_normal_v1 >= 0.35f) ? cfg->uv_plate_normal_v1 : 0.414063f;
                    u0 = (cfg->uv_plate_normal_u0 > 0.0f)   ? cfg->uv_plate_normal_u0 : 0.015625f;
                    u1 = (cfg->uv_plate_normal_u1 > 0.5f)   ? cfg->uv_plate_normal_u1 : 0.875000f;
                }
                draw_quad_layer(0, item_x, item_y, cfg->item_width, cfg->item_height, u0, v0, u1, v1, 0xFFFFFFFF);
            }

            int has_sub = show_sub && (item->subtitle[0] != '\0');
            float title_x = item_x + (show_num ? cfg->item_title_num_offset_x : cfg->item_title_no_num_offset_x);
            float badge_x = item_x + cfg->item_tag_badge_offset_x;
            int has_tag = show_tag && (item->tag[0] != '\0');

            float max_title_w = has_tag ? (badge_x - title_x - 10.0f) : (item_x + cfg->item_width - title_x - 16.0f);

		// Variável estática para animação do Marquee (scroll suave)
        static int s_marquee_timer = 0;
        s_marquee_timer++;

        for (int slot = 0; slot < slots; slot++) {
            int item_idx = start + slot;
            if (item_idx >= g_launcher_scroll.total_items) break;

            LauncherItem* item = &g_launcher_scroll.items[item_idx];
            int is_selected = (item_idx == g_launcher_scroll.selected_index);
            float item_x = cfg->item_start_x;
            float item_y = cfg->item_start_y + ((float)slot * cfg->item_step_y);

            // 4.1. Placa de Fundo
            if (draw_quad_layer) {
                float u0, v0, u1, v1;
                if (is_selected) {
                    u0 = cfg->uv_plate_selected_u0; v0 = cfg->uv_plate_selected_v0;
                    u1 = cfg->uv_plate_selected_u1; v1 = cfg->uv_plate_selected_v1;
                } else {
                    u0 = cfg->uv_plate_normal_u0;   v0 = cfg->uv_plate_normal_v0;
                    u1 = cfg->uv_plate_normal_u1;   v1 = cfg->uv_plate_normal_v1;
                }
                draw_quad_layer(0, item_x, item_y, cfg->item_width, cfg->item_height, u0, v0, u1, v1, 0xFFFFFFFF);
            }

            int has_sub = show_sub && (item->subtitle[0] != '\0');
            float title_x = item_x + (show_num ? cfg->item_title_num_offset_x : cfg->item_title_no_num_offset_x);
            float badge_x = item_x + cfg->item_tag_badge_offset_x;
            int has_tag = show_tag && (item->tag[0] != '\0');

            // Largura útil máxima para não invadir a Badge
            float max_title_w = has_tag ? (badge_x - title_x - 12.0f) : (item_x + cfg->item_width - title_x - 16.0f);

            // 4.2. Numeração 01. 02.
            if (show_num && draw_text) {
                char num_str[16];
                snprintf(num_str, sizeof(num_str), "%02d.", item_idx + 1);
                unsigned int num_col = is_selected ? cfg->color_text_selected : cfg->color_subtitle;
                float num_y = item_y + (has_sub ? 4.0f : ((cfg->item_height - (cfg->font_char_height * cfg->item_index_scale)) / 2.0f));
                draw_text(item_x + cfg->item_index_offset_x, num_y, num_str, num_col, cfg->item_index_scale);
            }

			// =================================================================
            // 4.3. Título & Subtítulo (Marquee Refinado + Alinhamento Visual)
            // =================================================================
            if (draw_text && item->title[0] != '\0') {
                unsigned int title_col = is_selected ? cfg->color_text_selected : cfg->color_text_normal;
                float title_scale = has_sub ? cfg->item_title_sub_scale : cfg->item_title_single_scale;
                float adv_unit = (cfg->font_glyph_advance + cfg->font_letter_spacing) * title_scale;
                
                // Centralização vertical automática quando não há subtítulo
                float title_y = has_sub ? (item_y + cfg->item_title_sub_offset_y) 
                                        : (item_y + (cfg->item_height - (cfg->font_char_height * title_scale)) / 2.0f);

                int max_visible_chars = (int)(max_title_w / adv_unit);
                int title_len = (int)strlen(item->title);

                if (title_len > max_visible_chars && max_visible_chars > 3) {
                    if (is_selected) {
                        // Marquee Suave e Legível a 60 FPS:
                        int overflow = title_len - max_visible_chars;
                        int pause_start = 280; //  pausa para ler o começo
                        int pause_end   = 280; //  de pausa no final
                        int frames_per_char = 60; // Velocidade confortável de leitura

                        int scroll_total_frames = overflow * frames_per_char;
                        int cycle = pause_start + scroll_total_frames + pause_end;
                        int t = s_marquee_timer % cycle;

                        int char_offset = 0;
                        if (t >= pause_start && t < (pause_start + scroll_total_frames)) {
                            char_offset = (t - pause_start) / frames_per_char;
                        } else if (t >= (pause_start + scroll_total_frames)) {
                            char_offset = overflow;
                        }

                        char marquee_buf[64];
                        strncpy(marquee_buf, item->title + char_offset, max_visible_chars);
                        marquee_buf[max_visible_chars] = '\0';
                        draw_text(title_x, title_y, marquee_buf, title_col, title_scale);
                    } else {
                        // Não selecionado: trunca com "..."
                        char trunc_buf[64];
                        strncpy(trunc_buf, item->title, max_visible_chars - 3);
                        trunc_buf[max_visible_chars - 3] = '\0';
                        strcat(trunc_buf, "...");
                        draw_text(title_x, title_y, trunc_buf, title_col, title_scale);
                    }
                } else {
                    draw_text(title_x, title_y, item->title, title_col, title_scale);
                }

                // Subtítulo: compensação de +3.0px para igualar o alinhamento visual com o título
                if (has_sub) {
                    float sub_x = title_x + 3.0f; 
                    float sub_adv = (cfg->font_glyph_advance + cfg->font_letter_spacing) * cfg->item_subtitle_scale;
                    int max_sub_chars = (int)((max_title_w - 3.0f) / sub_adv);
                    int sub_len = (int)strlen(item->subtitle);

                    if (sub_len > max_sub_chars && max_sub_chars > 3) {
                        char sub_trunc[64];
                        strncpy(sub_trunc, item->subtitle, max_sub_chars - 3);
                        sub_trunc[max_sub_chars - 3] = '\0';
                        strcat(sub_trunc, "...");
                        draw_text(sub_x, item_y + cfg->item_subtitle_offset_y, sub_trunc, cfg->color_subtitle, cfg->item_subtitle_scale);
                    } else {
                        draw_text(sub_x, item_y + cfg->item_subtitle_offset_y, item->subtitle, cfg->color_subtitle, cfg->item_subtitle_scale);
                    }
                }
            }

            // =================================================================
            // 4.4. Badge / Tag com Proteção Total de Limites (Nunca Vaza)
            // =================================================================
            if (has_tag) {
                float badge_y = item_y + cfg->item_tag_badge_offset_y;
                if (draw_quad_layer) {
                    draw_quad_layer(0, badge_x, badge_y, cfg->item_tag_badge_w, cfg->item_tag_badge_h,
                                    cfg->uv_tag_badge_u0, cfg->uv_tag_badge_v0, cfg->uv_tag_badge_u1, cfg->uv_tag_badge_v1, 0xFFFFFFFF);
                }
                if (draw_text) {
                    unsigned int tag_col = is_selected ? cfg->color_text_selected : cfg->color_tag_text;
                    float tag_scale = 0.46f;
                    float base_adv = (cfg->font_glyph_advance + cfg->font_letter_spacing);
                    
                    // Espaço útil dentro do badge (deixando 8px de margem interna nas pontas)
                    float usable_w = cfg->item_tag_badge_w - 16.0f;
                    if (usable_w < 10.0f) usable_w = 10.0f;

                    char tag_display[16];
                    strncpy(tag_display, item->tag, sizeof(tag_display) - 1);
                    tag_display[sizeof(tag_display) - 1] = '\0';
                    int tag_len = (int)strlen(tag_display);

                    // 1. Se passar do espaço útil, reduz a escala suavemente (Auto-fit)
                    float current_w = (float)tag_len * (base_adv * tag_scale);
                    if (current_w > usable_w && current_w > 0.0f) {
                        tag_scale = tag_scale * (usable_w / current_w);
                        if (tag_scale < 0.32f) tag_scale = 0.32f; // Limite mínimo para não ficar ilegível
                    }

                    // 2. Se mesmo com escala reduzida ainda for grande demais, trunca
                    float final_adv = base_adv * tag_scale;
                    int max_chars_fit = (int)(usable_w / final_adv);
                    if (max_chars_fit < 1) max_chars_fit = 1;
                    if (tag_len > max_chars_fit) {
                        tag_display[max_chars_fit] = '\0';
                        tag_len = max_chars_fit;
                    }

                    // 3. Centralização matemática absoluta (X e Y) dentro do badge
                    float real_tag_w = (float)tag_len * final_adv;
                    float tag_x = badge_x + ((cfg->item_tag_badge_w - real_tag_w) / 2.0f);
                    float tag_y = badge_y + ((cfg->item_tag_badge_h - (cfg->font_char_height * tag_scale)) / 2.0f);

                    draw_text(tag_x, tag_y, tag_display, tag_col, tag_scale);
                }
            }
		}
    }
}

 // 5. Barra de Rodapé (Ações do ELF)
    if (g_launcher_config.show_footer && cfg->footer_enabled && cfg->footer_prompts_count > 0) {
        if (draw_quad_layer) {
            // Fundo transparente (layer = -1 para desenho plano sem textura)
            draw_quad_layer(-1, 0.0f, FOOTER_Y, cfg->screen_width, FOOTER_HEIGHT, 0.0f, 0.0f, 0.0f, 0.0f, cfg->color_footer_bg);
        }

        float prompt_scale = 0.50f;
        float font_adv = cfg->font_glyph_advance * prompt_scale;
        float btn_size = 18.0f;
        float btn_y = FOOTER_Y + (FOOTER_HEIGHT - btn_size) / 2.0f;
        float text_y = FOOTER_Y + 5.0f;

        // 1. Calcula a largura total apenas com os botões ativos para centralizar perfeitamente
        float total_footer_w = 0.0f;
        for (int i = 0; i < cfg->footer_prompts_count && i < 8; i++) {
            if (cfg->footer_prompts[i].button[0] == '\0' || cfg->footer_prompts[i].text[0] == '\0') {
                continue;
            }

            total_footer_w += btn_size + 6.0f + ((float)strlen(cfg->footer_prompts[i].text) * font_adv);

            // Só contabiliza espaço de divisor se ainda houver outro botão ativo à frente
            int has_next = 0;
            for (int n = i + 1; n < cfg->footer_prompts_count && n < 8; n++) {
                if (cfg->footer_prompts[n].button[0] != '\0' && cfg->footer_prompts[n].text[0] != '\0') {
                    has_next = 1;
                    break;
                }
            }
            if (has_next) {
                total_footer_w += cfg->footer_show_dividers ? 20.0f : 18.0f;
            }
        }

        float cur_x = (cfg->screen_width - total_footer_w) / 2.0f;
        if (cur_x < 10.0f) cur_x = 10.0f;

        // 2. Desenha os ícones, textos e divisores (nunca desenha divisor no último item)
        for (int i = 0; i < cfg->footer_prompts_count && i < 8; i++) {
            if (cfg->footer_prompts[i].button[0] == '\0' || cfg->footer_prompts[i].text[0] == '\0') {
                continue;
            }

            float u0, v0, u1, v1;
            PS2_Button_GetUV(cfg->footer_prompts[i].button, &u0, &v0, &u1, &v1);

            if (draw_quad_layer) {
                // Ícone do botão puxado da Linha 6 do FONT_ATLAS (layer = 1)
                draw_quad_layer(1, cur_x, btn_y, btn_size, btn_size, u0, v0, u1, v1, 0xFFFFFFFF);
            }
            cur_x += btn_size + 6.0f;

            if (draw_text && cfg->footer_prompts[i].text[0] != '\0') {
                draw_text(cur_x, text_y, cfg->footer_prompts[i].text, cfg->color_footer_text, prompt_scale);
                cur_x += (float)strlen(cfg->footer_prompts[i].text) * font_adv;
            }

            // Checa se há outro botão preenchido após este antes de desenhar a barra divisória
            int has_next = 0;
            for (int n = i + 1; n < cfg->footer_prompts_count && n < 8; n++) {
                if (cfg->footer_prompts[n].button[0] != '\0' && cfg->footer_prompts[n].text[0] != '\0') {
                    has_next = 1;
                    break;
                }
            }

            if (cfg->footer_show_dividers && has_next) {
                cur_x += 10.0f;
                if (draw_quad_layer) {
                    draw_quad_layer(-1, cur_x, FOOTER_Y + 6.0f, 1.0f, FOOTER_HEIGHT - 12.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0x30FFFFFF);
                }
                cur_x += 10.0f;
            } else if (has_next) {
                cur_x += 18.0f;
            }
        }
    }

    // 6. Teclado Virtual (Se aberto via PS2_Keyboard_Open)
    if (g_osk.is_open) {
        PS2_Keyboard_Render(draw_quad_layer, draw_text);
    }
}

void PS2_Launcher_Render(
    void (*draw_quad)(float x, float y, float w, float h, float u0, float v0, float u1, float v1, unsigned int color),
    void (*draw_text)(float x, float y, const char* str, unsigned int color, float scale)
) {
    (void)draw_quad;
    (void)draw_text;
}