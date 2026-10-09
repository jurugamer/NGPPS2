
#include "neopop.h"
#include "TLCS900h_registers.h"
#include "interrupt.h"
#include "mem.h"
#include "bios.h"
#include "TLCS900h_interpret.h"
#include "TLCS900h_interpret_single.h"
#include "TLCS900h_interpret_src.h"
#include "TLCS900h_interpret_dst.h"
#include "TLCS900h_interpret_reg.h"

// =============================================================================
// OTIMIZAÇÃO: BUSCA RÁPIDA DE INSTRUÇÃO (FETCH8 INLINE)
// =============================================================================


static inline _u8 fetch8_inline(void) __attribute__((always_inline));
static inline _u8 fetch8_inline(void)
{
	// ROTA EXPRESSA DA ROM: 1 única instrução MIPS nativa (99.9% dos casos!)
	if (__builtin_expect(pc >= ROM_START && pc <= ROM_END, 1))
	{
		return rom.data[(pc++) - ROM_START];
	}

	// Fallback para RAM ou BIOS (apenas se o código estiver rodando fora da ROM)
	_u8* page = mem_page_read[(pc >> 16) & 0xFF];
	_u8 b = (page != NULL) ? page[pc & 0xFFFF] : loadB(pc);
	pc = (pc + 1) & 0xFFFFFF;
	return b;
}

#undef FETCH8
#define FETCH8 fetch8_inline()

// =============================================================================

_u32	mem;
int		size;

_u8		first;
_u8		R;
_u8		second;

BOOL	brCode;
_u8		rCode;

_u8		cycles;
_u8		cycles_extra;


// =============================================================================
// MATEMÁTICA BRANCHLESS (0 Laços e 0 Desvios Condicionais)
// =============================================================================

void parityB(_u8 value)
{
	value ^= value >> 4;
	value ^= value >> 2;
	value ^= value >> 1;
	SETFLAG_V(!(value & 1));
}

void parityW(_u16 value)
{
	value ^= value >> 8;
	value ^= value >> 4;
	value ^= value >> 2;
	value ^= value >> 1;
	SETFLAG_V(!(value & 1));
}

// =============================================================================
// PILHA ULTRA-RÁPIDA (Acesso nativo de 16 e 32 bits no MIPS)
// =============================================================================

void push8(_u8 data)
{
	REGXSP -= 1;
	if (__builtin_expect(REGXSP <= 0xFFFF, 1)) ram[REGXSP] = data;
	else storeB(REGXSP, data);
}

void push16(_u16 data)
{
	REGXSP -= 2;
	if (__builtin_expect(REGXSP <= 0xFFFE, 1)) {
		WRITE16(&ram[REGXSP], data);
	} else storeW(REGXSP, data);
}

void push32(_u32 data)
{
	REGXSP -= 4;
	if (__builtin_expect(REGXSP <= 0xFFFC, 1)) {
		WRITE32(&ram[REGXSP], data);
	} else storeL(REGXSP, data);
}

_u8 pop8(void)
{
	_u8 temp = __builtin_expect(REGXSP <= 0xFFFF, 1) ? ram[REGXSP] : loadB(REGXSP);
	REGXSP += 1;
	return temp;
}

_u16 pop16(void)
{
	_u16 temp = __builtin_expect(REGXSP <= 0xFFFE, 1) ? READ16(&ram[REGXSP]) : loadW(REGXSP);
	REGXSP += 2;
	return temp;
}

_u32 pop32(void)
{
	_u32 temp = __builtin_expect(REGXSP <= 0xFFFC, 1) ? READ32(&ram[REGXSP]) : loadL(REGXSP);
	REGXSP += 4;
	return temp;
}


// =============================================================================
// DIVISÃO ORIGINAL
// =============================================================================
_u16 generic_DIV_B(_u16 val, _u8 div) {
	if (div == 0) { SETFLAG_V1 return (val << 8) | ((val >> 8) ^ 0xFF); }
	_u32 quo = (_u32)val / (_u32)div; _u32 rem = (_u32)val % (_u32)div;
	if (quo > 0xFF) SETFLAG_V1 else SETFLAG_V0
	return (quo & 0xFF) | ((rem & 0xFF) << 8);
}

