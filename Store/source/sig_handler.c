
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <sys/signal.h>
#include <errno.h>
#include <utils.h>
#include <sys/mount.h>
#include <dumper.h>

#if 0
//defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <orbislink.h>
#include <libkernel.h>
#include <sys/mman.h>

#include <utils.h>

#include <dialog.h>
#include <dump_and_decrypt.h>

#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO


#endif

extern StoreOptions set,
* get;

char* title[300];
char* title_id[30];

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

bool dump_patch(char* title_id, char* usb_p, char* gtitle)
{
    char src_path[255];
    snprintf(src_path, 254, "/user/patch/%s/patch.pkg", title_id);

    print_memory();
    if (if_exists(src_path))
    {
        log_debug("Dumping Patch...");
        return Dumper(usb_p, title_id, GAME_PATCH, gtitle);
    }
    else
    {
        log_debug("Patch doesnt exist...");
        return true;
    }
}

bool dump_game(char *title_id, char *usb_p, char *gtitle)
{
    char buff[255];
    sprintf(buff, "/mnt/sandbox/pfsmnt/%s-app0-nest/pfs_image.dat", title_id);
    if (if_exists(buff)) {
        //false on success
        //true on error
        //true refers to the error flag, if the flag is true then fail
        if (Dumper(usb_p, title_id, BASE_GAME, gtitle)) {
            return dump_patch(title_id, usb_p, gtitle);
        }
        else {
            log_debug("Dumping Base game failed, trying to dump as Remaster...");
            if (Dumper(usb_p, title_id, REMASTER, gtitle))
                return dump_patch(title_id, usb_p, gtitle);
        }
        sprintf(buff, "%s/%s.complete", usb_p, title_id);
        touch_file(buff);
        sprintf(buff, "%s/%s-patch.complete", usb_p, title_id);
        touch_file(buff);
        sprintf(buff, "%s/%s.dumping", usb_p, title_id);
        unlink(buff);
        sprintf(buff, "%s/%s-patch.dumping", usb_p, title_id);
        unlink(buff);
    }
    else
        msgok(WARNING,"BETA ERROR: pfs_mount.dat cant be open");


    return false;
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
   msgok(NORMAL, "############# Program has gotten a FATAL Signal/Error: %i ##########\n\n %s\n\n", sig_numb, getLangSTR(PKG_TEAM));

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

void reload() {
    //New store loader is Store.self
    if (if_exists("/data/self/Store.self"))
        sceSystemServiceLoadExec("/data/self/Store.self", 0);
    else // the old store loader is eboot.bin
        sceSystemServiceLoadExec("/data/self/eboot.bin", 0);
}


static void OrbusKernelBacktrace(const char* reason) {
    char addr2line[MAX_STACK_FRAMES * 20];
    callframe_t frames[MAX_STACK_FRAMES];
    OrbisKernelVirtualQueryInfo info;
    char buf[MAX_MESSAGE_SIZE + 3];
    unsigned int nb_frames = 0;
    char temp[80];

    memset(addr2line, 0, sizeof addr2line);
    memset(frames, 0, sizeof frames);
    memset(buf, 0, sizeof buf);

    snprintf(buf, sizeof buf, "<118>[Crashlog]: %s\n", reason);

    strncat(buf, "<118>[Crashlog]: Backtrace:\n", MAX_MESSAGE_SIZE);
    sceKernelBacktraceSelf(frames, sizeof frames, &nb_frames, 0);
    for (unsigned int i = 0; i < nb_frames; i++) {
        memset(&info, 0, sizeof info);
        sceKernelVirtualQuery(frames[i].pc, 0, &info, sizeof info);

        snprintf(temp, sizeof temp,
            "<118>[Crashlog]:   #%02d %32s: 0x%lx\n",
            i + 1, info.name, frames[i].pc - info.unk01 - 1);
        strncat(buf, temp, MAX_MESSAGE_SIZE);

        snprintf(temp, sizeof temp,
            "0x%lx ", frames[i].pc - info.unk01 - 1);
        strncat(addr2line, temp, sizeof addr2line - 1);
    }

    strncat(buf, "<118>[Crashlog]: addr2line: ", MAX_MESSAGE_SIZE);
    strncat(buf, addr2line, MAX_MESSAGE_SIZE);
    strncat(buf, "\n", MAX_MESSAGE_SIZE);

    buf[MAX_MESSAGE_SIZE + 1] = '\n';
    buf[MAX_MESSAGE_SIZE + 2] = '\0';

    //log_debug(buf);
}


void SIG_Handler(int sig_numb)
{
#ifdef OBRIS_BACKTRACE
    OrbusKernelBacktrace("Fatal Signal");
#endif
    int libcmi = 1;
    sys_dynlib_load_prx("/app0/Media/jb.prx", &libcmi);
    sys_dynlib_dlsym(libcmi, "rejail_multi", &rejail_multi);

    void* bt_array[255];
    //

    switch (sig_numb)
    {
    case SIGABRT:
        log_debug("ABORT called");
        if (dump)
        {
            msgok(WARNING, getLangSTR(FATAL_DUMP_ERROR));
            reload();
        }
        break;
    case SIGTERM:
        log_debug("TERM called");
        break;
    case SIGINT:
        log_debug("SIGINT called");
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
            msgok(WARNING, getLangSTR(FATAL_DUMP_ERROR));
            reload();
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
    backtrace(bt_array, 255);
    Exit_App_With_Message(sig_numb);


    
App_suspended:
log_debug("placeholder 1");
App_Resumed:
log_debug("placeholder 2");

log_info("title_id: %s, title: %s, usb_path: %s", title_id, title, usbpath());


//  char* title_id = "CUSA02290";
    if (dump)
    {
        int progress;
        char n_buf[1024];
        int fd;


        progstart(getLangSTR(STARTING_DUMPER));

        sprintf(n_buf, "/mnt/usb0/%s/", title_id);
        if (if_exists(n_buf))
        {
            if(get->lang == 1 || get->lang == 18)
               ProgSetMessagewText(5, "Folder already Exists..\nDeleting %s", n_buf);

            rmtree(n_buf);
            sprintf(n_buf, "/mnt/usb0/%s.complete", title_id);
            unlink(n_buf);
        }

        sprintf(n_buf, "/mnt/usb0/%s-patch/", title_id);
        if (if_exists(n_buf))
        {
            if (get->lang == 1 || get->lang == 18)
               ProgSetMessagewText(8, "Folder already Exists..\nDeleting %s", n_buf);

            rmtree(n_buf);
        }
        
        //g

        if (!wait_for_game(title_id)){
            ProgSetMessagewText(5, "Waiting for game to launch...");
        }
        do {
            sceKernelSleep(1);

        }
        while (!wait_for_game(title_id));
        
        //dump_game has all the dialoge for copied proc
        if (dump_game(title_id, usbpath(), title))
            msgok(NORMAL, "%s %s %s", getLangSTR(DUMP_OF), title_id, getLangSTR(COMPLETE_WO_ERRORS));
        else
            msgok(WARNING, "%s %s %s", getLangSTR(DUMP_OF), title_id, getLangSTR(DUMP_FAILED));

        print_memory();

        log_info("finished");
        dump = false;
    }
    
}
