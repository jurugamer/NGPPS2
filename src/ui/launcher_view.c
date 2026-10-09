#include "launcher_view.h"
#include "menu_assets.h"
#include "menu_scroll.h"
#include "core/file_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <kernel.h>

typedef struct {
    char FileId[4];
    u8 FormatVersion, FormatId;
    u16 Pictures;
    u8 Reserved[8];
} __attribute__((packed)) TIM2_FILEHEADER;

typedef struct {
    u32 TotalSize, ClutSize, ImageSize;
    u16 HeaderSize, ClutColors;
    u8 PictFormat, MipMapTextures, ClutType, ImageType;
    u16 ImageWidth, ImageHeight;
    u64 GsTex0, GsTex1;
    u32 GsRegs, GsTexClut;
} __attribute__((packed)) TIM2_PICTUREHEADER;

static GSTEXTURE s_bg_tex;
static GSTEXTURE s_custom_bg_tex;
static GSTEXTURE s_font_tex;
static GSTEXTURE s_kbd_tex;

static u8 *s_bg_buf = NULL;
static u8 *s_font_buf = NULL;
static u8 *s_kbd_buf = NULL;
static u8 *s_custom_bg_buf = NULL;

// As 3 texturas na memória RAM do EE:
static GSTEXTURE s_tex_atlas_main;
static GSTEXTURE s_tex_atlas_config;
static GSTEXTURE s_tex_atlas_game;

static u8 *s_atlas_main_buf = NULL;
static u8 *s_atlas_config_buf = NULL;
static u8 *s_atlas_game_buf = NULL;

// Endereço único de VRAM compartilhado para os 3 atlas:
static u32 s_atlas_vram_addr = 0;
static MenuAtlasType s_current_atlas_type = ATLAS_MAIN;
static GSTEXTURE *s_active_spritesheet = NULL;

static int s_has_custom_bg = 0;
static GSGLOBAL *s_gsGlobal = NULL;

// -----------------------------------------------------------------------------
// DIFUSÃO DE ERRO FLOYD-STEINBERG PARA O DEGRADÊ 16-BIT (0% custo em tempo real)
// -----------------------------------------------------------------------------
static void Smooth16BitBackground(u16 *pixels, int w, int h) {
    if (!pixels || w <= 2 || h <= 2) return;

    int *err_curr_r = (int*)calloc(w + 2, sizeof(int));
    int *err_next_r = (int*)calloc(w + 2, sizeof(int));
    int *err_curr_g = (int*)calloc(w + 2, sizeof(int));
    int *err_next_g = (int*)calloc(w + 2, sizeof(int));
    int *err_curr_b = (int*)calloc(w + 2, sizeof(int));
    int *err_next_b = (int*)calloc(w + 2, sizeof(int));

    if (!err_curr_r || !err_next_r || !err_curr_g || !err_next_g || !err_curr_b || !err_next_b) {
        free(err_curr_r); free(err_next_r);
        free(err_curr_g); free(err_next_g);
        free(err_curr_b); free(err_next_b);
        return;
    }

    for (int y = 0; y < h; y++) {
        int row = y * w;
        memset(err_next_r, 0, (w + 2) * sizeof(int));
        memset(err_next_g, 0, (w + 2) * sizeof(int));
        memset(err_next_b, 0, (w + 2) * sizeof(int));

        for (int x = 0; x < w; x++) {
            u16 c = pixels[row + x];

            int r8 = (((c & 0x001F) * 255) / 31) + err_curr_r[x + 1];
            int g8 = ((((c >> 5) & 0x001F) * 255) / 31) + err_curr_g[x + 1];
            int b8 = ((((c >> 10) & 0x001F) * 255) / 31) + err_curr_b[x + 1];

            if (r8 < 0) r8 = 0; else if (r8 > 255) r8 = 255;
            if (g8 < 0) g8 = 0; else if (g8 > 255) g8 = 255;
            if (b8 < 0) b8 = 0; else if (b8 > 255) b8 = 255;

            int r5 = (r8 * 31 + 127) / 255;
            int g5 = (g8 * 31 + 127) / 255;
            int b5 = (b8 * 31 + 127) / 255;

            int er = r8 - ((r5 * 255) / 31);
            int eg = g8 - ((g5 * 255) / 31);
            int eb = b8 - ((b5 * 255) / 31);

            pixels[row + x] = (c & 0x8000) | (b5 << 10) | (g5 << 5) | r5;

            err_curr_r[x + 2] += (er * 7) / 16;
            err_next_r[x]     += (er * 3) / 16;
            err_next_r[x + 1] += (er * 5) / 16;
            err_next_r[x + 2] += (er * 1) / 16;

            err_curr_g[x + 2] += (eg * 7) / 16;
            err_next_g[x]     += (eg * 3) / 16;
            err_next_g[x + 1] += (eg * 5) / 16;
            err_next_g[x + 2] += (eg * 1) / 16;

            err_curr_b[x + 2] += (eb * 7) / 16;
            err_next_b[x]     += (eb * 3) / 16;
            err_next_b[x + 1] += (eb * 5) / 16;
            err_next_b[x + 2] += (eb * 1) / 16;
        }

        memcpy(err_curr_r, err_next_r, (w + 2) * sizeof(int));
        memcpy(err_curr_g, err_next_g, (w + 2) * sizeof(int));
        memcpy(err_curr_b, err_next_b, (w + 2) * sizeof(int));
    }

    free(err_curr_r); free(err_next_r);
    free(err_curr_g); free(err_next_g);
    free(err_curr_b); free(err_next_b);
}

