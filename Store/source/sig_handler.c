
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <sys/signal.h>
#include <errno.h>
#include <sig_handler.h>
#include <utils.h>
#include <sys/mount.h>

#if 0
//defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <orbislink.h>
#include <libkernel.h>
#include <sys/mman.h>
#include <ImeDialog.h>
#include <utils.h>
#include <MsgDialog.h>
#include <CommonDialog.h>
#include <dump_and_decrypt.h>

#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO


#endif


// the Settings
extern StoreOptions set,
                   *get;

char* title[300]; 
char* title_id[30];


int copyFileProgForDump(char* sourcefile, char* destfile, int total_dumped, int total_files, char* tid, char* app_name)
{
    long tot = CalcAppsize(sourcefile);
    char buf[1024];
    int src = sceKernelOpen(sourcefile, O_RDONLY, 0);
    if (src > 0)
    {

        int out = sceKernelOpen(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (out > 0)
        {
            size_t bytes;
            char* buffer = malloc(65536);
            if (buffer != NULL)
            {
                while (0 < (bytes = sceKernelRead(src, buffer, 65536)))
                {
                    uint32_t g_progress = (uint32_t)(((float)bytes / tot) * 100.f);

                    snprintf(buf, 1023, "Dump Info:\n\nTitle_ID: %s\nApp name: %s\nDumping File... %s to %s\n\n File %i / %i\n", tid, app_name, sourcefile, destfile, total_dumped, total_files);

                    sceMsgDialogProgressBarSetValue(0, g_progress);

                    sceMsgDialogProgressBarSetMsg(0, buf);

                    sceKernelWrite(out, buffer, bytes);
                }
                free(buffer);
            }
            else
            {
                sceKernelClose(src);
                sceKernelClose(out);
                return -1;
            }

            sceKernelClose(src);
            sceKernelClose(out);
            log_info("Copying was Successfully");
            return 0;
        }
        else
        {
            sceKernelClose(src);
            msgok(WARNING, "the File %s has failed copying\nReason %x\n to continue press OK", sourcefile, out);
            return -1;
        }
    }
    else
    {
        log_info("[ELFLOADER] fuxking error");
        msgok(WARNING, "the File %s has failed copying\nReason %x\n to continue press OK", sourcefile, src);
        return -1;
    }
}

int progstart(char* msg)
{

    int ret = 0;
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    ret = sceMsgDialogTerminate();

    log_info("sceMsgDialogTerminate = %i", ret);

    ret = sceMsgDialogInitialize();

    log_info("sceMsgDialogInitialize = %i", ret);


    OrbisMsgDialogParam dialogParam;
    OrbisMsgDialogParamInitialize(&dialogParam);
    dialogParam.mode = 2;

    OrbisMsgDialogProgressBarParam  progBarParam;
    memset(&progBarParam, 0, sizeof(OrbisMsgDialogProgressBarParam));

    dialogParam.progBarParam = &progBarParam;
    dialogParam.progBarParam->barType = 0;
    dialogParam.progBarParam->msg = msg;

    sceMsgDialogOpen(&dialogParam);

    return ret;
}


bool dump = false; 


bool touch_file(char* destfile)
{
    int fd = open(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd > 0) {
        close(fd);
        return true;
    }
    else
        return false;
}

bool decrypt_dir(char *sourcedir, char* destdir, char* gtitle, char* title_id)
{
    DIR *dir;
    struct dirent *dp;
    struct stat info;
    char src_path[1024], dst_path[1024];

    dir = opendir(sourcedir);
    if (!dir)
        return true;

    mkdir(destdir, 0777);

    while ((dp = readdir(dir)) != NULL)
    {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
        {
            // do nothing (straight logic)
        }
        else
        {
            snprintf(src_path, 1023, "%s/%s", sourcedir, dp->d_name);
            snprintf(dst_path, 1023, "%s/%s", destdir  , dp->d_name);
            if (!stat(src_path, &info))
            {
                if (S_ISDIR(info.st_mode))
                {
                    decrypt_dir(src_path, dst_path, gtitle, title_id);
                }
                else
                if (S_ISREG(info.st_mode))
                {
                   
                    if (is_self(src_path))
                    {
                       ProgSetMessagewText(90, "Dump info\n\nApp name: %s\nTITLE_ID %s\nDecrypting %s...\n", gtitle, title_id, src_path);

                       if (decrypt_and_dump_self(src_path, dst_path))
                       {
                           closedir(dir);
                           return true;
                       }
                    }
                    else
                    {
                        closedir(dir);
                        return false;
                    }
                }
            }
        }
    }
    closedir(dir);

    return false;
}

int wait_for_game(char *title_id)
{
    int res = 0;

    DIR *dir;
    struct dirent *dp;


    dir = opendir("/mnt/sandbox/pfsmnt");
    if (!dir)
        return 0;


    while ((dp = readdir(dir)) != NULL)
    {

        log_info("%s", dp->d_name);

        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
        {
            // do nothing (straight logic)
        }
        else
        {
            log_info("%s", dp->d_name);

            if (strstr(dp->d_name, "-app0") != NULL)
            {


                
                res = 1;
                break;
            }
        }
    }
    closedir(dir);
    log_info("wait_for_game finish");

    return res;
}

int wait_for_bdcopy(char *title_id)
{
    char path[256];
    char *buf;
    size_t filelen, progress;

    snprintf(path, 255, "/system_data/playgo/%s/bdcopy.pbm", title_id);
    FILE *pbm = fopen(path, "rb");
    if (!pbm) return 100;

    fseek(pbm, 0, SEEK_END);
    filelen = ftell(pbm);
    fseek(pbm, 0, SEEK_SET);

    buf = malloc(filelen);

    fread(buf, sizeof(char), filelen, pbm);
    fclose(pbm);

    progress = 0;
    for (int i = 0x100; i < filelen; i++)
    {
        if (buf [i]) progress++;
    }

    free(buf);

    return (progress * 100 / (filelen - 0x100));
}

bool (*Dump_Meta)(char* tid, char* title) = NULL;

bool dump_game(char *title_id, char *usb_path, char *gtitle)
{
    char base_path[64];
    char src_path[64];
    char dst_file[64];
    char dst_app[64];
    char dst_pat[64];
    char dump_sem[64];
    char comp_sem[64];
    bool flag = false;

    sprintf(base_path, "%s/%s", usb_path, title_id);

    sprintf(dump_sem, "%s.dumping", base_path);
    sprintf(comp_sem, "%s.complete", base_path);


    unlink(comp_sem);
    touch_file(dump_sem);

    sprintf(dst_app, "%s-app", base_path);
    sprintf(dst_pat, "%s-patch", base_path);
    mkdir(dst_app, 0777);
    mkdir(dst_pat, 0777);

    print_memory();

    sprintf(src_path, "/user/app/%s/app.pkg", title_id);
    //notify("Extracting app package...");

 
    log_info("Extracting app package...");

    if(unpkg(src_path, dst_app)!= 0) flag = true;
    log_info("flag: %i", flag);
    sprintf(src_path, "/system_data/priv/appmeta/%s/nptitle.dat", title_id);
    sprintf(dst_file,  "%s/sce_sys/nptitle.dat", dst_app);
    copyFile(src_path, dst_file);
    sprintf(src_path, "/system_data/priv/appmeta/%s/npbind.dat", title_id);
    sprintf(dst_file, "%s/sce_sys/npbind.dat", dst_app);
    copyFile(src_path, dst_file);

    log_info("Finished app package...");

    print_memory();

    sprintf(src_path, "/user/patch/%s/patch.pkg", title_id);
    if (if_exists(src_path))
    {

        //  notify("Merging patch package...");
         log_info("Extracting patch package......");

         ProgSetMessagewText(30, "Dump info\n\nApp name: %s\nTITLE_ID %s\n\nExtracting patch package...\n", gtitle, title_id);


        if(unpkg(src_path, dst_pat)!= 0) flag = true;
        log_info("flag: %i", flag);
        sprintf(src_path, "/system_data/priv/appmeta/%s/nptitle.dat", title_id);
        sprintf(dst_file,  "%s/sce_sys/nptitle.dat", dst_pat);
        copyFile(src_path, dst_file);
        sprintf(src_path, "/system_data/priv/appmeta/%s/npbind.dat", title_id);
        sprintf(dst_file,  "%s/sce_sys/npbind.dat", dst_pat);
        copyFile(src_path, dst_file);
    }

    print_memory();

    log_info("finished patch package......");

    sprintf(src_path, "/mnt/sandbox/pfsmnt/%s-app0-nest/pfs_image.dat", title_id);
    //  notify("Extracting app image...");
    log_info("Extracting app image......");
    if(unpfs(src_path, dst_app, gtitle, title_id) != 0) flag = true;
    log_info("flag: %i", flag);
    log_info("finish app image......");

    print_memory();

    sprintf(src_path, "/mnt/sandbox/pfsmnt/%s-patch0-nest/pfs_image.dat", title_id);
    if (if_exists(src_path))
    {
       
        log_info("Extracting patch image......");

        ProgSetMessagewText(80, "Dump info\n\nApp name: %s\nTITLE_ID %s\n\nExtracting patch image...\n", gtitle, title_id);
       
        if(unpfs(src_path, dst_pat, gtitle, title_id) != 0) flag = true;
        log_info("flag: %i", flag);
    }

    print_memory();

    sprintf(src_path, "/mnt/sandbox/pfsmnt/%s-app0", title_id);
    if (if_exists(src_path))
    {
        flag = decrypt_dir(src_path, dst_app, gtitle, title_id);
        log_info("flag: %i", flag);
    }

    print_memory();

    sprintf(src_path, "/mnt/sandbox/pfsmnt/%s-patch0", title_id);
    if (if_exists(src_path))
    {
        //notify("Decrypting patch...");
        flag = decrypt_dir(src_path, dst_pat, gtitle, title_id);
        log_info("flag: %i", flag);
    }

    snprintf(src_path, 63, "/system_data/priv/appmeta/%s/param.sfo", title_id);

    if (!if_exists(src_path))
        snprintf(src_path, 63, "/system_data/priv/appmeta/external/%s/param.sfo", title_id);

    sprintf(dst_file, "%s/sce_sys/param.sfo", dst_app);
    copyFile(src_path, dst_file);

    print_memory();

    if (if_exists("/mnt/sandbox/pfsmnt/NPXS39041-app0/Media/dump.prx"))
    {
        SceKernelModule module_id = -1;
        sys_dynlib_load_prx("/mnt/sandbox/pfsmnt/NPXS39041-app0/Media/dump.prx", &module_id);
        if (module_id >= 0)
        {
            //PRX_INTERFACE void LaunchGamePRX();
            int ret = sys_dynlib_dlsym(module_id, "Dump_Meta", &Dump_Meta);
            if (!ret)
            {
                if (Dump_Meta != NULL)
                {
                    flag = Dump_Meta(title_id, gtitle);
                    log_debug("Dump_Meta() returned %s\n", flag ? "true" : "false");
                }
            }
            else
            {
                log_debug("failed to resolve\n");
            }

        }
        else
        {
            log_debug("failed to load\n");
        }
    }
    unlink(dump_sem);
    touch_file(comp_sem);

    sceMsgDialogTerminate();
    log_info( "flag = %i", flag);

    print_memory();

    return flag;
}

static int (*rejail_multi)(void) = NULL;

void Exit_Success(const char* text)
{
    log_debug(text);



    if (rejail_multi != NULL) {
        log_debug("Rejailing App >>>");
        rejail_multi();
        log_debug("App ReJailed");
    }

    log_debug("Calling SystemService Exit");
    sceSystemServiceLoadExec("exit", 0);
}

void Exit_App_With_Message(int sig_numb)
{
   msgok(NORMAL, "############# Program has gotten a FATAL Signal/Error: %i ##########\n\n If this continues Contact the PKG-Zone team\n\n", sig_numb);

   char *buff[100];
   //init srand
   srand((unsigned)time(NULL));

   //try our best to make the name not always the same
   snprintf(buff, 99, "%s/Store_Crash_%i.log", usbpath(), rand() % 100);
   log_debug("App crashed writing log to %s", buff);

   if(touch_file(buff))
     copyFile(STORE_LOG, buff);

   Exit_Success("Exit_App_With_Message ->");
}

void SIG_Handler(int sig_numb)
{
    int libcmi = 1;
    sys_dynlib_load_prx("/app0/Media/jb.prx", &libcmi);
    sys_dynlib_dlsym(libcmi, "rejail_multi", &rejail_multi);

    void* array[100];
    //

    switch (sig_numb)
    {
    case SIGABRT:
        log_debug("ABORT called");
        break;
    case SIGTERM:
        log_debug("TERM called");
        break;
    case SIGINT:
        log_debug("TERM called");
        break;
    case SIGQUIT:
        Exit_Success("SIGQUIT Called");
        break;
    case SIGKILL:
        Exit_Success("SIGKILL Called");
        break;
    case SIGSTOP:
        log_debug("App has been suspeneded");
        goto App_suspended;
        break;
    case SIGCONT:
        log_debug("App has resumed");
        goto App_Resumed;
        break;
    case SIGSEGV: {
        log_debug("App has crashed");
        uint8_t* IPC_BUFFER = malloc(100);
        IPCSendCommand(DISABLE_HOME_REDIRECT, IPC_BUFFER);
        if (dump)
        {
            msgok(WARNING, "The Game Dump has failed to the app will now reload\n Press OK to continue");
            sceSystemServiceLoadExec("/data/self/eboot.bin", 0);
            goto App_Resumed;
        }
        free(IPC_BUFFER);
        goto backtrace_and_exit;
    }

    default:

        //Proc info and backtrace
        goto backtrace_and_exit;

        break;
    }


backtrace_and_exit:
    log_debug("backtrace_and_exit called");

    char profpath[150];

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

    log_debug("############# Program has gotten a FATAL Signal: %i ##########", sig_numb);
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
    Exit_App_With_Message(sig_numb);


    
App_suspended:
log_debug("placeholder 1");
App_Resumed:
log_debug("placeholder 2");

log_info("title_id: %s, title: %s", title_id, title);

//  char* title_id = "CUSA02290";
    if (dump)
    {
        int progress;
        bool flag_issue = false;
        char notify_buf[1024];
        int fd;


        progstart("Starting the Dumper...");

        sprintf(notify_buf, "/mnt/usb0/%s-app/", title_id);
        if (if_exists(notify_buf))
        {
            ProgSetMessagewText(5, "Folder already Exists..\nDeleting %s", notify_buf);
            rmtree(notify_buf);
            sprintf(notify_buf, "/mnt/usb0/%s.complete", title_id);
            unlink(notify_buf);
        }

        sprintf(notify_buf, "/mnt/usb0/%s-patch/", title_id);
        if (if_exists(notify_buf))
        {
            ProgSetMessagewText(8, "Folder already Exists..\nDeleting %s", notify_buf);
            rmtree(notify_buf);
        }
        
        //g

        if (!wait_for_game(title_id[0]))
        {


            ProgSetMessagewText(10, "Waiting for game to launch...");

            


        do {
            sceKernelSleep(1);

        }
        while (!wait_for_game(title_id));
        
      }


    if (wait_for_bdcopy(title_id) < 100)
    {
        do {
            sceKernelSleep(1);
            progress = wait_for_bdcopy(title_id);
            snprintf(notify_buf, 1023, "Waiting for game to copy\n\n%u%% completed...", progress);

            progstart(notify_buf);

            uint32_t g_progress = (uint32_t)(((float)progress / 100) * 100.f);

            ProgSetMessagewText(g_progress, notify_buf);


        }
        while (progress < 100);
        sceMsgDialogTerminate();
        
    }

        
        progstart("Starting the Game Dumper...");

        //dump_game has all the dialoge for copied proc
        flag_issue = dump_game(title_id, get->opt[ USB_PATH ], title);
       // Dump_Game(title_id, title);
        if(!flag_issue)
           msgok(NORMAL, "Dump of %s is Complete without Errors!", title_id);
        else
            msgok(WARNING, "Dump of %s Has Completed with Issues NOT all files were dumped!\n\nMake sure your exploit hoster has MMAP patches enabled", title_id);

        print_memory();

        log_info("finished");
        dump = false;
    }
    //
}
