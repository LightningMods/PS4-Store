#include "utils.h"
#include <stdarg.h>
#include <user_mem.h> 
#include <installpkg.h>
#include <stdbool.h>

#include "defines.h"

int PKG_ERROR(const char* name, int ret)
{
    msgok(WARNING, "%s\nHEX: %x Int: %i\nfrom Function %s",getLangSTR(INSTALL_FAILED), ret, ret, name);
    log_error( "%s error: %x", name, ret);

    return ret;
}

/* we use bgft heap menagement as init/fini as flatz already shown at 
 * https://github.com/flatz/ps4_remote_pkg_installer/blob/master/installer.c
 */

#define BGFT_HEAP_SIZE (1 * 1024 * 1024)

extern sceAppInst_done;
static bool   s_bgft_initialized = false;
static struct bgft_init_params  s_bgft_init_params;

bool app_inst_util_init(void) {
    int ret;

    if (sceAppInst_done) {
        goto done;
    }

    ret = sceAppInstUtilInitialize();
    if (ret) {
        log_debug( "sceAppInstUtilInitialize failed: 0x%08X", ret);
        goto err;
    }

    sceAppInst_done = true;

done:
    return true;

err:
    sceAppInst_done = false;

    return false;
}

void app_inst_util_fini(void) {
    int ret;

    if (!sceAppInst_done) {
        return;
    }

    ret = sceAppInstUtilTerminate();
    if (ret) {
        log_debug( "sceAppInstUtilTerminate failed: 0x%08X", ret);
    }

    sceAppInst_done = false;
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

bool bgft_download_get_task_progress(int task_id, struct bgft_download_task_progress_info* progress_info) {
    SceBgftTaskProgress tmp_progress_info;

    if (!s_bgft_initialized || task_id < 0 || !progress_info) {
        PKG_ERROR("bgft_download_get_task_progress INIT", 0x1337);
        return false;
    }


    memset(&tmp_progress_info, 0, sizeof(tmp_progress_info));
    int ret = sceBgftServiceDownloadGetProgress(task_id, &tmp_progress_info);
    if (ret) {
        PKG_ERROR("sceBgftDownloadGetProgress", ret);
        return false;
    }

    memset(progress_info, 0, sizeof(*progress_info));
    {
        progress_info->bits = tmp_progress_info.bits;
        progress_info->error_result = tmp_progress_info.errorResult;
        progress_info->length = tmp_progress_info.length;
        progress_info->transferred = tmp_progress_info.transferred;
        progress_info->length_total = tmp_progress_info.lengthTotal;
        progress_info->transferred_total = tmp_progress_info.transferredTotal;
        progress_info->num_index = tmp_progress_info.numIndex;
        progress_info->num_total = tmp_progress_info.numTotal;
        progress_info->rest_sec = tmp_progress_info.restSec;
        progress_info->rest_sec_total = tmp_progress_info.restSecTotal;
        progress_info->preparing_percent = tmp_progress_info.preparingPercent;
        progress_info->local_copy_percent = tmp_progress_info.localCopyPercent;
    }

    return true;
}

bool app_inst_util_is_exists(const char* title_id, bool* exists) {
    int flag;

    if (!title_id) return false;

    if (!sceAppInst_done) {
        log_debug("Starting app_inst_util_init..");
        if (!app_inst_util_init()) {
            log_error("app_inst_util_init has failed...");
            return false;
        }
    }

    int ret = sceAppInstUtilAppExists(title_id, &flag);
    if (ret) {
        log_error("sceAppInstUtilAppExists failed: 0x%08X\n", ret);
        return false;
    }

    if (exists) *exists = flag;

    return true;
}

/* sample ends */
/* install package wrapper:
   init, (install), then clean AppInstUtil and BGFT
   for next install */
extern bool Download_icons;

void *install_prog(void* argument)
{
    struct install_args* args = argument;
    struct bgft_download_task_progress_info progress_info;

    if (!args->is_thread)
       progstart("Store Installation on Going\n\nInstalling: %s\nTask: ID 0x%08X", args->title_id, args->task_id);
   else
       printf("Starting PKG Install\n");
    
    int prog = 0;
    while (prog != 100)
    {
        bgft_download_get_task_progress(args->task_id, &progress_info);
        if (IS_ERROR(progress_info.error_result)) {

            if (!args->is_thread)
              return PKG_ERROR("BGFT_ERROR", progress_info.error_result);
            else
            {
                log_error("BGFT_ERROR error: %x", progress_info.error_result);
                return NULL;
            }

        }

     prog = (uint32_t)(((float)progress_info.transferred / args->size) * 100.f);


     if (!args->is_thread) 
         ProgSetMessagewText(prog, "Store Installation on Going\n\nInstalling: %s\nTask: ID 0x%08X\n\nDont worry queued Downloads are still downloading in the background", args->title_id, args->task_id);

     if (progress_info.transferred % (4096 * 128) == 0)
         log_debug("%s, Install_Thread: reading data, %lub / %lub (%%%i)", __FUNCTION__, progress_info.transferred, args->size, prog);

    }

    if (prog == 100 && !IS_ERROR(progress_info.error_result)) {
        if (!args->is_thread) {
            sceMsgDialogTerminate();
            msgok(NORMAL, "%s %s %s", getLangSTR(INSTALL_OF),args->title_id,getLangSTR(COMPLETE_WO_ERRORS));
        }
    }
    else {
        if (!args->is_thread) {
            sceMsgDialogTerminate();
            msgok(WARNING, "%s %s %s 0x%x", getLangSTR(INSTALL_OF),args->title_id, getLangSTR(INSTALL_FAILED),progress_info.error_result);
        }
        else
            log_error("Installation of %s has failed with code 0x%x", args->title_id, progress_info.error_result);

    }

    log_info("Deleting PKG %s...", args->path);
    unlink(args->path);
    log_info("Finalizing Memory...");
    free(args->title_id);
    free(args->path);
    free(args);

    if (!args->is_thread) {
        if (!Download_icons)
            refresh_apps_for_cf(SORT_NA);
        else
            log_warn("CF NOT YET INIT");

        return NULL;
    }
   else
      pthread_exit(NULL);
}

uint8_t pkginstall(const char *path, unsigned long int size, bool Show_install_prog)
{
    char title_id[16];
    int  is_app, ret = -1;
    int  task_id = -1;
    char buffer[255];

    if( if_exists(path) )
    {
      if (sceAppInst_done) {
          log_info("Initializing AppInstUtil...");

          if (!app_inst_util_init())
              return PKG_ERROR("AppInstUtil", ret);
      }
        
        log_info("Initializing BGFT...");
        if (!bgft_init()) {
            return PKG_ERROR("BGFT_initialization", ret);
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
       log_info("%s 1", __FUNCTION__);
        ret = sceBgftServiceIntDownloadRegisterTaskByStorageEx(&download_params, &task_id);
        if(ret == 0x80990088)
        {
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
        if(ret) 
            return PKG_ERROR("sceBgftDownloadStartTask", ret);
    }
    else//bgft_download_get_task_progress
        return PKG_ERROR("no file at", ret);


    struct install_args* args = (struct install_args*)malloc(sizeof(struct install_args));
    args->title_id = strdup(title_id);
    args->task_id = task_id;
    args->size = size;
    args->path = strdup(path);
    args->is_thread = !Show_install_prog;

    if (Show_install_prog){
        install_prog((void*)args);
    }
    else {
        pthread_t thread = 0;
        ret = pthread_create(&thread, NULL, install_prog, (void*)args);
        log_debug("pthread_create for %x, ret:%d", task_id, ret);

        return 0;
    }

    log_info( "%s(%s), %s done.", __FUNCTION__, path, title_id);

    return 0;
}