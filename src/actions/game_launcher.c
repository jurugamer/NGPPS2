#include "game_launcher.h"
#include "../neopop/neopop_ps2.h"
#include <gsKit.h>
#include <stdio.h>

// Puxa o gsGlobal definido no main.c sem precisar passá-lo como argumento
extern GSGLOBAL *gsGlobal;

void GameLauncher_Launch(const LauncherItem *item) {
    if (!item) return;

    // Repassa gsGlobal e o caminho da ROM para o emulador
    NeoPop_PS2_Run(gsGlobal, item->filepath);
}