_u32 generic_DIV_W(_u32 val, _u16 div) {
	if (div == 0) { SETFLAG_V1 return (val << 16) | ((val >> 16) ^ 0xFFFF); }
	_u32 quo = val / (_u32)div; _u32 rem = val % (_u32)div;
	if (quo > 0xFFFF) SETFLAG_V1 else SETFLAG_V0
	return (quo & 0xFFFF) | ((rem & 0xFFFF) << 16);
}

_u16 generic_DIVS_B(_s16 val, _s8 div) {
	if (div == 0) { SETFLAG_V1 return (val << 8) | ((val >> 8) ^ 0xFF); }
	_s32 quo = (_s32)val / (_s32)div; _s32 rem = (_s32)val % (_s32)div;
	if (quo > 0xFF) SETFLAG_V1 else SETFLAG_V0
	return (quo & 0xFF) | ((rem & 0xFF) << 8);
}

_u32 generic_DIVS_W(_s32 val, _s16 div) {
	if (div == 0) { SETFLAG_V1 return (val << 16) | ((val >> 16) ^ 0xFFFF); }
	_s32 quo = val / (_s32)div; _s32 rem = val % (_s32)div;
	if (quo > 0xFFFF) SETFLAG_V1 else SETFLAG_V0
	return (quo & 0xFFFF) | ((rem & 0xFFFF) << 16);
}

// =============================================================================
// ALU BRANCHLESS (Adição e Subtração Otimizadas)
// =============================================================================
_u8 generic_ADD_B(_u8 dst, _u8 src)
{
	_u32 resultC = (_u32)dst + (_u32)src;
	_u8 result = (_u8)resultC;
	_u16 f = sr & 0xFF28;
	if (result & 0x80) f |= 0x80;
	if (result == 0)   f |= 0x40;
	if (((dst & 0xF) + (src & 0xF)) > 0xF) f |= 0x10;
	if (((dst ^ result) & (src ^ result)) & 0x80) f |= 0x04;
	if (resultC > 0xFF) f |= 0x01;
	sr = f;
	return result;
}

_u16 generic_ADD_W(_u16 dst, _u16 src)
{
	_u32 resultC = (_u32)dst + (_u32)src;
	_u16 result = (_u16)resultC;
	_u16 f = sr & 0xFF28;
	if (result & 0x8000) f |= 0x80;
	if (result == 0)      f |= 0x40;
	if (((dst & 0xF) + (src & 0xF)) > 0xF) f |= 0x10;
	if (((dst ^ result) & (src ^ result)) & 0x8000) f |= 0x04;
	if (resultC > 0xFFFF) f |= 0x01;
	sr = f;
	return result;
}
_u32 generic_ADD_L(_u32 dst, _u32 src)
{
	_u64 resultC = (_u64)dst + (_u64)src;
	_u32 result = (_u32)resultC;
	_u16 f = sr & 0xFF38;
	if (result & 0x80000000) f |= 0x80;
	if (result == 0)         f |= 0x40;
	if (((dst ^ result) & (src ^ result)) & 0x80000000) f |= 0x04;
	if (resultC > 0xFFFFFFFF) f |= 0x01;
	sr = f;
	return result;
}

_u8 generic_ADC_B(_u8 dst, _u8 src)
{
	_u8 c = FLAG_C ? 1 : 0;
	_u32 resultC = (_u32)dst + (_u32)src + c;
	_u8 result = (_u8)resultC;
	SETFLAG_S(result & 0x80); SETFLAG_Z(result == 0); SETFLAG_H(((dst & 0xF) + (src & 0xF) + c) > 0xF);
	SETFLAG_V(((dst ^ result) & (src ^ result)) & 0x80);
	SETFLAG_N0; SETFLAG_C(resultC > 0xFF);
	return result;
}

_u16 generic_ADC_W(_u16 dst, _u16 src)
{
	_u16 c = FLAG_C ? 1 : 0;
	_u32 resultC = (_u32)dst + (_u32)src + c;
	_u16 result = (_u16)resultC;
	SETFLAG_S(result & 0x8000); SETFLAG_Z(result == 0); SETFLAG_H(((dst & 0xF) + (src & 0xF) + c) > 0xF);
	SETFLAG_V(((dst ^ result) & (src ^ result)) & 0x8000);
	SETFLAG_N0; SETFLAG_C(resultC > 0xFFFF);
	return result;
}

