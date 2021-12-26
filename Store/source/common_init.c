#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <sys/signal.h>
#include <sig_handler.h>
#include <stdbool.h>
#include <utils.h>
#include <errno.h>
#include <sys/socket.h>
#include <user_mem.h> 
extern bool dump;

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <orbislink.h>
#include <libkernel.h>
#include <sys/mman.h>
#include <ImeDialog.h>
#include <utils.h>
#include <MsgDialog.h>
#include <CommonDialog.h>
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

#include <Header.h>
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


/* XXX: patches below are given for Piglet module from 4.74 Devkit PUP */
static void pgl_patches_cb(void* arg, uint8_t* base, uint64_t size)
{
    /* Patch runtime compiler check */
    const uint8_t p_set_eax_to_1[] = {
        0x31, 0xC0, 0xFF, 0xC0, 0x90,
    };
    memcpy(base + 0x5451F, p_set_eax_to_1, sizeof(p_set_eax_to_1));
    /* Tell that runtime compiler exists */
    *(uint8_t*)(base + 0xB2DEC) = 0;
    *(uint8_t*)(base + 0xB2DED) = 0;
    *(uint8_t*)(base + 0xB2DEE) = 1;
    *(uint8_t*)(base + 0xB2E21) = 1;
    /* Inform Piglet that we have shader compiler module loaded */
    *(int32_t*)(base + 0xB2E24) = s_shcomp_module;
}


bool is_connected_app = false;

extern uint8_t daemon_eboot[];
extern int32_t daemon_eboot_size;


#define  LATEST_DAEMON_VERSION 0x1001
static int (*rejail_multi)(void) = NULL;

bool is_daemon_outdated(void)
{
    int daemon_ver = INVALID;

    pl_ini_file file;
    if (!if_exists(DAEMON_PATH"/daemon.ini"))
        return false;
    else
    {
        log_debug("Daemon INI Does exist");
        pl_ini_load(&file, DAEMON_PATH"/daemon.ini");
        daemon_ver = pl_ini_get_int(&file, "Daemon", "version", 0x1337);
        log_info("Daemon Version: %x, Latest Version: %x, Is Outdated?: %s", daemon_ver, LATEST_DAEMON_VERSION, daemon_ver == LATEST_DAEMON_VERSION ? "No" : "Yes");

        /* Clean up */
        pl_ini_destroy(&file);
    }

    return (bool)daemon_ver == LATEST_DAEMON_VERSION;
}

