#include "neopop.h"
#include "TLCS900h_interpret.h"
#include "TLCS900h_registers.h"
#include "Z80_interface.h"
#include "interrupt.h"
#include "mem.h"

BOOL language_english;
COLOURMODE system_colour;
_u8 frameskip_count;

#include "perf_profiler.h"

void emulate(void)
{
    static int s_z80_accumulated = 0;
    static int s_cpu_overshoot = 0;

    int slice_ticks = s_cpu_overshoot;

    // 1. CPU Principal roda livre sem o peso do Z80:
    Profiler_ZoneStart(PROF_CPU_TLCS);
    while (slice_ticks < TIMER_HINT_RATE)
    {
        slice_ticks += TLCS900h_interpret();
        slice_ticks += TLCS900h_interpret();
        slice_ticks += TLCS900h_interpret();
        slice_ticks += TLCS900h_interpret();
    }
    Profiler_ZoneEnd(PROF_CPU_TLCS);

    s_cpu_overshoot = slice_ticks - TIMER_HINT_RATE;

    // 2. Gráficos e Timers da Linha:
    updateTimers_slow(slice_ticks);

    // 3. CPU Z80 roda FORA do while (em lote):
    if (Z80ACTIVE)
    {
        s_z80_accumulated += (slice_ticks >> 1);
        if (s_z80_accumulated >= 2000 || ram[0x8009] >= 199)
        {
            Profiler_ZoneStart(PROF_CPU_Z80);
            Z80_execute_slice(s_z80_accumulated);
            Profiler_ZoneEnd(PROF_CPU_Z80);
            
            s_z80_accumulated = 0;
        }
    }
}



#include <stdarg.h>

void instruction_error(char* vaMessage, ...)
{
    char message[256];
    va_list vl;

    va_start(vl, vaMessage);
    vsnprintf(message, sizeof(message), vaMessage, vl);
    va_end(vl);

    system_message("[CPU Error PC:%06X] %s", pc, message);
}