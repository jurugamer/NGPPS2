//---------------------------------------------------------------------------
// NEOPOP PS2 : AI "Clean Room" Monochrome Rasterizer (Ultra-Fast 2bpp)
// ZERO Z-Buffer, Painter's Algorithm, Scratchpad Padding, Smart Scroll Culling
//---------------------------------------------------------------------------

#include "neopop.h"
#include "mem.h"
#include "gfx.h"
#include <string.h>
#include <stdint.h>

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

extern _u16 s_color_lut[4096];

// Códigos de cor bruta do NGP Mono (0x0RGB):
static const _u16 s_mono_raw_ngp[8] = {
	0x0000, 0x0222, 0x0444, 0x0666, 0x0888, 0x0AAA, 0x0CCC, 0x0EEE
};

// Cache das 24 cores de paleta do modo Mono
static _u16 s_mono_pal_cache[24];

#define SPR_BASE ((_u16*)0x70000000)
#define SPR_CFB  (SPR_BASE + 64) // 64 pixels de margem de segurança nas laterais

//=============================================================================
// DRAWPATTERN CEGO & BRANCHLESS (2bpp com MOVN nativo)
//=============================================================================
static inline void draw_8px_trans(_u16* dest, _u16 data, const _u16* pal) __attribute__((always_inline));
static inline void draw_8px_trans(_u16* dest, _u16 data, const _u16* pal) {
    if (!data) return;
    _u32 p;
    p = (data >> 14) & 3; dest[0] = p ? pal[p] : dest[0];
    p = (data >> 12) & 3; dest[1] = p ? pal[p] : dest[1];
    p = (data >> 10) & 3; dest[2] = p ? pal[p] : dest[2];
    p = (data >>  8) & 3; dest[3] = p ? pal[p] : dest[3];
    p = (data >>  6) & 3; dest[4] = p ? pal[p] : dest[4];
    p = (data >>  4) & 3; dest[5] = p ? pal[p] : dest[5];
    p = (data >>  2) & 3; dest[6] = p ? pal[p] : dest[6];
    p = (data      ) & 3; dest[7] = p ? pal[p] : dest[7];
}

static inline void draw_8px_flip(_u16* dest, _u16 data, const _u16* pal) __attribute__((always_inline));
static inline void draw_8px_flip(_u16* dest, _u16 data, const _u16* pal) {
    if (!data) return;
    _u32 p;
    p = (data >> 14) & 3; dest[7] = p ? pal[p] : dest[7];
    p = (data >> 12) & 3; dest[6] = p ? pal[p] : dest[6];
    p = (data >> 10) & 3; dest[5] = p ? pal[p] : dest[5];
    p = (data >>  8) & 3; dest[4] = p ? pal[p] : dest[4];
    p = (data >>  6) & 3; dest[3] = p ? pal[p] : dest[3];
    p = (data >>  4) & 3; dest[2] = p ? pal[p] : dest[2];
    p = (data >>  2) & 3; dest[1] = p ? pal[p] : dest[1];
    p = (data      ) & 3; dest[0] = p ? pal[p] : dest[0];
}

//=============================================================================
// ESTRUTURA FAST-SPRITE (Mono)
//=============================================================================
typedef struct { 
    _s16 x; _u8 dy, pal_idx, prio, flipy; 
    _u16 tile, mirror; 
} FastSprite;

static FastSprite active_sprites[64];
static int active_count = 0;

static void render_sprites_prio(_u8 prio, const _u16* pal_sprites) {
    for (int i = active_count - 1; i >= 0; i--) {
        FastSprite* s = &active_sprites[i];
        if (s->prio == prio) {
            _u8 tiley = s->flipy ? (7 - s->dy) : s->dy;
            _u16 data = *(_u16*)(ram + 0xA000 + (s->tile << 4) + (tiley << 1));
            const _u16* pal = pal_sprites + s->pal_idx;

            if (s->mirror) draw_8px_flip(SPR_CFB + s->x, data, pal);
            else draw_8px_trans(SPR_CFB + s->x, data, pal);
        }
    }
}

//=============================================================================
// MOTOR DE SCROLL INTELIGENTE (Mono - Apenas 21 tiles por linha)
//=============================================================================
static void render_scroll_mono(const _u16* pal_scroll, _u16 map_base, _u8 scrollx, _u8 scrolly) {
    _u8 line = scanline + scrolly;
    _u8 row = line & 7;
    _u8 row_flipped = 7 - row;

    _u16 line_offset = ((line >> 3) << 5) << 1;
    _u16 *map_ptr = (_u16*)(ram + map_base + line_offset);

    _u8 tx_start = scrollx >> 3;
    _u8 fine_x = scrollx & 7;

    for (int i = 0; i < 21; i++) {
        _s16 sx = (i << 3) - fine_x;
        if (sx >= SCREEN_WIDTH) break;

        _u8 tx = (tx_start + i) & 31;
        _u16 data16 = map_ptr[tx];
        
        _u8 tiley = (data16 & 0x4000) ? row_flipped : row;
        _u16 tile_data = *(_u16*)(ram + 0xA000 + ((data16 & 0x01FF) << 4) + (tiley << 1));
        
        // No modo Mono, o bit 0x2000 seleciona entre paleta 0 ou paleta 1
        const _u16* pal_base = (data16 & 0x2000) ? (pal_scroll + 4) : pal_scroll;

        if (data16 & 0x8000) draw_8px_flip(SPR_CFB + sx, tile_data, pal_base);
        else draw_8px_trans(SPR_CFB + sx, tile_data, pal_base);
    }
}

