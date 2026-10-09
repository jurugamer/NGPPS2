//---------------------------------------------------------------------------
// NEOPOP : Z80 Interface (Optimized Fallback Path for PS2)
//---------------------------------------------------------------------------

#include "neopop.h"
#include "mem.h"
#include "sound.h"
#include "Z80_interface.h"
#include "TLCS900h_registers.h"
#include "interrupt.h"
#include "dma.h"
#include <string.h>
#include <stdio.h>

#ifdef DEBUG
int DAsm(char *S, byte *A);
#endif

Z80 Z80_regs;

//=============================================================================
// LEITURA E ESCRITA LENTA (Usadas apenas em registradores e periféricos)
//=============================================================================
_u8 RdZ80(_u16 address)
{
	if (__builtin_expect(address <= 0x0FFF, 1))
		return ram[0x7000 + address];

	if (address == 0x8000)
		return ram[0xBC];

	return 0;
}

void WrZ80_slow(_u16 address, _u8 value)
{
	switch (address)
	{
		case 0x4000:
			Write_SoundChipNoise(value);
			return;

		case 0x4001:
			Write_SoundChipTone(value);
			return;

		case 0x8000:
			ram[0xBC] = value;
			return;

		case 0xC000:
			if (statusIFF() <= (ram[0x71] & 0x7))
			{
				interrupt(6); // Z80 Int.

				if (ram[0x007C] == 0x0C) DMA_update(0);
				else if (ram[0x007D] == 0x0C) DMA_update(1);
				else if (ram[0x007E] == 0x0C) DMA_update(2);
				else if (ram[0x007F] == 0x0C) DMA_update(3);
			}
			return;

		default:
			return;
	}
}

void WrZ80(_u16 address, _u8 value)
{
	if (__builtin_expect(address <= 0x0FFF, 1))
	{
		ram[0x7000 + address] = value;
		return;
	}
	WrZ80_slow(address, value);
}

//=============================================================================
void OutZ80(_u16 port, _u8 value) { (void)port; (void)value; }
_u8 InZ80(_u16 port) { (void)port; return 0; }
void PatchZ80(Z80 *R) { (void)R; }
word LoopZ80(Z80 *R) { (void)R; return INT_QUIT; }

void Z80_nmi(void)
{
    IntZ80(&Z80_regs, INT_NMI);
    
    // Executa uma rajada imediata no Z80 para processar a nota e confirmar o handshake:
    if (Z80ACTIVE)
    {
        int burst = 250;
        Z80_regs.ICount += burst;
        while (Z80_regs.ICount > 0)
        {
            ExecZ80(&Z80_regs);
            if (Z80_regs.IFF & IFF_HALT) {
                Z80_regs.ICount = 0;
                break;
            }
        }
    }
}

void Z80_irq(void)
{
	Z80_regs.IFF |= IFF_1;
	IntZ80(&Z80_regs, INT_IRQ);
}

void Z80_reset(void)
{
	ResetZ80(&Z80_regs);
	Z80_regs.SP.W = 0;
}

_u16 Z80_getReg(_u8 reg)
{
	_u16* r = (_u16*)&Z80_regs;
	return r[reg];
}

void Z80_setReg(_u8 reg, _u16 value)
{
	_u16* r = (_u16*)&Z80_regs;
	r[reg] = value;
}

char* Z80_disassemble(_u16* pc_in)
{
#ifdef DEBUG
	int bcnt, i;
	_u16 pc = *pc_in;
	char instr[64];
	_u8 str[80];
	memset(str, 0, 80);
	
	sprintf((char*)str, "<z80> %03X: ", pc);
	bcnt = DAsm(instr, ram + 0x7000 + pc);
	strcat((char*)str, instr);

	for (i = strlen((char*)str); i < 32; i++) str[i] = ' ';
	str[32] = '\"';
	for (i = 0; i < bcnt; i++)
	{
		_u8 tmp[80];
		sprintf((char*)tmp, "%02X ", *(ram + 0x7000 + pc + i));
		strcat((char*)str, (char*)tmp);
	}
	str[strlen((char*)str) - 1] = '\"';

	*pc_in = pc + bcnt;
	return strdup((char*)str);
#else
	(void)pc_in;
	return NULL;
#endif
}


// Rajada rápida para responder ao aperto de mão do som na hora:
void Z80_catchup_handshake(void)
{
    if (!Z80ACTIVE) return;
    if (Z80_regs.IFF & IFF_HALT) return;

    int burst = 80;
    Z80_regs.ICount += burst;
    while (Z80_regs.ICount > 0)
    {
        ExecZ80(&Z80_regs);
        if (Z80_regs.IFF & IFF_HALT) {
            Z80_regs.ICount = 0;
            break;
        }
    }
}

void Z80_execute_slice(int cycles)
{
    if (!Z80ACTIVE) return;
    if (__builtin_expect(Z80_regs.IFF & IFF_HALT, 0)) return;

    Z80_regs.ICount += cycles;
    while (Z80_regs.ICount > 0)
    {
        ExecZ80(&Z80_regs);
        if (Z80_regs.IFF & IFF_HALT) {
            Z80_regs.ICount = 0;
            break;
        }
    }
}