_u32 generic_ADC_L(_u32 dst, _u32 src)
{
	_u64 c = FLAG_C ? 1 : 0;
	_u64 resultC = (_u64)dst + (_u64)src + c;
	_u32 result = (_u32)resultC;
	SETFLAG_S(result & 0x80000000); SETFLAG_Z(result == 0);
	SETFLAG_V(((dst ^ result) & (src ^ result)) & 0x80000000);
	SETFLAG_N0; SETFLAG_C(resultC > 0xFFFFFFFF);
	return result;
}

_u8 generic_SUB_B(_u8 dst, _u8 src)
{
	_u32 resultC = (_u32)dst - (_u32)src;
	_u8 result = (_u8)resultC;
	_u16 f = (sr & 0xFF28) | 0x02; // N = 1
	if (result & 0x80) f |= 0x80;
	if (result == 0)   f |= 0x40;
	if (((dst & 0xF) - (src & 0xF)) > 0xF) f |= 0x10;
	if (((dst ^ src) & (dst ^ result)) & 0x80) f |= 0x04;
	if (resultC > 0xFF) f |= 0x01;
	sr = f;
	return result;
}

_u16 generic_SUB_W(_u16 dst, _u16 src)
{
	_u32 resultC = (_u32)dst - (_u32)src;
	_u16 result = (_u16)resultC;
	_u16 f = (sr & 0xFF28) | 0x02; // N = 1
	if (result & 0x8000) f |= 0x80;
	if (result == 0)      f |= 0x40;
	if (((dst & 0xF) - (src & 0xF)) > 0xF) f |= 0x10;
	if (((dst ^ src) & (dst ^ result)) & 0x8000) f |= 0x04;
	if (resultC > 0xFFFF) f |= 0x01;
	sr = f;
	return result;
}

_u32 generic_SUB_L(_u32 dst, _u32 src)
{
	_u64 resultC = (_u64)dst - (_u64)src;
	_u32 result = (_u32)resultC;
	_u16 f = (sr & 0xFF38) | 0x02; // N = 1
	if (result & 0x80000000) f |= 0x80;
	if (result == 0)         f |= 0x40;
	if (((dst ^ src) & (dst ^ result)) & 0x80000000) f |= 0x04;
	if (resultC > 0xFFFFFFFF) f |= 0x01;
	sr = f;
	return result;
}

_u8 generic_SBC_B(_u8 dst, _u8 src)
{
	_u8 c = FLAG_C ? 1 : 0;
	_u32 resultC = (_u32)dst - (_u32)src - c;
	_u8 result = (_u8)resultC;
	SETFLAG_S(result & 0x80); SETFLAG_Z(result == 0); SETFLAG_H(((dst & 0xF) - (src & 0xF) - c) > 0xF);
	SETFLAG_V(((dst ^ src) & (dst ^ result)) & 0x80);
	SETFLAG_N1; SETFLAG_C(resultC > 0xFF);
	return result;
}

_u16 generic_SBC_W(_u16 dst, _u16 src)
{
	_u16 c = FLAG_C ? 1 : 0;
	_u32 resultC = (_u32)dst - (_u32)src - c;
	_u16 result = (_u16)resultC;
	SETFLAG_S(result & 0x8000); SETFLAG_Z(result == 0); SETFLAG_H(((dst & 0xF) - (src & 0xF) - c) > 0xF);
	SETFLAG_V(((dst ^ src) & (dst ^ result)) & 0x8000);
	SETFLAG_N1; SETFLAG_C(resultC > 0xFFFF);
	return result;
}

_u32 generic_SBC_L(_u32 dst, _u32 src)
{
	_u64 c = FLAG_C ? 1 : 0;
	_u64 resultC = (_u64)dst - (_u64)src - c;
	_u32 result = (_u32)resultC;
	SETFLAG_S(result & 0x80000000); SETFLAG_Z(result == 0);
	SETFLAG_V(((dst ^ src) & (dst ^ result)) & 0x80000000);
	SETFLAG_N1; SETFLAG_C(resultC > 0xFFFFFFFF);
	return result;
}

// =============================================================================

