#ifndef __SPR_MAP_H__
#define __SPR_MAP_H__

#include <stdint.h>

#define SPR_BASE_ADDR        0x70000000

// [0x0000 - 0x02FF] 768 Bytes: Buffer de Scanline
#define SPR_CFB_OFFSET       0x0000

// [0x0300 - 0x06FF] 1024 Bytes: Tabela de Páginas de Leitura
#define SPR_MEM_PAGE_READ    ((uint8_t**)(SPR_BASE_ADDR + 0x0300))

// [0x0700 - 0x0AFF] 1024 Bytes: Tabela de Páginas de Escrita
#define SPR_MEM_PAGE_WRITE   ((uint8_t**)(SPR_BASE_ADDR + 0x0700))

// [0x0B00 - 0x0B5F] 96 Bytes: Ponteiros de Registradores Ativos no SPR (0 ciclos de espera!)
#define SPR_CUR_GPR_MAP_B    ((uint8_t**)(SPR_BASE_ADDR + 0x0B00))
#define SPR_CUR_GPR_MAP_W    ((uint16_t**)(SPR_BASE_ADDR + 0x0B20))
#define SPR_CUR_GPR_MAP_L    ((uint32_t**)(SPR_BASE_ADDR + 0x0B40))

#endif