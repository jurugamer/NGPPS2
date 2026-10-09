#ifndef TIM2_UTIL_H
#define TIM2_UTIL_H

#include <tamtypes.h>

// Salva com respeito à área ativa e ao pitch do framebuffer
int TIM2_SaveFrom16Bit(const char *filepath, const u16 *src_pixels, int active_w, int active_h, int pitch_w);

#endif