// === TRANSFORME EM STATIC INLINE PARA NÃO GERAR JAL ===
static inline __attribute__((always_inline)) BOOL conditionCode(int cc)
{
	switch(cc)
	{
	case 0:	 return 0;
	case 1:	 return (FLAG_S ^ FLAG_V);
	case 2:	 return (FLAG_Z | (FLAG_S ^ FLAG_V));
	case 3:	 return (FLAG_C | FLAG_Z);
	case 4:  return (FLAG_V != 0);
	case 5:	 return (FLAG_S != 0);
	case 6:	 return (FLAG_Z != 0);
	case 7:	 return (FLAG_C != 0);
	case 8:	 return 1; // Incondicional
	case 9:	 return !(FLAG_S ^ FLAG_V);
	case 10: return !(FLAG_Z | (FLAG_S ^ FLAG_V));
	case 11: return !(FLAG_C | FLAG_Z);
	case 12: return (FLAG_V == 0);
	case 13: return (FLAG_S == 0);
	case 14: return (FLAG_Z == 0);
	case 15: return (FLAG_C == 0);
	}
	return FALSE;
}

// =============================================================================

_u8 get_rr_Target(void)
{
	_u8 target = 0x80;
	if (size == 0 && first == 0xC7) return rCode;
	switch(first & 7) {
	case 0: if (size == 1) target = 0xE0; break;
	case 1: if (size == 0) target = 0xE0; if (size == 1) target = 0xE4; break;
	case 2: if (size == 1) target = 0xE8; break;
	case 3: if (size == 0) target = 0xE4; if (size == 1) target = 0xEC; break;
	case 4: if (size == 1) target = 0xF0; break;
	case 5: if (size == 0) target = 0xE8; if (size == 1) target = 0xF4; break;
	case 6: if (size == 1) target = 0xF8; break;
	case 7: if (size == 0) target = 0xEC; if (size == 1) target = 0xFC; break;
	}
	return target;
}

_u8 get_RR_Target(void)
{
	_u8 target = 0x80;
	switch(second & 7) {
	case 0: if (size == 1) target = 0xE0; break;
	case 1: if (size == 0) target = 0xE0; if (size == 1) target = 0xE4; break;
	case 2: if (size == 1) target = 0xE8; break;
	case 3: if (size == 0) target = 0xE4; if (size == 1) target = 0xEC; break;
	case 4: if (size == 1) target = 0xF0; break;
	case 5: if (size == 0) target = 0xE8; if (size == 1) target = 0xF4; break;
	case 6: if (size == 1) target = 0xF8; break;
	case 7: if (size == 0) target = 0xEC; if (size == 1) target = 0xFC; break;
	}
	return target;
}

// =============================================================================
// FUNÇÕES RÁPIDAS DE MEMÓRIA (Ex*)
// =============================================================================
static inline void Ex8(void)   { mem = FETCH8; cycles_extra = 2; }
static inline void Ex16(void)  { mem = fetch16(); cycles_extra = 2; }
static inline void Ex24(void)  { mem = fetch24(); cycles_extra = 3; }

static inline void ExR32(void)
{
	_u8 data = FETCH8;
	if (data == 0x03) {
		_u8 r32 = FETCH8; _u8 rIndex = FETCH8;
		mem = rCodeL(r32) + (_s8)rCodeB(rIndex); cycles_extra = 8; return;
	}
	if (data == 0x07) {
		_u8 r32 = FETCH8; _u8 rIndex = FETCH8;
		mem = rCodeL(r32) + (_s16)rCodeW(rIndex); cycles_extra = 8; return;
	}
	if (data == 0x13) {
		mem = pc + (_s16)fetch16(); cycles_extra = 8; return;
	}
	cycles_extra = 5;
	if ((data & 3) == 1) mem = rCodeL(data) + (_s16)fetch16();
	else mem = rCodeL(data);
}

static inline void ExDec(void)
{
	_u8 data = FETCH8; _u8 r32 = data & 0xFC; cycles_extra = 3;
	switch(data & 3) {
	case 0: rCodeL(r32) -= 1; mem = rCodeL(r32); break;
	case 1: rCodeL(r32) -= 2; mem = rCodeL(r32); break;
	case 2: rCodeL(r32) -= 4; mem = rCodeL(r32); break;
	}
}

static inline void ExInc(void)
{
	_u8 data = FETCH8; _u8 r32 = data & 0xFC; cycles_extra = 3;
	switch(data & 3) {
	case 0: mem = rCodeL(r32); rCodeL(r32) += 1; break;
	case 1: mem = rCodeL(r32); rCodeL(r32) += 2; break;
	case 2: mem = rCodeL(r32); rCodeL(r32) += 4; break;
	}
}

