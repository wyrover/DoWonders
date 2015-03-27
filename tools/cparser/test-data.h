#define MAX_LANA       254

typedef struct _LANA_ENUM {
    unsigned char length;
    unsigned char lana[MAX_LANA+1];
} LANA_ENUM, *PLANA_ENUM;

#define TEST MAX_LANA + 1

#if defined(UNICODE)
# define __MINGW_NAME_AW(func) func##W
#else
# define __MINGW_NAME_AW(func) func##A
#endif

int lstrlenA(const char *str);
int lstrlenW(const unsigned short *str);

#define lstrlen __MINGW_NAME_AW(lstrlen)
