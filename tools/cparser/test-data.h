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

#define CCSIZEOF_STRUCT(structname, member)  (((int)((char *)(&((structname*)0)->member) - ((char *)((structname*)0)))) + sizeof(((structname*)0)->member))

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((unsigned long)(unsigned char)(ch0) | ((unsigned long)(unsigned char)(ch1) << 8) |   \
                ((unsigned long)(unsigned char)(ch2) << 16) | ((unsigned long)(unsigned char)(ch3) << 24 ))
#define mmioFOURCC(ch0, ch1, ch2, ch3)  MAKEFOURCC(ch0, ch1, ch2, ch3)
#define ACMDRIVERDETAILS_FCCCOMP_UNDEFINED  mmioFOURCC('\0', '\0', '\0', '\0')

#define _HUGE_ENUF  1e+300	/* _HUGE_ENUF*_HUGE_ENUF must overflow */

#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))  /* causes warning C4756: overflow in constant arithmetic (by design) */
#define NAN        ((float)(INFINITY * 0.0F))
