#pragma once

#include "defines.h"

#include <pl_ini.h>
#include <MsgDialog.h>
#include <ImeDialog.h>
#include <CommonDialog.h>
#include <KeyboardDialog.h>
#include <sys/param.h>   // MIN
#include "log.h"

#define DIM(x)  (sizeof(x)/sizeof(*(x)))
#define KB(x)   ((size_t) (x) << 10)
#define MB(x)   ((size_t) (x) << 20)

enum MSG_DIALOG {
    NORMAL,
    FATAL,
    WARNING
};

#define STORE_LOG "/user/app/NPXS39041/logs/log.txt"

//#define assert(expr) if (!(expr)) msgok(FATAL, "Assertion Failed!");

enum Flag
{
    Flag_None = 0,
    SkipLaunchCheck = 1,
    SkipResumeCheck = 1,
    SkipSystemUpdateCheck = 2,
    RebootPatchInstall = 4,
    VRMode = 8,
    NonVRMode = 16
};

typedef struct _LncAppParam
{
    uint32_t sz;
    uint32_t user_id;
    uint32_t app_opt;
    uint64_t crash_report;
    enum Flag check_flag;
}
LncAppParam;


static const char *option_panel_text[] =
{
    "Content Delivery Network",
    "Temporary Path",
    "Detected USB",
    "INI Path",
    "Custom FreeType font Path",
    // following doesn't store strings for any Paths...
    "Store Downloads on USB",
    "Clear cached images", //(deletes storedata and covers)
    "Enable CF reflections",
    "Enable CF bg shader",
    "Save Settings"
};

#define BETA 0
//#define LOCALHOST_WINDOWS 1

enum STR_type
{
    CDN_URL,
    TMP_PATH,
    USB_PATH,
    INI_PATH,
    FNT_PATH,
#if BETA==1
    BETA_KEY,
#endif
    NUM_OF_STRINGS
};


enum CHECK_OPTS
{
    MD5_HASH,
    COUNT,
    DL_COUNTER,
#if BETA==1
    BETA_CHECK,  
#endif
    NUM_OF_STRING
};

// indexed options
typedef struct
{
    char *opt[ NUM_OF_STRINGS ];
    int   StoreOnUSB;
    int   Legacy;
    // more options
} StoreOptions;

char *usbpath(void);
int LoadOptions(StoreOptions *set);
int SaveOptions(StoreOptions *set);

char *StoreKeyboard(const char *Title, char *initialTextBuffer);


// sysctl
uint32_t SysctlByName_get_sdk_version(void);

char *calculateSize(uint64_t size);


int  msgok(enum MSG_DIALOG level, char* format, ...);
int  loadmsg(char* format, ...);
long CalcAppsize(char *path);
char* cutoff(const char* str, int from, int to);

int getjson(int Pagenumb, char* cdn, bool legacy);
int MD5_hash_compare(const char* file1, const char* hash);
int copyFile(char* sourcefile, char* destfile);
void ProgSetMessagewText(int prog, const char* fmt, ...);
bool app_inst_util_uninstall_patch(const char* title_id, int* error);
bool app_inst_util_uninstall_game(const char *title_id, int *error);
char *check_from_url(const char *url_,  enum CHECK_OPTS opt);
int check_store_from_url(int page_number, char* cdn, enum CHECK_OPTS opt);
int check_download_counter(StoreOptions* set, char* title_id);
bool rmtree(const char path[]);
void setup_store_assets(StoreOptions* get);
void refresh_apps_for_cf(void);
//int check_store_from_url(char* cdn, enum CHECK_OPTS opt, int *page_number);

extern bool dump;

typedef struct OrbisUserServiceLoginUserIdList {
	int32_t userId[4];
}  OrbisUserServiceLoginUserIdList;

typedef struct OrbisUserServiceInitializeParams {
	int32_t priority;
} OrbisUserServiceInitializeParams;

#define SCE_LNC_UTIL_ERROR_ALREADY_RUNNING 0x8094000c

