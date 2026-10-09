#ifndef NEWLIB_PORT_AWARE
#define NEWLIB_PORT_AWARE
#endif

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <fileXio_rpc.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <iopheap.h>
#include <strings.h>

#include "core/pad.h"
#include "core/file_util.h"
#include "actions/game_launcher.h"
#include "ui/menu_assets.h"
#include "ui/menu_scroll.h"
#include "ui/launcher_view.h"
#include "ui/lang.h"

// =============================================================================
// MÓDULOS DE IOP EMBUTIDOS (.incbin)
// =============================================================================
extern unsigned char usbd_irx[], usbd_irx_end[];
asm(".global usbd_irx\n.global usbd_irx_end\n usbd_irx:\n .incbin \"/usr/local/ps2dev/ps2sdk/iop/irx/usbd.irx\"\n .align 4\n usbd_irx_end:\n");
#define size_usbd_irx ((u32)(usbd_irx_end - usbd_irx))

extern unsigned char bdm_irx[], bdm_irx_end[];
asm(".global bdm_irx\n.global bdm_irx_end\n bdm_irx:\n .incbin \"/usr/local/ps2dev/ps2sdk/iop/irx/bdm.irx\"\n .align 4\n bdm_irx_end:\n");
#define size_bdm_irx ((u32)(bdm_irx_end - bdm_irx))

extern unsigned char usbmass_bd_irx[], usbmass_bd_irx_end[];
asm(".global usbmass_bd_irx\n.global usbmass_bd_irx_end\n usbmass_bd_irx:\n .incbin \"/usr/local/ps2dev/ps2sdk/iop/irx/usbmass_bd.irx\"\n .align 4\n usbmass_bd_irx_end:\n");
#define size_usbmass_bd_irx ((u32)(usbmass_bd_irx_end - usbmass_bd_irx))

extern unsigned char bdmfs_fatfs_irx[], bdmfs_fatfs_irx_end[];
asm(".global bdmfs_fatfs_irx\n.global bdmfs_fatfs_irx_end\n bdmfs_fatfs_irx:\n .incbin \"/usr/local/ps2dev/ps2sdk/iop/irx/bdmfs_fatfs.irx\"\n .align 4\n bdmfs_fatfs_irx_end:\n");
#define size_bdmfs_fatfs_irx ((u32)(bdmfs_fatfs_irx_end - bdmfs_fatfs_irx))

extern unsigned char iomanX_irx[], iomanX_irx_end[];
asm(".global iomanX_irx\n.global iomanX_irx_end\n iomanX_irx:\n .incbin \"/usr/local/ps2dev/ps2sdk/iop/irx/iomanX.irx\"\n .align 4\n iomanX_irx_end:\n");
#define size_iomanX_irx ((u32)(iomanX_irx_end - iomanX_irx))

extern unsigned char fileXio_irx[], fileXio_irx_end[];
asm(".global fileXio_irx\n.global fileXio_irx_end\n fileXio_irx:\n .incbin \"/usr/local/ps2dev/ps2sdk/iop/irx/fileXio.irx\"\n .align 4\n fileXio_irx_end:\n");
#define size_fileXio_irx ((u32)(fileXio_irx_end - fileXio_irx))

extern unsigned char audsrv_irx[], audsrv_irx_end[];
asm(".global audsrv_irx\n.global audsrv_irx_end\n audsrv_irx:\n .incbin \"/usr/local/ps2dev/ps2sdk/iop/irx/audsrv.irx\"\n .align 4\n audsrv_irx_end:\n");
#define size_audsrv_irx ((u32)(audsrv_irx_end - audsrv_irx))

extern unsigned char ds34usb_irx[], ds34usb_irx_end[];
asm(".global ds34usb_irx\n.global ds34usb_irx_end\n ds34usb_irx:\n .incbin \"ds34usb.irx\"\n .align 4\n ds34usb_irx_end:\n");
#define size_ds34usb_irx ((u32)(ds34usb_irx_end - ds34usb_irx))

// Módulo Keep-Alive do BDM (Anti-sleep de baixo nível)
extern unsigned char keepalive_irx[], keepalive_irx_end[];
asm(".global keepalive_irx\n.global keepalive_irx_end\n keepalive_irx:\n .incbin \"keepalive.irx\"\n .align 4\n keepalive_irx_end:\n");
#define size_keepalive_irx ((u32)(keepalive_irx_end - keepalive_irx))

#define APP_FOLDER "NGPPS2"
#define MAX_GAMES 100

typedef enum {
    STATE_GAMES_LIST,
    STATE_GENERAL_SETTINGS,
    STATE_LANGUAGE_SUBMENU,
    STATE_GAME_OPTIONS,
    STATE_CONFIRM_DELETE
} AppState;

static AppState s_state_stack[8];
static int s_state_top = 0;

static const char *TARGET_GAME_EXTENSIONS[] = { ".ngc", ".ngp", NULL };

LauncherItem g_games_list[MAX_GAMES];
static char s_game_folders[MAX_GAMES][512];
static int  g_total_games = 0;
char        g_mount_prefix[16] = "";
static char s_cfg_launcher_path[128] = "";

GSGLOBAL *gsGlobal = NULL;

static int s_scroll_cooldown = 0;
static int s_pending_bg_load = 0;
static int s_current_selected_idx = 0;

int g_confirm_delete_enabled = 1;

static LauncherItem s_general_items[9];
static LauncherItem s_lang_items[3];
static LauncherItem s_game_opt_items[4];
static LauncherItem s_confirm_items[3];

