#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <sys/signal.h>
#include "defines.h"
#include <stdbool.h>
#include <utils.h>
#include <errno.h>
#include <sys/socket.h>
#include <user_mem.h> 
#include "lang.h"
extern bool dump;

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <orbislink.h>
#include <libkernel.h>
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

#include <orbislink.h>
extern OrbisGlobalConf globalConf;
int total_pages;

int s_piglet_module = -1;
int s_shcomp_module = -1;
int JsonErr = 0,
    page;

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


bool is_connected_app = false;

extern uint8_t daemon_eboot[];
extern int32_t daemon_eboot_size;


#define  LATEST_DAEMON_VERSION 0x1002
static int (*rejail_multi)(void) = NULL;
int daemon_ver = 0x1337;
static int daemon_ini(void* user, const char* section, const char* name,
    const char* value)
{

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("Daemon", "version")) {
        log_debug("DVERLL: %s", value);
        daemon_ver = atoi(value);
    }

    return 1;
}



bool is_daemon_outdated(void)
{
    
    if (!if_exists(DAEMON_PATH"/daemon.ini"))
        return false;
    else
    {
        log_debug("Daemon INI Does exist");
        int error = ini_parse(DAEMON_PATH"/daemon.ini", daemon_ini, NULL);
        if (error) { log_error("Bad config file (first error on line %d)!\n", error); return false; }
        log_info("Daemon Version: %x, Latest Version: %x, Is Outdated?: %s", daemon_ver, LATEST_DAEMON_VERSION, daemon_ver == LATEST_DAEMON_VERSION ? "No" : "Yes");
    }

    log_debug("D: %i.", (daemon_ver == LATEST_DAEMON_VERSION));

    return (daemon_ver == LATEST_DAEMON_VERSION);
}

