//---------------------------------------------------------------------------
// NEOPOP PS2 : Colour Engine Estável (Avaliação Dinâmica de Sprites & Cores Reais)
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

static _u16 s_pal_cache[192];
static _u16 s_last_raw_pal[192];
static int  s_pal_first = 1;

#define SPR_BASE ((_u16*)0x70000000)
#define SPR_CFB  (SPR_BASE + 64)

//=============================================================================
// DRAWPATTERN BRANCHLESS
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
// ESTRUTURA FAST-SPRITE (Avaliação Dinâmica por Linha)
//=============================================================================
typedef struct { 
    _s16 x; _u8 dy, pal, prio, flipy; 
    _u16 tile, mirror; 
} FastSprite;

static FastSprite active_sprites[64];
static int active_count = 0;

static void render_sprites_prio(_u8 prio, const _u16* pal_sprites) {
    for (int i = active_count - 1; i >= 0; i--) {
        if (active_sprites[i].prio == prio) {
            _u8 tiley = active_sprites[i].flipy ? (7 - active_sprites[i].dy) : active_sprites[i].dy;
            _u16 data = *(_u16*)(ram + 0xA000 + (active_sprites[i].tile << 4) + (tiley << 1));
            const _u16* pal = pal_sprites + (active_sprites[i].pal << 2);

            if (active_sprites[i].mirror) 
                draw_8px_flip(SPR_CFB + active_sprites[i].x, data, pal);
            else 
                draw_8px_trans(SPR_CFB + active_sprites[i].x, data, pal);
        }
    }
}

static inline void render_scroll(const _u16* pal_scroll, _u16 map_base, _u8 scrollx, _u8 scrolly) {
    _u8 line = scanline + scrolly;
    _u8 row = line & 7;
    _u8 row_flipped = 7 - row;

    _u16 line_offset = ((line >> 3) & 31) << 6;
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
        
        // Pula decodificação se todos os 8 pixels forem transparentes (zero)
        if (!tile_data) continue;

        const _u16* pal = pal_scroll + ((data16 & 0x1E00) >> 7);

        if (data16 & 0x8000) draw_8px_flip(SPR_CFB + sx, tile_data, pal);
        else draw_8px_trans(SPR_CFB + sx, tile_data, pal);
    }
}

