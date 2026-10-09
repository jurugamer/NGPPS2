//---------------------------------------------------------------------------
// NEOPOP PS2 : Optimized Interrupt & Timer Engine (MIPS R5900 Fast Path)
//---------------------------------------------------------------------------

#include "neopop.h"
#include "TLCS900h_registers.h"
#include "mem.h"
#include "gfx.h"
#include "interrupt.h"
#include "TLCS900h_interpret.h"
#include "Z80_interface.h"
#include "dma.h"
#include "perf_profiler.h"

//=============================================================================
void OutputAudioFrame(void);
_u32 timer_hint;
_u32 timer_clock0, timer_clock1, timer_clock2, timer_clock3;
_u8 timer[4];	// Up-counters

BOOL gfx_hack = FALSE;
static BOOL h_int = FALSE, timer0, timer2;

//=============================================================================

void interrupt(_u8 index)
{
	push32(pc);
	push16(sr);

	// Sobe o IFF no registrador de status
	_u32 iff = (sr & 0x7000) >> 12;
	if (iff < 7)
		setStatusIFF(iff + 1);

	// Acessa a tabela de vetores de interrupção diretamente
	pc = *(_u32*)(ram + 0x6FB8 + (index << 2));
}

//=============================================================================

static inline void gfx_hint(void) __attribute__((always_inline));
static inline void gfx_hint(void)
{
	if (ram[0x8009] < SCREEN_HEIGHT - 1 || ram[0x8009] == 198)
	{
		gfx_delayed_settings();

		if (ram[0x8000] & 0x40)
			h_int = TRUE;
	}
}

static inline void gfx_draw(void) __attribute__((always_inline));
static inline void gfx_draw(void)
{
	if (frameskip_count == 0)
	{
		if (ram[0x8009] < SCREEN_HEIGHT)
		{
			if (ram[0x6F95] == 0x10) 
				gfx_draw_scanline_colour();
			else                     
				gfx_draw_scanline_mono();
		}
	}
}

//=============================================================================
// TABELAS ESTÁTICAS DE TAXA
//=============================================================================
static const _u16 s_t0_rates[4] = { 0, TIMER_T1_RATE, TIMER_T4_RATE,  TIMER_T16_RATE };
static const _u16 s_t1_rates[4] = { 0, TIMER_T1_RATE, TIMER_T16_RATE, TIMER_T256_RATE };
static const _u16 s_t2_rates[4] = { 0, 56,            TIMER_T4_RATE,  TIMER_T16_RATE };
static const _u16 s_t3_rates[4] = { 0, TIMER_T1_RATE, TIMER_T16_RATE, TIMER_T256_RATE };

// Auxiliar para disparar DMA apenas quando os vetores estiverem configurados:
static inline void check_dma_vector(_u8 vector) __attribute__((always_inline));
static inline void check_dma_vector(_u8 vector)
{
	if (ram[0x007C] == vector)      DMA_update(0);
	else if (ram[0x007D] == vector) DMA_update(1);
	else if (ram[0x007E] == vector) DMA_update(2);
	else if (ram[0x007F] == vector) DMA_update(3);
}

