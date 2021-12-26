#include "defines.h"
#include <utils.h>
#include <sys/signal.h>
#include "log.h"
#include <ps4sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/signal.h>
#include <errno.h>
#include <sys/mount.h>


const unsigned char completeVersion[] = {
  VERSION_MAJOR_INIT,
  '.',
  VERSION_MINOR_INIT,
  '-',
  'V',
  '-',
  BUILD_YEAR_CH0,
  BUILD_YEAR_CH1,
  BUILD_YEAR_CH2,
  BUILD_YEAR_CH3,
  '-',
  BUILD_MONTH_CH0,
  BUILD_MONTH_CH1,
  '-',
  BUILD_DAY_CH0,
  BUILD_DAY_CH1,
  'T',
  BUILD_HOUR_CH0,
  BUILD_HOUR_CH1,
  ':',
  BUILD_MIN_CH0,
  BUILD_MIN_CH1,
  ':',
  BUILD_SEC_CH0,
  BUILD_SEC_CH1,
  '\0'
};


static int (*jailbreak_me)(void) = NULL;
static int (*rejail_multi)(void) = NULL;

int64_t sys_dynlib_load_prx(char* prxPath, int* moduleID)
{
    return (int64_t)syscall4(594, prxPath, 0, moduleID, 0);
}

int64_t sys_dynlib_unload_prx(int64_t prxID)
{
    return (int64_t)syscall1(595, (void*)prxID);
}


int64_t sys_dynlib_dlsym(int64_t moduleHandle, const char* functionName, void* destFuncOffset)
{
    return (int64_t)syscall3(591, (void*)moduleHandle, (void*)functionName, destFuncOffset);
}


int libjbc_module = 0;

bool rejail()
{
    int ret = sys_dynlib_dlsym(libjbc_module, "rejail_multi", &rejail_multi);
    if (!ret)
    {
        log_info("[Daemon] rejail_multi resolved from PRX");

        if ((ret = rejail_multi() != 0)) return false;
        else
            return true;
    }
    else
    {
        // fail con
        log_debug("[Daemon] rejail_multi failed to resolve");
        return false;
    }

    return false;
}

bool jailbreak(const char* prx_path)
{
    sys_dynlib_load_prx(prx_path, &libjbc_module);
    int ret = sys_dynlib_dlsym(libjbc_module, "jailbreak_me", &jailbreak_me);
    if (!ret)
    {
        log_info("[Daemon] jailbreak_me resolved from PRX");

        if ((ret = jailbreak_me() != 0))
        {
            log_debug("[Daemon] jailbreak_me returned %i", ret);
            return false;
        }
        else
            return true;
    }
    else
      log_debug("[Daemon] jailbreak_me failed to resolve");
 
    return false;
}


bool if_exists(const char* path)
{
    int dfd = open(path, O_RDONLY, 0); // try to open dir
    if (dfd < 0) {
        log_info("path %s, errno %s", path, strerror(errno));
        return false;
    }
    else
        close(dfd);


    return true;
}

void SIG_Handler(int sig_numb)
{
    char profpath[150];
    void* array[100];

    sceKernelIccSetBuzzer(2);

    snprintf(profpath, 149, "/mnt/proc/%i/", getpid());

    if (getuid() == 0 && !if_exists(profpath))
    {
        int result = mkdir("/mnt/proc", 0777);
        if (result < 0 && errno != 17)
        {
            log_debug("Failed to create /mnt/proc");
        }

        result = mount("procfs", "/mnt/proc", 0, NULL);
        if (result < 0)
        {
            log_debug("Failed to mount procfs: %s", strerror(errno));
        }
    }
    //

    log_debug("############# DAEMON HAS CRASHED ##########");
    log_debug("# Thread ID: %i", pthread_getthreadid_np());
    log_debug("# PID: %i", getpid());

    if (getuid() == 0)
        log_debug("# mounted ProcFS to %s", profpath);


    log_debug("# UID: %i", getuid());

    char buff[255];

    if (getuid() == 0)
    {
        FILE* fp;

        fp = fopen("/mnt/proc/curproc/status", "r");
        fscanf(fp, "%s", buff);

        log_debug("# Reading curproc...");

        log_debug("# Proc Name: %s", buff);

        fclose(fp);
    }
    log_debug("###################  Backtrace  ########################");
    backtrace(array, 100);

    notify("[ITEMZ] Daemon has crashed, Restarting...");

    sceSystemServiceLoadExec("/app0/eboot.bin", 0);

}

bool full_init()
{
    
    // pad
    int ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PAD);
    if (ret) return false;

    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYSUTIL);
    if (ret) return false;

    mkdir("/data/itemzflow_daemon", 0777);
    mkdir("/mnt/usb0/itemzflow", 0777);
    unlink(DAEMON_LOG_PS4);

    /*-- INIT LOGGING FUNCS --*/
    log_set_quiet(false);
    log_set_level(LOG_DEBUG);
    FILE* fp = fopen(DAEMON_LOG_PS4, "w");
    log_add_fp(fp, LOG_DEBUG);

    if (touch_file(DAEMON_LOG_USB))
    {
        fp = fopen(DAEMON_LOG_USB, "w");
        log_add_fp(fp, LOG_DEBUG);
    }
    /* -- END OF LOGINIT --*/


    log_info("------------------------ ItemzFlow[Daemon] Compiled Time: %s @ %s  -------------------------", __DATE__, __TIME__);
    log_info(" ---------------------------  Daemon Version: %s ------------------------------------", completeVersion);

    //Dump code and sig hanlder
    struct sigaction new_SIG_action;

    new_SIG_action.sa_handler = SIG_Handler;
    sigemptyset(&new_SIG_action.sa_mask);
    new_SIG_action.sa_flags = 0;
    //for now just SEGSEV
    sigaction(11, &new_SIG_action, NULL);
    
    sceSystemServiceHideSplashScreen();

    return true;
}

bool isRestMode()
{
    //return (unsigned int)sceSystemStateMgrGetCurrentState() == MAIN_ON_STANDBY;
    return false;
}

bool IsOn()
{
    //return (unsigned int)sceSystemStateMgrGetCurrentState() == WORKING;
    return true;
}


void notify(char* message)
{
    sceSysUtilSendSystemNotificationWithText(222, message);
}