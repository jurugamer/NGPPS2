#ifndef __MEM_H__
#define __MEM_H__

#include <stdint.h>
#include <string.h>

#define ROM_START	0x200000
#define ROM_END		0x3FFFFF
#define HIROM_START	0x800000
#define HIROM_END	0x9FFFFF
#define BIOS_START	0xFF0000
#define BIOS_END	0xFFFFFF

#ifndef RAM_END
#define RAM_END		0x00BFFF
#endif

extern BOOL memory_flash_error;
extern BOOL memory_unlock_flash_write;
extern BOOL memory_flash_command;
extern BOOL eepromStatusEnable;

extern uint8_t gfx_pal_dirty;
extern _u32 timer_hint;
extern _u8 ram[0x10000];

void Z80_catchup_handshake(void);

typedef struct { uint16_t v; } __attribute__((packed)) mem_u16_p;
typedef struct { uint32_t v; } __attribute__((packed)) mem_u32_p;

#define READ16(ptr)        (((const mem_u16_p*)(ptr))->v)
#define READ32(ptr)        (((const mem_u32_p*)(ptr))->v)
#define WRITE16(ptr, val)  (((mem_u16_p*)(ptr))->v = (val))
#define WRITE32(ptr, val)  (((mem_u32_p*)(ptr))->v = (val))

#include "spr_map.h"
#define mem_page_read  SPR_MEM_PAGE_READ
#define mem_page_write SPR_MEM_PAGE_WRITE

void reset_memory(void);
void post_write(_u32 address);
void* get_flash_ptr(_u32 address);

static inline _u8 loadB(_u32 address) {
    if (__builtin_expect(address <= 0xFFFF, 1)) {
        // Se a CPU principal estiver lendo a porta de resposta do som:
        if (__builtin_expect(address == 0xBC, 0)) {
            Z80_catchup_handshake(); // Faz o Z80 responder antes de ler!
            return ram[0xBC];
        }

        if (__builtin_expect(address == 0x8008, 0)) {
            int diff = 114 - (int)timer_hint;
            if (diff < 0) diff = -diff;
            return (_u8)(ram[0x8008] = (_u8)(diff >> 2));
        }
        return ram[address];
    }

    if (__builtin_expect(address <= 0xFFFF, 1)) {
        if (__builtin_expect(address == 0x8008, 0)) {
            int diff = 114 - (int)timer_hint;
            if (diff < 0) diff = -diff;
            return (_u8)(ram[0x8008] = (_u8)(diff >> 2));
        }
        return ram[address];
    }
    _u32 hi21 = address >> 21;
    if (__builtin_expect(hi21 == 1, 1)) {
        if (__builtin_expect(eepromStatusEnable, 0)) {
            eepromStatusEnable = FALSE;
            if (address == 0x220000 || address == 0x230000) return 0xFF; 
        }
        return rom.data[address - 0x200000];
    }
    if (__builtin_expect(hi21 == 4, 0)) { // HIROM
        if (__builtin_expect(eepromStatusEnable, 0)) {
            eepromStatusEnable = FALSE;
            if (address == 0x820000 || address == 0x830000) return 0xFF; 
        }
        return rom.data[0x200000 + (address - 0x800000)];
    }
    address &= 0xFFFFFF;
    const _u8* const page = mem_page_read[address >> 16];
    return __builtin_expect(page != NULL, 1) ? page[address & 0xFFFF] : 0xFF;
}

static inline _u16 loadW(_u32 address) {
    if (__builtin_expect(address <= 0xFFFE, 1)) return READ16(&ram[address]);
    _u32 hi21 = address >> 21;
    if (__builtin_expect(hi21 == 1, 1)) {
        if (__builtin_expect(eepromStatusEnable, 0)) {
            eepromStatusEnable = FALSE;
            if (address == 0x220000 || address == 0x230000) return 0xFFFF;
        }
        return READ16(rom.data + (address - 0x200000));
    }
    if (__builtin_expect(hi21 == 4, 0)) { // HIROM
        if (__builtin_expect(eepromStatusEnable, 0)) {
            eepromStatusEnable = FALSE;
            if (address == 0x820000 || address == 0x830000) return 0xFFFF; 
        }
        return READ16(rom.data + 0x200000 + (address - 0x800000));
    }
    address &= 0xFFFFFF;
    const _u8* const page = mem_page_read[address >> 16];
    return __builtin_expect(page != NULL, 1) ? READ16(&page[address & 0xFFFF]) : 0xFFFF;
}