void updateTimers_slow(_u16 cputicks)
{
	_u8 current_iff = (_u8)((sr >> 12) & 7);

	timer_hint += cputicks;
	if (timer_hint >= TIMER_HINT_RATE)
	{
		timer_hint -= TIMER_HINT_RATE;
		_u8 cur_line = ram[0x8009];

		if (cur_line < SCREEN_HEIGHT)
		{
			if (frameskip_count == 0)
			{
				Profiler_ZoneStart(PROF_GFX_SCANLINE);
				if (ram[0x6F95] == 0x10)
					gfx_draw_scanline_colour();
				else
					gfx_draw_scanline_mono();
				Profiler_ZoneEnd(PROF_GFX_SCANLINE);
			}

			gfx_hint();
		}

		ram[0x8009] = cur_line + 1;

		if (__builtin_expect((ram[0xB2] & 1) == 0, 0))
		{
		}

		if (ram[0x8009] == SCREEN_HEIGHT)
		{
			ram[0x8010] = 0x40;

			if (current_iff <= 4 && (ram[0x8000] & 0x80))
			{
				interrupt(5);
				if (__builtin_expect(*(_u32*)(ram + 0x7C) != 0, 0))
					check_dma_vector(0x0B);
			}
		}

		if (ram[0x8009] >= 199)
		{
			ram[0x8009] = 0;
			ram[0x8010] = 0;

			OutputAudioFrame();
			system_VBL();
			frameskip_count = (frameskip_count + 1) % system_frameskip_key;
		}
	}

	// =========================================================================
	// 2. TIMERS DE HARDWARE
	// =========================================================================
	_u8 trun = ram[0x20];
	if (!trun) return;

	_u32 dma_check = *(_u32*)(ram + 0x7C);

	// Timers 0 e 1...
	if (trun & 0x03)
	{
		_u8 t01mod = ram[0x24];
		timer0 = FALSE;

		if (trun & 0x01)
		{
			_u8 mode = t01mod & 0x03;
			if (mode == 0) {
				if (h_int) { timer[0]++; timer_clock0 = 0; h_int = FALSE; }
			} else {
				timer_clock0 += cputicks;
				_u16 rate = s_t0_rates[mode];
				while (timer_clock0 >= rate) { timer[0]++; timer_clock0 -= rate; }
			}

			_u8 target0 = ram[0x22];
			if (target0 && timer[0] >= target0) {
				timer[0] = 0;
				timer0 = TRUE;
				if (current_iff <= (ram[0x73] & 0x7)) interrupt(7);
				if (__builtin_expect(dma_check != 0, 0)) check_dma_vector(0x10);
			}
		}

		if (trun & 0x02)
		{
			_u8 mode = (t01mod >> 2) & 0x03;
			if (mode == 0) {
				if (timer0) { timer[1]++; timer_clock1 = 0; }
			} else {
				timer_clock1 += cputicks;
				_u16 rate = s_t1_rates[mode];
				while (timer_clock1 >= rate) { timer[1]++; timer_clock1 -= rate; }
			}

			_u8 target1 = ram[0x23];
			if (target1 && timer[1] >= target1) {
				timer[1] = 0;
				if (current_iff <= ((ram[0x73] >> 4) & 0x7)) interrupt(8);
				if (__builtin_expect(dma_check != 0, 0)) check_dma_vector(0x11);
			}
		}
	}

	// Timers 2 e 3 (Áudio e IRQ do Z80)
	if (trun & 0x0C)
	{
		_u8 t23mod = ram[0x28];
		timer2 = FALSE;

		if (trun & 0x04)
		{
			_u8 mode = t23mod & 0x03;
			if (mode != 0) {
				timer_clock2 += cputicks;
				_u16 rate = s_t2_rates[mode];
				while (timer_clock2 >= rate) { timer[2]++; timer_clock2 -= rate; }
			}

			_u8 target2 = ram[0x26];
			if (target2 && timer[2] >= target2) {
				timer[2] = 0;
				timer2 = TRUE;
				if (current_iff <= (ram[0x74] & 0x07)) interrupt(9);
				if (__builtin_expect(dma_check != 0, 0)) check_dma_vector(0x12);
			}
		}

		if (trun & 0x08)
		{
			_u8 mode = (t23mod >> 2) & 0x03;
			if (mode == 0) {
				if (timer2) { timer[3]++; timer_clock3 = 0; }
			} else {
				timer_clock3 += cputicks;
				_u16 rate = s_t3_rates[mode];
				while (timer_clock3 >= rate) { timer[3]++; timer_clock3 -= rate; }
			}

			_u8 target3 = ram[0x27];
			if (target3 && timer[3] >= target3) {
				timer[3] = 0;
				Z80_irq(); // IRQ periódica para o Z80
				if (current_iff <= ((ram[0x74] >> 4) & 0x7)) interrupt(10);
				if (__builtin_expect(dma_check != 0, 0)) check_dma_vector(0x13);
			}
		}
	}
}

//=============================================================================

void reset_timers(void)
{
	timer_hint = 0;


	timer[0] = 0;
	timer[1] = 0;
	timer[2] = 0;
	timer[3] = 0;
	
	timer_clock0 = 0;
	timer_clock1 = 0;
	timer_clock2 = 0;
	timer_clock3 = 0;
}