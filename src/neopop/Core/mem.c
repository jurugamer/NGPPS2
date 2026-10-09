#include "neopop.h"
#include "TLCS900h_registers.h"
#include "Z80_interface.h"
#include "bios.h"
#include "gfx.h"
#include "mem.h"
#include "interrupt.h"
#include "sound.h"
#include "flash.h"

uint8_t gfx_pal_dirty = 1;

BOOL memory_flash_error = FALSE;
BOOL memory_unlock_flash_write = FALSE;
BOOL memory_flash_command = FALSE;
BOOL eepromStatusEnable = FALSE;

// Arrays com padding extra para evitar vazamento em leituras L/W no limite
__attribute__((aligned(64))) _u8 ram[0x10000];
__attribute__((aligned(64))) _u8 dummy_page_read[0x10000];
__attribute__((aligned(64))) _u8 dummy_page_write[0x10000];

void post_write(_u32 address)
{
    switch (address)
    {
    case 0x20:
    {
        const _u8 trun = ram[0x20];
        if (!(trun & 0x01)) timer[0] = 0;
        if (!(trun & 0x02)) timer[1] = 0;
        if (!(trun & 0x04)) timer[2] = 0;
        if (!(trun & 0x08)) timer[3] = 0;
        break;
    }
    case 0xA0:
        if (READ16(ram + 0xB8) == 0xAA55)
            Write_SoundChipNoise(ram[0xA0]);
        break;
    case 0xA1:
        if (READ16(ram + 0xB8) == 0xAA55)
            Write_SoundChipTone(ram[0xA1]);
        break;
    case 0xA2:
        dac_write();
        break;
    case 0xBA:
        Z80_nmi();
        break;
    default:
        break;
    }
}

// =============================================================================
// CONTROLADOR DE GRAVAÇÃO FLASH 
// =============================================================================
void* get_flash_ptr(_u32 address)
{
    if (memory_unlock_flash_write)
    {
        if (rom.data && address >= ROM_START && address <= ROM_END) {
            _u32 offset = address - ROM_START;
            if (offset < rom.length) return rom.data + offset;
        } 
        else if (rom.data && address >= HIROM_START && address <= HIROM_END) {
            if (rom.length > 0x200000) {
                _u32 offset = address - HIROM_START;
                if ((0x200000 + offset) < rom.length) return rom.data + 0x200000 + offset;
            }
        }
        if ((address & 0xFF0000) != 0x200000 && (address & 0xFF0000) != 0x800000) {
            memory_flash_error = TRUE;
        }
        return NULL;
    }

    if (address == 0x202AAA || address == 0x205555 || 
        address == 0x20AAAA || address == 0x205554 ||
        address == 0x802AAA || address == 0x805555 || 
        address == 0x80AAAA || address == 0x805554)
    {
        memory_flash_command = TRUE;
        return NULL; 
    }

    if (address == 0x220000 || address == 0x230000 || address == 0x820000 || address == 0x830000)
    {
        eepromStatusEnable = TRUE;
        return NULL;
    }

    if (memory_flash_command)
    {
        flash_write(address & 0xFFFF00, 256);
        memory_flash_command = FALSE;
        
        if (rom.data && address >= ROM_START && address <= ROM_END) {
            _u32 offset = address - ROM_START;
            if (offset < rom.length) return rom.data + offset;
        } 
        else if (rom.data && address >= HIROM_START && address <= HIROM_END) {
            if (rom.length > 0x200000) {
                _u32 offset = address - HIROM_START;
                if ((0x200000 + offset) < rom.length) return rom.data + 0x200000 + offset;
            }
        }
    }

    return NULL;
}

static const _u8 systemMemory[] = 
{
	0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0xFF,
	0x34, 0x3C, 0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x3F, 0xFF, 0x2D, 0x01, 0xFF, 0xFF, 0x03, 0xB2,
	0x80, 0x00, 0x01, 0x90, 0x03, 0xB0, 0x90, 0x62, 0x05, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x4C, 0x4C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x20, 0xFF, 0x80, 0x7F,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x20, 0x69, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x03, 0x03, 0x02, 0x00, 0x00, 0x4E,
	0x02, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void reset_memory(void)
{
    memset(ram, 0, sizeof(ram));

    // Inicializa a LUT de Página Fantasma com segurança
    memset(dummy_page_read, 0xFF, sizeof(dummy_page_read));
    memset(dummy_page_write, 0x00, sizeof(dummy_page_write));

    for (int i = 0; i < 256; i++) {
        mem_page_read[i] = dummy_page_read;
        mem_page_write[i] = dummy_page_write;
    }

    mem_page_read[0x00]  = ram;
    mem_page_write[0x00] = ram;

    if (rom.data) {
        int rom_pages = (rom.length + 0xFFFF) >> 16;
        if (rom_pages > 32) rom_pages = 32;

        for (int i = 0; i < rom_pages; i++) {
            mem_page_read[0x20 + i] = rom.data + (i << 16);
        }

        if (rom.length > 0x200000) {
            int hi_pages = ((rom.length - 0x200000) + 0xFFFF) >> 16;
            for (int i = 0; i < hi_pages; i++) {
                mem_page_read[0x80 + i] = rom.data + 0x200000 + (i << 16);
            }
        }
    }

    mem_page_read[0xFF] = bios;

    memcpy(ram, systemMemory, sizeof(systemMemory));

    if (rom.data) {
        WRITE32(ram + 0x6C00, rom_header->startPC);
        WRITE16(ram + 0x6E82, rom_header->catalog);
        WRITE16(ram + 0x6C04, rom_header->catalog);
        ram[0x6E84] = ram[0x6C06] = rom_header->subCatalog;
        memcpy(ram + 0x6C08, rom.data + 0x24, 12);
        
        ram[0x6C58] = 0x01; 
        ram[0x6C59] = (rom.length > 0x200000) ? 0x01 : 0x00; 
        ram[0x6C55] = 1;    
    }

    ram[0x6F80] = 0xFF; 
    ram[0x6F81] = 0x03; 
    ram[0x6F84] = 0x40; 
    ram[0x6F85] = 0x00; 
    ram[0x6F86] = 0x00; 
    ram[0x6F87] = (_u8)language_english;

    ram[0x6F91] = (rom.data) ? rom_header->mode : 0x10;
    ram[0x6F92] = ram[0x6C58]; 
    ram[0x6F93] = ram[0x6C59]; 
    ram[0x6F95] = ram[0x6F91]; 
    ram[0x6F91] = ram[0x6F95] = (rom.data) ? rom_header->mode : 0x10;

    for (int i = 0; i < 0x12; i++)
        WRITE32(ram + 0x6FB8 + (i * 4), 0x00FF23DF);

    ram[0x8000] = 0xC0;
    ram[0x8004] = 0xFF;
    ram[0x8005] = 0xFF;
    ram[0x8006] = 0xC6;
    ram[0x8118] = 0x80;
    ram[0x83E0] = 0xFF;
    ram[0x83E1] = 0x0F;
    ram[0x83F0] = 0xFF;
    ram[0x83F1] = 0x0F;
    ram[0x8400] = 0xFF;
    ram[0x8402] = 0x80;
	interlace = 0;
    eepromStatusEnable = FALSE;
    memory_flash_command = FALSE;
}