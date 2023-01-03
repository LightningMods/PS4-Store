#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <signal.h>
#include "defines.h"
#include <stdbool.h>
#include <utils.h>
#include <errno.h>
#include <sys/socket.h>
#include "lang.h"
#include "sqlite3.h"
extern bool dump;
uint32_t daemon_appid;
#if defined (__ORBIS__)

#include <orbis/libkernel.h>
#include <sys/mman.h>
#include <utils.h>

#include <dialog.h>
#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO

#else // on linux

#include <stdio.h>
#define  debugNetPrintf  fprintf
#define  ERROR           stderr
#define  DEBUG           stdout
#define  INFO            stdout

#endif
#include <md5.h>
#include <orbisPad.h>
#define SCE_SYSMODULE_INTERNAL_PAD 0x80000024

extern OrbisGlobalConf globalConf;
int total_pages;

int s_piglet_module = -1;
int s_shcomp_module = -1;
int JsonErr = 0,
    page;


const char* m1 = "\x72\x6F\x6F\x74\x65\x64\x2E\x67\x71";
const char* m2 = "\x72\x6F\x6F\x74\x65\x64\x2E\x67\x71";


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

extern StoreOptions set,
                   *get;


OrbisGlobalConf globalConf;

#define STORE_APP_PATH "/user/app/NPXS39041/"

int initGL_for_the_store(bool reload_apps, int ref_pages)
{
    int ret = 0;
    char tmp[100];

    
    unlink(STORE_LOG);
    //old log
    unlink("/user/app/NPXS39041/logs/store.log");
    //Keep people from backing up the Sig file
    unlink("/user/app/NPXS39041/homebrew.elf.sig");

    
    /*-- INIT LOGGING FUNCS --*/
    log_set_quiet(false);
    log_set_level(LOG_DEBUG);
    FILE* fp = fopen(STORE_LOG, "w");
    if (fp != NULL)
      log_add_fp(fp, LOG_DEBUG);
    log_info("Clearing the Download folder...");

    
    rmtree("/user/app/NPXS39041/downloads"); 
    mkdir("/user/app/NPXS39041/downloads", 0777);


    log_info("------------------------ Store[GL] Compiled Time: %s @ %s  -------------------------", __DATE__, __TIME__);
    log_info(" ---------------------------  STORE Version: %s -------------------------------", completeVersion);

    get = &set;

    // internals: net, user_service, system_service
    	// load common modules
    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE)) return INIT_FAILED;
    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PAD)) return INIT_FAILED;
    if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG)) return INIT_FAILED;
    if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NET)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SSL)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_COMMON_DIALOG)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYSUTIL)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT)) return INIT_FAILED;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_APPINSTUTIL)) return INIT_FAILED;
    sceSysmoduleLoadModule(0x009A);  // internal FreeType, libSceFreeTypeOl

    //Dump code and sig hanlder
    struct sigaction new_SIG_action;

    new_SIG_action.sa_handler = SIG_Handler;
    sigemptyset(&new_SIG_action.sa_mask);
    new_SIG_action.sa_flags = 0;

    for (int i = 0; i < 43; i++) {
        if(i != SIGUSR2)
           sigaction(i, &new_SIG_action, NULL);
    }

    
    sceCommonDialogInitialize();
    sceMsgDialogInitialize();
    
    if (!orbisPadInit()) return INIT_FAILED;

    globalConf.confPad = orbisPadGetConf();

    mkdir(STORE_APP_PATH "logs", 0777);
    mkdir(STORE_APP_PATH "storedata", 0777);
    mkdir(STORE_APP_PATH "downloads", 0777);

    if (!LoadOptions(get)) msgok(WARNING, (char*)getLangSTR(INI_ERROR));

    if(!init_curl())
        msgok(FATAL, "curl init failed");
    else
        log_info("CURL Initialized");

#ifndef HAVE_SHADER_COMPILER
    if((ret = sceKernelLoadStartModule("/system/common/lib/libScePigletv2VSH.sprx", 0, NULL, 0, NULL, NULL)) >= PS4_OK)
#else
    if ((s_piglet_module = sceKernelLoadStartModule("/data/piglet.sprx", 0, NULL, 0, NULL, NULL)) >= PS4_OK &&
        (s_shcomp_module = sceKernelLoadStartModule("/data/compiler.sprx", 0, NULL, 0, NULL, NULL)) >= PS4_OK)
#endif

    {
       
        if (strstr(get->opt[CDN_URL], m1) != NULL) {
            raise(SIGQUIT);
        }

        loadmsg(getLangSTR(DL_CACHE));

        if ( check_store_from_url(get->opt[CDN_URL], MD5_HASH))
            log_info("[StoreCore] DB is already Up-to-date");
        else {
            log_info("[StoreCore] DB is Outdated!, Downloading the new one ...");

            snprintf(&tmp[0], sizeof(tmp), "%s/store.db", get->opt[CDN_URL]);
            if (dl_from_url(&tmp[0], SQL_STORE_DB, NULL, false) != 0)
                msgok(FATAL, (char*)getLangSTR(CACHE_FAILED));
            else
                log_info("[StoreCore] New DB Downloaded");
        }

        log_info("[StoreCore] Starting SQL Services...");
        log_info("[StoreCore] SQL library Version: %s", sqlite3_libversion());
        if (!SQL_Load_DB(SQL_STORE_DB)) {
            log_fatal("[StoreCore][FATAL] SQL DB Could not be loaded, Exiting....");
            msgok(FATAL, "\n\n%s: DB_CANNOT_BE_LOADDED", getLangSTR(FAILED_W_CODE));
        }
        log_info("[StoreCore] Started SQL Services ");


#if BETA==1
        if ( check_store_from_url( NULL, BETA_CHECK) == 200)
            msgok(NORMAL, getLangSTR(BETA_LOGGED_IN));
        else {
            msgok(FATAL, getLangSTR(BETA_REVOKED));
            return INIT_FAILED;
        }
#endif

    }
    else {
#ifndef HAVE_SHADER_COMPILER 
        msgok(FATAL, "%s 0x%x\nPS4 Path: /system/common/lib/libScePigletv2VSH.sprx", ret, getLangSTR(PIG_FAIL));
#else
        msgok(FATAL, "HAS_SHADER_COMP: Piglet (custom) failed to load with 0x%x\nPiglet Path: /data/piglet.sprx, Compiler Path: /data/compiler.sprx", s_piglet_module);
#endif
        return INIT_FAILED;
    }

    //dont_show_donate_message
    if (!if_exists("/data/DSDM"))
       msgok(NORMAL, "Do You enjoy the Homebrew Store?\n\nIf you do, consider supporting us here at https://pkg-zone.com\nOR\nBy one of the following methods\nKo-fi: https://ko-fi.com/lightningmods\nBTC: bc1qgclk220glhffjkgraju7d8xjlf7teks3cnwuu9");
    
    // all fine.
    return PS4_OK;
}
