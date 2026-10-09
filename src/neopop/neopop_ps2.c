#include "neopop_ps2.h"
#include "neopop.h"
#include "bios.h"
#include "mem.h"
#include "sound.h"
#include "flash.h"
#include "state.h"

#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <audsrv.h>

#include "../core/pad.h"
#include "../core/file_util.h"
#include "../core/tim2_util.h"
#include "../ui/lang.h"
#include "../ui/menu_assets.h"
#include "../ui/menu_scroll.h"
#include "../ui/launcher_view.h"
#include "perf_profiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fileXio_rpc.h>

#define NGP_ACTUAL_W 160
#define NGP_ACTUAL_H 152
#define TEX_WIDTH    256
#define TEX_HEIGHT   152

#define SAMPLES_BASE 367
#define SAMPLES_MAX  1024

static _s16 s_audio_mix[SAMPLES_MAX * 2];
_u8 system_frameskip_key = 1;

static GSTEXTURE s_game_tex;
static u16 *s_tex_buffer = NULL;
static GSGLOBAL *s_gsGlobal = NULL;
static volatile int s_running = 0;
static int s_vram_allocated = 0;
static int s_initial_clears = 0;

int save_cooldown_timer = 0;
extern int g_flash_dirty;

u16 s_color_lut[4096];
static int s_lut_initialized = 0;

extern int g_confirm_delete_enabled;

typedef enum {
    PMENU_MAIN,
    PMENU_SAVE_SLOTS,
    PMENU_LOAD_SLOTS,
    PMENU_CONFIRM_DELETE
} PauseMenuState;

static int s_is_paused = 0;
static PauseMenuState s_pmenu_state = PMENU_MAIN;
static int s_action_to_confirm = 0;
static int s_selected_slot = 0;

static int s_slot_cooldown = 0;
static int s_slot_pending_bg = 0;

static LauncherItem s_pause_main_items[7]; // Expandido para 7 itens
static LauncherItem s_pause_slot_items[9];
static LauncherItem s_pause_confirm_items[3];


static int s_hdd_keepalive_timer = 0;