static int LoadTIM2_gsKit(GSGLOBAL *gsGlobal, GSTEXTURE *texture, const char *path, u32 force_vram_addr, u8 **stored_buf, int is_bg) {
    u8 *buf = NULL;
    u32 file_size = 0;

    if (ReadFileToBuffer(path, &buf, &file_size) <= 0 || !buf) return -1;

    TIM2_FILEHEADER *file_hdr = (TIM2_FILEHEADER*)buf;
    if (memcmp(file_hdr->FileId, "TIM2", 4) != 0) {
        free(buf);
        return -1;
    }

    TIM2_PICTUREHEADER *pic = (TIM2_PICTUREHEADER*)(buf + sizeof(TIM2_FILEHEADER));
    texture->Width = pic->ImageWidth;
    texture->Height = pic->ImageHeight;
    texture->Filter = GS_FILTER_LINEAR;
    texture->Delayed = 0;

    u32 total_pixels = pic->ImageWidth * pic->ImageHeight;
    u32 bpp = (total_pixels > 0) ? (pic->ImageSize / total_pixels) : 4;

    texture->PSM = (bpp == 2) ? GS_PSM_CT16 : GS_PSM_CT32;
    texture->TBW = texture->Width / 64;
    if (texture->Width % 64) texture->TBW++;

    texture->Mem = (u32*)((u8*)pic + pic->HeaderSize);
    texture->Clut = NULL;

    if (is_bg && bpp == 2) {
        Smooth16BitBackground((u16*)texture->Mem, texture->Width, texture->Height);
    }

    if (force_vram_addr != 0) {
        texture->Vram = force_vram_addr;
    } else {
        texture->Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(texture->Width, texture->Height, texture->PSM), GSKIT_ALLOC_USERBUFFER);
    }

    SyncDCache(buf, (void*)((u32)buf + file_size));
    gsKit_texture_upload(gsGlobal, texture);

    if (stored_buf) *stored_buf = buf;
    else free(buf);

    return 0;
}

void LauncherView_SetAtlas(GSGLOBAL *gsGlobal, MenuAtlasType type) {
    if (s_current_atlas_type == type && s_active_spritesheet != NULL) return;

    GSTEXTURE *target_tex = NULL;
    switch (type) {
        case ATLAS_MAIN:   target_tex = &s_tex_atlas_main;   break;
        case ATLAS_CONFIG: target_tex = &s_tex_atlas_config; break;
        case ATLAS_GAME:   target_tex = &s_tex_atlas_game;   break;
    }

    if (target_tex) {
        target_tex->Vram = s_atlas_vram_addr;
        gsKit_texture_upload(gsGlobal, target_tex);
        s_current_atlas_type = type;
        s_active_spritesheet = target_tex;
    }
}

int LauncherView_Init(GSGLOBAL *gsGlobal, 
                       const char *bg_path, 
                       const char *atlas_main_path, 
                       const char *atlas_cfg_path, 
                       const char *atlas_game_path, 
                       const char *font_path, 
                       const char *kbd_path) {
    s_gsGlobal = gsGlobal;

    // 1. Carrega o Fundo padrão
    if (LoadTIM2_gsKit(gsGlobal, &s_bg_tex, bg_path, 0, &s_bg_buf, 1) != 0) return -1;

    // 2. Carrega o Atlas Principal e aloca o VRAM compartilhado
    if (LoadTIM2_gsKit(gsGlobal, &s_tex_atlas_main, atlas_main_path, 0, &s_atlas_main_buf, 0) != 0) return -1;
    s_atlas_vram_addr = s_tex_atlas_main.Vram; // Guarda o endereço de VRAM único

    // 3. Carrega os outros 2 Atlas para a RAM apontando para o MESMO VRAM
    LoadTIM2_gsKit(gsGlobal, &s_tex_atlas_config, atlas_cfg_path, s_atlas_vram_addr, &s_atlas_config_buf, 0);
    LoadTIM2_gsKit(gsGlobal, &s_tex_atlas_game, atlas_game_path, s_atlas_vram_addr, &s_atlas_game_buf, 0);

    // Garante que o Atlas Principal está na VRAM no início
    gsKit_texture_upload(gsGlobal, &s_tex_atlas_main);
    s_current_atlas_type = ATLAS_MAIN;
    s_active_spritesheet = &s_tex_atlas_main;

    // 4. Font Atlas (32-bit com Alpha)
    if (LoadTIM2_gsKit(gsGlobal, &s_font_tex, font_path, 0, &s_font_buf, 0) != 0) return -1;

    // 5. Keyboard Atlas (32-bit com Alpha)
    LoadTIM2_gsKit(gsGlobal, &s_kbd_tex, kbd_path, 0, &s_kbd_buf, 0);

    s_has_custom_bg = 0;
    return 0;
}

