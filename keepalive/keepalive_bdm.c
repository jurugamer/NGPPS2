#include <types.h>
#include <irx.h>
#include <loadcore.h>
#include "irx_imports.h"

#define MODNAME "bdm_keepalive"
IRX_ID(MODNAME, 1, 1);

static int keepalive_thid = -1;
static u8 keepalive_buf[512] __attribute__((aligned(64)));

// Thread idêntica à do OPL (roda a cada 60 segundos)
static void keepalive_thread(void *arg)
{
    struct block_device *pbd[1];

    while (1) {
        // Dorme por 60 segundos (60.000.000 microssegundos)
        DelayThread(60 * 1000 * 1000);

        // Pega o dispositivo de bloco USB conectado no BDM
        pbd[0] = NULL;
        bdm_get_bd(pbd, 1);

        // Se o HD estiver conectado, força a leitura física no LBA 0 (SCSI READ 10)
        if (pbd[0] != NULL) {
            pbd[0]->read(pbd[0], 0, keepalive_buf, 1);
        }
    }
}

int _start(int argc, char *argv[])
{
    iop_thread_t th;
    th.attr = TH_C;
    th.thread = keepalive_thread;
    th.priority = 0x75; // Prioridade ultra-baixa (não afeta o jogo)
    th.stacksize = 0x800;
    th.option = 0;

    keepalive_thid = CreateThread(&th);
    if (keepalive_thid > 0) {
        StartThread(keepalive_thid, NULL);
    }

    return MODULE_RESIDENT_END;
}