bool init_daemon_services(bool redirect)
{


    uint8_t* IPC_BUFFER = malloc(100);
    int fd = -1;
    if (!if_exists(DAEMON_PATH) || !is_daemon_outdated())
    {
        if (!!mountfs("/dev/da0x4.crypt", "/system", "exfatfs", "511", 0x00010000))
        {
            log_error("mounting /system failed with %s", strerror(errno));
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

                    pl_ini_file file;
                    pl_ini_create(&file);
                    pl_ini_set_int(&file, "Daemon", "version", LATEST_DAEMON_VERSION);
                    pl_ini_save(&file, DAEMON_PATH"/daemon.ini");
                    chmod(DAEMON_PATH"/daemon.ini", 0777);
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
            IPCSendCommand(DEAMON_UPDATE, IPC_BUFFER);
        }
    }

    //Launch Daemon with silent
    uint32_t res = Launch_App("ITEM00002", true);
    if (res != SCE_LNC_UTIL_ERROR_ALREADY_RUNNING)
    {
        if (rejail_multi != NULL)
        {
            int libcmi = 1;
            sys_dynlib_load_prx("/system/vsh/app/ITEM00002/Media/jb.prx", &libcmi);
            if (!sys_dynlib_dlsym(libcmi, "rejail_multi", &rejail_multi))
                rejail_multi();
        }
        sceSystemServiceLoadExec("/data/self/eboot.bin");
    }
    

    loadmsg("Waiting for Daemon's Welcome Response (max 1 min)");

    int error = INVALID, wait = INVALID;
    // Wait for the Daemon to respond
    do {

        if (wait >= 60)
        {
            log_error("Daemon timed out");
            return false;
        }
        else
            wait++;

        sleep(1);

     //File Flag the Daemon creates when initialization is complete
    // and the Daemon IPC server is active
    } while (!if_exists("/system_tmp/IPC_init")); 
    
     error = IPCSendCommand(CONNECTION_TEST, IPC_BUFFER);
     log_info("---- Error: %s", error == INVALID ? "Failed to Connect" : "Success");
     if (error == NO_ERROR) {
         sceMsgDialogTerminate();
         log_debug("Took the Daemon %i extra commands attempts to respond", wait);
         is_connected_app = true;
      }
    


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
    //Keep people from backing up the Sig file
    unlink("/user/app/NPXS39041/homebrew.elf.sig");

    /*-- INIT LOGGING FUNCS --*/
    log_set_quiet(false);
    log_set_level(LOG_DEBUG);
    FILE* fp = fopen(STORE_LOG, "w");
    log_add_fp(fp, LOG_DEBUG);
    //USB LOGGING
    if (usbpath() != NULL)
    {
        sprintf(&tmp[0], "%s/Store-log.txt", usbpath());
        unlink(tmp);
        fp = fopen(tmp, "w");
        log_add_fp(fp, LOG_DEBUG);
    }
    /* -- END OF LOGINIT --*/


    log_info("------------------------ Store[GL] Compiled Time: %s @ %s  -------------------------", __DATE__, __TIME__);
    log_info(" ---------------------------  STORE Version: %s ------------------------------------", completeVersion);
if(reload_apps)
    log_info("----------------------------  reload_apps: %s, Total_pages % i ----------------------", reload_apps ? "true" : "false", ref_pages);

    get = &set;

    // internals: net, user_service, system_service
    ret = loadModulesVanilla();
    // pad
    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PAD);
    if(ret) return -1;
    //Ime
    ret = sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG);
    if(ret) return -1;
    //MSGDIALOG
    ret = sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
    if(ret) return -1; 

    ret =  sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL);
    if(ret) return -1;

    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NET);
    if(ret) return -1;
    
    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP);
    if(ret) return -1;

    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SSL);
    if(ret) return -1;

    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_COMMON_DIALOG); 
    if(ret) return -1;
    
    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYSUTIL);
    if(ret) return -1;
    
    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT);  
    if(ret) return -1;
    
    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_APPINSTUTIL);  
    if(ret) return -1;

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

    ret = orbisPadInit();
    if (ret != 1) return -2;


    if(MD5_hash_compare("/user/app/NPXS39041/pig.sprx",   "854a0e5556eeb68c23a97ba024ed2aca") == SAME_HASH
    && MD5_hash_compare("/user/app/NPXS39041/shacc.sprx", "8a21eb3ed8a6786d3fa1ebb1dcbb8ed0") == SAME_HASH)
    {
        globalConf.confPad = orbisPadGetConf();

        mkdir("/user/app/NPXS39041/covers", 0777);
        // customs
        s_piglet_module = sceKernelLoadStartModule("/user/app/NPXS39041/pig.sprx",   0, NULL, 0, NULL, NULL);
        s_shcomp_module = sceKernelLoadStartModule("/user/app/NPXS39041/shacc.sprx", 0, NULL, 0, NULL, NULL);

        if(! patch_module("pig.sprx", &pgl_patches_cb, NULL, /*debugnetlevel*/3)) return -3;

        if(!LoadOptions(get)) msgok(WARNING, "Could NOT find/open the INI File");

        if (get->Daemon_on_start)
        {
            if (!init_daemon_services(get->HomeMenu_Redirection))
                msgok(WARNING, "The Itemzflow init_daemon_services failed\nThis may cause some things not work\nif you have a USB Inserted check the log");
            else
                log_debug("The Itemzflow init_daemon_services succeeded");
        }
        else
            msgok(WARNING, "The Itemzflow Daemon Auto start is turned off\nThis may cause some things not work");
        

        

        if (reload_apps)
        {
            if (get->Legacy)
                sceSystemServiceLoadExec("/data/self/eboot.bin", NULL);

            total_pages = check_store_from_url(0, get->opt[CDN_URL], COUNT);
            if(total_pages == ref_pages)
                 return 0;
        }

        loadmsg("Downloading and Caching Website files....");

        log_info("get->Legacy? %s (%i)", get->Legacy ? "true" : "false", get->Legacy);

        if (!get->Legacy)
        {
            total_pages = check_store_from_url(0, get->opt[CDN_URL], COUNT);
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
            msgok(NORMAL, "You have been Logged in as a Beta User");
        else
            msgok(FATAL, "The BETA has ended or been Revoked please change your CDN");
#endif

        setup_store_assets(get);

        sceMsgDialogTerminate();
    }
    else
    {
        //Delete SPRXS with wrong hash, if they dont exist loader will redownload
        unlink("/user/app/NPXS39041/pig.sprx");
        unlink("/user/app/NPXS39041/shacc.sprx");
        msgok(FATAL, "SPRX ARE NOT THE SAME HASH ABORTING");
    }


    // all fine.
    return 0;
}
