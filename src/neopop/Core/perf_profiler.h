#ifndef PERF_PROFILER_H
#define PERF_PROFILER_H

// =============================================================================
// CONTROLE DE DEBUG
// 0 = Desativado (Zero overhead)
// 1 = Medição Padrão (Média a cada 60 frames)
// 2 = Detalhado Silencioso (Só cospe no console se o frame estourar)
// =============================================================================
#define PERF_DEBUG_MODE 0

// Limiar para disparar o alarme (em milissegundos). Acima de 16.68ms o PS2 cai pra 30 FPS.
#define PERF_SPIKE_THRESHOLD_MS 16.68f

typedef enum {
    PROF_CPU_TLCS = 0,
    PROF_CPU_Z80,
    PROF_GFX_SCANLINE,
    PROF_GS_DMA,
    PROF_AUDIO_RENDER,
    PROF_MAX
} ProfilerZone;

#if PERF_DEBUG_MODE > 0
void Profiler_FrameStart(void);
void Profiler_ZoneStart(ProfilerZone zone);
void Profiler_ZoneEnd(ProfilerZone zone);
void Profiler_MarkBeforeFlip(void);
void Profiler_FrameEndAndReport(void);
#else
#define Profiler_FrameStart()
#define Profiler_ZoneStart(zone)
#define Profiler_ZoneEnd(zone)
#define Profiler_MarkBeforeFlip()
#define Profiler_FrameEndAndReport()
#endif

#endif // PERF_PROFILER_H