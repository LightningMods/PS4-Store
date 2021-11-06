
#include "Header.h"
#include <stdarg.h>
#include <stdlib.h> // calloc
#include <installpkg.h>
#include <stdbool.h>

#include "defines.h"


int PKG_ERROR(const char* name, int ret)
{
    msgok(WARNING, "Install Failed with codeHEX: %x Int: %ifrom Function %s", ret, ret, name);
    log_error( "%s error: %i", ret);

    return ret;
}

/* we use bgft heap menagement as init/fini as flatz already shown at 
 * https://github.com/flatz/ps4_remote_pkg_installer/blob/master/installer.c
 */

#define BGFT_HEAP_SIZE (1 * 1024 * 1024)

static bool   s_app_inst_util_initialized = false;
static bool   s_bgft_initialized = false;
static struct bgft_init_params  s_bgft_init_params;

bool app_inst_util_init(void) {
    int ret;

    if (s_app_inst_util_initialized) {
        goto done;
    }

    ret = sceAppInstUtilInitialize();
    if (ret) {
        log_debug( "sceAppInstUtilInitialize failed: 0x%08X", ret);
        goto err;
    }

    s_app_inst_util_initialized = true;

done:
    return true;

err:
    s_app_inst_util_initialized = false;

    return false;
}

void app_inst_util_fini(void) {
    int ret;

    if (!s_app_inst_util_initialized) {
        return;
    }

    ret = sceAppInstUtilTerminate();
    if (ret) {
        log_debug( "sceAppInstUtilTerminate failed: 0x%08X", ret);
    }

    s_app_inst_util_initialized = false;
}

bool bgft_init(void) {
    int ret;

    if (s_bgft_initialized) {
        goto done;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));
    {
        s_bgft_init_params.heapSize = BGFT_HEAP_SIZE;
        s_bgft_init_params.heap = (uint8_t*)malloc(s_bgft_init_params.heapSize);
        if (!s_bgft_init_params.heap) {
            log_debug( "No memory for BGFT heap.");
            goto err;
        }
        memset(s_bgft_init_params.heap, 0, s_bgft_init_params.heapSize);
    }

    ret = sceBgftServiceInit(&s_bgft_init_params);
    if (ret) {
        log_debug( "sceBgftInitialize failed: 0x%08X", ret);
        goto err_bgft_heap_free;
    }

    s_bgft_initialized = true;

done:
    return true;

err_bgft_heap_free:
    if (s_bgft_init_params.heap) {
        free(s_bgft_init_params.heap);
        s_bgft_init_params.heap = NULL;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));

err:
    s_bgft_initialized = false;

    return false;
}

void bgft_fini(void) {
    int ret;

    if (!s_bgft_initialized) {
        return;
    }

    ret = sceBgftServiceTerm();
    if (ret) {
        log_debug( "sceBgftServiceTerm failed: 0x%08X", ret);
    }

    if (s_bgft_init_params.heap) {
        free(s_bgft_init_params.heap);
        s_bgft_init_params.heap = NULL;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));

    s_bgft_initialized = false;
}
/* sample ends */


/* install package wrapper:
   init, (install), then clean AppInstUtil and BGFT
   for next install */

uint8_t pkginstall(const char *path)
{
    char title_id[16];
    int  is_app, ret = -1;
    int  task_id = -1;
    char buffer[255];

    if( if_exists(path) )
    {
//      log_info("Initializing AppInstUtil...");
        if (!app_inst_util_init()) {
            log_debug( "AppInstUtil initialization failed.");
            goto err_user_service_terminate;
        }
//      log_info("Initializing BGFT...");
        if (!bgft_init()) {
            log_debug( "BGFT initialization failed.");
            goto err_appinstutil_finalize;
        }

        ret = sceAppInstUtilGetTitleIdFromPkg(path, title_id, &is_app);
        if (ret) 
            return PKG_ERROR("sceAppInstUtilGetTitleIdFromPkg", ret);
        snprintf(buffer, 254, "%s via Store", title_id);
        log_info( "%s", buffer);
        struct bgft_download_param_ex download_params;
        memset(&download_params, 0, sizeof(download_params));
        download_params.param.entitlement_type = 5;
        download_params.param.id = "";
        download_params.param.content_url = path;
        download_params.param.content_name = buffer;
        download_params.param.icon_path = "/update/fakepic.png";
        download_params.param.playgo_scenario_id = "0";
        download_params.param.option = BGFT_TASK_OPTION_DELETE_AFTER_UPLOAD;
        download_params.slot = 0;

    retry:
//      log_info("%s 1", __FUNCTION__);
        ret = sceBgftServiceIntDownloadRegisterTaskByStorageEx(&download_params, &task_id);

        if(ret == 0x80990088)
        {
//          log_info("%s 2", __FUNCTION__);
            ret = sceAppInstUtilAppUnInstall(&title_id[0]);

            if(ret != 0)
                return PKG_ERROR("sceAppInstUtilAppUnInstall", ret);

            goto retry;
        }
        else
        if(ret) 
            return PKG_ERROR("sceBgftServiceIntDownloadRegisterTaskByStorageEx", ret);

        log_info("Task ID(s): 0x%08X", task_id);

        ret = sceBgftServiceDownloadStartTask(task_id);
//      log_info("%s 4", __FUNCTION__);
        if(ret) 
            return PKG_ERROR("sceBgftDownloadStartTask", ret);
    }
    else
        return PKG_ERROR("no file at", ret);

err_user_service_terminate:

err_bgft_finalize:
//    log_info("Finalizing BGFT...");
    bgft_fini();

err_appinstutil_finalize:
//    log_info("Finalizing AppInstUtil...");
    app_inst_util_fini();

    log_info( "%s(%s), %s done.", __FUNCTION__, path, title_id);

    refresh_apps_for_cf();

    return 0;
}
