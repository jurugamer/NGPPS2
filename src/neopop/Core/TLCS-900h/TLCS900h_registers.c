#include "neopop.h"
#include "TLCS900h_registers.h"

//=============================================================================

_u32 pc, gprBank[4][4], gpr[4];
_u16 sr;
_u8 f_dash;

// Tabela única ativa de registradores (Apenas 1.7 KB em vez de 7.1 KB!)
_u8*  curRegCodeMapB[256] __attribute__((aligned(64)));
_u16* curRegCodeMapW[128] __attribute__((aligned(64)));
_u32* curRegCodeMapL[64]  __attribute__((aligned(64)));

_u32 rErr;
_u8 statusRFP;

//=============================================================================
// MAPAS GPR BÁSICOS (8 registradores por banco)
//=============================================================================
_u8* gprMapB[4][8] = {
    #include "TLCS900h_registers_mapB.h"
};

_u16* gprMapW[4][8] = {
    #include "TLCS900h_registers_mapW.h"
};

_u32* gprMapL[4][8] = {
    #include "TLCS900h_registers_mapL.h"
};

//=============================================================================
// INICIALIZAÇÃO DA TABELA FIXA (Executada 1 única vez)
//=============================================================================
static void init_static_reg_maps(void)
{
    int i;

    // 1. Preenche tudo inicialmente com rErr (Inválidos)
    for (i = 0; i < 256; i++) curRegCodeMapB[i] = (_u8*)&rErr;
    for (i = 0; i < 128; i++) curRegCodeMapW[i] = (_u16*)&rErr;
    for (i = 0; i < 64;  i++) curRegCodeMapL[i] = (_u32*)&rErr;

    // 2. Bancos 0, 1, 2, 3 fixos (0x00 .. 0x3F)
    for (int b = 0; b < 4; b++)
    {
        for (int r = 0; r < 4; r++)
        {
            int baseB = (b << 4) + (r << 2);
            curRegCodeMapB[baseB + 0] = ((_u8*)&gprBank[b][r]) + 0;
            curRegCodeMapB[baseB + 1] = ((_u8*)&gprBank[b][r]) + 1;
            curRegCodeMapB[baseB + 2] = ((_u8*)&gprBank[b][r]) + 2;
            curRegCodeMapB[baseB + 3] = ((_u8*)&gprBank[b][r]) + 3;

            int baseW = (b << 3) + (r << 1);
            curRegCodeMapW[baseW + 0] = (_u16*)(((_u8*)&gprBank[b][r]) + 0);
            curRegCodeMapW[baseW + 1] = (_u16*)(((_u8*)&gprBank[b][r]) + 2);

            int baseL = (b << 2) + r;
            curRegCodeMapL[baseL]     = &gprBank[b][r];
        }
    }

    // 3. Registradores Especiais (XIX, XIY, XIZ, XSP) em 0xF0..0xFF
    for (int r = 0; r < 4; r++)
    {
        int baseB = 0xF0 + (r << 2);
        curRegCodeMapB[baseB + 0] = ((_u8*)&gpr[r]) + 0;
        curRegCodeMapB[baseB + 1] = ((_u8*)&gpr[r]) + 1;
        curRegCodeMapB[baseB + 2] = ((_u8*)&gpr[r]) + 2;
        curRegCodeMapB[baseB + 3] = ((_u8*)&gpr[r]) + 3;

        int baseW = 0x78 + (r << 1); // (0xF0 >> 1) = 0x78
        curRegCodeMapW[baseW + 0] = (_u16*)(((_u8*)&gpr[r]) + 0);
        curRegCodeMapW[baseW + 1] = (_u16*)(((_u8*)&gpr[r]) + 2);

        int baseL = 0x3C + r;        // (0xF0 >> 2) = 0x3C
        curRegCodeMapL[baseL]     = &gpr[r];
    }
}

//=============================================================================

_u8 statusIFF(void)	
{
	_u8 iff = (sr & 0x7000) >> 12;
	return (iff == 1) ? 0 : iff;
}

