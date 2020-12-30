#pragma once

#include "md5.h"
#include <libSceSysmodule.h>
#include <CommonDialog.h>
#include <libkernel.h>
#include <orbis/libkernel.h>
#include <orbislink.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <Http.h>
#include <Ssl.h>
#include <sys/_types.h>
#include <sys/fcntl.h> //O_CEAT

#define SCE_KERNEL_MAX_MODULES 256
#define SCE_KERNEL_MAX_NAME_LENGTH 256
#define SCE_KERNEL_MAX_SEGMENTS 4
#define SCE_KERNEL_NUM_FINGERPRINT 20

#ifndef ARRAY_SIZE
#	define ARRAY_SIZE(ar) (sizeof(ar) / sizeof((ar)[0]))
#endif


int msgok(char* format, ...);
int loadmsg(char* format, ...);
int pingtest(int libnetMemId, int libhttpCtxId, const char* src);
int download_file(int libnetMemId, int libhttpCtxId, const char* src, const char* dst);
int32_t netInit(void);
void logshit(char* format, ...);
void init_STOREGL_modules();
int pl_ini_load(pl_ini_file* file, const char* path);
int pl_ini_get_string(const pl_ini_file* file, const char* section, const char* key, const char* default_value, char* copy_to, int dest_len);

#define DIFFERENT_HASH 1
#define SAME_HASH 0

#define SSL_HEAP_SIZE	(304 * 1024)
#define NET_HEAP_SIZE	(16 * 1024)

#define DKS_TIMEOUT 0x804101E2


#define klog printf

#define TRUE 1
#define FALSE 0

struct filedesc {
	void* useless1[3];
	void* fd_rdir;
	void* fd_jdir;
};

struct proc {
	char useless[64];
	struct ucred* p_ucred;
	struct filedesc* p_fd;
};

struct thread {
	void* useless;
	struct proc* td_proc;
};

struct auditinfo_addr {
	char useless[184];
};

struct ucred {
	uint32_t useless1;
	uint32_t cr_uid;     // effective user id
	uint32_t cr_ruid;    // real user id
	uint32_t useless2;
	uint32_t useless3;
	uint32_t cr_rgid;    // real group id
	uint32_t useless4;
	void* useless5;
	void* useless6;
	void* cr_prison;     // jail(2)
	void* useless7;
	uint32_t useless8;
	void* useless9[2];
	void* useless10;
	struct auditinfo_addr useless11;
	uint32_t* cr_groups; // groups
	uint32_t useless12;
};

void logshit(char* format, ...);

#define SSL_HEAP_SIZE	(304 * 1024)
#define HTTP_HEAP_SIZE	(48 * 1024)
#define NET_HEAP_SIZE	(16 * 1024)
#define TEST_USER_AGENT	"StoreHAX/GL"

#define SCE_SYSMODULE_INTERNAL_SYS_CORE 0x80000004
#define SCE_SYSMODULE_INTERNAL_NETCTL 0x80000009
#define SCE_SYSMODULE_INTERNAL_HTTP 0x8000000A
#define SCE_SYSMODULE_INTERNAL_SSL 0x8000000B
#define SCE_SYSMODULE_INTERNAL_NP_COMMON 0x8000000C
#define SCE_SYSMODULE_INTERNAL_SYSTEM_SERVICE 0x80000010
#define SCE_SYSMODULE_INTERNAL_USER_SERVICE 0x80000011
#define SCE_SYSMODULE_INTERNAL_APPINSTUTIL 0x80000014
#define SCE_SYSMODULE_INTERNAL_NET 0x8000001C
#define SCE_SYSMODULE_INTERNAL_IPMI 0x8000001D
#define SCE_SYSMODULE_INTERNAL_VIDEO_OUT 0x80000022
#define SCE_SYSMODULE_INTERNAL_BGFT 0x8000002A
#define SCE_SYSMODULE_INTERNAL_PRECOMPILED_SHADERS 0x80000064


#define VERSION_MAJOR 1
#define VERSION_MINOR 4

#define BUILD_YEAR_CH0 (__DATE__[ 7])
#define BUILD_YEAR_CH1 (__DATE__[ 8])
#define BUILD_YEAR_CH2 (__DATE__[ 9])
#define BUILD_YEAR_CH3 (__DATE__[10])


#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')


#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')

#define BUILD_MONTH_CH1 \
    ( \
        (BUILD_MONTH_IS_JAN) ? '1' : \
        (BUILD_MONTH_IS_FEB) ? '2' : \
        (BUILD_MONTH_IS_MAR) ? '3' : \
        (BUILD_MONTH_IS_APR) ? '4' : \
        (BUILD_MONTH_IS_MAY) ? '5' : \
        (BUILD_MONTH_IS_JUN) ? '6' : \
        (BUILD_MONTH_IS_JUL) ? '7' : \
        (BUILD_MONTH_IS_AUG) ? '8' : \
        (BUILD_MONTH_IS_SEP) ? '9' : \
        (BUILD_MONTH_IS_OCT) ? '0' : \
        (BUILD_MONTH_IS_NOV) ? '1' : \
        (BUILD_MONTH_IS_DEC) ? '2' : \
        /* error default */    '?' \
    )

#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[ 5])



// Example of __TIME__ string: "21:06:19"
//                              01234567

#define BUILD_HOUR_CH0 (__TIME__[0])
#define BUILD_HOUR_CH1 (__TIME__[1])

#define BUILD_MIN_CH0 (__TIME__[3])
#define BUILD_MIN_CH1 (__TIME__[4])

#define BUILD_SEC_CH0 (__TIME__[6])
#define BUILD_SEC_CH1 (__TIME__[7])


#if VERSION_MAJOR > 100

#define VERSION_MAJOR_INIT \
    ((VERSION_MAJOR / 100) + '0'), \
    (((VERSION_MAJOR % 100) / 10) + '0'), \
    ((VERSION_MAJOR % 10) + '0')

#elif VERSION_MAJOR > 10

#define VERSION_MAJOR_INIT \
    ((VERSION_MAJOR / 10) + '0'), \
    ((VERSION_MAJOR % 10) + '0')

#else

#define VERSION_MAJOR_INIT \
    (VERSION_MAJOR + '0')

#endif

#if VERSION_MINOR > 100

#define VERSION_MINOR_INIT \
    ((VERSION_MINOR / 100) + '0'), \
    (((VERSION_MINOR % 100) / 10) + '0'), \
    ((VERSION_MINOR % 10) + '0')

#elif VERSION_MINOR > 10

#define VERSION_MINOR_INIT \
    ((VERSION_MINOR / 10) + '0'), \
    ((VERSION_MINOR % 10) + '0')

#else

#define VERSION_MINOR_INIT \
    (VERSION_MINOR + '0')
#endif