static int s_osk_edit_field = 0;
static int s_del_target_type = 0;

static char* trim(char* str) {
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    end[1] = '\0';
    return str;
}

static AppState CurrentState(void) {
    return s_state_stack[s_state_top];
}

static void PushState(AppState st) {
    if (s_state_top < 7) {
        s_state_top++;
        s_state_stack[s_state_top] = st;
    }
}

static void PopState(void) {
    if (s_state_top > 0) s_state_top--;
}

static void SaveGameConfig(int game_idx) {
    if (game_idx < 0 || game_idx >= g_total_games) return;
    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/config_game.cfg", s_game_folders[game_idx]);

    char buf[512];
    snprintf(buf, sizeof(buf),
        "[game]\n"
        "title = %s\n"
        "subtitle = %s\n"
        "tag = %s\n"
        "show_subtitle = 1\n"
        "show_tag = 1\n"
        "enabled = 1\n",
        g_games_list[game_idx].title,
        g_games_list[game_idx].subtitle,
        g_games_list[game_idx].tag
    );

    int fd = fileXioOpen(cfg_path, 0x0002 | 0x0200 | 0x0400, 0666);
    if (fd >= 0) {
        fileXioWrite(fd, buf, strlen(buf));
        fileXioClose(fd);
    }
}

static void SaveLauncherConfigExtended(void) {
    PS2_LauncherConfig cfg = PS2_Launcher_GetDefaultConfig();
    const char *exit_p = PS2_Launcher_GetExitPath();

    char buf[512];
    snprintf(buf, sizeof(buf),
        "[features]\n"
        "show_logo = %d\n"
        "show_numbers = %d\n"
        "show_subtitles = %d\n"
        "show_tags = %d\n"
        "show_scrollbar = %d\n"
        "custom_background = %d\n"
        "confirm_delete = %d\n"
        "language = %s\n"
        "exit_path = %s\n\n"
        "[messages]\n"
        "empty_message = %s\n"
        "empty_subtitle = %s\n",
        cfg.show_logo,
        cfg.show_numbers,
        cfg.show_subtitles,
        cfg.show_tags,
        cfg.show_scrollbar,
        PS2_Launcher_GetCustomBackground(),
        g_confirm_delete_enabled,
        Lang_GetCode(),
        exit_p ? exit_p : "BOOT.ELF",
        cfg.empty_msg ? cfg.empty_msg : "NENHUM JOGO ENCONTRADO",
        cfg.empty_sub ? cfg.empty_sub : "PRESSIONE START PARA RECARREGAR"
    );

    int fd = fileXioOpen(s_cfg_launcher_path, 0x0002 | 0x0200 | 0x0400, 0666);
    if (fd >= 0) {
        fileXioWrite(fd, buf, strlen(buf));
        fileXioClose(fd);
    }
}

static void LoadLauncherConfigExtended(void) {
    u8 *buf = NULL;
    u32 size = 0;
    if (ReadFileToBuffer(s_cfg_launcher_path, &buf, &size) <= 0 || !buf) return;

    PS2_Launcher_LoadLauncherConfig(s_cfg_launcher_path);

    char *line = strtok((char*)buf, "\r\n");
    while (line != NULL) {
        char *l = trim(line);
        if (l[0] != '#' && l[0] != ';' && l[0] != '[' && l[0] != '\0') {
            char *eq = strchr(l, '=');
            if (eq != NULL) {
                *eq = '\0';
                char *k = trim(l);
                char *v = trim(eq + 1);
                if (strcmp(k, "confirm_delete") == 0) g_confirm_delete_enabled = atoi(v);
                else if (strcmp(k, "language") == 0) {
                    Lang_SetCode(v);
                    char lpath[256];
                    snprintf(lpath, sizeof(lpath), "%s/%s/assets/translate/%s.lang", g_mount_prefix, APP_FOLDER, v);
                    Lang_Load(lpath);
                }
            }
        }
        line = strtok(NULL, "\r\n");
    }
    free(buf);
}

