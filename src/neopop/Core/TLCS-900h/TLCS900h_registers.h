#ifndef __TLCS900H_REGISTERS__
#define __TLCS900H_REGISTERS__
//=============================================================================

void reset_registers(void);
void dump_registers_TLCS900h(void);

//The value read by bad rCodes, leave 0, improves "Gals Fighters"
#define RERR_VALUE		0

//=============================================================================

extern _u32 pc;
extern _u16	sr;
extern _u8 f_dash;

extern _u32 gprBank[4][4], gpr[4];

extern _u32 rErr;

extern _u8 statusRFP;

#include "spr_map.h"
#define curGprMapB SPR_CUR_GPR_MAP_B
#define curGprMapW SPR_CUR_GPR_MAP_W
#define curGprMapL SPR_CUR_GPR_MAP_L

// Tabela única ativa compacta (economiza 5.3 KB de D-Cache!)
extern _u8*  curRegCodeMapB[256];
extern _u16* curRegCodeMapW[128];
extern _u32* curRegCodeMapL[64];

// Acesso direto em 1 ciclo:
#define regB(x)	(*(curGprMapB[(x)]))
#define regW(x)	(*(curGprMapW[(x)]))
#define regL(x)	(*(curGprMapL[(x)]))

#define rCodeB(r)	(*(curRegCodeMapB[(r)]))
#define rCodeW(r)	(*(curRegCodeMapW[(r) >> 1]))
#define rCodeL(r)	(*(curRegCodeMapL[(r) >> 2]))

//Common Registers
#define REGA		(regB(1))
#define REGWA		(regW(0))
#define REGBC		(regW(1))
#define REGXSP		(gpr[3])

//=============================================================================

_u8 statusIFF(void);
void setStatusIFF(_u8 iff);

void setStatusRFP(_u8 rfp);
void changedSP(void);

#define FLAG_S ((sr & 0x0080) >> 7)
#define FLAG_Z ((sr & 0x0040) >> 6)
#define FLAG_H ((sr & 0x0010) >> 4)
#define FLAG_V ((sr & 0x0004) >> 2)
#define FLAG_N ((sr & 0x0002) >> 1)
#define FLAG_C (sr & 1)

#define SETFLAG_S(s) { _u16 sr1 = sr & 0xFF7F; if (s) sr1 |= 0x0080; sr = sr1; }
#define SETFLAG_Z(z) { _u16 sr1 = sr & 0xFFBF; if (z) sr1 |= 0x0040; sr = sr1; }
#define SETFLAG_H(h) { _u16 sr1 = sr & 0xFFEF; if (h) sr1 |= 0x0010; sr = sr1; }
#define SETFLAG_V(v) { _u16 sr1 = sr & 0xFFFB; if (v) sr1 |= 0x0004; sr = sr1; }
#define SETFLAG_N(n) { _u16 sr1 = sr & 0xFFFD; if (n) sr1 |= 0x0002; sr = sr1; }
#define SETFLAG_C(c) { _u16 sr1 = sr & 0xFFFE; if (c) sr1 |= 0x0001; sr = sr1; }

#define SETFLAG_S0		{ sr &= 0xFF7F;	}
#define SETFLAG_Z0		{ sr &= 0xFFBF;	}
#define SETFLAG_H0		{ sr &= 0xFFEF;	}
#define SETFLAG_V0		{ sr &= 0xFFFB;	}
#define SETFLAG_N0		{ sr &= 0xFFFD;	}
#define SETFLAG_C0		{ sr &= 0xFFFE;	}

#define SETFLAG_S1		{ sr |= 0x0080; }
#define SETFLAG_Z1		{ sr |= 0x0040; }
#define SETFLAG_H1		{ sr |= 0x0010; }
#define SETFLAG_V1		{ sr |= 0x0004; }
#define SETFLAG_N1		{ sr |= 0x0002; }
#define SETFLAG_C1		{ sr |= 0x0001; }

//=============================================================================
#endif
