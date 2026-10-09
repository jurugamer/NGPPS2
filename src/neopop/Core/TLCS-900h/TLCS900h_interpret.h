//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland - Fast ROM Fetch
//---------------------------------------------------------------------------

#ifndef __TLCS900H_INTERPRET__
#define __TLCS900H_INTERPRET__

#include "neopop.h"
#include "mem.h"
#include "TLCS900h_registers.h"

//=============================================================================

_u8 TLCS900h_interpret(void);

//=============================================================================

extern _u32 mem;	
extern int size;
extern _u8 first;
extern _u8 second;
extern _u8 R;
extern _u8 rCode;
extern _u8 cycles;
extern BOOL brCode;

//=============================================================================

void __cdecl instruction_error(char* vaMessage,...);

static inline _u8 fast_fetch8_inline(void) __attribute__((always_inline));
static inline _u8 fast_fetch8_inline(void) {
    _u32 p = pc++;
    return mem_page_read[p >> 16][p & 0xFFFF];
}

static inline _u16 fast_fetch16_inline(void) __attribute__((always_inline));
static inline _u16 fast_fetch16_inline(void) {
    _u32 p = pc;
    pc += 2;
    return READ16(&mem_page_read[p >> 16][p & 0xFFFF]);
}

static inline _u32 fast_fetch24_inline(void) __attribute__((always_inline));
static inline _u32 fast_fetch24_inline(void) {
    _u32 p = pc;
    pc += 3;
    const _u8* ptr = &mem_page_read[p >> 16][p & 0xFFFF];
    return (_u32)(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16));
}

static inline _u32 fast_fetch32_inline(void) __attribute__((always_inline));
static inline _u32 fast_fetch32_inline(void) {
    _u32 p = pc;
    pc += 4;
    return READ32(&mem_page_read[p >> 16][p & 0xFFFF]);
}

#undef FETCH8
#define FETCH8      fast_fetch8_inline()
#define fetch16()   fast_fetch16_inline()
#define fetch24()   fast_fetch24_inline()
#define fetch32()   fast_fetch32_inline()



void parityB(_u8 value);
void parityW(_u16 value);

void push8(_u8 data);
void push16(_u16 data);
void push32(_u32 data);

_u8 pop8(void);
_u16 pop16(void);
_u32 pop32(void);

//DIV ===============
_u16 generic_DIV_B(_u16 val, _u8 div);
_u32 generic_DIV_W(_u32 val, _u16 div);

//DIVS ===============
_u16 generic_DIVS_B(_s16 val, _s8 div);
_u32 generic_DIVS_W(_s32 val, _s16 div);

//ADD ===============
_u8  generic_ADD_B(_u8 dst, _u8 src);
_u16 generic_ADD_W(_u16 dst, _u16 src);
_u32 generic_ADD_L(_u32 dst, _u32 src);

//ADC ===============
_u8  generic_ADC_B(_u8 dst, _u8 src);
_u16 generic_ADC_W(_u16 dst, _u16 src);
_u32 generic_ADC_L(_u32 dst, _u32 src);

//SUB ===============
_u8  generic_SUB_B(_u8 dst, _u8 src);
_u16 generic_SUB_W(_u16 dst, _u16 src);
_u32 generic_SUB_L(_u32 dst, _u32 src);

//SBC ===============
_u8  generic_SBC_B(_u8 dst, _u8 src);
_u16 generic_SBC_W(_u16 dst, _u16 src);
_u32 generic_SBC_L(_u32 dst, _u32 src);


_u8 get_rr_Target(void);
_u8 get_RR_Target(void);

#endif