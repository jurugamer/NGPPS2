#include "lang.h"
#include "../core/file_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static LangEntry s_entries[MAX_LANG_ENTRIES];
static int s_count = 0;
static char s_cur_code[16] = "pt_br";

static char* trim(char* str) {
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    end[1] = '\0';
    return str;
}

int Lang_Load(const char *filepath) {
    u8 *buf = NULL;
    u32 size = 0;
    if (ReadFileToBuffer(filepath, &buf, &size) <= 0 || !buf) return 0;

    s_count = 0;
    char *line = strtok((char*)buf, "\r\n");
    while (line != NULL && s_count < MAX_LANG_ENTRIES) {
        char *l = trim(line);
        if (l[0] != '#' && l[0] != ';' && l[0] != '[' && l[0] != '\0') {
            char *eq = strchr(l, '=');
            if (eq != NULL) {
                *eq = '\0';
                char *k = trim(l);
                char *v = trim(eq + 1);
                strncpy(s_entries[s_count].key, k, LANG_KEY_LEN - 1);
                strncpy(s_entries[s_count].value, v, LANG_VAL_LEN - 1);
                s_count++;
            }
        }
        line = strtok(NULL, "\r\n");
    }
    free(buf);
    return 1;
}

const char* Lang_Get(const char *key, const char *fallback) {
    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_entries[i].key, key) == 0) return s_entries[i].value;
    }
    return fallback ? fallback : key;
}

const char* Lang_GetCode(void) {
    return s_cur_code;
}

void Lang_SetCode(const char *code) {
    if (code) {
        strncpy(s_cur_code, code, sizeof(s_cur_code) - 1);
        s_cur_code[sizeof(s_cur_code) - 1] = '\0';
    }
}

const char* Lang_GetTag(void) {
    if (strcmp(s_cur_code, "en_us") == 0) return "[ EN ]";
    if (strcmp(s_cur_code, "es_es") == 0) return "[ ES ]";
    return "[ PT ]";
}