//=============================================================================
// SCANLINE PRINCIPAL
//=============================================================================
void gfx_draw_scanline_colour(void)
{
    scanline = ram[0x8009];
    _u16* real_cfb_scanline = cfb + (scanline * 256);
    uint64_t* spr_cfb64 = (uint64_t*)SPR_CFB;

    // 1. Atualização Rápida de Paleta em 64-bit
    const _u16* raw_pal = (const _u16*)(ram + 0x8200);
    uint64_t* p_curr = (uint64_t*)raw_pal;
    uint64_t* p_last = (uint64_t*)s_last_raw_pal;
    int pal_changed = s_pal_first;

    if (!pal_changed) {
        for (int i = 0; i < 48; i++) {
            if (p_curr[i] != p_last[i]) { pal_changed = 1; break; }
        }
    }

    if (__builtin_expect(pal_changed, 0)) {
        s_pal_first = 0;
        for (int i = 0; i < 48; i++) p_last[i] = p_curr[i];
        if (negative) {
            for (int i = 0; i < 192; i++) s_pal_cache[i] = s_color_lut[(~raw_pal[i]) & 0x0FFF];
        } else {
            for (int i = 0; i < 192; i++) s_pal_cache[i] = s_color_lut[raw_pal[i] & 0x0FFF];
        }
    }

    _u16 raw_win = *(_u16*)(ram + 0x83F0 + (oowc << 1));
    if (negative) raw_win = ~raw_win;
    _u16 win_color = s_color_lut[raw_win & 0x0FFF];

    _u16 raw_bg = *(_u16*)(ram + 0x83E0 + ((bgc & 7) << 1));
    if (negative) raw_bg = ~raw_bg;
    _u16 bg_color = s_color_lut[raw_bg & 0x0FFF];
    uint64_t bg64 = ((uint64_t)bg_color << 48) | ((uint64_t)bg_color << 32) | ((uint64_t)bg_color << 16) | (uint64_t)bg_color;

    for (int x = 0; x < 40; x++) spr_cfb64[x] = bg64;

 // 2. Coleta Otimizada de Sprites (Branchless & Fast Reject)
    active_count = 0;
    _s16 lastSpriteX = 0, lastSpriteY = 0;

    for (int spr = 0; spr < 64; spr++) {
        _u32 spr_entry = *(_u32*)(ram + 0x8800 + (spr << 2));
        _u16 spr_data16 = (_u16)(spr_entry & 0xFFFF);
        
        _s16 sx = (_u8)((spr_entry >> 16) & 0xFF);
        _s16 sy = (_u8)((spr_entry >> 24) & 0xFF);
        
        // Mantém a amarração de coordenadas
        _s16 x = (spr_data16 & 0x0400) ? (lastSpriteX + sx) : sx;
        _s16 y = (spr_data16 & 0x0200) ? (lastSpriteY + sy) : sy;
        lastSpriteX = x; 
        lastSpriteY = y;

        // Se for invisível, pula na hora (economiza milhares de ciclos)
        _u8 priority = (spr_data16 & 0x1800) >> 11;
        if (!priority) continue;

        // Rejeição ultrarrápida de distância vertical
        _u8 dy = (_u8)(scanline - ((y + scrollspry) & 0xFF));
        if (dy >= 8) continue;

        x += scrollsprx;
        x = (x > 248 && x < 256) ? (x - 256) : (x & 0xFF);

        active_sprites[active_count].x = x;
        active_sprites[active_count].dy = dy;
        active_sprites[active_count].tile = spr_data16 & 0x01FF;
        active_sprites[active_count].mirror = spr_data16 & 0x8000;
        active_sprites[active_count].flipy = (spr_data16 & 0x4000) != 0;
        active_sprites[active_count].prio = priority;
        active_sprites[active_count].pal = ram[0x8C00 + spr] & 0xF;
        active_count++;
    }

    // 3. Ordem Original de Paletas do NGP Color
    const _u16* pal_sprites = s_pal_cache;        // Sprites usam o início do buffer (0x8200)
    const _u16* pal_scroll1 = s_pal_cache + 64;   // Scroll 1 (+64)
    const _u16* pal_scroll2 = s_pal_cache + 128;  // Scroll 2 (+128)

    // 4. Renderização em Camadas
    render_sprites_prio(1, pal_sprites);

    if (planeSwap) {
        render_scroll(pal_scroll1, 0x9000, scroll1x, scroll1y);
        render_sprites_prio(2, pal_sprites);
        render_scroll(pal_scroll2, 0x9800, scroll2x, scroll2y);
    } else {
        render_scroll(pal_scroll2, 0x9800, scroll2x, scroll2y);
        render_sprites_prio(2, pal_sprites);
        render_scroll(pal_scroll1, 0x9000, scroll1x, scroll1y);
    }

    // 5. Máscara da Janela (Corta Cenários sem Deletar HUD)
    if (scanline < winy || scanline >= winy + winh) {
        uint64_t win64 = ((uint64_t)win_color << 48) | ((uint64_t)win_color << 32) | ((uint64_t)win_color << 16) | (uint64_t)win_color;
        for (int x = 0; x < 40; x++) spr_cfb64[x] = win64;
    } else {
	// OTIMIZAÇÃO: Só percorre o corte horizontal se a janela realmente tiver bordas ativas!
		if (winx > 0) {
			int end_left = (winx < SCREEN_WIDTH) ? winx : SCREEN_WIDTH;
			for (int x = 0; x < end_left; x++) SPR_CFB[x] = win_color;
		}
		
		int right_edge = winx + winw;
		if (right_edge < SCREEN_WIDTH) {
			for (int x = right_edge; x < SCREEN_WIDTH; x++) SPR_CFB[x] = win_color;
		}
    }

    render_sprites_prio(3, pal_sprites);

    // 6. Envio Direto de 160 Pixels para a VRAM
    memcpy(real_cfb_scanline, SPR_CFB, SCREEN_WIDTH * sizeof(_u16));
}