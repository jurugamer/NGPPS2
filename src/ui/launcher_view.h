#ifndef LAUNCHER_VIEW_H
#define LAUNCHER_VIEW_H

#include <tamtypes.h>
#include <gsKit.h>

typedef enum {
    ATLAS_MAIN,   // spritesheet_atlas.tm2 (Menu principal / Lista de jogos)
    ATLAS_CONFIG, // config_atlas.tm2      (Menu de Configurações Gerais)
    ATLAS_GAME    // game_atlas.tm2        (Opções de Jogo & Menu de Pausa in-game)
} MenuAtlasType;

// Função para trocar o atlas ativo instantaneamente via DMA
void LauncherView_SetAtlas(GSGLOBAL *gsGlobal, MenuAtlasType type);

// Inicialização com os 3 caminhos de atlas
int  LauncherView_Init(GSGLOBAL *gsGlobal, 
                       const char *bg_path, 
                       const char *atlas_main_path, 
                       const char *atlas_cfg_path, 
                       const char *atlas_game_path, 
                       const char *font_path, 
                       const char *kbd_path);

void LauncherView_Draw(GSGLOBAL *gsGlobal);
void LauncherView_SetCustomBackground(GSGLOBAL *gsGlobal, const char *tm2_path);
void LauncherView_ResetBackground(void);

#endif // LAUNCHER_VIEW_H