// -----------------------------------------------------------------------------
// RECARREGA ITENS DE CONFIGURAÇÃO (SEM RESETAR O ÍNDICE SELECIONADO)
// -----------------------------------------------------------------------------
static void UpdateGeneralSettingsItems(void) {
    PS2_LauncherConfig cfg = PS2_Launcher_GetDefaultConfig();
    const char *exit_p = PS2_Launcher_GetExitPath();

    snprintf(s_general_items[0].title, 64, "%s", Lang_Get("OPT_SHOW_LOGO", ""));
    snprintf(s_general_items[0].subtitle, 64, "%s", Lang_Get("OPT_SHOW_LOGO_SUB", ""));
    snprintf(s_general_items[0].tag, 16, cfg.show_logo ? "[ ON ]" : "[ OFF ]");

    snprintf(s_general_items[1].title, 64, "%s", Lang_Get("OPT_SHOW_NUM", ""));
    snprintf(s_general_items[1].subtitle, 64, "%s", Lang_Get("OPT_SHOW_NUM_SUB", ""));
    snprintf(s_general_items[1].tag, 16, cfg.show_numbers ? "[ ON ]" : "[ OFF ]");

    snprintf(s_general_items[2].title, 64, "%s", Lang_Get("OPT_SHOW_SUB", ""));
    snprintf(s_general_items[2].subtitle, 64, "%s", Lang_Get("OPT_SHOW_SUB_SUB", ""));
    snprintf(s_general_items[2].tag, 16, cfg.show_subtitles ? "[ ON ]" : "[ OFF ]");

    snprintf(s_general_items[3].title, 64, "%s", Lang_Get("OPT_SHOW_TAGS", ""));
    snprintf(s_general_items[3].subtitle, 64, "%s", Lang_Get("OPT_SHOW_TAGS_SUB", ""));
    snprintf(s_general_items[3].tag, 16, cfg.show_tags ? "[ ON ]" : "[ OFF ]");

    snprintf(s_general_items[4].title, 64, "%s", Lang_Get("OPT_SHOW_SCROLL", ""));
    snprintf(s_general_items[4].subtitle, 64, "%s", Lang_Get("OPT_SHOW_SCROLL_SUB", ""));
    snprintf(s_general_items[4].tag, 16, cfg.show_scrollbar ? "[ ON ]" : "[ OFF ]");

    snprintf(s_general_items[5].title, 64, "%s", Lang_Get("OPT_CUSTOM_BG", ""));
    snprintf(s_general_items[5].subtitle, 64, "%s", Lang_Get("OPT_CUSTOM_BG_SUB", ""));
    snprintf(s_general_items[5].tag, 16, PS2_Launcher_GetCustomBackground() ? "[ ON ]" : "[ OFF ]");

    snprintf(s_general_items[6].title, 64, "%s", Lang_Get("OPT_CONFIRM_DEL", ""));
    snprintf(s_general_items[6].subtitle, 64, "%s", Lang_Get("OPT_CONFIRM_DEL_SUB", ""));
    snprintf(s_general_items[6].tag, 16, g_confirm_delete_enabled ? "[ ON ]" : "[ OFF ]");

    snprintf(s_general_items[7].title, 64, "%s", Lang_Get("OPT_LANG", ""));
    snprintf(s_general_items[7].subtitle, 64, "%s", Lang_Get("OPT_LANG_SUB", ""));
    snprintf(s_general_items[7].tag, 16, "%s", Lang_GetTag());

    snprintf(s_general_items[8].title, 64, "%s", Lang_Get("OPT_EXIT_PATH", ""));
    snprintf(s_general_items[8].subtitle, 64, "%s", (exit_p && exit_p[0] != '\0') ? exit_p : Lang_Get("OPT_EXIT_PATH_SUB", ""));
    s_general_items[8].tag[0] = '\0';
}

static void EnterGeneralSettingsMenu(void) {
    LauncherView_SetAtlas(gsGlobal, ATLAS_CONFIG); // Troca para o Atlas de Configuração
    LauncherView_ResetBackground();
    UpdateGeneralSettingsItems();
    PS2_Launcher_SetSettingsMode(1);
    PS2_Launcher_ScrollInit(s_general_items, 9);

    PS2_Launcher_SetFooterPrompt(0, "cross",  Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "circle", Lang_Get("BTN_BACK", ""));
    PS2_Launcher_SetFooterPrompt(2, "", "");
    PS2_Launcher_SetFooterPrompt(3, "", "");
}

static void RefreshGeneralSettingsMenu(void) {
    UpdateGeneralSettingsItems();
    PS2_Launcher_SetItems(s_general_items, 9);
}

static void BuildLanguageMenu(void) {
    LauncherView_ResetBackground();

    snprintf(s_lang_items[0].title, 64, "%s", Lang_Get("LANG_PT", ""));
    snprintf(s_lang_items[0].subtitle, 64, "%s", Lang_Get("LANG_PT_SUB", ""));
    snprintf(s_lang_items[0].tag, 16, "[ PT ]");

    snprintf(s_lang_items[1].title, 64, "%s", Lang_Get("LANG_EN", ""));
    snprintf(s_lang_items[1].subtitle, 64, "%s", Lang_Get("LANG_EN_SUB", ""));
    snprintf(s_lang_items[1].tag, 16, "[ EN ]");

    snprintf(s_lang_items[2].title, 64, "%s", Lang_Get("LANG_ES", ""));
    snprintf(s_lang_items[2].subtitle, 64, "%s", Lang_Get("LANG_ES_SUB", ""));
    snprintf(s_lang_items[2].tag, 16, "[ ES ]");

    PS2_Launcher_SetSettingsMode(1);
    PS2_Launcher_ScrollInit(s_lang_items, 3);

    PS2_Launcher_SetFooterPrompt(0, "cross",  Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "circle", Lang_Get("BTN_BACK", ""));
    PS2_Launcher_SetFooterPrompt(2, "", "");
    PS2_Launcher_SetFooterPrompt(3, "", "");
}

static void BuildGameOptionsMenu(int idx) {
    LauncherView_SetAtlas(gsGlobal, ATLAS_GAME); // Troca para o Atlas do Jogo
    LauncherView_ResetBackground();
    LauncherItem *item = &g_games_list[idx];

    snprintf(s_game_opt_items[0].title, 64, "%s", Lang_Get("GAME_TITLE", ""));
    snprintf(s_game_opt_items[0].subtitle, 64, "%s", item->title);
    s_game_opt_items[0].tag[0] = '\0';

    snprintf(s_game_opt_items[1].title, 64, "%s", Lang_Get("GAME_SUBTITLE", ""));
    snprintf(s_game_opt_items[1].subtitle, 64, "%s", item->subtitle);
    s_game_opt_items[1].tag[0] = '\0';

    snprintf(s_game_opt_items[2].title, 64, "%s", Lang_Get("GAME_TAG", ""));
    snprintf(s_game_opt_items[2].subtitle, 64, "%s", item->tag);
    s_game_opt_items[2].tag[0] = '\0';

    snprintf(s_game_opt_items[3].title, 64, "%s", Lang_Get("GAME_DEL_BG", ""));
    snprintf(s_game_opt_items[3].subtitle, 64, "%s", Lang_Get("GAME_DEL_BG_SUB", ""));
    s_game_opt_items[3].tag[0] = '\0';

    PS2_Launcher_SetSettingsMode(1);
    PS2_Launcher_ScrollInit(s_game_opt_items, 4);

    PS2_Launcher_SetFooterPrompt(0, "cross",  Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "circle", Lang_Get("BTN_BACK", ""));
    PS2_Launcher_SetFooterPrompt(2, "", "");
    PS2_Launcher_SetFooterPrompt(3, "", "");
}

