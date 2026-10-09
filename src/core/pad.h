#ifndef PAD_H
#define PAD_H

#include <tamtypes.h>

typedef struct {
    u32 held;
    u32 pressed;
} PadInput;

typedef PadInput PadState;

void Pad_Init(void);
void Pad_InitUSB(void);
void Pad_SetAnalogMode(int enable);
void Pad_WaitRelease(void); // <-- Drena o buffer e espera soltar todos os botões
void Pad_Update(PadInput *input);

#define pad_init_controller Pad_Init
#define pad_update Pad_Update

#endif