#pragma once


#include "defines.h"


#include <MsgDialog.h>
#include <ImeDialog.h>
#include <CommonDialog.h>
#include <KeyboardDialog.h>

#define DIM(x)  (sizeof(x)/sizeof(*(x)))

#define NORMAL    1
#define FATAL     2
#define WARNING   3
#define STORE_LOG "/user/app/NPXS39041/logs/log.txt"


typedef struct
{
    char* StoreCDN;
    char* temppath;
    int StoreOnUSB;
    char*  USBPath;
    char* INIPath;
} StoreOptions;

char *checkedsize;
char *calculateSize(uint64_t size);

void *(*libc_memset)(void *, int, size_t);

int  msgok(int level, char* format, ...);
int  loadmsg(char* format, ...);
void klog(const char *format, ...);
void CalcAppsize(char *path);
char* cutoff(const char* str, int from, int to);
uint32_t SysctlByName_get_sdk_version(void);
char* usbpath(void);
void SaveOptions(StoreOptions* set);
int getjson(int Pagenumb, char* cdn);
int LoadOptions(StoreOptions* set);
int MD5_hash_compare(const char* file1, const char* hash);