static void BuildConfirmDeleteMenu(void) {
    LauncherView_ResetBackground();

    snprintf(s_confirm_items[0].title, 64, "%s", Lang_Get("CONFIRM_YES", ""));
    snprintf(s_confirm_items[0].subtitle, 64, "%s", Lang_Get("CONFIRM_YES_SUB", ""));
    s_confirm_items[0].tag[0] = '\0';

    snprintf(s_confirm_items[1].title, 64, "%s", Lang_Get("CONFIRM_NO", ""));
    snprintf(s_confirm_items[1].subtitle, 64, "%s", Lang_Get("CONFIRM_NO_SUB", ""));
    s_confirm_items[1].tag[0] = '\0';

    snprintf(s_confirm_items[2].title, 64, "%s", Lang_Get("CONFIRM_DONT_ASK", ""));
    snprintf(s_confirm_items[2].subtitle, 64, "%s", Lang_Get("CONFIRM_DONT_ASK_SUB", ""));
    s_confirm_items[2].tag[0] = '\0';

    PS2_Launcher_SetSettingsMode(1);
    PS2_Launcher_ScrollInit(s_confirm_items, 3);

    PS2_Launcher_SetFooterPrompt(0, "cross",  Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "circle", Lang_Get("BTN_BACK", ""));
    PS2_Launcher_SetFooterPrompt(2, "", "");
    PS2_Launcher_SetFooterPrompt(3, "", "");
}

// -----------------------------------------------------------------------------
// RESTAURA A LISTA PRESERVANDO A POSIÇÃO DO JOGO
// -----------------------------------------------------------------------------
static void RestoreGamesList(int keep_idx) {
    LauncherView_SetAtlas(gsGlobal, ATLAS_MAIN); // Retorna ao Atlas Principal
    PS2_Launcher_SetSettingsMode(0);
    PS2_Launcher_ScrollInit(g_games_list, g_total_games);

    if (keep_idx >= 0 && keep_idx < g_total_games) {
        for (int i = 0; i < keep_idx; i++) {
            PS2_Launcher_ScrollInput(0, 1, 0, 0);
        }
        s_current_selected_idx = keep_idx;
    } else {
        s_current_selected_idx = 0;
    }

    LauncherView_ResetBackground();
    s_scroll_cooldown = 15;
    s_pending_bg_load = 1;

    PS2_Launcher_SetFooterPrompt(0, "cross",    Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "triangle", Lang_Get("BTN_OPTIONS", ""));
    PS2_Launcher_SetFooterPrompt(2, "circle",   Lang_Get("BTN_EXIT", ""));
    PS2_Launcher_SetFooterPrompt(3, "start",    Lang_Get("BTN_SETTINGS", ""));
}

static void ValidateAssets(void) {
    const char *files[] = {
        "assets/background.tm2",
        "assets/spritesheet_atlas.tm2",
        "assets/config_atlas.tm2",
        "assets/game_atlas.tm2",
        "assets/font_atlas.tm2",
        "assets/keyboard_atlas.tm2",
        "assets/config_theme.cfg",
        "assets/config_launcher.cfg",
        "assets/translate/pt_br.lang",
        NULL
    };

    for (int i = 0; files[i] != NULL; i++) {
        char test_path[256];
        snprintf(test_path, sizeof(test_path), "%s/%s/%s", g_mount_prefix, APP_FOLDER, files[i]);
        int fd = fileXioOpen(test_path, 0x0001);
        if (fd < 0) {
            init_scr();
            scr_clear();
            scr_printf("=====================================================\n");
            scr_printf("       [ERRO CRITICO] ARQUIVO ESSENCIAL AUSENTE!     \n");
            scr_printf("=====================================================\n\n");
            scr_printf("Arquivo ausente:\n  %s\n\n", test_path);
            scr_printf("Verifique se o pendrive contem a pasta NGPPS2 completa.\n");
            while (1) { }
        }
        fileXioClose(fd);
    }
}

int DetectActiveDrive(void) {
    const char *prefixes[] = {"host:", "host0:", "mass0:", "mass:", "mass1:", NULL};
    for (int i = 0; prefixes[i] != NULL; i++) {
        char test_path[128];
        snprintf(test_path, sizeof(test_path), "%s/%s", prefixes[i], APP_FOLDER);
        int fd = fileXioDopen(test_path);
        if (fd >= 0) {
            fileXioDclose(fd);
            strncpy(g_mount_prefix, prefixes[i], sizeof(g_mount_prefix) - 1);
            g_mount_prefix[sizeof(g_mount_prefix) - 1] = '\0';
            return 1;
        }
    }
    return 0;
}

