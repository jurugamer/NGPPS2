//---------------------------------------------------------------------------
// dma.c - Otimizado para PS2 (MIPS R5900)
//---------------------------------------------------------------------------

#include "neopop.h"
#include "dma.h"
#include "mem.h"
#include "interrupt.h"
#include <stdint.h>
// Alinhados a 16 bytes para melhor acesso de barramento do EE
__attribute__((aligned(16))) _u32 dmaS[4];
__attribute__((aligned(16))) _u32 dmaD[4];
__attribute__((aligned(16))) _u16 dmaC[4];
__attribute__((aligned(16))) _u8  dmaM[4];

void reset_dma(void)
{
	memset(dmaS, 0, sizeof(dmaS));
	memset(dmaD, 0, sizeof(dmaD));
	memset(dmaC, 0, sizeof(dmaC));
	memset(dmaM, 0, sizeof(dmaM));
}

// Tabela de passos por tamanho (0=1, 1=2, 2=4)
static const uint32_t step_table[4] = { 1, 2, 4, 1 };

void DMA_update(int channel)
{
	uint32_t count = dmaC[channel];
	if (__builtin_expect(!count, 0))
		return;

	const uint32_t m = dmaM[channel];
	const uint32_t mode = (m >> 2) & 0x07;
	const uint32_t size = m & 0x03;
	const uint32_t step = step_table[size];

	if (__builtin_expect(mode == 5, 0)) // Counter Mode
	{
		dmaS[channel]++;
	}
	else if (__builtin_expect(mode <= 4, 1)) // Transfer Modes
	{
		const uint32_t src = dmaS[channel];
		const uint32_t dst = dmaD[channel];

		// Leitura e Escrita direta sem overhead
		switch (size)
		{
		case 0: storeB(dst, loadB(src)); break;
		case 1: storeW(dst, loadW(src)); break;
		case 2: storeL(dst, loadL(src)); break;
		default: break;
		}

		// Atualização dos ponteiros calculada sem switches pesados
		switch (mode)
		{
		case 0: dmaD[channel] = dst + step; break; // Dest INC
		case 1: dmaD[channel] = dst - step; break; // Dest DEC
		case 2: dmaS[channel] = src + step; break; // Src INC
		case 3: dmaS[channel] = src - step; break; // Src DEC
		case 4: /* Endereço fixo */        break;
		}
	}
	else
	{
		system_message("Bad DMA mode %d", m);
		return;
	}

	// Decrementa contador
	count--;
	dmaC[channel] = (uint16_t)count;

	if (__builtin_expect(count == 0, 0))
	{
		interrupt(14 + channel);
		ram[0x7C + channel] = 0;
	}
}

// Inlined ou com jumps rápidos para registradores de DMA
void dmaStoreB(_u8 cr, _u8 data)
{
	switch (cr)
	{
	case 0x22: dmaM[0] = data; break;
	case 0x26: dmaM[1] = data; break;
	case 0x2A: dmaM[2] = data; break;
	case 0x2E: dmaM[3] = data; break;
	default: break;
	}
}

void dmaStoreW(_u8 cr, _u16 data)
{
	switch (cr)
	{
	case 0x20: dmaC[0] = data; break;
	case 0x24: dmaC[1] = data; break;
	case 0x28: dmaC[2] = data; break;
	case 0x2C: dmaC[3] = data; break;
	default: break;
	}
}

void dmaStoreL(_u8 cr, _u32 data)
{
	// Deslocamento direto baseado no offset para evitar saltos
	if (cr <= 0x0C && !(cr & 0x03))
		dmaS[cr >> 2] = data;
	else if (cr >= 0x10 && cr <= 0x1C && !(cr & 0x03))
		dmaD[(cr - 0x10) >> 2] = data;
}

_u8 dmaLoadB(_u8 cr)
{
	switch (cr)
	{
	case 0x22: return dmaM[0];
	case 0x26: return dmaM[1];
	case 0x2A: return dmaM[2];
	case 0x2E: return dmaM[3];
	default:   return 0;
	}
}

_u16 dmaLoadW(_u8 cr)
{
	switch (cr)
	{
	case 0x20: return dmaC[0];
	case 0x24: return dmaC[1];
	case 0x28: return dmaC[2];
	case 0x2C: return dmaC[3];
	default:   return 0;
	}
}

_u32 dmaLoadL(_u8 cr)
{
	if (cr <= 0x0C && !(cr & 0x03))
		return dmaS[cr >> 2];
	if (cr >= 0x10 && cr <= 0x1C && !(cr & 0x03))
		return dmaD[(cr - 0x10) >> 2];
	return 0;
}