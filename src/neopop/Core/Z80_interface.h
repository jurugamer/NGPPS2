#ifndef __Z80_CONTROL__
#define __Z80_CONTROL__

// 1. Includes essenciais SEMPRE no topo:
#include "neopop.h"
#include "Z80.h"

// 2. Declaração do Z80 para todo o projeto poder acessar o estado:
extern Z80 Z80_regs;

void Z80_reset(void);
void Z80_irq(void);
void Z80_nmi(void);

void Z80_catchup_handshake(void);
void Z80_execute_slice(int cycles);

#define Z80ACTIVE		(ram[0xb9] == 0x55)

// OTIMIZAÇÃO PS2: Se o Z80 está em repouso (HALT), consome o tempo sem entrar na CPU!
#define Z80EMULATE do { \
	if (__builtin_expect(!(Z80_regs.IFF & IFF_HALT), 1)) \
		ExecZ80(&Z80_regs); \
} while(0)

#define Z80_REG_AF	0
#define Z80_REG_BC	1
#define Z80_REG_DE	2
#define Z80_REG_HL	3
#define Z80_REG_IX	4
#define Z80_REG_IY	5
#define Z80_REG_PC	6
#define Z80_REG_SP	7
#define Z80_REG_AF1	8
#define Z80_REG_BC1	9
#define Z80_REG_DE1	10
#define Z80_REG_HL1	11

_u16 Z80_getReg(_u8 reg);
void Z80_setReg(_u8 reg, _u16 value);

char* Z80_disassemble(_u16* pc);

#endif // __Z80_CONTROL__