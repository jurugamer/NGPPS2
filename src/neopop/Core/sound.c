//---------------------------------------------------------------------------
// NEOPOP PS2 : Optimized Sound Synthesizer (Zero Dead-Code & Single-Pass Mix)
//---------------------------------------------------------------------------

#include "neopop.h"
#include "mem.h"
#include "sound.h"
#include <string.h>

BOOL mute;

SoundChip toneChip;
SoundChip noiseChip;

#define DAC_BUFFERSIZE		(256 * 1024)

int dacBufferRead, dacBufferWrite, dacBufferCount;
_u8 dacBufferL[DAC_BUFFERSIZE];

#define SOUNDCHIPCLOCK	(3072000)
#define MAX_OUTPUT      0x7fff
#define STEP            0x10000

static _u32 VolTable[16];
static _u32 UpdateStep;

#define FB_WNOISE 0x14002
#define FB_PNOISE 0x08000
#define NG_PRESET 0x0f35

//=============================================================================
// SINTETIZADOR DE TONS ULTRA-RÁPIDO (Canais 0, 1 e 2 - Fase Direta)
//=============================================================================
static inline _s32 sample_chip_tone_fast(void) __attribute__((always_inline));
static inline _s32 sample_chip_tone_fast(void)
{
	_s32 out = 0;

	// Canal 0
	if (toneChip.Volume[0]) {
		toneChip.Count[0] -= STEP;
		if (__builtin_expect(toneChip.Count[0] <= 0, 0)) {
			do {
				toneChip.Count[0] += toneChip.Period[0];
				toneChip.Output[0] ^= 1;
			} while (toneChip.Count[0] <= 0);
		}
		if (toneChip.Output[0]) out += toneChip.Volume[0];
	}

	// Canal 1
	if (toneChip.Volume[1]) {
		toneChip.Count[1] -= STEP;
		if (__builtin_expect(toneChip.Count[1] <= 0, 0)) {
			do {
				toneChip.Count[1] += toneChip.Period[1];
				toneChip.Output[1] ^= 1;
			} while (toneChip.Count[1] <= 0);
		}
		if (toneChip.Output[1]) out += toneChip.Volume[1];
	}

	// Canal 2
	if (toneChip.Volume[2]) {
		toneChip.Count[2] -= STEP;
		if (__builtin_expect(toneChip.Count[2] <= 0, 0)) {
			do {
				toneChip.Count[2] += toneChip.Period[2];
				toneChip.Output[2] ^= 1;
			} while (toneChip.Count[2] <= 0);
		}
		if (toneChip.Output[2]) out += toneChip.Volume[2];
	}

	return out;
}

//=============================================================================
// SINTETIZADOR DE RUÍDO ULTRA-RÁPIDO (Canal 3)
//=============================================================================
static inline _s32 sample_chip_noise_fast(void) __attribute__((always_inline));
static inline _s32 sample_chip_noise_fast(void)
{
	if (!noiseChip.Volume[3]) return 0;

	noiseChip.Count[3] -= STEP;
	if (__builtin_expect(noiseChip.Count[3] <= 0, 0)) {
		do {
			noiseChip.Count[3] += noiseChip.Period[3];
			if (noiseChip.RNG & 1) noiseChip.RNG ^= noiseChip.NoiseFB;
			noiseChip.RNG >>= 1;
			noiseChip.Output[3] = noiseChip.RNG & 1;
		} while (noiseChip.Count[3] <= 0);
	}

	return noiseChip.Output[3] ? noiseChip.Volume[3] : 0;
}

//=============================================================================
// RENDERIZAÇÃO COMPLETA COM MIXAGEM DIRETA (0.15 ms no PS2)
//=============================================================================
static _u32 s_dac_pos = 0;