void LauncherView_ResetBackground(void) {
    s_has_custom_bg = 0;
}

void LauncherView_SetCustomBackground(GSGLOBAL *gsGlobal, const char *tm2_path) {
    if (s_custom_bg_buf) {
        free(s_custom_bg_buf);
        s_custom_bg_buf = NULL;
    }
    s_has_custom_bg = 0;

    if (tm2_path && tm2_path[0] != '\0') {
        if (LoadTIM2_gsKit(gsGlobal, &s_custom_bg_tex, tm2_path, s_custom_bg_tex.Vram, &s_custom_bg_buf, 1) == 0) {
            s_has_custom_bg = 1;
        }
    }
}

static inline u64 MakeGsKitColor(unsigned int color) {
    u32 r = (((color      ) & 0xFF) * 128) / 255;
    u32 g = (((color >>  8) & 0xFF) * 128) / 255;
    u32 b = (((color >> 16) & 0xFF) * 128) / 255;
    u32 a = (((color >> 24) & 0xFF) * 128) / 255;
    return GS_SETREG_RGBAQ(r, g, b, a, 0);
}

static void Callback_DrawQuadLayer(int layer, float x, float y, float w, float h, 
                                   float u0, float v0, float u1, float v1, 
                                   unsigned int color) {
    if (!s_gsGlobal) return;
    u64 gs_color = MakeGsKitColor(color);

    if (layer == -1) {
        gsKit_prim_sprite(s_gsGlobal, x, y, x + w, y + h, 1, gs_color);
        return;
    }

    // Usa o atlas atualmente ativo na VRAM
    GSTEXTURE *tex = s_active_spritesheet ? s_active_spritesheet : &s_tex_atlas_main;
    if (layer == 1) tex = &s_font_tex;
    else if (layer == 2) tex = &s_kbd_tex;

    int u_start = (int)(u0 * (float)tex->Width);
    int v_start = (int)(v0 * (float)tex->Height);
    int u_end   = (int)(u1 * (float)tex->Width);
    int v_end   = (int)(v1 * (float)tex->Height);

    gsKit_prim_sprite_texture(s_gsGlobal, tex,
                              x, y, u_start, v_start,
                              x + w, y + h, u_end, v_end,
                              1, gs_color);
}

static void Callback_DrawText(float x, float y, const char* str, unsigned int color, float scale) {
    if (!s_gsGlobal || !str) return;
    u64 gs_color = MakeGsKitColor(color);

    const PS2_ThemeConfig* cfg = PS2_Launcher_GetThemeConfig();
    float gw = cfg->font_char_width * scale;
    float gh = cfg->font_char_height * scale;
    // Soma font_glyph_advance + font_letter_spacing para espaçamento real
    float advance = (cfg->font_glyph_advance + cfg->font_letter_spacing) * scale;
    float cur_x = x;
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (c == ' ') {
            cur_x += advance;
            continue;
        }

        float u0, v0, u1, v1;
        PS2_Font_GetCharUV(c, &u0, &v0, &u1, &v1);

        int u_start = (int)(u0 * (float)s_font_tex.Width);
        int v_start = (int)(v0 * (float)s_font_tex.Height);
        int u_end   = (int)(u1 * (float)s_font_tex.Width);
        int v_end   = (int)(v1 * (float)s_font_tex.Height);

        gsKit_prim_sprite_texture(s_gsGlobal, &s_font_tex,
                                  cur_x, y, u_start, v_start,
                                  cur_x + gw, y + gh, u_end, v_end,
                                  2, gs_color);
        cur_x += advance;
    }
}

void LauncherView_Draw(GSGLOBAL *gsGlobal) {
    s_gsGlobal = gsGlobal;
    u64 color_white = GS_SETREG_RGBAQ(128, 128, 128, 128, 0);
    const PS2_ThemeConfig* cfg = PS2_Launcher_GetThemeConfig();

    // 1. Fundo desenhado como Quad Texturizado com filtragem bilinear real da GPU
    gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;
    GSTEXTURE *bg = (s_has_custom_bg) ? &s_custom_bg_tex : &s_bg_tex;

    gsKit_prim_quad_texture(gsGlobal, bg,
        0.0f, 0.0f, 0, 0,
        cfg->screen_width, 0.0f, bg->Width, 0,
        0.0f, cfg->screen_height, 0, bg->Height,
        cfg->screen_width, cfg->screen_height, bg->Width, bg->Height,
        0, color_white);

    // 2. Elementos com transparência (Menu + Rodapé + Teclado)
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    PS2_Launcher_RenderEx(Callback_DrawQuadLayer, Callback_DrawText);
}