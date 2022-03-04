#pragma once
#include <ps4sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "defines.h"
#include "log.h"

#define DIM(x)  (sizeof(x)/sizeof(*(x)))
#define KB(x)   ((size_t) (x) << 10)
#define MB(x)   ((size_t) (x) << 20)

#define DAEMON_LOG_PS4 "/data/itemzflow_daemon/log.txt"
#define DAEMON_LOG_USB "/mnt/usb0/itemzflow/log.txt"
bool touch_file(char* destfile);
enum IPC_Errors
{
    INVALID = -1,
    NO_ERROR = 0,
    OPERATION_FAILED = 1
};

enum cmd
{
    CONNECTION_TEST = 1,
    ENABLE_HOME_REDIRECT = 2,
    DISABLE_HOME_REDIRECT = 3,
    DEAMON_UPDATE = 100
};


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


struct clientArgs {
    char* ip;
    int socket;
    int cl_nmb;
};

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

typedef struct OrbisUserServiceLoginUserIdList {
    int32_t userId[4];
}  OrbisUserServiceLoginUserIdList;

typedef struct OrbisUserServiceInitializeParams {
    int32_t priority;
} OrbisUserServiceInitializeParams;


int64_t sys_dynlib_load_prx(char* prxPath, int* moduleID);
int64_t sys_dynlib_unload_prx(int64_t prxID);
int64_t sys_dynlib_dlsym(int64_t moduleHandle, const char* functionName, void* destFuncOffset);

bool rejail();
bool jailbreak(const char* prx_path);
bool full_init();
bool isRestMode();
bool IsOn();
void notify(char* message);

extern int DaemonSocket;

#define networkSendMessage(socket, format, ...)\
do {\
	char msgBuffer[512];\
	int msgSize = sprintf(msgBuffer, format, ##__VA_ARGS__);\
	sceNetSend(socket, msgBuffer, msgSize, 0);\
} while(0)

void handleIPC(struct clientArgs* client, uint8_t* buffer, uint32_t length);
void* network_loop();
void* ipc_client(void* args);
bool check_update_from_url(const char* url);