static inline void ExRC(void) { brCode = TRUE; rCode = FETCH8; cycles_extra = 1; }

// =========================================================================
// OTIMIZAÇÃO PS2: TABELAS CONSTANTES DE DISPATCH (LTO DEVIRTUALIZATION)
// Isso força o GCC a transformar ponteiros de função em Jump Tables locais!
// =========================================================================
static void e(void)  { instruction_error("Unknown instruction %02X", first); }
static void es(void) { instruction_error("Unknown [src] instruction %02X", second); }
static void ed(void) { instruction_error("Unknown [dst] instruction %02X", second); }
static void er(void) { instruction_error("Unknown [reg] instruction %02X", second); }

static void (* const srcDecode[256])(void) = {
	es,es,es,es,srcPUSH,es,srcRLD,srcRRD,es,es,es,es,es,es,es,es,
	srcLDI,srcLDIR,srcLDD,srcLDDR,srcCPI,srcCPIR,srcCPD,srcCPDR,es,srcLD16m,es,es,es,es,es,es,
	srcLD,srcLD,srcLD,srcLD,srcLD,srcLD,srcLD,srcLD,es,es,es,es,es,es,es,es,
	srcEX,srcEX,srcEX,srcEX,srcEX,srcEX,srcEX,srcEX,srcADDi,srcADCi,srcSUBi,srcSBCi,srcANDi,srcXORi,srcORi,srcCPi,
	srcMUL,srcMUL,srcMUL,srcMUL,srcMUL,srcMUL,srcMUL,srcMUL,srcMULS,srcMULS,srcMULS,srcMULS,srcMULS,srcMULS,srcMULS,srcMULS,
	srcDIV,srcDIV,srcDIV,srcDIV,srcDIV,srcDIV,srcDIV,srcDIV,srcDIVS,srcDIVS,srcDIVS,srcDIVS,srcDIVS,srcDIVS,srcDIVS,srcDIVS,
	srcINC,srcINC,srcINC,srcINC,srcINC,srcINC,srcINC,srcINC,srcDEC,srcDEC,srcDEC,srcDEC,srcDEC,srcDEC,srcDEC,srcDEC,
	es,es,es,es,es,es,es,es,srcRLC,srcRRC,srcRL,srcRR,srcSLA,srcSRA,srcSLL,srcSRL,
	srcADDRm,srcADDRm,srcADDRm,srcADDRm,srcADDRm,srcADDRm,srcADDRm,srcADDRm,srcADDmR,srcADDmR,srcADDmR,srcADDmR,srcADDmR,srcADDmR,srcADDmR,srcADDmR,
	srcADCRm,srcADCRm,srcADCRm,srcADCRm,srcADCRm,srcADCRm,srcADCRm,srcADCRm,srcADCmR,srcADCmR,srcADCmR,srcADCmR,srcADCmR,srcADCmR,srcADCmR,srcADCmR,
	srcSUBRm,srcSUBRm,srcSUBRm,srcSUBRm,srcSUBRm,srcSUBRm,srcSUBRm,srcSUBRm,srcSUBmR,srcSUBmR,srcSUBmR,srcSUBmR,srcSUBmR,srcSUBmR,srcSUBmR,srcSUBmR,
	srcSBCRm,srcSBCRm,srcSBCRm,srcSBCRm,srcSBCRm,srcSBCRm,srcSBCRm,srcSBCRm,srcSBCmR,srcSBCmR,srcSBCmR,srcSBCmR,srcSBCmR,srcSBCmR,srcSBCmR,srcSBCmR,
	srcANDRm,srcANDRm,srcANDRm,srcANDRm,srcANDRm,srcANDRm,srcANDRm,srcANDRm,srcANDmR,srcANDmR,srcANDmR,srcANDmR,srcANDmR,srcANDmR,srcANDmR,srcANDmR,
	srcXORRm,srcXORRm,srcXORRm,srcXORRm,srcXORRm,srcXORRm,srcXORRm,srcXORRm,srcXORmR,srcXORmR,srcXORmR,srcXORmR,srcXORmR,srcXORmR,srcXORmR,srcXORmR,
	srcORRm,srcORRm,srcORRm,srcORRm,srcORRm,srcORRm,srcORRm,srcORRm,srcORmR,srcORmR,srcORmR,srcORmR,srcORmR,srcORmR,srcORmR,srcORmR,
	srcCPRm,srcCPRm,srcCPRm,srcCPRm,srcCPRm,srcCPRm,srcCPRm,srcCPRm,srcCPmR,srcCPmR,srcCPmR,srcCPmR,srcCPmR,srcCPmR,srcCPmR,srcCPmR
};

