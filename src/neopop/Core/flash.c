#include "neopop.h"
#include "flash.h"
#include "mem.h"

// === EMULAÇÃO DA MÁQUINA DE ESTADOS FLASH (CHIP TOSHIBA 16Mbit) ===
#define NO_COMMAND              0x00
#define COMMAND_BYTE_PROGRAM    0xA0
#define COMMAND_BLOCK_ERASE     0x30
#define COMMAND_CHIP_ERASE      0x10
#define COMMAND_INFO_READ       0x90

extern int save_cooldown_timer; // Avisa que o timer existe
int g_flash_dirty = 0;

// =============================================================================
// EMULAÇÃO DO CHIP DE FLASH TOSHIBA TC58FV1600 (Usado pelo Sonic / SNK)
// =============================================================================
unsigned char currentWriteCycle = 1;
unsigned char currentCommand = 0x00;

unsigned char flashReadInfo(_u32 addr) {
    currentWriteCycle = 1;
    switch(addr & 0x03) {
        case 0: return 0x98; // Toshiba
        case 1: return 0x2F; // 16Mbit
        case 2: return 0x00; // Desprotegido
        default: return 0x80;
    }
}

void flashChipWrite(_u32 addr, _u8 data) {
    _u32 offset = addr & 0xFFFF;

    // Comando de Reset do Hardware tira o chip de qualquer modo na hora!
    if (data == 0xF0) { 
        currentWriteCycle = 1;
        currentCommand = 0x00;
        return;
    }

    switch(currentWriteCycle) {
        case 1:
            if (offset == 0x5555 && data == 0xAA) currentWriteCycle = 2;
            else { currentWriteCycle = 1; currentCommand = 0x00; }
            break;
        case 2:
            if (offset == 0x2AAA && data == 0x55) currentWriteCycle = 3;
            else { currentWriteCycle = 1; currentCommand = 0x00; }
            break;
        case 3:
            if (offset == 0x5555 && data == 0x80) currentWriteCycle = 4;
            else if (offset == 0x5555 && data == 0x90) {
                currentWriteCycle = 4;
                currentCommand = 0x90; // INFO READ
            }
            else if (offset == 0x5555 && data == 0xA0) {
                currentWriteCycle = 4;
                currentCommand = 0xA0; // PROGRAM
            }
            else { currentWriteCycle = 1; currentCommand = 0x00; }
            break;
        case 4:
            if (currentCommand == 0xA0) { // Salvando um Byte
                _u32 rom_offset = (addr >= 0x800000) ? (addr - 0x600000) : (addr - 0x200000);
                if (rom.data && rom_offset < rom.length) {
                    rom.data[rom_offset] &= data;
                    flash_write(addr & 0xFFFF00, 256);
                }
                currentWriteCycle = 1;
                currentCommand = 0x00;
            }
            else if (offset == 0x5555 && data == 0xAA) currentWriteCycle = 5;
            else { currentWriteCycle = 1; currentCommand = 0x00; }
            break;
        case 5:
            if (offset == 0x2AAA && data == 0x55) currentWriteCycle = 6;
            else { currentWriteCycle = 1; currentCommand = 0x00; }
            break;
        case 6:
            if (data == 0x30 || data == 0x50) { // BLOCK ERASE
                _u32 bank_base = (addr >= 0x800000) ? 0x800000 : 0x200000;
                _u32 relative_addr = addr - bank_base;
                
                _u32 block_size = 0x10000;
                _u32 block_base = relative_addr & 0xFF0000;
                
                // Trata as partições pequenas do final da fita (Boot Blocks)
                if (relative_addr >= 0x1F0000) {
                    _u32 boot = relative_addr - 0x1F0000;
                    if (boot < 0x8000) { block_size = 0x8000; block_base = 0x1F0000; }
                    else if (boot < 0xA000) { block_size = 0x2000; block_base = 0x1F8000; }
                    else if (boot < 0xC000) { block_size = 0x2000; block_base = 0x1FA000; }
                    else { block_size = 0x4000; block_base = 0x1FC000; }
                }
                
                if (rom.data && (block_base + block_size) <= rom.length) {
                    memset(rom.data + block_base, 0xFF, block_size);
                    flash_write(bank_base + block_base, block_size);
                }
            }
            currentWriteCycle = 1;
            currentCommand = 0x00;
            break;
        default:
            currentWriteCycle = 1;
            currentCommand = 0x00;
            break;
    }
}

#define FLASH_VALID_ID		0x0053

//Number of different flash blocks, this should be enough.

#define FLASH_MAX_BLOCKS	256

typedef struct
{
	//Flash Id
	_u16 valid_flash_id;		// = FLASH_VALID_ID
	
	_u16 block_count;			//Number of flash data blocks

	_u32 total_file_length;		// header + block[0 - block_count]

} FlashFileHeader;

typedef struct
{
	_u32 start_address;		// 24 bit address
	_u16 data_length;		// length of following data

	//Followed by data_length bytes of the actual data.

} FlashFileBlockHeader;

//-----------------------------------------------------------------------------
// Local Data
//-----------------------------------------------------------------------------
static FlashFileBlockHeader	blocks[256];
static _u16 block_count;

//=============================================================================

