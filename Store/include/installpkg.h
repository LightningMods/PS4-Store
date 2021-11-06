#include <stdarg.h>
#include <stdlib.h> // calloc

#pragma once

#include "utils.h"

struct _SceBgftDownloadRegisterErrorInfo {
    /* TODO */
    uint8_t buf[0x100];
};

enum bgft_task_option_t {
    BGFT_TASK_OPTION_NONE = 0x0,
    BGFT_TASK_OPTION_DELETE_AFTER_UPLOAD = 0x1,
    BGFT_TASK_OPTION_INVISIBLE = 0x2,
    BGFT_TASK_OPTION_ENABLE_PLAYGO = 0x4,
    BGFT_TASK_OPTION_FORCE_UPDATE = 0x8,
    BGFT_TASK_OPTION_REMOTE = 0x10,
    BGFT_TASK_OPTION_COPY_CRASH_REPORT_FILES = 0x20,
    BGFT_TASK_OPTION_DISABLE_INSERT_POPUP = 0x40,
    BGFT_TASK_OPTION_DISABLE_CDN_QUERY_PARAM = 0x10000,
};

struct bgft_download_param {
    int user_id;
    int entitlement_type;
    const char* id;
    const char* content_url;
    const char* content_ex_url;
    const char* content_name;
    const char* icon_path;
    const char* sku_id;
    enum bgft_task_option_t option;
    const char* playgo_scenario_id;
    const char* release_date;
    const char* package_type;
    const char* package_sub_type;
    unsigned long package_size;
};



struct bgft_download_param_ex {
    struct bgft_download_param param;
    unsigned int slot;
};



struct bgft_task_progress_internal {
    unsigned int bits;
    int error_result;
    unsigned long length;
    unsigned long transferred;
    unsigned long length_total;
    unsigned long transferred_total;
    unsigned int num_index;
    unsigned int num_total;
    unsigned int rest_sec;
    unsigned int rest_sec_total;
    int preparing_percent;
    int local_copy_percent;
};


struct bgft_init_params {
    void  *heap;
    size_t heapSize;
};


int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppInstallPkg(const char* file_path, int reserved);
int sceAppInstUtilGetTitleIdFromPkg(const char* pkg_path, char* title_id, int* is_app);
int sceAppInstUtilCheckAppSystemVer(const char* title_id, uint64_t buf, uint64_t bufs);
int sceAppInstUtilAppPrepareOverwritePkg(const char* pkg_path);
int sceAppInstUtilGetPrimaryAppSlot(const char* title_id, unsigned int* slot);
int sceAppInstUtilAppUnInstall(char* title_id);
int sceAppInstUtilAppGetSize(const char* title_id, uint64_t buf);
int sceBgftServiceInit(struct bgft_init_params*  params);
int sceBgftServiceIntDownloadRegisterTaskByStorageEx(struct bgft_download_param_ex* params, int* task_id);
int sceBgftServiceDownloadStartTask(int task_id);
void sceBgftFinalize(void);
