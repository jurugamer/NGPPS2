#ifndef __FLASH__
#define __FLASH__
//=============================================================================

void flash_read(void);

//Marks flash blocks for saving.
void flash_write(_u32 start_address, _u16 length);

//Stores the flash data
void flash_commit(void);

extern unsigned char currentCommand;
unsigned char flashReadInfo(_u32 addr);
void flashChipWrite(_u32 addr, _u8 data);

//=============================================================================
#endif
