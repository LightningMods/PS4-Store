
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <signal.h>
#include <errno.h>
#include <utils.h>
#include <utils.h>
#include <orbis/libkernel.h>
#include <sys/mman.h>
#include <orbis/SystemService.h>

int pthread_getthreadid_np();
#define OBRIS_BACKTRACE 1
#define SYS_mount 21

int sys_mount(const char *type, const char	*dir, int flags, void *data){
    return syscall_alt(SYS_mount, type, dir, flags, data);
}
extern bool reboot_app;
static int (*rejail_multi)(void) = NULL;
 int backtrace(void **buffer, int size);
 
void Exit_Success(const char* text)
{
    log_debug(text);
    if (rejail_multi != NULL) {
        log_debug("Rejailing App >>>");
        rejail_multi();
        log_debug("App ReJailed");
    }
    log_debug("Calling SystemService Exit");
    if(reboot_app)
      sceSystemServiceLoadExec("/app0/eboot.bin", 0);
    else
      sceSystemServiceLoadExec("exit", 0);
}

void Exit_App_With_Message(int sig_numb)
{
    msgok(NORMAL, "############# Program has gotten a FATAL Signal/Error: %i ##########\n\n %s\n\n", sig_numb, getLangSTR(PKG_TEAM));

    char tmp[100];

    if (usbpath() != -1) {

        //try our best to make the name not always the same
        snprintf(&tmp[0], 99, "/mnt/usb%d/Store", usbpath());
        mkdir(&tmp[0], 0777);
        snprintf(&tmp[0], 99, "%s/Store.log", &tmp[0]);

        if (touch_file(&tmp[0])) {
            log_debug("App crashed writing log to %s", &tmp[0]);
            copyFile(STORE_LOG, &tmp[0]);
        }
    }

    Exit_Success("Exit_App_With_Message ->");
}
 
#ifdef OBRIS_BACKTRACE 
int32_t sceKernelVirtualQuery(const void *, int, OrbisKernelVirtualQueryInfo *, size_t);

static void OrbisKernelBacktrace(const char* reason) {
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
            i + 1, info.name, frames[i].pc - (char*)info.unk01 - 1);
        strncat(buf, temp, MAX_MESSAGE_SIZE);

        snprintf(temp, sizeof temp,
            "0x%lx ", frames[i].pc - (char*)info.unk01 - 1);
        strncat(addr2line, temp, sizeof addr2line - 1);
    }

    strncat(buf, "<118>[Crashlog]: addr2line: ", MAX_MESSAGE_SIZE);
    strncat(buf, addr2line, MAX_MESSAGE_SIZE);
    strncat(buf, "\n", MAX_MESSAGE_SIZE);

    buf[MAX_MESSAGE_SIZE + 1] = '\n';
    buf[MAX_MESSAGE_SIZE + 2] = '\0';

    log_error("%s", buf);
}
#endif

void SIG_Handler(int sig_numb)
{
    int libcmi = 1;
    libcmi = sceKernelLoadStartModule("/app0/Media/jb.prx", 0, NULL, 0, 0, 0);
    sceKernelDlsym(libcmi, "rejail_multi", (void**)&rejail_multi);
   // libcmi = sceKernelLoadStartModule("/app0/Media/jb.prx", 0, NULL, 0, 0, 0);
    //sceKernelDlsym(libcmi, "sceCoredumpRegisterCoredumpHandler", (void**)&sceCoredumpRegisterCoredumpHandler);
   // sceCoredumpRegisterCoredumpHandler(NULL, 0x1000, NULL);

    void* bt_array[255];
    //
    switch (sig_numb)
    {
    case SIGABRT:
        log_debug("ABORT called.");
        goto SIGSEGV_handler;
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
SIGSEGV_handler:
        log_debug("App has crashed");
        goto backtrace_and_exit;
    }

    default:

        //Proc info and backtrace
        goto backtrace_and_exit;

        break;
    }


backtrace_and_exit:
    log_debug("backtrace_and_exit called");
    mkdir("/user/store_recovery.flag", 0777);

    char profpath[150];

    snprintf(&profpath[0], 149, "/mnt/proc/%i/", getpid());

    if (getuid() == 0 && !if_exists(&profpath[0]))
    {
        int result = mkdir("/mnt/proc", 0777);
        if (result < 0 && errno != 17)
           log_debug("Failed to create /mnt/proc");
        
        result = sys_mount("procfs", "/mnt/proc", 0, NULL);
        if (result < 0)
            log_debug("Failed to mount procfs: %s", strerror(errno));
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
        fscanf(fp, "%s", &buff[0]);

        log_debug("# Reading curproc...");

        log_debug("# Proc Name: %s", &buff[0]);

        fclose(fp);
    }
    log_debug("###################  Backtrace  ########################");
    backtrace(bt_array, 255);
#ifdef OBRIS_BACKTRACE
    OrbisKernelBacktrace("Fatal Signal");
#endif
    Exit_App_With_Message(sig_numb);


    
App_suspended:
log_debug("App Suspended Checking for opened games...");
App_Resumed:
log_debug("App Resumed Checking for opened games...");
    
}