static void ResumeEmulation(void) {
    s_is_paused = 0;
    LauncherView_ResetBackground();

    s_gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;

    for (int i = 0; i < 2; i++) {
        gsKit_clear(s_gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
        gsKit_queue_exec(s_gsGlobal);
        gsKit_sync_flip(s_gsGlobal);
    }

    s_initial_clears = 2;

    static _s16 s_resume_silence[SAMPLES_BASE * 2];
    memset(s_resume_silence, 0, sizeof(s_resume_silence));
    for (int i = 0; i < 2; i++) {
        audsrv_play_audio((const char*)s_resume_silence, sizeof(s_resume_silence));
    }

    mute = FALSE;
    audsrv_set_volume(MAX_VOLUME);
}

BOOL system_io_state_read(char* filename, _u8* buffer, _u32 bufferLength) {
    if (!filename || !buffer || bufferLength == 0) return FALSE;
    int fd = fileXioOpen(filename, 0x0001);
    if (fd < 0) return FALSE;

    _u32 total = 0;
    while (total < bufferLength) {
        int r = fileXioRead(fd, buffer + total, bufferLength - total);
        if (r <= 0) break;
        total += r;
    }
    fileXioClose(fd);
    return (total == bufferLength) ? TRUE : FALSE;
}

BOOL system_io_state_write(char* filename, _u8* buffer, _u32 bufferLength) {
    if (!filename || !buffer || bufferLength == 0) return FALSE;
    int fd = fileXioOpen(filename, 0x0002 | 0x0200 | 0x0400, 0666);
    if (fd < 0) return FALSE;

    _u32 total = 0;
    while (total < bufferLength) {
        int w = fileXioWrite(fd, buffer + total, bufferLength - total);
        if (w <= 0) break;
        total += w;
    }
    fileXioClose(fd);
    return (total == bufferLength) ? TRUE : FALSE;
}

void OutputAudioFrame(void) {
    if (mute || s_is_paused) return;

    Profiler_ZoneStart(PROF_AUDIO_RENDER);
    int queued = audsrv_queued();
    int samples = SAMPLES_BASE;
    if (queued < (SAMPLES_BASE * 4)) samples = 370;
    else if (queued > (SAMPLES_BASE * 12)) samples = 364;

    sound_render_frame(s_audio_mix, samples);
    audsrv_play_audio((const char*)s_audio_mix, samples * 4);
    Profiler_ZoneEnd(PROF_AUDIO_RENDER);
}

static void GetGameDir(char *out_dir, size_t max_len) {
    strncpy(out_dir, (const char*)rom.filename, max_len - 1);
    out_dir[max_len - 1] = '\0';
    char *slash = strrchr(out_dir, '/');
    if (!slash) slash = strrchr(out_dir, '\\');
    if (slash) *slash = '\0';
}

static void EnsureStateDir(void) {
    char gdir[256], sdir[256];
    GetGameDir(gdir, sizeof(gdir));
    snprintf(sdir, sizeof(sdir), "%s/state", gdir);
    fileXioMkdir(sdir, 0777);
}

static void GetSlotPaths(int slot, char *out_ngs, char *out_tm2, size_t max_len) {
    char gdir[256];
    GetGameDir(gdir, sizeof(gdir));
    if (out_ngs) snprintf(out_ngs, max_len, "%s/state/slot%d.ngs", gdir, slot + 1);
    if (out_tm2) snprintf(out_tm2, max_len, "%s/state/slot%d.tm2", gdir, slot + 1);
}

static int SlotExists(int slot) {
    char ngs_path[256];
    GetSlotPaths(slot, ngs_path, NULL, sizeof(ngs_path));
    int fd = fileXioOpen(ngs_path, 0x0001);
    if (fd >= 0) {
        fileXioClose(fd);
        return 1;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// MENU DE PAUSA COM 7 OPÇÕES
// -----------------------------------------------------------------------------
static void BuildPauseMainMenu(void) {
    LauncherView_ResetBackground();

    snprintf(s_pause_main_items[0].title, 64, "%s", Lang_Get("PAUSE_CONTINUE", ""));
    snprintf(s_pause_main_items[0].subtitle, 64, "%s", Lang_Get("PAUSE_CONTINUE_SUB", ""));
    s_pause_main_items[0].tag[0] = '\0';

    // 1. Resetar Game (Logo abaixo de Continuar)
    snprintf(s_pause_main_items[1].title, 64, "%s", Lang_Get("PAUSE_RESET", ""));
    snprintf(s_pause_main_items[1].subtitle, 64, "%s", Lang_Get("PAUSE_RESET_SUB", ""));
    s_pause_main_items[1].tag[0] = '\0';

    snprintf(s_pause_main_items[2].title, 64, "%s", Lang_Get("PAUSE_SAVE_STATE", ""));
    snprintf(s_pause_main_items[2].subtitle, 64, "%s", Lang_Get("PAUSE_SAVE_STATE_SUB", ""));
    s_pause_main_items[2].tag[0] = '\0';

    snprintf(s_pause_main_items[3].title, 64, "%s", Lang_Get("PAUSE_LOAD_STATE", ""));
    snprintf(s_pause_main_items[3].subtitle, 64, "%s", Lang_Get("PAUSE_LOAD_STATE_SUB", ""));
    s_pause_main_items[3].tag[0] = '\0';

    snprintf(s_pause_main_items[4].title, 64, "%s", Lang_Get("PAUSE_SAVE_BG", ""));
    snprintf(s_pause_main_items[4].subtitle, 64, "%s", Lang_Get("PAUSE_SAVE_BG_SUB", ""));
    s_pause_main_items[4].tag[0] = '\0';

    snprintf(s_pause_main_items[5].title, 64, "%s", Lang_Get("PAUSE_DEL_SAVE", ""));
    snprintf(s_pause_main_items[5].subtitle, 64, "%s", Lang_Get("PAUSE_DEL_SAVE_SUB", ""));
    s_pause_main_items[5].tag[0] = '\0';

    snprintf(s_pause_main_items[6].title, 64, "%s", Lang_Get("PAUSE_EXIT", ""));
    snprintf(s_pause_main_items[6].subtitle, 64, "%s", Lang_Get("PAUSE_EXIT_SUB", ""));
    s_pause_main_items[6].tag[0] = '\0';

    PS2_Launcher_SetSettingsMode(1);
    PS2_Launcher_ScrollInit(s_pause_main_items, 7);

    PS2_Launcher_SetFooterPrompt(0, "cross",  Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "circle", Lang_Get("BTN_BACK", ""));
    PS2_Launcher_SetFooterPrompt(2, "", "");
    PS2_Launcher_SetFooterPrompt(3, "", "");
}

static void BuildPauseSlotsMenu(int is_save) {
    EnsureStateDir();
    for (int i = 0; i < 9; i++) {
        snprintf(s_pause_slot_items[i].title, 64, "Slot %02d", i + 1);
        if (SlotExists(i)) {
            snprintf(s_pause_slot_items[i].subtitle, 64, "%s", Lang_Get("SLOT_SAVED", ""));
            snprintf(s_pause_slot_items[i].tag, 16, "[ NGS ]");
        } else {
            snprintf(s_pause_slot_items[i].subtitle, 64, "%s", Lang_Get("SLOT_EMPTY", ""));
            snprintf(s_pause_slot_items[i].tag, 16, "[ --- ]");
        }
    }

    s_selected_slot = 0;
    PS2_Launcher_SetSettingsMode(1);
    PS2_Launcher_ScrollInit(s_pause_slot_items, 9);

    PS2_Launcher_SetFooterPrompt(0, "cross",    Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "circle",   Lang_Get("BTN_BACK", ""));
    PS2_Launcher_SetFooterPrompt(2, "triangle", Lang_Get("BTN_DELETE", ""));
    PS2_Launcher_SetFooterPrompt(3, "", "");

    LauncherView_ResetBackground();
    s_slot_cooldown = 15;
    s_slot_pending_bg = 1;
}

static void BuildPauseConfirmDeleteMenu(void) {
    LauncherView_ResetBackground();

    snprintf(s_pause_confirm_items[0].title, 64, "%s", Lang_Get("CONFIRM_YES", ""));
    snprintf(s_pause_confirm_items[0].subtitle, 64, "%s", Lang_Get("CONFIRM_YES_SUB", ""));
    s_pause_confirm_items[0].tag[0] = '\0';

    snprintf(s_pause_confirm_items[1].title, 64, "%s", Lang_Get("CONFIRM_NO", ""));
    snprintf(s_pause_confirm_items[1].subtitle, 64, "%s", Lang_Get("CONFIRM_NO_SUB", ""));
    s_pause_confirm_items[1].tag[0] = '\0';

    snprintf(s_pause_confirm_items[2].title, 64, "%s", Lang_Get("CONFIRM_DONT_ASK", ""));
    snprintf(s_pause_confirm_items[2].subtitle, 64, "%s", Lang_Get("CONFIRM_DONT_ASK_SUB", ""));
    s_pause_confirm_items[2].tag[0] = '\0';

    PS2_Launcher_SetSettingsMode(1);
    PS2_Launcher_ScrollInit(s_pause_confirm_items, 3);

    PS2_Launcher_SetFooterPrompt(0, "cross",  Lang_Get("BTN_SELECT", ""));
    PS2_Launcher_SetFooterPrompt(1, "circle", Lang_Get("BTN_BACK", ""));
    PS2_Launcher_SetFooterPrompt(2, "", "");
    PS2_Launcher_SetFooterPrompt(3, "", "");
}

static void ExecutePendingDelete(void) {
    if (s_action_to_confirm == 0) {
        char ngs[256], tm2[256];
        GetSlotPaths(s_selected_slot, ngs, tm2, sizeof(ngs));
        fileXioRemove(ngs);
        fileXioRemove(tm2);
        LauncherView_ResetBackground();
    } else if (s_action_to_confirm == 1) {
        char ngf_path[256];
        snprintf(ngf_path, sizeof(ngf_path), "%s.ngf", rom.filename);
        fileXioRemove(ngf_path);
    }
}

static void ProcessPauseMenuInputs(const PadInput *pad) {
    int up   = (pad->pressed & PAD_UP)   ? 1 : 0;
    int down = (pad->pressed & PAD_DOWN) ? 1 : 0;
    if (up || down) {
        PS2_Launcher_ScrollInput(up, down, 0, 0);

        if (s_pmenu_state == PMENU_SAVE_SLOTS || s_pmenu_state == PMENU_LOAD_SLOTS) {
            int new_slot = PS2_Launcher_GetSelectedIndex();
            if (new_slot != s_selected_slot) {
                s_selected_slot = new_slot;
                LauncherView_ResetBackground();
                s_slot_cooldown = 15;
                s_slot_pending_bg = 1;
            }
        }
    }

    if (s_pmenu_state == PMENU_MAIN) {
        if (pad->pressed & PAD_CIRCLE) {
            ResumeEmulation();
            return;
        }

        if (pad->pressed & PAD_CROSS) {
            int sel = PS2_Launcher_GetSelectedIndex();
            switch (sel) {
                case 0: // Continuar Jogo
                    ResumeEmulation();
                    break;
                case 1: // Resetar Game (Reinicia o core)
                    reset();
                    ResumeEmulation();
                    break;
                case 2: // Salvar Estado
                    s_pmenu_state = PMENU_SAVE_SLOTS;
                    BuildPauseSlotsMenu(1);
                    break;
                case 3: // Carregar Estado
                    s_pmenu_state = PMENU_LOAD_SLOTS;
                    BuildPauseSlotsMenu(0);
                    break;
                case 4: { // Salvar como Fundo
                    char gdir[256], bg_path[256];
                    GetGameDir(gdir, sizeof(gdir));
                    snprintf(bg_path, sizeof(bg_path), "%s/background.tm2", gdir);
                    TIM2_SaveFrom16Bit(bg_path, s_tex_buffer, NGP_ACTUAL_W, NGP_ACTUAL_H, TEX_WIDTH);
                    ResumeEmulation();
                    break;
                }
                case 5: { // Apagar Savegame (.ngf)
                    s_action_to_confirm = 1;
                    if (g_confirm_delete_enabled) {
                        s_pmenu_state = PMENU_CONFIRM_DELETE;
                        BuildPauseConfirmDeleteMenu();
                    } else {
                        ExecutePendingDelete();
                    }
                    break;
                }
                case 6: // Sair para o Menu
                    s_is_paused = 0;
                    mute = FALSE;
                    audsrv_set_volume(MAX_VOLUME);
                    s_running = 0;
                    LauncherView_ResetBackground();
                    break;
            }
        }
    }
    else if (s_pmenu_state == PMENU_SAVE_SLOTS || s_pmenu_state == PMENU_LOAD_SLOTS) {
        if (pad->pressed & PAD_CIRCLE) {
            LauncherView_ResetBackground();
            s_pmenu_state = PMENU_MAIN;
            BuildPauseMainMenu();
            return;
        }

        if (pad->pressed & PAD_TRIANGLE) {
            s_selected_slot = PS2_Launcher_GetSelectedIndex();
            if (SlotExists(s_selected_slot)) {
                s_action_to_confirm = 0;
                if (g_confirm_delete_enabled) {
                    s_pmenu_state = PMENU_CONFIRM_DELETE;
                    BuildPauseConfirmDeleteMenu();
                } else {
                    ExecutePendingDelete();
                    BuildPauseSlotsMenu(s_pmenu_state == PMENU_SAVE_SLOTS);
                }
            }
            return;
        }

        if (pad->pressed & PAD_CROSS) {
            s_selected_slot = PS2_Launcher_GetSelectedIndex();
            char ngs[256], tm2[256];
            GetSlotPaths(s_selected_slot, ngs, tm2, sizeof(ngs));

            if (s_pmenu_state == PMENU_SAVE_SLOTS) {
                state_store(ngs);
                TIM2_SaveFrom16Bit(tm2, s_tex_buffer, NGP_ACTUAL_W, NGP_ACTUAL_H, TEX_WIDTH);
                ResumeEmulation();
            } else if (s_pmenu_state == PMENU_LOAD_SLOTS) {
                if (SlotExists(s_selected_slot)) {
                    state_restore(ngs);
                    ResumeEmulation();
                }
            }
        }
    }
    else if (s_pmenu_state == PMENU_CONFIRM_DELETE) {
        if (pad->pressed & PAD_CIRCLE) {
            if (s_action_to_confirm == 0) {
                s_pmenu_state = PMENU_SAVE_SLOTS;
                BuildPauseSlotsMenu(1);
            } else {
                s_pmenu_state = PMENU_MAIN;
                BuildPauseMainMenu();
            }
            return;
        }

        if (pad->pressed & PAD_CROSS) {
            int sel = PS2_Launcher_GetSelectedIndex();
            if (sel == 0) {
                ExecutePendingDelete();
            } else if (sel == 2) {
                g_confirm_delete_enabled = 0;
                ExecutePendingDelete();
            }

            if (s_action_to_confirm == 0) {
                s_pmenu_state = PMENU_SAVE_SLOTS;
                BuildPauseSlotsMenu(1);
            } else {
                s_pmenu_state = PMENU_MAIN;
                BuildPauseMainMenu();
            }
        }
    }
}

static void UpdatePadInput(void) {
    PadInput pad;
    Pad_Update(&pad);

    if ((pad.held & PAD_SELECT) && (pad.held & PAD_START)) {
        if (!s_is_paused) {
            s_is_paused = 1;
            audsrv_set_volume(0);
            mute = TRUE;
            ram[0x6F82] = 0;

            s_pmenu_state = PMENU_MAIN;
            BuildPauseMainMenu();
        }
        return;
    }

    if (s_is_paused) return;

    u8 ngp_pad = 0;
    if (pad.held & PAD_UP)    ngp_pad |= 0x01;
    if (pad.held & PAD_DOWN)  ngp_pad |= 0x02;
    if (pad.held & PAD_LEFT)  ngp_pad |= 0x04;
    if (pad.held & PAD_RIGHT) ngp_pad |= 0x08;
    if (pad.held & PAD_CROSS)   ngp_pad |= 0x10;
    if (pad.held & (PAD_CIRCLE | PAD_SQUARE))  ngp_pad |= 0x20;
    if (pad.held & PAD_START)   ngp_pad |= 0x40;

    ram[0x6F82] = ngp_pad;
}

void system_VBL(void) {
    UpdatePadInput();
    if (!s_running || !s_gsGlobal || !s_tex_buffer || s_is_paused) return;

    Profiler_ZoneStart(PROF_GS_DMA);
    gsKit_texture_upload(s_gsGlobal, &s_game_tex);

    s_gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;

    if (s_initial_clears > 0) {
        gsKit_clear(s_gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
        s_initial_clears--;
    }

    float draw_w = 448.0f;
    float draw_h = 425.0f;
    float pos_x  = (640.0f - draw_w) / 2.0f;
    float pos_y  = (448.0f - draw_h) / 2.0f;

    gsKit_prim_sprite_texture(s_gsGlobal, &s_game_tex,
        pos_x, pos_y, 0.0f, 0.0f,
        pos_x + draw_w, pos_y + draw_h,
        (float)NGP_ACTUAL_W, (float)NGP_ACTUAL_H,
        2, GS_SETREG_RGBAQ(128, 128, 128, 0x80, 0));

    gsKit_queue_exec(s_gsGlobal);
    Profiler_ZoneEnd(PROF_GS_DMA);

    Profiler_MarkBeforeFlip();
    gsKit_sync_flip(s_gsGlobal);
    Profiler_FrameEndAndReport();
    Profiler_FrameStart();
	
    // ========================================================
    // AUTO-SAVE FÍSICO
    // ========================================================
    if (g_flash_dirty) {
        save_cooldown_timer++;
        if (save_cooldown_timer > 60) {
            flash_commit();
            g_flash_dirty = 0;
            save_cooldown_timer = 0;
            s_hdd_keepalive_timer = 0; // <--- O save acordou o HD, reseta o keep-alive!
        }
    } else {
        save_cooldown_timer = 0;
    }
}

static void InitColorLUT(void) {
    if (s_lut_initialized) return;
    for (int color = 0; color < 4096; color++) {
        u32 r4 = (color & 0x000F);
        u32 g4 = (color & 0x00F0) >> 4;
        u32 b4 = (color & 0x0F00) >> 8;
        u16 r5 = (r4 << 1) | (r4 >> 3);
        u16 g5 = (g4 << 1) | (g4 >> 3);
        u16 b5 = (b4 << 1) | (b4 >> 3);
        s_color_lut[color] = 0x8000 | (b5 << 10) | (g5 << 5) | r5;
    }
    s_lut_initialized = 1;
}

static int LoadRom(const char *filepath) {
    rom_unload();
    u8 *data = NULL;
    u32 size = 0;
    if (ReadFileToBuffer(filepath, &data, &size) <= 0 || !data) return 0;
    rom.data = data;
    rom.length = size;
    strncpy((char*)rom.filename, filepath, sizeof(rom.filename) - 1);
    rom_loaded();
    return 1;
}

int NeoPop_PS2_Init(GSGLOBAL *gsGlobal) {
    s_gsGlobal = gsGlobal;
    InitColorLUT();

    if (!s_tex_buffer) {
        s_tex_buffer = (u16*)memalign(128, TEX_WIDTH * TEX_HEIGHT * sizeof(u16));
    }
    memset(s_tex_buffer, 0, TEX_WIDTH * TEX_HEIGHT * sizeof(u16));
    cfb = s_tex_buffer;

    static u32 s_tex_vram_addr = 0;
    memset(&s_game_tex, 0, sizeof(GSTEXTURE));
    s_game_tex.Width = TEX_WIDTH;
    s_game_tex.Height = TEX_HEIGHT;
    s_game_tex.PSM = GS_PSM_CT16;
    s_game_tex.Mem = (void*)s_tex_buffer;
    s_game_tex.Filter = GS_FILTER_NEAREST;

    if (!s_vram_allocated) {
        s_tex_vram_addr = gsKit_vram_alloc(s_gsGlobal,
            gsKit_texture_size(s_game_tex.Width, s_game_tex.Height, s_game_tex.PSM),
            GSKIT_ALLOC_USERBUFFER);
        s_vram_allocated = 1;
    }
    s_game_tex.Vram = s_tex_vram_addr;

    if (!bios_install()) return 0;

	struct audsrv_fmt_t format;
    format.bits = 16;
    format.freq = 22050; // <--- Mude de 44100 para 22050!
    format.channels = 2;
    audsrv_set_format(&format);
    audsrv_set_volume(MAX_VOLUME);

    language_english = TRUE;
    system_colour = COLOURMODE_AUTO;
    mute = FALSE;
    sound_init(22050);   // <--- Mude de 44100 para 22050!

    return 1;
}

void NeoPop_PS2_Run(GSGLOBAL *gsGlobal, const char *rom_path) {
    if (!NeoPop_PS2_Init(gsGlobal)) return;
    if (!LoadRom(rom_path)) return;

    reset();
    s_running = 1;
    s_is_paused = 0;
    mute = FALSE;
    audsrv_set_volume(MAX_VOLUME);

    Pad_SetAnalogMode(1);

    static _s16 s_silence[SAMPLES_BASE * 2];
    memset(s_silence, 0, sizeof(s_silence));
    for (int i = 0; i < 3; i++) {
        audsrv_play_audio((const char*)s_silence, sizeof(s_silence));
    }

    s_initial_clears = 3;
    s_gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;

    while (s_running) {
        if (s_is_paused) {
            PadInput p;
            Pad_Update(&p);
            ProcessPauseMenuInputs(&p);

            if (!s_is_paused) {
                continue;
            }

            if (s_pmenu_state == PMENU_SAVE_SLOTS || s_pmenu_state == PMENU_LOAD_SLOTS) {
                if (s_slot_cooldown > 0) {
                    s_slot_cooldown--;
                } else if (s_slot_pending_bg) {
                    s_slot_pending_bg = 0;
                    char tm2_preview[256];
                    GetSlotPaths(s_selected_slot, NULL, tm2_preview, sizeof(tm2_preview));
                    LauncherView_SetCustomBackground(s_gsGlobal, tm2_preview);
                }
            }

            gsKit_clear(s_gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
            LauncherView_Draw(s_gsGlobal);
            gsKit_sync_flip(s_gsGlobal);
            gsKit_queue_exec(s_gsGlobal);
        } else {
            emulate();
        }
    }

    audsrv_stop_audio();
    rom_unload();

    Pad_SetAnalogMode(0);

    s_gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsKit_set_primalpha(s_gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_clear(s_gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0, 0));
    gsKit_queue_exec(s_gsGlobal);
    gsKit_sync_flip(s_gsGlobal);

    Pad_WaitRelease();
}

void NeoPop_PS2_Shutdown(void) {
    if (s_tex_buffer) {
        free(s_tex_buffer);
        s_tex_buffer = NULL;
    }
}

void system_message(char *vaMessage, ...) {
    char buf[512];
    va_list vl;
    va_start(vl, vaMessage);
    vsnprintf(buf, sizeof(buf), vaMessage, vl);
    va_end(vl);
    printf("[NeoPop] %s\n", buf);
}

char* system_get_string(STRINGS string_id) { return "NeoPop System"; }
void system_sound_chipreset(void) { sound_init(44100); }
void system_sound_silence(void) {}
BOOL system_comms_read(_u8 *buffer) { return FALSE; }
BOOL system_comms_poll(_u8 *buffer) { return FALSE; }
void system_comms_write(_u8 data) {}

BOOL system_io_flash_read(_u8* buffer, _u32 bufferLength) {
    if (!rom.filename[0] || !buffer || bufferLength == 0) return FALSE;
    
    char path[256];
    snprintf(path, sizeof(path), "%s.ngf", rom.filename);
    
    int fd = fileXioOpen(path, 0x0001); // O_RDONLY
    if (fd < 0) return FALSE;

    _u32 total = 0;
    while (total < bufferLength) {
        int r = fileXioRead(fd, buffer + total, bufferLength - total);
        if (r <= 0) break;
        total += r;
    }
    fileXioClose(fd);

    return (total == bufferLength) ? TRUE : FALSE;
}

BOOL system_io_flash_write(_u8* buffer, _u32 bufferLength) {
    if (!rom.filename[0] || !buffer || bufferLength == 0) return FALSE;
    
    char path[256];
    snprintf(path, sizeof(path), "%s.ngf", rom.filename);
    
    // O_WRONLY | O_CREAT | O_TRUNC
    int fd = fileXioOpen(path, 0x0002 | 0x0200 | 0x0400, 0666);
    if (fd < 0) return FALSE;

    _u32 total = 0;
    while (total < bufferLength) {
        int w = fileXioWrite(fd, buffer + total, bufferLength - total);
        if (w <= 0) break;
        total += w;
    }
    fileXioClose(fd);

    return (total == bufferLength) ? TRUE : FALSE;
}

BOOL system_io_rom_read(char* filename, _u8* buffer, _u32 bufferLength) { return FALSE; }