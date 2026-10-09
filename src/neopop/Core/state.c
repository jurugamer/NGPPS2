#include "neopop.h"
#include "state.h"
#include "TLCS900h_registers.h"
#include "interrupt.h"
#include "dma.h"
#include "mem.h"
#include <string.h>

// Alocado estaticamente na memória global (BSS) - ZERO bytes consumidos da pilha!
static NEOPOPSTATE0050 s_neopop_state;

static void read_state_0050(char* filename);

//-----------------------------------------------------------------------------
// state_restore()
//-----------------------------------------------------------------------------
BOOL state_restore(char* filename)
{
	_u16 version;

	if (system_io_state_read(filename, (_u8*)&version, sizeof(_u16)))
	{
		switch(version)
		{
		case 0x0050:	read_state_0050(filename);	break;

		default:
			system_message(system_get_string(IDS_BADSTATE));
			return FALSE;
		}

#ifdef NEOPOP_DEBUG
		system_debug_message("Restoring State ...");
		system_debug_refresh();
#endif

		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// state_store()
//-----------------------------------------------------------------------------
BOOL state_store(char* filename)
{
	int i,j;

	// Preenche a estrutura estática sem sobrecarregar a pilha
	s_neopop_state.valid_state_id = 0x0050;
	memcpy(&s_neopop_state.header, rom_header, sizeof(RomHeader));

	s_neopop_state.eepromStatusEnable = eepromStatusEnable;

	// TLCS-900h Registers
	s_neopop_state.pc = pc;
	s_neopop_state.sr = sr;
	s_neopop_state.f_dash = f_dash;

	for (i = 0; i < 4; i++)
	{
		s_neopop_state.gpr[i] = gpr[i];
		for (j = 0; j < 4; j++)
			s_neopop_state.gprBank[i][j] = gprBank[i][j];
	}

	// Z80 Registers
	memcpy(&s_neopop_state.Z80_regs, &Z80_regs, sizeof(Z80));

	// Sound Chips
	memcpy(&s_neopop_state.toneChip, &toneChip, sizeof(SoundChip));
	memcpy(&s_neopop_state.noiseChip, &noiseChip, sizeof(SoundChip));

	// Memory
	memcpy(&s_neopop_state.ram, ram, 0xC000);

	// Timers
	s_neopop_state.timer_hint = timer_hint;

	for (i = 0; i < 4; i++)	// Up-counters
		s_neopop_state.timer[i] = timer[i];

	s_neopop_state.timer_clock0 = timer_clock0;
	s_neopop_state.timer_clock1 = timer_clock1;
	s_neopop_state.timer_clock2 = timer_clock2;
	s_neopop_state.timer_clock3 = timer_clock3;

	// DMA
	for (i = 0; i < 4; i++)
	{
		s_neopop_state.dmaS[i] = dmaS[i];
		s_neopop_state.dmaD[i] = dmaD[i];
		s_neopop_state.dmaC[i] = dmaC[i];
		s_neopop_state.dmaM[i] = dmaM[i];
	}

#ifdef NEOPOP_DEBUG
	system_debug_message("Saving State ...");
#endif

	return system_io_state_write(filename, (_u8*)&s_neopop_state, sizeof(NEOPOPSTATE0050));
}

//=============================================================================

static void read_state_0050(char* filename)
{
	int i,j;

	if (system_io_state_read(filename, (_u8*)&s_neopop_state, sizeof(NEOPOPSTATE0050)))
	{
		// Verifica se pertence à ROM correta
		if (memcmp(rom_header, &s_neopop_state.header, sizeof(RomHeader)) != 0)
		{
			system_message(system_get_string(IDS_WRONGROM));
			return;
		}

		// Aplica o estado
		reset();

		eepromStatusEnable = s_neopop_state.eepromStatusEnable;

		// TLCS-900h Registers
		pc = s_neopop_state.pc;
		sr = s_neopop_state.sr;				changedSP();
		f_dash = s_neopop_state.f_dash;

		eepromStatusEnable = s_neopop_state.eepromStatusEnable;

		for (i = 0; i < 4; i++)
		{
			gpr[i] = s_neopop_state.gpr[i];
			for (j = 0; j < 4; j++)
				gprBank[i][j] = s_neopop_state.gprBank[i][j];
		}

		// Timers
		timer_hint = s_neopop_state.timer_hint;

		for (i = 0; i < 4; i++)	// Up-counters
			timer[i] = s_neopop_state.timer[i];

		timer_clock0 = s_neopop_state.timer_clock0;
		timer_clock1 = s_neopop_state.timer_clock1;
		timer_clock2 = s_neopop_state.timer_clock2;
		timer_clock3 = s_neopop_state.timer_clock3;

		// Z80 Registers
		memcpy(&Z80_regs, &s_neopop_state.Z80_regs, sizeof(Z80));

		// Sound Chips
		memcpy(&toneChip, &s_neopop_state.toneChip, sizeof(SoundChip));
		memcpy(&noiseChip, &s_neopop_state.noiseChip, sizeof(SoundChip));

		// DMA
		for (i = 0; i < 4; i++)
		{
			dmaS[i] = s_neopop_state.dmaS[i];
			dmaD[i] = s_neopop_state.dmaD[i];
			dmaC[i] = s_neopop_state.dmaC[i];
			dmaM[i] = s_neopop_state.dmaM[i];
		}

		// Memory
		memcpy(ram, &s_neopop_state.ram, 0xC000);
	}
}