#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#ifndef NEWLIB_PORT_AWARE
#define NEWLIB_PORT_AWARE
#endif

#include <tamtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <fileXio_rpc.h>

// Garante as constantes de modo de leitura para o IOP e Newlib
#ifndef FIO_O_RDONLY
#define FIO_O_RDONLY 0x0001
#endif

#ifndef O_RDONLY
#define O_RDONLY 0x0000
#endif

// -----------------------------------------------------------------------------
// ABERTURA UNIVERSAL (Usa FIO_O_RDONLY para USB e O_RDONLY para host:)
// -----------------------------------------------------------------------------
static inline int OpenFileUniversal(const char *path, int *is_filexio) {
    if (is_filexio) *is_filexio = 0;

    // 1. Tenta o caminho exato passado
    int fd = fileXioOpen(path, FIO_O_RDONLY);
    if (fd >= 0) {
        if (is_filexio) *is_filexio = 1;
        return fd;
    }

    int p_fd = open(path, O_RDONLY);
    if (p_fd >= 0) {
        if (is_filexio) *is_filexio = 0;
        return p_fd;
    }

    // 2. Variação: se tiver "mass0:/", tenta "mass0:" e vice-versa
    char alt_path[256];
    char *colon = strchr(path, ':');
    if (colon) {
        if (colon[1] == '/') {
            snprintf(alt_path, sizeof(alt_path), "%.*s:%s", (int)(colon - path), path, colon + 2);
        } else {
            snprintf(alt_path, sizeof(alt_path), "%.*s:/%s", (int)(colon - path), path, colon + 1);
        }

        fd = fileXioOpen(alt_path, FIO_O_RDONLY);
        if (fd >= 0) {
            if (is_filexio) *is_filexio = 1;
            return fd;
        }

        p_fd = open(alt_path, O_RDONLY);
        if (p_fd >= 0) {
            if (is_filexio) *is_filexio = 0;
            return p_fd;
        }
    }

    return -1;
}

static inline int ReadUniversal(int fd, int is_filexio, void *buf, int count) {
    if (is_filexio) {
        return fileXioRead(fd, buf, count);
    } else {
        return read(fd, buf, count);
    }
}

static inline void CloseUniversal(int fd, int is_filexio) {
    if (is_filexio) {
        fileXioClose(fd);
    } else {
        close(fd);
    }
}

// -----------------------------------------------------------------------------
// Leitor de Texto Seguro (Sem depender de SEEK_END)
// -----------------------------------------------------------------------------
static inline int ReadTextFile(const char* filepath, char* out_buf, int max_size) {
    int is_filexio = 0;
    int fd = OpenFileUniversal(filepath, &is_filexio);
    if (fd < 0) return 0;

    int total = 0;
    while (total < max_size - 1) {
        int r = ReadUniversal(fd, is_filexio, out_buf + total, max_size - 1 - total);
        if (r <= 0) break;
        total += r;
    }
    CloseUniversal(fd, is_filexio);
    out_buf[total] = '\0';
    return total;
}
// -----------------------------------------------------------------------------
// Descobre o tamanho exato do arquivo no PS2 (host: ou USB)
// -----------------------------------------------------------------------------
static inline int GetFileSizeUniversal(int fd, int is_filexio) {
    int size = -1;
    if (is_filexio) {
        size = fileXioLseek(fd, 0, 2); // 2 = FIO_SEEK_END
        fileXioLseek(fd, 0, 0);        // 0 = FIO_SEEK_SET (rebobina)
    } else {
        size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
    }
    return size;
}

// -----------------------------------------------------------------------------
// Leitor Universal Seguro para ROMs de até 4 MB (Sem estourar a RAM)
// -----------------------------------------------------------------------------
static inline int ReadFileToBuffer(const char *path, u8 **out_buf, u32 *out_size) {
    if (out_buf) *out_buf = NULL;
    if (out_size) *out_size = 0;

    int is_filexio = 0;
    int fd = OpenFileUniversal(path, &is_filexio);
    if (fd < 0) return -1;

    // 1. Descobre o tamanho exato do arquivo primeiro:
    int file_size = GetFileSizeUniversal(fd, is_filexio);

    // Se soubermos o tamanho exato (funciona em 99% das vezes):
    if (file_size > 0) {
        // Aloca EXATAMENTE o tamanho da ROM (evita pedir 8 MB à toa no PS2):
        u8 *buf = (u8*)memalign(128, file_size + 16);
        if (!buf) {
            CloseUniversal(fd, is_filexio);
            return -1;
        }

        // Lê o arquivo em fatias seguras de 64 KB direto na memória:
        int total_read = 0;
        while (total_read < file_size) {
            int chunk = file_size - total_read;
            if (chunk > 64 * 1024) chunk = 64 * 1024;

            int r = ReadUniversal(fd, is_filexio, buf + total_read, chunk);
            if (r <= 0) break;
            total_read += r;
        }

        CloseUniversal(fd, is_filexio);

        if (total_read <= 0) {
            free(buf);
            return -1;
        }

        buf[total_read] = '\0';
        if (out_buf) *out_buf = buf;
        if (out_size) *out_size = (u32)total_read;
        return total_read;
    }

    // 2. FALLBACK SEGURO (Caso algum dispositivo estranho não suporte SEEK_END):
    // Em vez de dobrar descontroladamente para 8MB, cresce suavemente de 512KB em 512KB!
    int capacity = 512 * 1024;
    u8 *buf = (u8*)memalign(128, capacity);
    if (!buf) {
        CloseUniversal(fd, is_filexio);
        return -1;
    }

    int total_read = 0;
    while (1) {
        if (total_read + (64 * 1024) > capacity) {
            capacity += (512 * 1024); // Crescimento linear seguro
            u8 *new_buf = (u8*)memalign(128, capacity);
            if (!new_buf) {
                free(buf);
                CloseUniversal(fd, is_filexio);
                return -1;
            }
            memcpy(new_buf, buf, total_read);
            free(buf);
            buf = new_buf;
        }

        int r = ReadUniversal(fd, is_filexio, buf + total_read, 64 * 1024);
        if (r <= 0) break;
        total_read += r;
    }
    CloseUniversal(fd, is_filexio);

    if (total_read <= 0) {
        free(buf);
        return -1;
    }

    buf[total_read] = '\0';
    if (out_buf) *out_buf = buf;
    if (out_size) *out_size = (u32)total_read;
    return total_read;
}
#endif /* FILE_UTIL_H */