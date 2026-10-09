#ifndef NEOPOP_PS2_H
#define NEOPOP_PS2_H

#include <tamtypes.h>
#include <gsKit.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa estruturas do emulador (BIOS HLE, tabelas de cor e textura)
int  NeoPop_PS2_Init(GSGLOBAL *gsGlobal);

// Carrega a ROM e executa o loop do jogo até o jogador sair (SELECT + START)
void NeoPop_PS2_Run(GSGLOBAL *gsGlobal, const char *rom_path);

// Libera buffers alocados para o emulador
void NeoPop_PS2_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // NEOPOP_PS2_H