void sound_render_frame(_s16* out_buffer, int samples) {
	if (mute) return;

	_u32 dac_step = (samples > 400) ? 11888 : 23777;

	for (int i = 0; i < samples; i++)
	{
		// 1. Gera PSG
		_s32 psg_raw = sample_chip_tone_fast() + sample_chip_noise_fast();
		_s32 psg_signed = (psg_raw >> 1) - 0x4000;

		// 2. Extrai voz/tiros (DAC) de forma rápida
		_s32 dac_signed = 0;
		if (__builtin_expect(dacBufferCount > 0, 0))
		{
			dac_signed = ((_s32)dacBufferL[dacBufferRead] - 0x80) * 70;
			s_dac_pos += dac_step;

			if (s_dac_pos >= 0x10000)
			{
				dacBufferRead = (dacBufferRead + 1) & (DAC_BUFFERSIZE - 1);
				dacBufferCount--;
				s_dac_pos &= 0xFFFF; // Operação bitwise mais rápida que subtração
			}
		}

		// 3. Mixagem e Limite
		_s32 mixed = psg_signed + dac_signed;
		if (mixed > 32000)  mixed = 32000;
		else if (mixed < -32000) mixed = -32000;

		_s16 final_sample = (_s16)mixed;
		*(out_buffer++) = final_sample;
		*(out_buffer++) = final_sample;
	}
}

// Compatibilidade
void sound_update_stereo(_u16* chip_buffer, int length_bytes) {}
void sound_update(_u16* chip_buffer, int length_bytes) {}
void dac_update(_u8* dac_buffer, int length_bytes) {}

//=============================================================================
// ESCRITA NOS REGISTRADORES DE SOM
//=============================================================================
void WriteSoundChip(SoundChip* chip, _u8 data)
{
	if (data & 0x80)
	{
		int r = (data & 0x70) >> 4;
		int c = r / 2;

		chip->LastRegister = r;
		chip->Register[r] = (chip->Register[r] & 0x3f0) | (data & 0x0f);
	
		switch(r)
		{
		case 0:
		case 2:
		case 4:
			chip->Period[c] = UpdateStep * chip->Register[r];
			if (chip->Period[c] == 0) chip->Period[c] = UpdateStep;
			if (r == 4 && (chip->Register[6] & 0x03) == 0x03)
				chip->Period[3] = 2 * chip->Period[2];
			break;

		case 1:
		case 3:
		case 5:
		case 7:
			chip->Volume[c] = VolTable[data & 0xF];
			break;

		case 6:
			chip->NoiseFB = (chip->Register[6] & 4) ? FB_WNOISE : FB_PNOISE;
			chip->Period[3] = ((chip->Register[6] & 3) == 3) ? (2 * chip->Period[2]) : (UpdateStep << (5 + (chip->Register[6] & 3)));
			chip->RNG = NG_PRESET;
			chip->Output[3] = chip->RNG & 1;
			break;
		}
	}
	else
	{
		int r = chip->LastRegister;
		int c = r / 2;

		if (r == 0 || r == 2 || r == 4)
		{
			chip->Register[r] = (chip->Register[r] & 0x0f) | ((data & 0x3f) << 4);
			chip->Period[c] = UpdateStep * chip->Register[r];
			if (chip->Period[c] == 0) chip->Period[c] = UpdateStep;
			if (r == 4 && (chip->Register[6] & 0x03) == 0x03)
				chip->Period[3] = 2 * chip->Period[2];
		}
	}
}

void dac_write(void)
{
	if (mute) return;

	dacBufferL[dacBufferWrite] = ram[0xA2];
	dacBufferWrite++;
	if (dacBufferWrite == DAC_BUFFERSIZE) dacBufferWrite = 0;

	dacBufferCount++;
	if (dacBufferCount == DAC_BUFFERSIZE) {
		dacBufferCount = 0;
	}
}

void sound_init(int SampleRate)
{
	
	int i;
	double out;

	UpdateStep = (_u32)(((double)STEP * SampleRate * 16) / SOUNDCHIPCLOCK);

	memset(&toneChip, 0, sizeof(SoundChip));
	memset(&noiseChip, 0, sizeof(SoundChip));

	for (i = 0; i < 8; i += 2) {
		toneChip.Register[i] = noiseChip.Register[i] = 0;
		toneChip.Register[i + 1] = noiseChip.Register[i + 1] = 0x0f;
	}

	for (i = 0; i < 4; i++) {
		toneChip.Output[i] = noiseChip.Output[i] = 0;
		toneChip.Period[i] = toneChip.Count[i] = UpdateStep;
		noiseChip.Period[i] = noiseChip.Count[i] = UpdateStep;
	}

	out = MAX_OUTPUT / 3;
	for (i = 0; i < 15; i++) {
		VolTable[i] = (_u32)out;
		out /= 1.258925412;
	}
	VolTable[15] = 0;

	for (i = 0; i < DAC_BUFFERSIZE; i++) dacBufferL[i] = 0x80;
	dacBufferCount = dacBufferRead = dacBufferWrite = 0;
}