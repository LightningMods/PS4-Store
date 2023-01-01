#pragma once

#include "defines.h"
#include "ini.h"
#include <sys/param.h>   // MIN
#include "log.h"
#include "lang.h"
#include <sqlite3.h>
#include <stdatomic.h>
#include  "GLES2_common.h"
#include "utils.h"

#ifdef __ORBIS__
#include <user_mem.h> 
#include "installpkg.h"
#include <dialog.h>
#endif

#define DIM(x)  (sizeof(x)/sizeof(*(x)))
#define KB(x)   ((size_t) (x) << 10)
#define MB(x)   ((size_t) (x) << 20)
#define GB(x)   ((size_t) (x) << 30)
#define B2GB(x)   ((size_t) (x) >> 30)
#define B2MB(x)   ((size_t) (x) >> 20)
/*
* | ============================
* | SQL DEFINES
* |
*/


/*
| SQL Table names in ORDER
| ==============================
*/
#define DB_NAME " homebrews"

/*
| OTHER SQL COMMANDS
*/
#define SQL_COUNT "SELECT COUNT(*) FROM"DB_NAME
// Supply a Number
#define SQL_SELECT_ROW_BY_NUMB "SELECT * WHERE pid LIKE "
// Supply a Number
#define SQL_SELECT_NAME_BY_ROW_NUMB "SELECT name WHERE pid LIKE "
#define ORBIS_SYSMODULE_MESSAGE_DIALOG 0x00A4 // libSceMsgDialog.sprx
//DATABASE STORE USES
#define SQL_STORE_DB APP_PATH("store.db")


extern sqlite3* db;

bool SQL_Load_DB(const char* dir);
bool SQL_Exec(const char* smt, int (*cb)(void*, int, char**, char**));
int SQL_Get_Count(void);
/*
|
| SQL END
| =============================
*/

struct AppStatus
{
    int appId;
    int launchRequestAppId;
    char appType;
};

enum MSG_DIALOG {
    NORMAL,
    FATAL,
    WARNING
};

extern uint32_t daemon_appid;


enum SORT_APPS_BY {
    SORT_NA = -1,
    SORT_TID,
    SORT_ALPHABET
};


enum IPC_Errors
{
    INVALID = -1,
    NO_ERROR = 0,
    REDIRECT_OPERATION_FAILED = 1,
    DEAMON_UPDATING = 100
};


enum IPC_Commands
{
    CONNECTION_TEST = 1,
    ENABLE_HOME_REDIRECT = 2,
    DISABLE_HOME_REDIRECT = 3,
    DEAMON_UPDATE = 100
};




typedef struct SceNetEtherAddr {
    uint8_t data[6];
} SceNetEtherAddr;

typedef union SceNetCtlInfo {
    uint32_t device;
    SceNetEtherAddr ether_addr;
    uint32_t mtu;
    uint32_t link;
    SceNetEtherAddr bssid;
    char ssid[33];
    uint32_t wifi_security;
    int32_t rssi_dbm;
    uint8_t rssi_percentage;
    uint8_t channel;
    uint32_t ip_config;
    char dhcp_hostname[256];
    char pppoe_auth_name[128];
    char ip_address[16];
    char netmask[16];
    char default_route[16];
    char primary_dns[16];
    char secondary_dns[16];
    uint32_t http_proxy_config;
    char http_proxy_server[256];
    uint16_t http_proxy_port;
} SceNetCtlInfo;

#define STORE_MAX_LIMIT_PAGES 1000
#define STORE_PAGE_SIZE 15
#define STORE_LOG "/user/app/NPXS39041/logs/store.log"
#define STANDALONE_APP 0
#define DAEMON_PATH "/system/vsh/app/ITEM00002"
#define DAEMON_INI_DOESNT_EXIST 0
#define PENDING_DOWNLOADS -999
#define UNUSED(x) (void)(x)
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


extern int HDD_count;
enum CHECK_OPTS
{
    MD5_HASH,
    DL_COUNTER,
    COUNT,
#if BETA==1
    BETA_CHECK,  
#endif
    NUM_OF_STRING
};

