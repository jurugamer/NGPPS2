#include "neopop.h"
#include "flash.h"
#include "interrupt.h"
#include <stdint.h>
RomInfo rom;
RomHeader* rom_header;

static void rom_hack(void)
{

	gfx_hack = FALSE;

	const uint16_t cat = rom_header->catalog;
	const char* const name = rom.name;

	// Graphics timing hacks: Checagem rápida de números inteiros primeiro
	// O strstr só é executado se os IDs numéricos não baterem
	if (cat == 89  || cat == 149 || cat == 100 || cat == 2   || 
	    cat == 57  || cat == 133 || cat == 148 || cat == 105 || 
	    cat == 48  || cat == 102 || cat == 1   || cat == 35  ||
	    strstr(name, "SONIC")    || strstr(name, "LAST BLADE") || 
	    strstr(name, "GEKKA")    || strstr(name, "NEOGEO CUP") || 
	    strstr(name, "OGRE")     || strstr(name, "ROCKMAN")    || 
	    strstr(name, "MEGAMAN")  || strstr(name, "SNK")        || 
	    strstr(name, "SAMURAI")  || strstr(name, "WRESTLE")    || 
	    strstr(name, "KOF"))
	{
		gfx_hack = TRUE;
	}

	// Patches específicos
	if (cat == 0 || strstr(name, "NEO-NEO"))
		rom.data[0x23] = 0x10;

	if (cat == 4660 || strstr(name, "COOL"))
		rom.data[0x23] = 0x10;

	if (cat == 51 || strstr(name, "MAHJONG"))
		rom.data[0x23] = 0x00;

	if (cat == 65 || strstr(name, "PUYO"))
		memset(&rom.data[0x8F0], 0, 12); // Substituído loop por memset

	if (cat == 97 || strstr(name, "2ND") || strstr(name, "SLUG 2"))
	{
		rom.data[0x1F] = 0xFF;
		rom.data[0x8DDF8] = 0xF0;
	}
}

void rom_loaded(void)
{
	rom_header = (RomHeader*)(rom.data);

	for (int i = 0; i < 12; i++)
	{
		uint8_t c = rom_header->name[i];
		rom.name[i] = (c >= 32 && c < 128) ? c : ' ';
	}
	rom.name[12] = '\0';

	rom_hack();
	flash_read();
}

void rom_unload(void)
{
	if (rom.data)
	{
		flash_commit();
		free(rom.data);
		rom.data = NULL;
		rom.length = 0;
		rom_header = NULL;

		memset(rom.name, 0, sizeof(rom.name));
		memset(rom.filename, 0, sizeof(rom.filename));
		reset();
	}
}