static int MatchesAnyExtension(const char *filename, const char *ext_list[]) {
    if (!filename || !ext_list) return 0;
    size_t fn_len = strlen(filename);
    for (int i = 0; ext_list[i] != NULL; i++) {
        const char *ext = ext_list[i];
        size_t ext_len = strlen(ext);
        if (fn_len >= ext_len && strcasecmp(filename + fn_len - ext_len, ext) == 0) return 1;
    }
    return 0;
}

static int FindGameFileInDirectory(const char *dir_path, const char *ext_list[], char *out_filepath, size_t max_len) {
    int fd = fileXioDopen(dir_path);
    if (fd < 0) return 0;
    iox_dirent_t file_record;
    int found = 0;
    while (fileXioDread(fd, &file_record) > 0) {
        if (file_record.name[0] == '.') continue;
        if (MatchesAnyExtension(file_record.name, ext_list)) {
            snprintf(out_filepath, max_len, "%s/%s", dir_path, file_record.name);
            found = 1;
            break;
        }
    }
    fileXioDclose(fd);
    return found;
}

int ScanGamesDirectory(const char* root_dir) {
    int fd = fileXioDopen(root_dir);
    if (fd < 0) return 0;

    iox_dirent_t record;
    int count = 0;

    while (fileXioDread(fd, &record) > 0 && count < MAX_GAMES) {
        if (record.name[0] == '.') continue;

        char subfolder[512], auto_file[512] = "";
        snprintf(subfolder, sizeof(subfolder), "%s/%s", root_dir, record.name);

        if (!FindGameFileInDirectory(subfolder, TARGET_GAME_EXTENSIONS, auto_file, sizeof(auto_file))) continue;

        LauncherItem *item = &g_games_list[count];
        memset(item, 0, sizeof(LauncherItem));
        snprintf(s_game_folders[count], sizeof(s_game_folders[count]), "%s", subfolder);
        strncpy(item->filepath, auto_file, sizeof(item->filepath) - 1);

        char cfg_path[512], buf[1024];
        snprintf(cfg_path, sizeof(cfg_path), "%s/config_game.cfg", subfolder);
        int enabled = 1, show_sub = 1, show_tag = 1;

        if (ReadTextFile(cfg_path, buf, sizeof(buf)) > 0) {
            char *line = strtok(buf, "\r\n");
            while (line != NULL) {
                char *l = trim(line);
                if (l[0] != '#' && l[0] != ';' && l[0] != '[' && l[0] != '\0') {
                    char *eq = strchr(l, '=');
                    if (eq != NULL) {
                        *eq = '\0';
                        char *k = trim(l), *v = trim(eq + 1);
                        if (strcmp(k, "title") == 0) strncpy(item->title, v, sizeof(item->title) - 1);
                        else if (strcmp(k, "subtitle") == 0) strncpy(item->subtitle, v, sizeof(item->subtitle) - 1);
                        else if (strcmp(k, "tag") == 0) strncpy(item->tag, v, sizeof(item->tag) - 1);
                        else if (strcmp(k, "enabled") == 0) enabled = atoi(v);
                        else if (strcmp(k, "show_subtitle") == 0) show_sub = atoi(v);
                        else if (strcmp(k, "show_tag") == 0) show_tag = atoi(v);
                    }
                }
                line = strtok(NULL, "\r\n");
            }
        }

        if (!enabled) continue;
        if (!show_sub) item->subtitle[0] = '\0';
        if (!show_tag) item->tag[0] = '\0';
        if (item->title[0] == '\0') snprintf(item->title, sizeof(item->title), "%s", record.name);

        count++;
    }
    fileXioDclose(fd);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcasecmp(g_games_list[i].title, g_games_list[j].title) > 0) {
                LauncherItem temp_item = g_games_list[i];
                g_games_list[i] = g_games_list[j];
                g_games_list[j] = temp_item;

                char temp_folder[512];
                memcpy(temp_folder, s_game_folders[i], sizeof(temp_folder));
                memcpy(s_game_folders[i], s_game_folders[j], sizeof(temp_folder));
                memcpy(s_game_folders[j], temp_folder, sizeof(temp_folder));
            }
        }
    }

    return count;
}

static int TryLoadAndExecELF(const char *path) {
    if (!path || path[0] == '\0') return 0;

    char target[256];
    if (strchr(path, ':') != NULL) {
        strncpy(target, path, sizeof(target) - 1);
        target[sizeof(target) - 1] = '\0';
    } else {
        snprintf(target, sizeof(target), "%s/%s", g_mount_prefix, path);
    }

    t_ExecData exec;
    memset(&exec, 0, sizeof(exec));
    int ret = SifLoadElf(target, &exec);

    if (ret != 0 || exec.epc == 0) {
        char alt[256];
        char *colon = strchr(target, ':');
        if (colon && colon[1] == '/') {
            snprintf(alt, sizeof(alt), "%.*s:%s", (int)(colon - target), target, colon + 2);
            ret = SifLoadElf(alt, &exec);
            if (ret == 0 && exec.epc != 0) {
                strncpy(target, alt, sizeof(target) - 1);
            }
        }
    }

    if (ret != 0 || exec.epc == 0) {
        return 0;
    }

    padPortClose(0, 0);
    padEnd();
    fileXioExit();
    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();

    static char s_targ[256];
    strncpy(s_targ, target, sizeof(s_targ) - 1);
    static char *args[1];
    args[0] = s_targ;

    FlushCache(0);
    FlushCache(2);
    __asm__ __volatile__("di\n");
    ExecPS2((void*)exec.epc, (void*)exec.gp, 1, args);
    return 1;
}