//-----------------------------------------------------------------------------
// optimise_blocks()
//-----------------------------------------------------------------------------
static void optimise_blocks(void)
{
	int i, j;

	// Bubble Sort by address
	for (i = 0; i < block_count - 1; i++)
	{
		for (j = i+1; j < block_count; j++)
		{
			//Swap?
			if (blocks[i].start_address > blocks[j].start_address)
			{
				_u32 temp32;
				_u16 temp16;

				temp32 = blocks[i].start_address;
				blocks[i].start_address = blocks[j].start_address;
				blocks[j].start_address = temp32;

				temp16 = blocks[i].data_length;
				blocks[i].data_length = blocks[j].data_length;
				blocks[j].data_length = temp16;
			}
		}
	}

	//Join contiguous blocks
	//Only advance 'i' if required, this will allow subsequent
	//blocks to be compared to the newly expanded block.
	for (i = 0; i < block_count - 1; /**/)
	{
		//Next block lies within (or borders) this one?
		if (blocks[i+1].start_address <=
			(blocks[i].start_address + blocks[i].data_length))
		{
			//Extend the first block
			blocks[i].data_length = 
				(_u16)((blocks[i+1].start_address + blocks[i+1].data_length) - 
				blocks[i].start_address);

			//Remove the next one.
			for (j = i+2; j < block_count; j++)
			{
				blocks[j-1].start_address = blocks[j].start_address;
				blocks[j-1].data_length = blocks[j].data_length;
			}
			block_count --;
		}
		else
		{
			i++;	// Try the next block
		}
	}
}

//=============================================================================

//-----------------------------------------------------------------------------
// flash_read()
//-----------------------------------------------------------------------------
void flash_read(void)
{
	FlashFileHeader header;
	_u8* flashdata, *fileptr;
	_u16 i;
	_u32 j;

	//Initialise the internal flash configuration
	block_count = 0;

	//Read flash buffer header
	if (system_io_flash_read((_u8*)&header, sizeof(FlashFileHeader)) == FALSE)
		return;	//Silent failure - no flash data yet.

	//Verify correct flash id
	if (header.valid_flash_id != FLASH_VALID_ID)
	{
		system_message(system_get_string(IDS_BADFLASH));
		return;
	}

	//Read the flash data
	flashdata = (_u8*)malloc(header.total_file_length * sizeof(_u8));
	system_io_flash_read(flashdata, header.total_file_length);

	//Read header
	block_count = header.block_count;
	fileptr = flashdata + sizeof(FlashFileHeader);

	//Copy blocks
	memory_unlock_flash_write = TRUE;
	for (i = 0; i < block_count; i++)
	{
		FlashFileBlockHeader* current = (FlashFileBlockHeader*)fileptr;
		fileptr += sizeof(FlashFileBlockHeader);
		
		blocks[i].start_address = current->start_address;
		blocks[i].data_length = current->data_length;

		//Copy data
		for (j = 0; j < blocks[i].data_length; j++)
		{
			storeB(blocks[i].start_address + j, *fileptr);
			fileptr++;
		}
	}
	memory_unlock_flash_write = FALSE;

	//Tidy up.
	free(flashdata);

	optimise_blocks();		//Optimise


	//Output block list...
/*	for (i = 0; i < block_count; i++)
		system_debug_message("flash block: %06X, %d bytes", 
			blocks[i].start_address, blocks[i].data_length);*/
}

//-----------------------------------------------------------------------------
// flash_write()
//-----------------------------------------------------------------------------
void flash_write(_u32 start_address, _u16 length)
{
	_u16 i;

	//Now we need a new flash command before the next flash write will work!
	memory_flash_command = FALSE;

//	system_debug_message("flash write: %06X, %d bytes", start_address, length);

	for (i = 0; i < block_count; i++)
	{
		//Got this block with enough bytes to cover it
		if (blocks[i].start_address == start_address &&
			blocks[i].data_length >= length)
		{
			return; //Nothing to do, block already registered.
		}

		//Got this block with but it's length is too short
		if (blocks[i].start_address == start_address &&
			blocks[i].data_length < length)
		{
			blocks[i].data_length = length;	//Enlarge block updating.
			return;
		}
	}

	// New block needs to be added
	blocks[block_count].start_address = start_address;
	blocks[block_count].data_length = length;
	block_count++;
	g_flash_dirty = 1; 
    save_cooldown_timer = 0; // <--- Toda vez que o jogo escreve, o timer volta pro zero!
}

//-----------------------------------------------------------------------------
// flash_commit()
//-----------------------------------------------------------------------------
void flash_commit(void)
{
	int i;
	FlashFileHeader header;
	_u8 *flashdata, *fileptr;

	//No flash data?
	if (block_count == 0)
		return;

	//Optimise before writing
	optimise_blocks();

	//Build a header;
	header.valid_flash_id = FLASH_VALID_ID;
	header.block_count = block_count;
	header.total_file_length = sizeof(FlashFileHeader);
	for (i = 0; i < block_count; i++)
	{
		header.total_file_length += sizeof(FlashFileBlockHeader);
		header.total_file_length += blocks[i].data_length;
	}

	//Write the flash data
	flashdata = (_u8*)malloc(header.total_file_length * sizeof(_u8));

	//Copy header
	memcpy(flashdata, &header, sizeof(FlashFileHeader));
	fileptr = flashdata + sizeof(FlashFileHeader);

	//Copy blocks
	for (i = 0; i < block_count; i++)
	{
		_u32 j;

		memcpy(fileptr, &blocks[i], sizeof(FlashFileBlockHeader));
		fileptr += sizeof(FlashFileBlockHeader);

		//Copy data
		for (j = 0; j < blocks[i].data_length; j++)
		{
			*fileptr = loadB(blocks[i].start_address + j);
			fileptr++;
		}
	}

	//Try to Write flash buffer
	system_io_flash_write(flashdata, header.total_file_length);

	free(flashdata);
	g_flash_dirty = 0;
}

//=============================================================================