static void (* const dstDecode[256])(void) = {
	dstLDBi,ed,dstLDWi,ed,dstPOPB,ed,dstPOPW,ed,ed,ed,ed,ed,ed,ed,ed,ed,
	ed,ed,ed,ed,dstLDBm16,ed,dstLDWm16,ed,ed,ed,ed,ed,ed,ed,ed,ed,
	dstLDAW,dstLDAW,dstLDAW,dstLDAW,dstLDAW,dstLDAW,dstLDAW,dstLDAW,dstANDCFA,dstORCFA,dstXORCFA,dstLDCFA,dstSTCFA,ed,ed,ed,
	dstLDAL,dstLDAL,dstLDAL,dstLDAL,dstLDAL,dstLDAL,dstLDAL,dstLDAL,ed,ed,ed,ed,ed,ed,ed,ed,
	dstLDBR,dstLDBR,dstLDBR,dstLDBR,dstLDBR,dstLDBR,dstLDBR,dstLDBR,ed,ed,ed,ed,ed,ed,ed,ed,
	dstLDWR,dstLDWR,dstLDWR,dstLDWR,dstLDWR,dstLDWR,dstLDWR,dstLDWR,ed,ed,ed,ed,ed,ed,ed,ed,
	dstLDLR,dstLDLR,dstLDLR,dstLDLR,dstLDLR,dstLDLR,dstLDLR,dstLDLR,ed,ed,ed,ed,ed,ed,ed,ed,
	ed,ed,ed,ed,ed,ed,ed,ed,ed,ed,ed,ed,ed,ed,ed,ed,
	dstANDCF,dstANDCF,dstANDCF,dstANDCF,dstANDCF,dstANDCF,dstANDCF,dstANDCF,dstORCF,dstORCF,dstORCF,dstORCF,dstORCF,dstORCF,dstORCF,dstORCF,
	dstXORCF,dstXORCF,dstXORCF,dstXORCF,dstXORCF,dstXORCF,dstXORCF,dstXORCF,dstLDCF,dstLDCF,dstLDCF,dstLDCF,dstLDCF,dstLDCF,dstLDCF,dstLDCF,
	dstSTCF,dstSTCF,dstSTCF,dstSTCF,dstSTCF,dstSTCF,dstSTCF,dstSTCF,dstTSET,dstTSET,dstTSET,dstTSET,dstTSET,dstTSET,dstTSET,dstTSET,
	dstRES,dstRES,dstRES,dstRES,dstRES,dstRES,dstRES,dstRES,dstSET,dstSET,dstSET,dstSET,dstSET,dstSET,dstSET,dstSET,
	dstCHG,dstCHG,dstCHG,dstCHG,dstCHG,dstCHG,dstCHG,dstCHG,dstBIT,dstBIT,dstBIT,dstBIT,dstBIT,dstBIT,dstBIT,dstBIT,
	dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,dstJP,
	dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,dstCALL,
	dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET,dstRET
};