static void ExitLauncher(void) {
    const char *cfg_path = PS2_Launcher_GetExitPath();

    if (cfg_path && cfg_path[0] != '\0' && strcmp(cfg_path, "rom0:OSDSYS") != 0) {
        if (TryLoadAndExecELF(cfg_path)) return;
    }

    // Tenta carregar no dispositivo USB ativo (mass0: ou mass:)
    char usb_boot[64];
    snprintf(usb_boot, sizeof(usb_boot), "%s/APPS/BOOT/BOOT.ELF", g_mount_prefix);
    if (TryLoadAndExecELF(usb_boot)) return;

    snprintf(usb_boot, sizeof(usb_boot), "%s/BOOT/BOOT.ELF", g_mount_prefix);
    if (TryLoadAndExecELF(usb_boot)) return;

    // Tenta carregar dos Memory Cards
    if (TryLoadAndExecELF("mc0:/BOOT/BOOT.ELF")) return;
    if (TryLoadAndExecELF("mc1:/BOOT/BOOT.ELF")) return;

    // Fallback absoluto: Menu do PS2
    padPortClose(0, 0);
    padEnd();
    fileXioExit();
    SifExitRpc();
    LoadExecPS2("rom0:OSDSYS", 0, NULL);
}

int main(int argc, char *argv[]) {
    int ret;
    int is_ps2link = (argc > 0 && argv[0] && (strncmp(argv[0], "host:", 5) == 0 || strncmp(argv[0], "host0:", 6) == 0));

    SifInitRpc(0);
    if (!is_ps2link) {
        while (SifIopReset("", 0) < 0);
        while (!SifIopSync());
        SifInitRpc(0);
    }

    SifLoadFileInit();
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    // 1. Sistema de I/O básico
    SifExecModuleBuffer(iomanX_irx, size_iomanX_irx, 0, NULL, &ret);
    SifExecModuleBuffer(fileXio_irx, size_fileXio_irx, 0, NULL, &ret);
    fileXioInit();

    // 2. Memory Cards (Habilita mc0: e mc1:)
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:MCMAN", 0, NULL);

    // 3. Sistema BDM e Armazenamento USB
    SifExecModuleBuffer(usbd_irx, size_usbd_irx, 0, NULL, &ret);
    SifExecModuleBuffer(bdm_irx, size_bdm_irx, 0, NULL, &ret);
    SifExecModuleBuffer(usbmass_bd_irx, size_usbmass_bd_irx, 0, NULL, &ret);
    SifExecModuleBuffer(bdmfs_fatfs_irx, size_bdmfs_fatfs_irx, 0, NULL, &ret); // Única chamada!
    SifExecModuleBuffer(keepalive_irx, size_keepalive_irx, 0, NULL, &ret);       // Keep-Alive do BDM
    SifExecModuleBuffer(ds34usb_irx, size_ds34usb_irx, 0, NULL, &ret);

    // 4. Áudio
    SifLoadModule("rom0:LIBSD", 0, NULL);
    SifExecModuleBuffer(audsrv_irx, size_audsrv_irx, 0, NULL, &ret);
    audsrv_init();

    Pad_Init();
    Pad_InitUSB();

    gsGlobal = gsKit_init_global();
    gsGlobal->Mode = GS_MODE_NTSC;
    gsGlobal->Width = 640;
    gsGlobal->Height = 448;
    gsGlobal->PSM = GS_PSM_CT16;
    gsGlobal->PSMZ = GS_PSMZ_16S;
    gsGlobal->ZBuffering = GS_SETTING_OFF;
    gsGlobal->DoubleBuffering = GS_SETTING_ON;
    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);

    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    int timeout = 0;
    while (timeout < 20 && !DetectActiveDrive()) {
        usleep(500000);
        timeout++;
    }
    if (strlen(g_mount_prefix) == 0) strcpy(g_mount_prefix, "mass0:");

    ValidateAssets();

    char cfg_theme_path[256];
    snprintf(cfg_theme_path, sizeof(cfg_theme_path), "%s/%s/assets/config_theme.cfg", g_mount_prefix, APP_FOLDER);
    snprintf(s_cfg_launcher_path, sizeof(s_cfg_launcher_path), "%s/%s/assets/config_launcher.cfg", g_mount_prefix, APP_FOLDER);

    PS2_Launcher_LoadThemeConfig(cfg_theme_path);
    LoadLauncherConfigExtended();

    if (strlen(Lang_GetCode()) == 0) {
        Lang_SetCode("pt_br");
        char def_lang[256];
        snprintf(def_lang, sizeof(def_lang), "%s/%s/assets/translate/pt_br.lang", g_mount_prefix, APP_FOLDER);
        Lang_Load(def_lang);
    }

    char games_dir[256];
    snprintf(games_dir, sizeof(games_dir), "%s/%s/games", g_mount_prefix, APP_FOLDER);
    g_total_games = ScanGamesDirectory(games_dir);

    char bg_path[256], atlas_path[256], cfg_atlas_path[256], game_atlas_path[256], font_path[256], kbd_path[256];
    snprintf(bg_path, sizeof(bg_path), "%s/%s/assets/background.tm2", g_mount_prefix, APP_FOLDER);
    snprintf(atlas_path, sizeof(atlas_path), "%s/%s/assets/spritesheet_atlas.tm2", g_mount_prefix, APP_FOLDER);
    snprintf(cfg_atlas_path, sizeof(cfg_atlas_path), "%s/%s/assets/config_atlas.tm2", g_mount_prefix, APP_FOLDER);
    snprintf(game_atlas_path, sizeof(game_atlas_path), "%s/%s/assets/game_atlas.tm2", g_mount_prefix, APP_FOLDER);
    snprintf(font_path, sizeof(font_path), "%s/%s/assets/font_atlas.tm2", g_mount_prefix, APP_FOLDER);
    snprintf(kbd_path, sizeof(kbd_path), "%s/%s/assets/keyboard_atlas.tm2", g_mount_prefix, APP_FOLDER);

    // Passa todos os caminhos para o LauncherView_Init
    LauncherView_Init(gsGlobal, bg_path, atlas_path, cfg_atlas_path, game_atlas_path, font_path, kbd_path);

    s_state_top = 0;
    s_state_stack[0] = STATE_GAMES_LIST;
    RestoreGamesList(0);

    PadInput pad;

    while (1) {
        Pad_Update(&pad);

        if (PS2_Keyboard_IsOpen()) {
            PS2_Keyboard_Input(
                (pad.pressed & PAD_UP) ? 1 : 0,
                (pad.pressed & PAD_DOWN) ? 1 : 0,
                (pad.pressed & PAD_LEFT) ? 1 : 0,
                (pad.pressed & PAD_RIGHT) ? 1 : 0,
                (pad.pressed & PAD_CROSS) ? 1 : 0,
                (pad.pressed & PAD_SQUARE) ? 1 : 0,
                (pad.pressed & PAD_TRIANGLE) ? 1 : 0,
                (pad.pressed & PAD_START) ? 1 : 0,
                (pad.pressed & PAD_CIRCLE) ? 1 : 0,
                (pad.pressed & PAD_SELECT) ? 1 : 0,
                (pad.pressed & PAD_L1) ? 1 : 0,
                (pad.pressed & PAD_R1) ? 1 : 0
            );

            if (!PS2_Keyboard_IsOpen()) {
                if (!PS2_Keyboard_IsCancelled()) {
                    const char *res = PS2_Keyboard_GetText();
                    if (s_osk_edit_field == 3) {
                        PS2_Launcher_SetExitPath(res);
                        SaveLauncherConfigExtended();
                        RefreshGeneralSettingsMenu();
                    } else {
                        LauncherItem *g = &g_games_list[s_current_selected_idx];
                        if (s_osk_edit_field == 0) strncpy(g->title, res, 63);
                        else if (s_osk_edit_field == 1) strncpy(g->subtitle, res, 63);
                        else if (s_osk_edit_field == 2) strncpy(g->tag, res, 15);
                        SaveGameConfig(s_current_selected_idx);
                        BuildGameOptionsMenu(s_current_selected_idx);
                    }
                } else {
                    if (s_osk_edit_field == 3) RefreshGeneralSettingsMenu();
                    else BuildGameOptionsMenu(s_current_selected_idx);
                }
            }

            gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0, 0));
            LauncherView_Draw(gsGlobal);
            gsKit_sync_flip(gsGlobal);
            gsKit_queue_exec(gsGlobal);
            continue;
        }

        if (CurrentState() == STATE_GAMES_LIST) {
            if (pad.pressed & PAD_CROSS) {
                const LauncherItem *sel = PS2_Launcher_GetSelectedItem();
                if (sel) {
                    int played_idx = s_current_selected_idx;
                    GameLauncher_Launch(sel);
                    RestoreGamesList(played_idx);
                    Pad_WaitRelease();
                }
            }
            else if (pad.pressed & PAD_TRIANGLE) {
                if (g_total_games > 0) {
                    PushState(STATE_GAME_OPTIONS);
                    BuildGameOptionsMenu(s_current_selected_idx);
                }
            }
            else if (pad.pressed & PAD_START) {
                PushState(STATE_GENERAL_SETTINGS);
                EnterGeneralSettingsMenu();
            }
            else if (pad.pressed & PAD_CIRCLE) {
                ExitLauncher();
            }
        }
        else if (CurrentState() == STATE_GAME_OPTIONS) {
            if (pad.pressed & PAD_CIRCLE) {
                PopState();
                RestoreGamesList(s_current_selected_idx);
            }
            else if (pad.pressed & PAD_CROSS) {
                int sel_idx = PS2_Launcher_GetSelectedIndex();
                LauncherItem *g = &g_games_list[s_current_selected_idx];

                if (sel_idx == 0) {
                    s_osk_edit_field = 0;
                    PS2_Keyboard_Open(g->title, 60);
                } else if (sel_idx == 1) {
                    s_osk_edit_field = 1;
                    PS2_Keyboard_Open(g->subtitle, 60);
                } else if (sel_idx == 2) {
                    s_osk_edit_field = 2;
                    PS2_Keyboard_Open(g->tag, 14);
                } else if (sel_idx == 3) {
                    s_del_target_type = 0;
                    if (g_confirm_delete_enabled) {
                        PushState(STATE_CONFIRM_DELETE);
                        BuildConfirmDeleteMenu();
                    } else {
                        char bg_tm2[256];
                        snprintf(bg_tm2, sizeof(bg_tm2), "%s/background.tm2", s_game_folders[s_current_selected_idx]);
                        fileXioRemove(bg_tm2);
                        LauncherView_ResetBackground();
                    }
                }
            }
        }
        else if (CurrentState() == STATE_GENERAL_SETTINGS) {
            if (pad.pressed & PAD_CIRCLE) {
                PopState();
                SaveLauncherConfigExtended();
                RestoreGamesList(s_current_selected_idx);
            }
            else if (pad.pressed & PAD_CROSS) {
                int sel_idx = PS2_Launcher_GetSelectedIndex();
                PS2_LauncherConfig cfg = PS2_Launcher_GetDefaultConfig();

                switch (sel_idx) {
                    case 0: cfg.show_logo = !cfg.show_logo; PS2_Launcher_SetConfig(&cfg); break;
                    case 1: cfg.show_numbers = !cfg.show_numbers; PS2_Launcher_SetConfig(&cfg); break;
                    case 2: cfg.show_subtitles = !cfg.show_subtitles; PS2_Launcher_SetConfig(&cfg); break;
                    case 3: cfg.show_tags = !cfg.show_tags; PS2_Launcher_SetConfig(&cfg); break;
                    case 4: cfg.show_scrollbar = !cfg.show_scrollbar; PS2_Launcher_SetConfig(&cfg); break;
                    case 5: PS2_Launcher_SetCustomBackground(!PS2_Launcher_GetCustomBackground()); break;
                    case 6: g_confirm_delete_enabled = !g_confirm_delete_enabled; break;
                    case 7:
                        PushState(STATE_LANGUAGE_SUBMENU);
                        BuildLanguageMenu();
                        break;
                    case 8: {
                        s_osk_edit_field = 3;
                        const char *cur = PS2_Launcher_GetExitPath();
                        PS2_Keyboard_Open((cur && cur[0] != '\0') ? cur : "mc0:/BOOT/BOOT.ELF", 120);
                        break;
                    }
                }
                SaveLauncherConfigExtended();
                if (CurrentState() == STATE_GENERAL_SETTINGS && sel_idx < 7) {
                    RefreshGeneralSettingsMenu();
                }
            }
        }
        else if (CurrentState() == STATE_LANGUAGE_SUBMENU) {
            if (pad.pressed & PAD_CIRCLE) {
                PopState();
                RefreshGeneralSettingsMenu();
            }
            else if (pad.pressed & PAD_CROSS) {
                int sel_idx = PS2_Launcher_GetSelectedIndex();
                const char *code = (sel_idx == 0) ? "pt_br" : ((sel_idx == 1) ? "en_us" : "es_es");
                Lang_SetCode(code);

                char lpath[256];
                snprintf(lpath, sizeof(lpath), "%s/%s/assets/translate/%s.lang", g_mount_prefix, APP_FOLDER, code);
                Lang_Load(lpath);

                SaveLauncherConfigExtended();
                PopState();
                RefreshGeneralSettingsMenu();
            }
        }
        else if (CurrentState() == STATE_CONFIRM_DELETE) {
            if (pad.pressed & PAD_CIRCLE) {
                PopState();
                BuildGameOptionsMenu(s_current_selected_idx);
            }
            else if (pad.pressed & PAD_CROSS) {
                int sel_idx = PS2_Launcher_GetSelectedIndex();

                if (sel_idx == 0) {
                    if (s_del_target_type == 0) {
                        char bg_tm2[256];
                        snprintf(bg_tm2, sizeof(bg_tm2), "%s/background.tm2", s_game_folders[s_current_selected_idx]);
                        fileXioRemove(bg_tm2);
                        LauncherView_ResetBackground();
                    }
                    PopState();
                    BuildGameOptionsMenu(s_current_selected_idx);
                }
                else if (sel_idx == 1) {
                    PopState();
                    BuildGameOptionsMenu(s_current_selected_idx);
                }
                else if (sel_idx == 2) {
                    g_confirm_delete_enabled = 0;
                    SaveLauncherConfigExtended();
                    if (s_del_target_type == 0) {
                        char bg_tm2[256];
                        snprintf(bg_tm2, sizeof(bg_tm2), "%s/background.tm2", s_game_folders[s_current_selected_idx]);
                        fileXioRemove(bg_tm2);
                        LauncherView_ResetBackground();
                    }
                    PopState();
                    BuildGameOptionsMenu(s_current_selected_idx);
                }
            }
        }

        int up   = (pad.pressed & PAD_UP)   ? 1 : 0;
        int down = (pad.pressed & PAD_DOWN) ? 1 : 0;
        int pgup = (pad.held & PAD_L1)      ? 1 : 0;
        int pgdn = (pad.held & PAD_R1)      ? 1 : 0;

        if (up || down || pgup || pgdn) {
            PS2_Launcher_ScrollInput(up, down, pgup, pgdn);
            if (CurrentState() == STATE_GAMES_LIST) {
                const LauncherItem *sel = PS2_Launcher_GetSelectedItem();
                if (sel) {
                    int nidx = (int)(sel - g_games_list);
                    if (nidx != s_current_selected_idx) {
                        s_current_selected_idx = nidx;
                        LauncherView_ResetBackground();
                        s_scroll_cooldown = 15;
                        s_pending_bg_load = 1;
                    }
                }
            }
        }

        if (CurrentState() == STATE_GAMES_LIST) {
            if (s_scroll_cooldown > 0) {
                s_scroll_cooldown--;
            } else if (s_pending_bg_load) {
                s_pending_bg_load = 0;
                if (PS2_Launcher_GetCustomBackground() && g_total_games > 0) {
                    char bg_tm2[256];
                    snprintf(bg_tm2, sizeof(bg_tm2), "%s/background.tm2", s_game_folders[s_current_selected_idx]);
                    LauncherView_SetCustomBackground(gsGlobal, bg_tm2);
                }
            }
        }

        gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0, 0));
        LauncherView_Draw(gsGlobal);
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_exec(gsGlobal);
    }

    return 0;
}