bool init_daemon_services(bool redirect)
{

    char* buff[100];
    memset(buff, 0, 99);
    uint8_t* IPC_BUFFER = malloc(100);
    int fd = -1;
    if (!if_exists(DAEMON_PATH) || !is_daemon_outdated())
    {
        if (!!mountfs("/dev/da0x4.crypt", "/system", "exfatfs", "511", 0x00010000))
        {
            log_error("mounting /system failed with %s.", strerror(errno));
            return false;
        }
        else
        {

            log_error("Remount Successful");
            //Delete the folder and all its files
            rmtree(DAEMON_PATH);

            mkdir(DAEMON_PATH, 0777);
            mkdir(DAEMON_PATH"/Media", 0777);
            mkdir(DAEMON_PATH"/sce_sys", 0777);
            if (copyFile("/mnt/sandbox/pfsmnt/NPXS39041-app0/Media/jb.prx", DAEMON_PATH"/Media/jb.prx") != -1 && copyFile("/system/vsh/app/NPXS21007/sce_sys/param.sfo", DAEMON_PATH"/sce_sys/param.sfo") != -1)
            {
                
                if ((fd = open(DAEMON_PATH"/eboot.bin", O_WRONLY | O_CREAT | O_TRUNC, 0777)) > 0 && fd != -1) {
                    write(fd, daemon_eboot, daemon_eboot_size);
                    close(fd);

                    IPCSendCommand(DEAMON_UPDATE, IPC_BUFFER);

                    snprintf(buff, 99, "[Daemon]\nversion=%i\n", LATEST_DAEMON_VERSION);
                    fd = open(DAEMON_PATH"/daemon.ini", O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (fd >= 0)
                    {
                        write(fd, buff, strlen(buff));
                        close(fd);
                    }
                    else {
                        return false;
                    }

                    chmod(DAEMON_PATH"/daemon.ini", 0777);

                    //New store loader is Store.self
                    if (if_exists("/data/self/Store.self"))
                        sceSystemServiceLoadExec("/data/self/Store.self", 0);
                    else // the old store loader is eboot.bin
                        sceSystemServiceLoadExec("/data/self/eboot.bin", 0);
                }
                else
                {
                    log_error("Creating the Daemon eboot failed to create: %s", strerror(errno));
                    return false;
                }
            }
            else
            {
                log_error("Copying Daemon files failed");
                return false;
            }
        }
    }
    //Launch Daemon with silent
    uint32_t res = Launch_App("ITEM00002", true);
    log_info("[StoreCore][APP] sceLncLaunchApp returned 0x%x", res);
    if (res != SCE_LNC_UTIL_ERROR_ALREADY_RUNNING && res != ORBIS_KERNEL_EAGAIN) //EAGAIN
    {
        //New store loader is Store.self
       if (if_exists("/data/self/Store.self"))
            sceSystemServiceLoadExec("/data/self/Store.self", 0);
       else // the old store loader is eboot.bin
            sceSystemServiceLoadExec("/data/self/eboot.bin", 0);
    }
    if (res == ORBIS_KERNEL_EAGAIN)
    {
        log_error("ORBIS_KERNEL_EAGAIN Returned");
        return false;
    }
    

    loadmsg(getLangSTR(WAITING_FOR_DAEMON));


    int error = INVALID, wait = INVALID;
    // Wait for the Daemon to respond
    do {

        error = IPCSendCommand(CONNECTION_TEST, IPC_BUFFER);
        log_info("---- Error: %s", error == INVALID ? "Failed to Connect" : "Success");
        if (error == NO_ERROR) {
            sceMsgDialogTerminate();
            log_debug("Took the Daemon %i extra commands attempts to respond", wait);
            is_connected_app = true;
        }

        if (wait >= 60)
            break;

        sleep(1);
        wait++;

     //File Flag the Daemon creates when initialization is complete
    // and the Daemon IPC server is active
    } while (error == INVALID);

    if (is_connected_app)
    {
        if (redirect) // is setting get->HomeMenu_Redirection enabled
        {
            log_info("Redirect on with app connected");

            error = IPCSendCommand(ENABLE_HOME_REDIRECT, IPC_BUFFER);
            if (error == NO_ERROR) {
                log_debug("HOME MENU REDIRECT IS ENABLED");

            }

        }
    }
    else
        return false;


    free(IPC_BUFFER);

    return true;

}


int initGL_for_the_store(bool reload_apps, int ref_pages)
{
    int ret = 0;
    char tmp[100];

    unlink(STORE_LOG);
    mkdir("/user/app/NPXS39041/pages", 0777);
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
    //USB LOGGING
   /* if (strstr(usbpath(), "/mnt/usb"))
    {
        sprintf(&tmp[0], "%s/Store-log.txt", usbpath());
        unlink(tmp);
        fp = fopen(tmp, "w");
        if(fp != NULL)
        log_add_fp(fp, LOG_DEBUG);
    }count_availables_json*/
    /* -- END OF LOGINIT --*/


    log_info("------------------------ Store[GL] Compiled Time: %s @ %s  -------------------------", __DATE__, __TIME__);
    log_info(" ---------------------------  STORE Version: %s ------------------------------------", completeVersion);
if(reload_apps)
    log_info("----------------------------  reload_apps: %s, Total_pages % i ----------------------", reload_apps ? "true" : "false", ref_pages);

    get = &set;

    // internals: net, user_service, system_service
    loadModulesVanilla();
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

    //Dump code and sig hanlder
    struct sigaction new_SIG_action;

    new_SIG_action.sa_handler = SIG_Handler;
    sigemptyset(&new_SIG_action.sa_mask);
    new_SIG_action.sa_flags = 0;

    for (int i = 0; i < 43; i++) {
        if(i != SIGUSR2)
           sigaction(i, &new_SIG_action, NULL);
    }

    
    OrbisUserServiceInitializeParams params;
    memset(&params, 0, sizeof(params));
    params.priority = 700;
    
    sceCommonDialogInitialize();
    sceMsgDialogInitialize();
    
    if (!orbisPadInit()) return INIT_FAILED;

    globalConf.confPad = orbisPadGetConf();

    //rif_exp("/user/license/freeIV0002-NPXS39041_00.rif");

    mkdir("/user/app/NPXS39041/covers", 0777);

    if (!LoadOptions(get)) msgok(WARNING, getLangSTR(INI_ERROR));
    
#ifndef HAVE_SHADER_COMPILER
    if((ret = sceKernelLoadStartModule("/system/common/lib/libScePigletv2VSH.sprx", NULL, NULL, NULL, NULL, NULL)) >= PS4_OK)
#else
    if ((s_piglet_module = sceKernelLoadStartModule("/data/piglet.sprx", NULL, NULL, NULL, NULL, NULL)) >= PS4_OK &&
        (s_shcomp_module = sceKernelLoadStartModule("/data/compiler.sprx", NULL, NULL, NULL, NULL, NULL)) >= PS4_OK)
#endif

    {
       
        
        if (get->Daemon_on_start)
        {
            
            if (!init_daemon_services(get->HomeMenu_Redirection))
                msgok(WARNING, getLangSTR(FAILED_DAEMON));
            else
                log_debug("The Itemzflow init_daemon_services succeeded.");
        }
        else
            msgok(WARNING, getLangSTR(DAEMON_OFF));
       

        if (reload_apps)
        {
            if (get->Legacy)
                sceSystemServiceLoadExec("/data/self/eboot.bin", NULL);

            total_pages = check_store_from_url(0, get->opt[CDN_URL], COUNT);
            if(total_pages == ref_pages) return PS4_OK;
        }

        loadmsg("%s", getLangSTR(DL_CACHE));
        log_info("get->Legacy? %s (%i)", get->Legacy ? "true" : "false", get->Legacy);
        
        if (!get->Legacy)
        {
            
            total_pages = check_store_from_url(0, get->opt[CDN_URL], COUNT);
            if (count_availables_json() > total_pages) {
                log_info("count_availables_json(%i) > total_pages(%i) clearing folder...", count_availables_json(), total_pages);
                rmtree("/user/app/NPXS39041/pages");
            }
            for (int i = 1; total_pages >= i; i++)
            {
                getjson(i, get->opt[CDN_URL], false);
                log_info("current page: %i", i);
            }
        }
        else
        {

            page = 1; //first
            while (JsonErr == 0)
            {
                JsonErr = getjson(page, get->opt[CDN_URL], true);
                page++;
            }
        }

#if BETA==1
        if (check_store_from_url(NULL, NULL, BETA_CHECK) == 200)
            msgok(NORMAL, getLangSTR(BETA_LOGGED_IN));
        else {
            msgok(FATAL, getLangSTR(BETA_REVOKED));
            return INIT_FAILED;
        }
#endif

        sceMsgDialogTerminate();
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
        msgok(NORMAL, "Do You enjoy the Homebrew Store?\n\nIf you do, consider supporting us here at https://pkg-zone.com\nOR\nBy one of the following methods\nKo-fi: https://ko-fi.com/lightningmods\nBTC: 3MEuZAaA7gfKxh9ai4UwYgHZr5DVWfR6Lw");
    
    // all fine.
    return PS4_OK;
}
