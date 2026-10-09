#include "pad.h"
#include <libpad.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>

static char s_padBuf[256] __attribute__((aligned(64)));
static u32 s_old_pad = 0;
static int s_pad_initialized = 0;

static int s_current_analog_mode = -1;
static int s_target_analog_mode = 0;

// =========================================================================
// VARIÁVEIS DO CLIENTE RPC (MÓDULO DS34USB / IPEGA)
// =========================================================================
#define DS34USB_BIND_RPC_ID 0x18E3878E
static SifRpcClientData_t s_ds34_client __attribute__((aligned(64)));
static u8 s_ds34_buffer[64] __attribute__((aligned(64)));
static int s_ds34_connected = 0;
static int s_ds34_rpc_busy = 0;

void Pad_Init(void) {
    if (s_pad_initialized) return;

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    padInit(0);
    memset(s_padBuf, 0, sizeof(s_padBuf));
    padPortOpen(0, 0, s_padBuf);

    s_pad_initialized = 1;
}

void Pad_InitUSB(void) {
    if (SifBindRpc(&s_ds34_client, DS34USB_BIND_RPC_ID, 0) >= 0 && s_ds34_client.server != NULL) {
        s_ds34_buffer[0] = 1;
        SifCallRpc(&s_ds34_client, 1, 0, s_ds34_buffer, 1, s_ds34_buffer, 1, NULL, NULL);
        s_ds34_connected = 1;
    } else {
        printf("[PAD] Falha ao conectar ao RPC do DS34USB.\n");
    }
}

// Insira logo após Pad_SetAnalogMode:
void Pad_WaitRelease(void) {
    if (!s_pad_initialized) return;

    // 1. Aguarda o controle terminar comandos internos (sair de PAD_STATE_EXECCMD)
    int timeout = 0;
    while (timeout < 40) {
        int state = padGetState(0, 0);
        if (state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) break;
        usleep(5000);
        timeout++;
    }

    // 2. Aguarda até o usuário soltar fisicamente o botão X / qualquer botão
    PadInput p;
    timeout = 0;
    do {
        Pad_Update(&p);
        if (p.held != 0) usleep(5000);
        timeout++;
    } while (p.held != 0 && timeout < 80);

    // 3. Zera o histórico para garantir que nenhum clique fantasma seja registrado
    s_old_pad = 0;
}

// -----------------------------------------------------------------------------
// CONTROLE DO MODO ANALÓGICO (DUALSHOCK 2 / DIGITAL)
// -----------------------------------------------------------------------------
void Pad_SetAnalogMode(int enable) {
    s_target_analog_mode = enable ? 1 : 0;
    if (!s_pad_initialized) return;

    int state = padGetState(0, 0);
    if (state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) {
        if (s_target_analog_mode) {
            // Liga o LED analógico e bloqueia o botão no controle
            padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
        } else {
            // Desliga o LED analógico e volta para o modo digital
            padSetMainMode(0, 0, PAD_MMODE_DIGITAL, PAD_MMODE_UNLOCK);
        }
        s_current_analog_mode = s_target_analog_mode;
    }
}

void Pad_Update(PadInput *input) {
    input->held = 0;
    input->pressed = 0;

    u16 native_btns = 0xFFFF;
    u16 usb_btns    = 0xFFFF;

    // =========================================================================
    // 1. CONTROLE ORIGINAL (PORTA 1 - DUALSHOCK 2)
    // =========================================================================
    if (s_pad_initialized) {
        int state = padGetState(0, 0);
        if (state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) {
            // Aplica a mudança de modo analógico pendente se o controle acabou de conectar
            if (s_current_analog_mode != s_target_analog_mode) {
                if (s_target_analog_mode) {
                    padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
                } else {
                    padSetMainMode(0, 0, PAD_MMODE_DIGITAL, PAD_MMODE_UNLOCK);
                }
                s_current_analog_mode = s_target_analog_mode;
            }

            struct padButtonStatus buttons __attribute__((aligned(64)));
            if (padRead(0, 0, &buttons) != 0) {
                native_btns = buttons.btns;

                // Se estiver no modo Analógico / DualShock (LED aceso):
                // buttons.ljoy_h e ljoy_v: Centro = 128 (0x80)
                int pad_type = buttons.mode >> 4;
                if (pad_type == 0x07 || pad_type == 0x05 || buttons.mode == 0x73 || buttons.mode == 0x79) {
                    if (buttons.ljoy_h < 64)  native_btns &= ~PAD_LEFT;
                    if (buttons.ljoy_h > 192) native_btns &= ~PAD_RIGHT;
                    if (buttons.ljoy_v < 64)  native_btns &= ~PAD_UP;
                    if (buttons.ljoy_v > 192) native_btns &= ~PAD_DOWN;
                }
            }
        }
    }

    // =========================================================================
    // 2. CONTROLE USB (IPEGA / DS3 / DS4)
    // =========================================================================
    if (s_ds34_connected) {
        if (s_ds34_rpc_busy) {
            if (SifCheckStatRpc(&s_ds34_client) == 0) {
                s_ds34_rpc_busy = 0;
                u8 b0 = s_ds34_buffer[0];
                u8 b1 = s_ds34_buffer[1];

                if (b0 != 0xFF || b1 != 0xFF) {
                    usb_btns = (b1 << 8) | b0;
                }

                // Lê o analógico esquerdo do controle USB se estiver conectado
                if (s_target_analog_mode) {
                    u8 lx = s_ds34_buffer[2];
                    u8 ly = s_ds34_buffer[3];
                    if (lx < 64)  usb_btns &= ~PAD_LEFT;
                    if (lx > 192) usb_btns &= ~PAD_RIGHT;
                    if (ly < 64)  usb_btns &= ~PAD_UP;
                    if (ly > 192) usb_btns &= ~PAD_DOWN;
                }
            }
        }

        if (!s_ds34_rpc_busy) {
            s_ds34_buffer[0] = 0;
            s_ds34_rpc_busy = 1;
            SifCallRpc(&s_ds34_client, 7, SIF_RPC_M_NOWAIT, s_ds34_buffer, 1, s_ds34_buffer, 18, NULL, NULL);
        }
    }

    // =========================================================================
    // 3. COMBINAÇÃO E INVERSÃO DE LÓGICA
    // =========================================================================
    u16 final_btns = native_btns & usb_btns;
    u32 data = 0xFFFF ^ final_btns;

    input->held = data;
    input->pressed = data & ~s_old_pad;
    s_old_pad = data;
}