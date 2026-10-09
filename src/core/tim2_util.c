#include "tim2_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fileXio_rpc.h>

typedef struct {
    char FileId[4];
    u8 FormatVersion, FormatId;
    u16 Pictures;
    u8 Reserved[8];
} __attribute__((packed)) TIM2_FILEHEADER;

typedef struct {
    u32 TotalSize, ClutSize, ImageSize;
    u16 HeaderSize, ClutColors;
    u8 PictFormat, MipMapTextures, ClutType, ImageType;
    u16 ImageWidth, ImageHeight;
    u64 GsTex0, GsTex1;
    u32 GsRegs, GsTexClut;
} __attribute__((packed)) TIM2_PICTUREHEADER;

// Buffer estático alinhado em 128 bytes para 256x256 (130 KB total, zero malloc)
#define OUT_W 256
#define OUT_H 256
#define TIM2_BUF_SIZE (sizeof(TIM2_FILEHEADER) + sizeof(TIM2_PICTUREHEADER) + (OUT_W * OUT_H * 2))
static u8 s_tim2_work_buf[TIM2_BUF_SIZE] __attribute__((aligned(128)));

int TIM2_SaveFrom16Bit(const char *filepath, const u16 *src_pixels, int active_w, int active_h, int pitch_w) {
    if (!src_pixels || active_w <= 0 || active_h <= 0 || pitch_w <= 0) return 0;

    u32 img_size = OUT_W * OUT_H * sizeof(u16);
    u32 total_size = sizeof(TIM2_FILEHEADER) + sizeof(TIM2_PICTUREHEADER) + img_size;

    memset(s_tim2_work_buf, 0, sizeof(TIM2_FILEHEADER) + sizeof(TIM2_PICTUREHEADER));

    TIM2_FILEHEADER *fhdr = (TIM2_FILEHEADER*)s_tim2_work_buf;
    memcpy(fhdr->FileId, "TIM2", 4);
    fhdr->FormatVersion = 0x04;
    fhdr->Pictures = 1;

    TIM2_PICTUREHEADER *phdr = (TIM2_PICTUREHEADER*)(s_tim2_work_buf + sizeof(TIM2_FILEHEADER));
    phdr->TotalSize = sizeof(TIM2_PICTUREHEADER) + img_size;
    phdr->ImageSize = img_size;
    phdr->HeaderSize = sizeof(TIM2_PICTUREHEADER);
    phdr->PictFormat = 1; // 16-bit
    phdr->MipMapTextures = 1;
    phdr->ImageType = 1;  // RGBA16
    phdr->ImageWidth = OUT_W;
    phdr->ImageHeight = OUT_H;

    u16 *dst = (u16*)(s_tim2_work_buf + sizeof(TIM2_FILEHEADER) + sizeof(TIM2_PICTUREHEADER));

    // Escala acelerada por bitshift (>> 8 substitui divisão lenta da CPU do PS2)
    for (int y = 0; y < OUT_H; y++) {
        int sy = (y * active_h) >> 8;
        int row_offset = sy * pitch_w;
        int dst_row = y * OUT_W;
        for (int x = 0; x < OUT_W; x++) {
            int sx = (x * active_w) >> 8;
            dst[dst_row + x] = src_pixels[row_offset + sx];
        }
    }

    int fd = fileXioOpen(filepath, 0x0002 | 0x0200 | 0x0400, 0666);
    if (fd >= 0) {
        fileXioWrite(fd, s_tim2_work_buf, total_size);
        fileXioClose(fd);
        return 1;
    }

    FILE *f = fopen(filepath, "wb");
    if (f) {
        fwrite(s_tim2_work_buf, 1, total_size, f);
        fclose(f);
        return 1;
    }

    return 0;
}