static void (* const regDecode[256])(void) = {
	er,er,er,regLDi,regPUSH,regPOP,regCPL,regNEG,regMULi,regMULSi,regDIVi,regDIVSi,regLINK,regUNLK,regBS1F,regBS1B,
	regDAA,er,regEXTZ,regEXTS,regPAA,er,regMIRR,er,er,regMULA,er,er,regDJNZ,er,er,er,
	regANDCFi,regORCFi,regXORCFi,regLDCFi,regSTCFi,er,er,er,regANDCFA,regORCFA,regXORCFA,regLDCFA,regSTCFA,er,regLDCcrr,regLDCrcr,
	regRES,regSET,regCHG,regBIT,regTSET,er,er,er,regMINC1,regMINC2,regMINC4,er,regMDEC1,regMDEC2,regMDEC4,er,
	regMUL,regMUL,regMUL,regMUL,regMUL,regMUL,regMUL,regMUL,regMULS,regMULS,regMULS,regMULS,regMULS,regMULS,regMULS,regMULS,
	regDIV,regDIV,regDIV,regDIV,regDIV,regDIV,regDIV,regDIV,regDIVS,regDIVS,regDIVS,regDIVS,regDIVS,regDIVS,regDIVS,regDIVS,
	regINC,regINC,regINC,regINC,regINC,regINC,regINC,regINC,regDEC,regDEC,regDEC,regDEC,regDEC,regDEC,regDEC,regDEC,
	regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,regSCC,
	regADD,regADD,regADD,regADD,regADD,regADD,regADD,regADD,regLDRr,regLDRr,regLDRr,regLDRr,regLDRr,regLDRr,regLDRr,regLDRr,
	regADC,regADC,regADC,regADC,regADC,regADC,regADC,regADC,regLDrR,regLDrR,regLDrR,regLDrR,regLDrR,regLDrR,regLDrR,regLDrR,
	regSUB,regSUB,regSUB,regSUB,regSUB,regSUB,regSUB,regSUB,regLDr3,regLDr3,regLDr3,regLDr3,regLDr3,regLDr3,regLDr3,regLDr3,
	regSBC,regSBC,regSBC,regSBC,regSBC,regSBC,regSBC,regSBC,regEX,regEX,regEX,regEX,regEX,regEX,regEX,regEX,
	regAND,regAND,regAND,regAND,regAND,regAND,regAND,regAND,regADDi,regADCi,regSUBi,regSBCi,regANDi,regXORi,regORi,regCPi,
	regXOR,regXOR,regXOR,regXOR,regXOR,regXOR,regXOR,regXOR,regCPr3,regCPr3,regCPr3,regCPr3,regCPr3,regCPr3,regCPr3,regCPr3,
	regOR,regOR,regOR,regOR,regOR,regOR,regOR,regOR,regRLCi,regRRCi,regRLi,regRRi,regSLAi,regSRAi,regSLLi,regSRLi,
	regCP,regCP,regCP,regCP,regCP,regCP,regCP,regCP,regRLCA,regRRCA,regRLA,regRRA,regSLAA,regSRAA,regSLLA,regSRLA
};

// =============================================================================

static void src_B(void) { second = FETCH8; R = second & 7; size = 0; (*srcDecode[second])(); }
static void src_W(void) { second = FETCH8; R = second & 7; size = 1; (*srcDecode[second])(); }
static void src_L(void) { second = FETCH8; R = second & 7; size = 2; (*srcDecode[second])(); }
static void dst(void)   { second = FETCH8; R = second & 7; (*dstDecode[second])(); }

static _u8 rCodeConversionB[8] = { 0xE1, 0xE0, 0xE5, 0xE4, 0xE9, 0xE8, 0xED, 0xEC };
static _u8 rCodeConversionW[8] = { 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC };
static _u8 rCodeConversionL[8] = { 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC };

static void reg_B(void) {
	second = FETCH8; R = second & 7; size = 0;
	if (!brCode) { brCode = TRUE; rCode = rCodeConversionB[first & 7]; }
	(*regDecode[second])();
}
static void reg_W(void) {
	second = FETCH8; R = second & 7; size = 1;
	if (!brCode) { brCode = TRUE; rCode = rCodeConversionW[first & 7]; }
	(*regDecode[second])();
}
static void reg_L(void) {
	second = FETCH8; R = second & 7; size = 2;
	if (!brCode) { brCode = TRUE; rCode = rCodeConversionL[first & 7]; }
	(*regDecode[second])();
}