//=============================================================================
// SCANLINE PRINCIPAL (Mono)
//=============================================================================
void gfx_draw_scanline_mono(void)
{
    scanline = ram[0x8009];
    _u16* real_cfb_scanline = cfb + (scanline * 256);
    uint64_t* spr_cfb64 = (uint64_t*)SPR_CFB;

    // 1. Converte os 8 tons de cinza para o formato do PS2
    _u16 ps2_mono_shades[8];
    for (int i = 0; i < 8; i++) {
        int shade_index = negative ? i : (7 - i);
        ps2_mono_shades[i] = s_color_lut[s_mono_raw_ngp[shade_index]];
    }

    // 2. Cor da Janela / Borda
    _u16 win_color = ps2_mono_shades[oowc & 7];
    uint64_t win64 = ((uint64_t)win_color << 48) | ((uint64_t)win_color << 32) | 
                     ((uint64_t)win_color << 16) |  (uint64_t)win_color;

    // Se estiver fora da área visível vertical:
    if (scanline < winy || scanline >= winy + winh) {
        for (int x = 0; x < (SCREEN_WIDTH / 4); x++) spr_cfb64[x] = win64;
        memcpy(real_cfb_scanline, SPR_CFB, SCREEN_WIDTH * sizeof(_u16));
        return;
    }

    // 3. Cor de Fundo Nativa
    _u16 bg_color;
    if ((bgc & 0xC0) == 0x80)
        bg_color = ps2_mono_shades[bgc & 7];
    else
        bg_color = negative ? s_color_lut[0x000] : s_color_lut[0x0FFF];

    uint64_t bg64 = ((uint64_t)bg_color << 48) | ((uint64_t)bg_color << 32) | 
                    ((uint64_t)bg_color << 16) |  (uint64_t)bg_color;

    for (int x = 0; x < (SCREEN_WIDTH / 4); x++) spr_cfb64[x] = bg64;

    // 4. Carrega as 24 cores de paleta ativas
    const _u8* raw_pal = ram + 0x8100;
    for (int i = 0; i < 24; i++) {
        s_mono_pal_cache[i] = ps2_mono_shades[raw_pal[i] & 7];
    }

    const _u16* pal_sprites = s_mono_pal_cache;       // 0..7
    const _u16* pal_scroll1 = s_mono_pal_cache + 8;   // 8..15
    const _u16* pal_scroll2 = s_mono_pal_cache + 16;  // 16..23

    // 5. Pré-Processa Sprites (Fast-Reject)
    active_count = 0;
    _s16 lastSpriteX = 0, lastSpriteY = 0;

    for (int spr = 0; spr < 64; spr++) {
        _u32 spr_entry = *(_u32*)(ram + 0x8800 + (spr << 2));
        _u16 spr_data16 = (_u16)(spr_entry & 0xFFFF);
        _s16 sx = (_u8)((spr_entry >> 16) & 0xFF);
        _s16 sy = (_u8)((spr_entry >> 24) & 0xFF);
        
        _s16 x = (spr_data16 & 0x0400) ? (lastSpriteX + sx) : sx;
        _s16 y = (spr_data16 & 0x0200) ? (lastSpriteY + sy) : sy;
        lastSpriteX = x; lastSpriteY = y;
        
        _u8 priority = (spr_data16 & 0x1800) >> 11;
        if (!priority) continue;

        x += scrollsprx;
        y += scrollspry;

        x = (x > 248 && x < 256) ? (x - 256) : (x & 0xFF);
        y = (y > 248 && y < 256) ? (y - 256) : (y & 0xFF);

        _s16 dy = scanline - y;
        if (dy < 0 || dy >= 8) continue; // Rejeição de altura imediata

        active_sprites[active_count].x = x;
        active_sprites[active_count].dy = (_u8)dy;
        active_sprites[active_count].tile = spr_data16 & 0x01FF;
        active_sprites[active_count].mirror = spr_data16 & 0x8000;
        active_sprites[active_count].flipy = spr_data16 & 0x4000;
        active_sprites[active_count].prio = priority;
        active_sprites[active_count].pal_idx = (spr_data16 & 0x2000) ? 4 : 0;
        active_count++;
    }

    // 6. PAINTER'S ALGORITHM (Sem Z-Buffer!)
    render_sprites_prio(1, pal_sprites);

    if (planeSwap) {
        render_scroll_mono(pal_scroll1, 0x9000, scroll1x, scroll1y);
        render_sprites_prio(2, pal_sprites);
        render_scroll_mono(pal_scroll2, 0x9800, scroll2x, scroll2y);
    } else {
        render_scroll_mono(pal_scroll2, 0x9800, scroll2x, scroll2y);
        render_sprites_prio(2, pal_sprites);
        render_scroll_mono(pal_scroll1, 0x9000, scroll1x, scroll1y);
    }

    render_sprites_prio(3, pal_sprites);

    // 7. WINDOW CLIPPING VIRTUAL
    int end_left = min(winx, SCREEN_WIDTH);
    for (int x = 0; x < end_left; x++) SPR_CFB[x] = win_color;
    
    int start_right = min(winx + winw, SCREEN_WIDTH);
    for (int x = start_right; x < SCREEN_WIDTH; x++) SPR_CFB[x] = win_color;

    // 8. Envia para a VRAM real via cópia em bloco
    memcpy(real_cfb_scanline, SPR_CFB, SCREEN_WIDTH * sizeof(_u16));
}