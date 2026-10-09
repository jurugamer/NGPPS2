#ifndef __GFX__
#define __GFX__

#include <stdint.h>

#define ZDEPTH_BACK_SPRITE			2
#define ZDEPTH_BACKGROUND_SCROLL	3
#define ZDEPTH_MIDDLE_SPRITE		4
#define ZDEPTH_FOREGROUND_SCROLL	5
#define ZDEPTH_FRONT_SPRITE			6

// Alinhado a 64 bytes (otimizado para cache line do EE e transferências DMA/GS)
extern uint8_t zbuffer[256] __attribute__((aligned(64)));
extern uint16_t* cfb;
extern uint16_t* cfb_scanline;

extern uint8_t scanline;
extern uint8_t interlace;

// Usando variáveis compactas externamente, mas tratadas como registradores
extern uint8_t winx, winw;
extern uint8_t winy, winh;
extern uint8_t scroll1x, scroll1y;
extern uint8_t scroll2x, scroll2y;
extern uint8_t scrollsprx, scrollspry;
extern uint8_t planeSwap;
extern uint8_t bgc, oowc, negative;

extern uint16_t r, g, b;

void gfx_delayed_settings(void);
void gfx_draw_scanline_colour(void);
void gfx_draw_scanline_mono(void);

#endif