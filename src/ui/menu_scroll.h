/**
 * PS2 LAUNCHER ENGINE - MOTOR DE ROLAGEM & CONTROLE (HEADER)
 * Modificado para inclusão da barra de rodapé com atlas de botões.
 * Protótipos exportados para o main.c do ELF.
 */

#ifndef MENU_SCROLL_H
#define MENU_SCROLL_H

#include "menu_assets.h"

#ifdef __cplusplus
extern "C" {
#endif

// Protótipos exportados para o main.c
int  PS2_Launcher_LoadLauncherConfig(const char* filepath);
int  PS2_Launcher_GetSelectedIndex(void);
int  PS2_Launcher_GetCustomBackground(void);
void PS2_Launcher_SetCustomBackground(int enabled);
void PS2_Launcher_SetSettingsMode(int enabled);
int  PS2_Launcher_SaveLauncherConfig(const char* filepath);

// Funções da Barra de Rodapé (Demonstrativa / Prompts de Ação do ELF)
int  PS2_Launcher_GetFooterEnabled(void);
void PS2_Launcher_SetFooterEnabled(int enabled);
int  PS2_Launcher_GetFooterPromptsCount(void);
void PS2_Launcher_SetFooterPrompt(int index, const char* button, const char* text);

// Motor de rolagem dinâmico e gestão de itens
void PS2_Launcher_ScrollInit(LauncherItem* items, int total_items);
void PS2_Launcher_SetItems(LauncherItem* items, int total_items);
LauncherItem* PS2_Launcher_GetSelectedItem(void);
void PS2_Launcher_ScrollInput(int up, int down, int page_up, int page_down);
void PS2_Launcher_GetScrollbarThumb(float* out_thumb_y, float* out_thumb_h);

// Carregamento de tema em tempo de execução (.cfg)
int  PS2_Launcher_LoadThemeConfig(const char* filepath);
const PS2_ThemeConfig* PS2_Launcher_GetThemeConfig(void);
void PS2_Launcher_SetThemeConfig(const PS2_ThemeConfig* theme);

// Renderização completa (menu, lista, logo, scrollbar e barra de rodapé com atlas de botões)
void PS2_Launcher_Render(
    void (*draw_quad)(float x, float y, float w, float h, float u0, float v0, float u1, float v1, unsigned int color),
    void (*draw_text)(float x, float y, const char* str, unsigned int color, float scale)
);

// Teclado Virtual do PS2 (On-Screen Keyboard - OSK)
void PS2_Keyboard_Open(const char* initial_text, int max_len);
void PS2_Keyboard_Close(void);
int  PS2_Keyboard_IsOpen(void);
const char* PS2_Keyboard_GetText(void);
void PS2_Keyboard_SetText(const char* text);
void PS2_Keyboard_Input(int up, int down, int left, int right, int btn_cross, int btn_square, int btn_triangle, int btn_start, int btn_circle, int btn_select, int btn_l1, int btn_r1);
void PS2_Keyboard_Render(
    void (*draw_quad_layer)(int layer, float x, float y, float w, float h, float u0, float v0, float u1, float v1, unsigned int color),
    void (*draw_text)(float x, float y, const char* str, unsigned int color, float scale)
);

void PS2_Launcher_RenderEx(
    void (*draw_quad_layer)(int layer, float x, float y, float w, float h, float u0, float v0, float u1, float v1, unsigned int color),
    void (*draw_text)(float x, float y, const char* str, unsigned int color, float scale)
);

const char* PS2_Launcher_GetExitPath(void);
void        PS2_Launcher_SetExitPath(const char* path);

#ifdef __cplusplus
}
#endif

#endif /* MENU_SCROLL_H */