void setStatusIFF(_u8 iff)
{
	sr = (sr & 0x8FFF) | ((iff & 0x7) << 12);
}

void setStatusRFP(_u8 rfp)
{
	sr = (sr & 0xF8FF) | ((rfp & 0x3) << 8);
	changedSP();
}

void changedSP(void)
{
	statusRFP = ((sr & 0x300) >> 8);

	// Copia os 8 ponteiros ativos direto para o Scratchpad (ultra-rápido)
	memcpy(curGprMapB, gprMapB[statusRFP], 8 * sizeof(_u8*));
	memcpy(curGprMapW, gprMapW[statusRFP], 8 * sizeof(_u16*));
	memcpy(curGprMapL, gprMapL[statusRFP], 8 * sizeof(_u32*));

	// 1. Atualiza o Banco Anterior (0xD0..0xDF)
	if (statusRFP == 0)
	{
		for (int i = 0; i < 16; i++) curRegCodeMapB[0xD0 + i] = (_u8*)&rErr;
		for (int i = 0; i < 8;  i++) curRegCodeMapW[0x68 + i] = (_u16*)&rErr; // 0xD0 >> 1
		for (int i = 0; i < 4;  i++) curRegCodeMapL[0x34 + i] = (_u32*)&rErr; // 0xD0 >> 2
	}
	else
	{
		int pb = statusRFP - 1;
		for (int r = 0; r < 4; r++)
		{
			curRegCodeMapB[0xD0 + (r << 2) + 0] = ((_u8*)&gprBank[pb][r]) + 0;
			curRegCodeMapB[0xD0 + (r << 2) + 1] = ((_u8*)&gprBank[pb][r]) + 1;
			curRegCodeMapB[0xD0 + (r << 2) + 2] = ((_u8*)&gprBank[pb][r]) + 2;
			curRegCodeMapB[0xD0 + (r << 2) + 3] = ((_u8*)&gprBank[pb][r]) + 3;

			curRegCodeMapW[0x68 + (r << 1) + 0] = (_u16*)(((_u8*)&gprBank[pb][r]) + 0);
			curRegCodeMapW[0x68 + (r << 1) + 1] = (_u16*)(((_u8*)&gprBank[pb][r]) + 2);

			curRegCodeMapL[0x34 + r]            = &gprBank[pb][r];
		}
	}

	// 2. Atualiza o Banco Atual (0xE0..0xEF)
	for (int r = 0; r < 4; r++)
	{
		curRegCodeMapB[0xE0 + (r << 2) + 0] = ((_u8*)&gprBank[statusRFP][r]) + 0;
		curRegCodeMapB[0xE0 + (r << 2) + 1] = ((_u8*)&gprBank[statusRFP][r]) + 1;
		curRegCodeMapB[0xE0 + (r << 2) + 2] = ((_u8*)&gprBank[statusRFP][r]) + 2;
		curRegCodeMapB[0xE0 + (r << 2) + 3] = ((_u8*)&gprBank[statusRFP][r]) + 3;

		curRegCodeMapW[0x70 + (r << 1) + 0] = (_u16*)(((_u8*)&gprBank[statusRFP][r]) + 0); // 0xE0 >> 1
		curRegCodeMapW[0x70 + (r << 1) + 1] = (_u16*)(((_u8*)&gprBank[statusRFP][r]) + 2);

		curRegCodeMapL[0x38 + r]            = &gprBank[statusRFP][r]; // 0xE0 >> 2
	}
}

void reset_registers(void)
{
	memset(gprBank, 0, sizeof(gprBank));
	memset(gpr, 0, sizeof(gpr));

	if (rom.data)
		pc = rom_header->startPC & 0xFFFFFF;
	else
		pc = 0xFFFFFE;

	sr = 0xF800;
	f_dash = 0;
	rErr = RERR_VALUE;
	REGXSP = 0x00006C00;

	init_static_reg_maps();
	changedSP();
}