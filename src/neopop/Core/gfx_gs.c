#include "neopop.h"
#include "mem.h"
#include <gsKit.h>
#include <dmaKit.h>

// 512 tiles de 8x8 pixels dispostos em uma textura de 128x256 pixels
#define ATLAS_W 128
#define ATLAS_H 256

static GSTEXTURE s_tile_atlas;
static u8 s_atlas_buffer[ATLAS_W * ATLAS_H]; // 8bpp buffer intermediário
static u32 s_atlas_vram = 0;
static int s_atlas_init = 0;

// Inicializa a textura do Atlas na VRAM do PS2
void gfx_gs_init(GSGLOBAL *gsGlobal) {
    if (s_atlas_init) return;

    memset(&s_tile_atlas, 0, sizeof(GSTEXTURE));
    s_tile_atlas.Width = ATLAS_W;
    s_tile_atlas.Height = ATLAS_H;
    s_tile_atlas.PSM = GS_PSM_T8; // 8 bits por pixel indexado (paletizado)
    s_tile_atlas.ClutPSM = GS_PSM_CT16; // Paleta de 16-bit
    s_tile_atlas.Filter = GS_FILTER_NEAREST;
    s_tile_atlas.Mem = s_atlas_buffer;

    s_atlas_vram = gsKit_vram_alloc(gsGlobal,
        gsKit_texture_size(s_tile_atlas.Width, s_tile_atlas.Height, s_tile_atlas.PSM),
        GSKIT_ALLOC_USERBUFFER);
    s_tile_atlas.Vram = s_atlas_vram;

    s_atlas_init = 1;
}

// Decodifica a Tile RAM de 2bpp do NGPC para a textura 8bpp do PS2
static void update_tile_atlas(GSGLOBAL *gsGlobal) {
    // 512 tiles: 16 colunas x 32 linhas de tiles
    for (int t = 0; t < 512; t++) {
        int tile_x = (t % 16) * 8;
        int tile_y = (t / 16) * 8;
        _u16* src_tile = (_u16*)(ram + 0xA000 + (t << 4));

        for (int y = 0; y < 8; y++) {
            _u16 data = src_tile[y];
            u8* dest_row = &s_atlas_buffer[(tile_y + y) * ATLAS_W + tile_x];

            dest_row[0] = (data >> 14) & 3;
            dest_row[1] = (data >> 12) & 3;
            dest_row[2] = (data >> 10) & 3;
            dest_row[3] = (data >>  8) & 3;
            dest_row[4] = (data >>  6) & 3;
            dest_row[5] = (data >>  4) & 3;
            dest_row[6] = (data >>  2) & 3;
            dest_row[7] = (data      ) & 3;
        }
    }
    // Faz o upload do atlas completo para a VRAM (rápido via DMA)
    gsKit_texture_upload(gsGlobal, &s_tile_atlas);
}

// Desenha um tile individual na tela usando primitivas do GS
static inline void draw_tile_quad(GSGLOBAL *gsGlobal, float dest_x, float dest_y, 
                                  _u16 tile_data, int flip_x, int flip_y, float scale) {
    int tile_id = tile_data & 0x01FF;
    float u0 = (float)((tile_id % 16) * 8);
    float v0 = (float)((tile_id / 16) * 8);
    float u1 = u0 + 8.0f;
    float v1 = v0 + 8.0f;

    if (flip_x) { float temp = u0; u0 = u1; u1 = temp; }
    if (flip_y) { float temp = v0; v0 = v1; v1 = temp; }

    gsKit_prim_sprite_texture(gsGlobal, &s_tile_atlas,
        dest_x, dest_y, u0, v0,
        dest_x + (8.0f * scale), dest_y + (8.0f * scale), u1, v1,
        1, GS_SETREG_RGBAQ(128, 128, 128, 0x80, 0));
}