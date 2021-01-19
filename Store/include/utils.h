#pragma once

#include "defines.h"


#include <MsgDialog.h>
#include <ImeDialog.h>
#include <CommonDialog.h>
#include <KeyboardDialog.h>
#include <pl_ini.h>

#define DIM(x)  (sizeof(x)/sizeof(*(x)))

#define NORMAL    1
#define FATAL     2
#define WARNING   3
#define STORE_LOG "/user/app/NPXS39041/logs/log.txt"

static const char *option_panel_text[] =
{
    "Content Delivery Network",
    "Temporary Path",
    "Detected USB",
    "INI Path",
    "Custom FreeType font Path",
    // following doesn't store strings for any Paths...
    "Store Downloads on USB",
    "Save Settings"
};

enum STR_type
{
    CDN_URL,
    TMP_PATH,
    USB_PATH,
    INI_PATH,
    FNT_PATH,
    NUM_OF_STRINGS
};

// indexed options
typedef struct
{
    char *opt[ NUM_OF_STRINGS ];
    int   StoreOnUSB;
    // more options
} StoreOptions;

char *usbpath(void);
int LoadOptions(StoreOptions *set);
int SaveOptions(StoreOptions *set);

// sysctl
uint32_t SysctlByName_get_sdk_version(void);

char *checkedsize;
char *calculateSize(uint64_t size);

void *(*libc_memset)(void *, int, size_t);

int  msgok(int level, char* format, ...);
int  loadmsg(char* format, ...);
void klog(const char *format, ...);
void CalcAppsize(char *path);
char* cutoff(const char* str, int from, int to);

int getjson(int Pagenumb, char* cdn);
int MD5_hash_compare(const char* file1, const char* hash);



#define assert(expr) if (!(expr)) msgok(FATAL, "Assertion Failed!");