static inline _u32 loadL(_u32 address) {
    if (__builtin_expect(address <= 0xFFFC, 1)) return READ32(&ram[address]);
    _u32 hi21 = address >> 21;
    if (__builtin_expect(hi21 == 1, 1)) {
        if (__builtin_expect(eepromStatusEnable, 0)) {
            eepromStatusEnable = FALSE;
            if (address == 0x220000 || address == 0x230000) return 0xFFFFFFFF;
        }
        return READ32(rom.data + (address - 0x200000));
    }
    if (__builtin_expect(hi21 == 4, 0)) { // HIROM
        if (__builtin_expect(eepromStatusEnable, 0)) {
            eepromStatusEnable = FALSE;
            if (address == 0x820000 || address == 0x830000) return 0xFFFFFFFF; 
        }
        return READ32(rom.data + 0x200000 + (address - 0x800000));
    }
    address &= 0xFFFFFF;
    const _u8* const page = mem_page_read[address >> 16];
    return __builtin_expect(page != NULL, 1) ? READ32(&page[address & 0xFFFF]) : 0xFFFFFFFF;
}

static inline _u32 load24(_u32 address) {
    return loadB(address) | (loadB(address + 1) << 8) | (loadB(address + 2) << 16);
}

// ==========================================
// FUNÇÕES DE ESCRITA ULTRA-RÁPIDAS (ZERO JAL / ZERO OVERHEAD)
// ==========================================
static inline void storeB(_u32 address, _u8 data) {
    if (__builtin_expect(address <= 0xFFFF, 1)) {
        ram[address] = data;
        // Só chama post_write se for registrador de hardware (<= 0xBA). 
        // 99.99% das escritas pulam isso sem gastar ciclos!
        if (__builtin_expect(address <= 0xBA, 0)) {
            post_write(address);
        }
        return;
    }
    
    // Caminho lento e raro (ROM/Flash): só executa na hora de salvar!
    _u32 hi21 = address >> 21;
    if (__builtin_expect(hi21 == 1 || hi21 == 4, 0)) {
        _u8 *ptr = (_u8*)get_flash_ptr(address);
        if (ptr) *ptr = data;
        return;
    }
    
    address &= 0xFFFFFF;
    _u8* const page = mem_page_write[address >> 16];
    if (__builtin_expect(page != NULL, 1)) {
        page[address & 0xFFFF] = data;
        if (__builtin_expect(address <= 0xBA, 0)) post_write(address);
    }
}

static inline void storeW(_u32 address, _u16 data) {
    if (__builtin_expect(address <= 0xFFFE, 1)) {
        WRITE16(&ram[address], data);
        if (__builtin_expect(address <= 0xBA, 0)) {
            post_write(address);
        }
        return;
    }
    
    _u32 hi21 = address >> 21;
    if (__builtin_expect(hi21 == 1 || hi21 == 4, 0)) {
        _u8 *ptr = (_u8*)get_flash_ptr(address);
        if (ptr) {
            ptr[0] = data & 0xFF;
            ptr[1] = data >> 8;
        }
        return;
    }
    
    address &= 0xFFFFFF;
    _u8* const page = mem_page_write[address >> 16];
    if (__builtin_expect(page != NULL, 1)) {
        WRITE16(&page[address & 0xFFFF], data);
        if (__builtin_expect(address <= 0xBA, 0)) post_write(address);
    }
}

static inline void storeL(_u32 address, _u32 data) {
    if (__builtin_expect(address <= 0xFFFC, 1)) {
        WRITE32(&ram[address], data);
        if (__builtin_expect(address <= 0xBA, 0)) {
            post_write(address);
        }
        return;
    }
    
    _u32 hi21 = address >> 21;
    if (__builtin_expect(hi21 == 1 || hi21 == 4, 0)) {
        _u8 *ptr = (_u8*)get_flash_ptr(address);
        if (ptr) {
            ptr[0] = data & 0xFF;
            ptr[1] = (data >> 8) & 0xFF;
            ptr[2] = (data >> 16) & 0xFF;
            ptr[3] = (data >> 24) & 0xFF;
        }
        return;
    }
    
    address &= 0xFFFFFF;
    _u8* const page = mem_page_write[address >> 16];
    if (__builtin_expect(page != NULL, 1)) {
        WRITE32(&page[address & 0xFFFF], data);
        if (__builtin_expect(address <= 0xBA, 0)) post_write(address);
    }
}

static inline void store24(_u32 address, _u32 data) {
    storeB(address, (_u8)(data & 0xFF));
    storeB(address + 1, (_u8)((data >> 8) & 0xFF));
    storeB(address + 2, (_u8)((data >> 16) & 0xFF));
}

#endif