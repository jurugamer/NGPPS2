#include "neopop.h"
#include "mem.h"
#include "gfx.h"

_u16* cfb;
__attribute__((aligned(64))) _u8 zbuffer[256];

_u16* cfb_scanline;
_u8 scanline;
_u8 interlace;

_u8 winx = 0, winw = SCREEN_WIDTH;
_u8 winy = 0, winh = SCREEN_HEIGHT;
_u8 scroll1x = 0, scroll1y = 0;
_u8 scroll2x = 0, scroll2y = 0;
_u8 scrollsprx = 0, scrollspry = 0;
_u8 planeSwap = 0;
_u8 bgc = 0, oowc = 0;

_u16 r, g, b;
_u8 negative;

void gfx_delayed_settings(void)
{
	// O MIPS lida muito melhor agrupando leituras
	const _u8* const r_ptr = ram;

	winx = r_ptr[0x8002];
	winy = r_ptr[0x8003];
	winw = r_ptr[0x8004];
	winh = r_ptr[0x8005];

	scroll1x = r_ptr[0x8032];
	scroll1y = r_ptr[0x8033];
	scroll2x = r_ptr[0x8034];
	scroll2y = r_ptr[0x8035];

	scrollsprx = r_ptr[0x8020];
	scrollspry = r_ptr[0x8021];

	planeSwap = r_ptr[0x8030] & 0x80;
	bgc       = r_ptr[0x8118];

	const _u8 ctrl2d = r_ptr[0x8012];
	oowc     = ctrl2d & 0x07;
	negative = ctrl2d & 0x80;
}