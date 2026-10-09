#include <tamtypes.h>
#include <stdio.h>
#include "perf_profiler.h"
#include "Z80_interface.h"
#if PERF_DEBUG_MODE > 0

extern u32 pc; // PC do TLCS-900H

static u32 s_frame_start_tick = 0;
static u32 s_before_flip_tick = 0;
static u32 s_zone_start[PROF_MAX];
static u32 s_zone_total[PROF_MAX];

static int s_frame_counter = 0;

static inline u32 read_mips_count(void) {
    u32 count;
    __asm__ __volatile__("mfc0 %0, $9" : "=r"(count));
    return count;
}

void Profiler_FrameStart(void) {
    s_frame_start_tick = read_mips_count();
    for (int i = 0; i < PROF_MAX; i++) {
        s_zone_total[i] = 0;
    }
}

void Profiler_ZoneStart(ProfilerZone zone) {
#if PERF_DEBUG_MODE == 2
    s_zone_start[zone] = read_mips_count();
#endif
}

void Profiler_ZoneEnd(ProfilerZone zone) {
#if PERF_DEBUG_MODE == 2
    s_zone_total[zone] += (read_mips_count() - s_zone_start[zone]);
#endif
}

void Profiler_MarkBeforeFlip(void) {
    s_before_flip_tick = read_mips_count();
}

void Profiler_FrameEndAndReport(void) {
    u32 after_flip_tick = read_mips_count();

    float work_ms = (float)(s_before_flip_tick - s_frame_start_tick) / 294912.0f;
    float wait_ms = (float)(after_flip_tick - s_before_flip_tick) / 294912.0f;
    float total_ms = work_ms + wait_ms;

#if PERF_DEBUG_MODE == 1
    s_frame_counter++;
    if (s_frame_counter >= 60) {
        printf("[PERF] Trabalho: %.2f ms | Espera VSync: %.2f ms | Total: %.2f ms (%.1f FPS)\n",
               work_ms, wait_ms, total_ms, 1000.0f / total_ms);
        s_frame_counter = 0;
    }

#elif PERF_DEBUG_MODE == 2
    if (work_ms >= PERF_SPIKE_THRESHOLD_MS) {
        float tlcs_ms  = (float)s_zone_total[PROF_CPU_TLCS] / 294912.0f;
        float z80_ms   = (float)s_zone_total[PROF_CPU_Z80] / 294912.0f;
        float gfx_ms   = (float)s_zone_total[PROF_GFX_SCANLINE] / 294912.0f;
        float dma_ms   = (float)s_zone_total[PROF_GS_DMA] / 294912.0f;
        float audio_ms = (float)s_zone_total[PROF_AUDIO_RENDER] / 294912.0f;

         u16 z80_pc = Z80_regs.PC.W;

        printf("\n================ [ALERTA DE QUEDA DE DESEMPENHO] ================\n");
        printf("Tempo de Trabalho Estourado: %.2f ms (Limite: %.2f ms) -> Total: %.2f ms\n", 
               work_ms, PERF_SPIKE_THRESHOLD_MS, total_ms);
        printf("CULPADOS DETALHADOS:\n");
        printf("  [1] TLCS-900H (CPU):     %5.2f ms (%4.1f%%)\n", tlcs_ms, (tlcs_ms / work_ms) * 100.0f);
        printf("  [2] CZ80 (Audio CPU):    %5.2f ms (%4.1f%%)\n", z80_ms, (z80_ms / work_ms) * 100.0f);
        printf("  [3] Render Gráfico:     %5.2f ms (%4.1f%%)\n", gfx_ms, (gfx_ms / work_ms) * 100.0f);
        printf("  [4] DMA/Upload Textura:  %5.2f ms (%4.1f%%)\n", dma_ms, (dma_ms / work_ms) * 100.0f);
        printf("  [5] Sintetizador Audio:  %5.2f ms (%4.1f%%)\n", audio_ms, (audio_ms / work_ms) * 100.0f);
        printf("ESTADO DO HARDWARE NO INSTANTE DO SPIKE:\n");
        printf("  -> TLCS PC: 0x%06X | Z80 PC: 0x%04X\n", (unsigned int)pc, z80_pc);
        printf("=================================================================\n\n");
    }
#endif
}

#endif