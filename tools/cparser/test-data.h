#define MAX_LANA       254

typedef struct _LANA_ENUM {
    unsigned char length;
    unsigned char lana[MAX_LANA+1];
} LANA_ENUM, *PLANA_ENUM;
