#ifndef LANG_H
#define LANG_H

#define MAX_LANG_ENTRIES 96
#define LANG_KEY_LEN     32
#define LANG_VAL_LEN     64

typedef struct {
    char key[LANG_KEY_LEN];
    char value[LANG_VAL_LEN];
} LangEntry;

int         Lang_Load(const char *filepath);
const char* Lang_Get(const char *key, const char *fallback);
const char* Lang_GetCode(void);
void        Lang_SetCode(const char *code);
const char* Lang_GetTag(void); // Retorna "[ PT ]", "[ EN ]", "[ ES ]"

#endif /* LANG_H */