#define SCE_SYSMODULE_INTERNAL_COMMON_DIALOG 0x80000018
#define SCE_SYSMODULE_INTERNAL_SYSUTIL 0x80000018

// indexed options
typedef struct
{
    char *opt[ NUM_OF_STRINGS ];
    atomic_bool auto_install, Legacy_Install;
    int lang;
    // more options
} StoreOptions;

// the Settings
extern StoreOptions set,
* get;
 
bool LoadOptions(StoreOptions *set);
bool SaveOptions(StoreOptions *set);

bool Keyboard(const char* Title, const char* initialTextBuffer, char* out_buffer, bool is_url);
char* StoreKeyboard(const char* Title, char* initialTextBuffer);
int sceMsgDialogTerminate();
void CheckUpdate(const char* tid, item_t *li);

// sysctl
uint32_t SysctlByName_get_sdk_version(void);

char *calculateSize(uint64_t size);

extern bool is_connected_app;
void  msgok(enum MSG_DIALOG level, const char* format, ...);
void  loadmsg(const char* format, ...);
void drop_some_icon0();
long CalcAppsize(char *path);
char* cutoff(const char* str, int from, int to);
bool touch_file(char* destfile);
int sql_index_tokens(layout_t* l, int count);


void* syscall1(
	uint64_t number,
	void* arg1
);

void* syscall2(
	uint64_t number,
	void* arg1,
	void* arg2
);

void* syscall3(
	uint64_t number,
	void* arg1,
	void* arg2,
	void* arg3
);

void* syscall4(
	uint64_t number,
	void* arg1,
	void* arg2,
	void* arg3,
	void* arg4
);

void* syscall5(
	uint64_t number,
	void* arg1,
	void* arg2,
	void* arg3,
	void* arg4,
	void* arg5
);
void trigger_dump_frame();
void print_memory();
void dump_frame(void);
int getjson(int Pagenumb, char* cdn, bool legacy);
bool MD5_hash_compare(const char* file1, const char* hash);
void CheckUpdate(const char* tid, item_t *li);
int copyFile(char* sourcefile, char* destfile);
void ProgSetMessagewText(int prog, const char* fmt, ...);
bool app_inst_util_uninstall_patch(const char* title_id, int* error);
bool app_inst_util_uninstall_game(const char *title_id, int *error);
char *check_from_url(const char *url_,  enum CHECK_OPTS opt, bool silent);
int check_store_from_url(char* cdn, enum CHECK_OPTS opt);
int check_download_counter(StoreOptions* set, char* title_id);
bool rmtree(const char path[]);
void setup_store_assets(StoreOptions* get);
void refresh_apps_for_cf(enum SORT_APPS_BY op);
int64_t sys_dynlib_load_prx(char* prxPath, int* moduleID);
int64_t sys_dynlib_unload_prx(int64_t prxID);
int64_t sys_dynlib_dlsym(int64_t moduleHandle, const char* functionName, void* destFuncOffset);
struct sockaddr_in IPCAddress(uint16_t port);
int OpenConnection(const char* name);
bool IPCOpenConnection();
int IPCReceiveData(uint8_t* buffer, int32_t size);
int IPCSendData(uint8_t* buffer, int32_t size);
int IPCCloseConnection();
#define DAEMON_BUFF_MAX 100
void GetIPCMessageWithoutError(uint8_t* buf, uint32_t sz);
uint32_t Launch_App(char* TITLE_ID, bool silent);
int mountfs(const char* device, const char* mountpoint, const char* fstype, const char* mode, uint64_t flags);
int check_free_space(const char* mountPoint);
extern bool dump;

#define SCE_LNC_UTIL_ERROR_ALREADY_RUNNING 0x8094000c

extern char* title[300];
extern char* title_id[30]; 

void SIG_Handler(int sig_numb);
const char* Language_GetName(int m_Code);
unsigned int usbpath();
int progstart(char* format, ...);
void loadModulesVanilla();
int number_of_iapps(const char *path);
bool init_curl();
int syscall_alt(long num, ...);
void* prx_func_loader(const char* prx_path, const char* symbol);
bool pingtest(const char* server);