static void (* const decode[256])(void) = {
	sngNOP,sngNORMAL,sngPUSHSR,sngPOPSR,sngMAX,sngHALT,sngEI,sngRETI,sngLD8_8,sngPUSH8,sngLD8_16,sngPUSH16,sngINCF,sngDECF,sngRET,sngRETD,
	sngRCF,sngSCF,sngCCF,sngZCF,sngPUSHA,sngPOPA,sngEX,sngLDF,sngPUSHF,sngPOPF,sngJP16,sngJP24,sngCALL16,sngCALL24,sngCALR,iBIOSHLE,
	sngLDB,sngLDB,sngLDB,sngLDB,sngLDB,sngLDB,sngLDB,sngLDB,sngPUSHW,sngPUSHW,sngPUSHW,sngPUSHW,sngPUSHW,sngPUSHW,sngPUSHW,sngPUSHW,
	sngLDW,sngLDW,sngLDW,sngLDW,sngLDW,sngLDW,sngLDW,sngLDW,sngPUSHL,sngPUSHL,sngPUSHL,sngPUSHL,sngPUSHL,sngPUSHL,sngPUSHL,sngPUSHL,
	sngLDL,sngLDL,sngLDL,sngLDL,sngLDL,sngLDL,sngLDL,sngLDL,sngPOPW,sngPOPW,sngPOPW,sngPOPW,sngPOPW,sngPOPW,sngPOPW,sngPOPW,
	e,e,e,e,e,e,e,e,sngPOPL,sngPOPL,sngPOPL,sngPOPL,sngPOPL,sngPOPL,sngPOPL,sngPOPL,
	sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,sngJR,
	sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,sngJRL,
	src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,src_B,
	src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,src_W,
	src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,src_L,
	dst,dst,dst,dst,dst,dst,dst,dst,dst,dst,dst,dst,dst,dst,dst,dst,
	src_B,src_B,src_B,src_B,src_B,src_B,e,reg_B,reg_B,reg_B,reg_B,reg_B,reg_B,reg_B,reg_B,reg_B,
	src_W,src_W,src_W,src_W,src_W,src_W,e,reg_W,reg_W,reg_W,reg_W,reg_W,reg_W,reg_W,reg_W,reg_W,
	src_L,src_L,src_L,src_L,src_L,src_L,e,reg_L,reg_L,reg_L,reg_L,reg_L,reg_L,reg_L,reg_L,reg_L,
	dst,dst,dst,dst,dst,dst,e,sngLDX,sngSWI,sngSWI,sngSWI,sngSWI,sngSWI,sngSWI,sngSWI,sngSWI
};

_u8 TLCS900h_interpret(void)
{
	brCode = FALSE;
	cycles_extra = 0;

	first = FETCH8;

	// =========================================================================
	// VIA EXPRESSA SEGURA (Fast-Paths de 1 ciclo no MIPS)
	// =========================================================================

	// 1. NOP
	if (first == 0x00) {
		return 2;
	}

	// 2. HALT (Opcode 0x05 - Muito usado pelo Sonic no loop de espera de VBlank!)
	if (first == 0x05) {
		return 8;
	}

	// 3. JR incondicional (Opcode 0x68 - Salta direto sem checar flags!)
	if (first == 0x68) {
		pc = (pc + (_s8)FETCH8) & 0xFFFFFF;
		return 8;
	}

	// 4. JRL incondicional (Opcode 0x78 - Salta direto sem checar flags!)
	if (first == 0x78) {
		pc = (pc + (_s16)fetch16()) & 0xFFFFFF;
		return 8;
	}

	// 5. JR condicional (Opcodes 0x60 até 0x6F)
	if ((first & 0xF0) == 0x60) {
		if (conditionCode(first & 0x0F)) {
			pc = (pc + (_s8)FETCH8) & 0xFFFFFF;
			return 8;
		} else {
			pc = (pc + 1) & 0xFFFFFF;
			return 4;
		}
	}

	// 6. JRL condicional (Opcodes 0x70 até 0x7F)
	if ((first & 0xF0) == 0x70) {
		if (conditionCode(first & 0x0F)) {
			pc = (pc + (_s16)fetch16()) & 0xFFFFFF;
			return 8;
		} else {
			pc = (pc + 2) & 0xFFFFFF;
			return 4;
		}
	}

	// =========================================================================
	// RESTANTE DAS INSTRUÇÕES (>= 0x80 é muito frequente, sem penalidade de branch)
	// =========================================================================

	if (first >= 0x80)
	{
		if (first < 0xC0)
		{
			if (first & 0x08) {
				mem = regL(first & 7) + (_s8)FETCH8;
				cycles_extra = 2;
			} else {
				mem = regL(first & 7);
			}
		}
		else
		{
			_u8 fn = first & 0x0F;
			switch (fn) {
				case 0: Ex8(); break;
				case 1: Ex16(); break;
				case 2: Ex24(); break;
				case 3: ExR32(); break;
				case 4: ExDec(); break;
				case 5: ExInc(); break;
				case 7: if (first != 0xF7) ExRC(); break;
				default: break;
			}
		}
	}

	(*decode[first])();

	return cycles + cycles_extra;
}

#include "TLCS900h_interpret_dst.c"
#include "TLCS900h_interpret_src.c"
#include "TLCS900h_interpret_reg.c"
#include "TLCS900h_interpret_single.c"
#include "TLCS900h_registers.c"
