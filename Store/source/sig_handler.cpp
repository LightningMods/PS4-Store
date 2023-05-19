
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
#include <sstream>
#include <fstream>
#include "classes.hpp"

int pthread_getthreadid_np();
#define OBRIS_BACKTRACE 1
#define SYS_mount 21

int sys_mount(const char *type, const char	*dir, int flags, void *data){
    return syscall_alt(SYS_mount, type, dir, flags, data);
}
extern bool reboot_app;
static int (*rejail_multi)(void) = NULL;
extern "C" int backtrace(void **buffer, int size);

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string readFile(const std::string &filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string generate_random_log_filename() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);
    std::stringstream ss;
    ss << "Store_" << buffer << "_" << rand() % 10000 << ".log";
    return ss.str();
}

std::string generate_timestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &tm);
    return std::string(buffer);
}

std::string XORencryptDecrypt(const std::string& toEncrypt) {

    const char *key = "00000000000000000000000000000000000";

    std::string output = toEncrypt;
    
    for (int i = 0; i < toEncrypt.size(); i++)
        output[i] = toEncrypt[i] ^ key[i % strlen(key)];
    
    return output;
}

void upload_crash_log(){
    CURL *curl = nullptr;
    CURLcode res;
    std::string tk, tmp;
    if(!if_exists("/user/app/NPXS39041/logs/store.log")){
        log_debug("Log file does not exist");
        msgok(NORMAL, "Log does not exist!");
        return;
    }
    std::string logContent = readFile("/user/app/NPXS39041/logs/store.log");
    std::string encodedContent = Base64::Encode(logContent);
    std::string path = generate_random_log_filename();
    std::string commitMessage = "The Store has crashed on " + generate_timestamp();
    std::string apiUrl = "https://api.github.com/repos/ps4-logger/store-app-logs/contents/" + path;

    curl = curl_easy_init();
    if(curl) {
        //Base64::Decode("BA5AWB8vBzMEBm5mdgQDP2MaEXtRLnNdPgkkN24HNjw0MB4SCQQHAQJjCAJoJlEXKRNXBhlZFWt7RjBBLz4Maic3GnM9FksbdABkJRcXD2NURHkkfT8QI2IAIRAX", tmp);
        const char*t = "\x04\x0e\x40\x58\x1f\x2f\x07\x33\x04\x06\x6e\x66\x76\x04\x03\x3f\x63\x1a\x11\x7b\x51\x19\x41\x3a\x2f\x08\x11\x26\x34\x7a\x41\x07\x3c\x30\x10\x30\x62\x25\x43\x26\x02\x13\x13\x52\x31\x01\x5a\x73\x1b\x22\x6b\x31\x27\x3e\x07\x35\x02\x11\x77\x2a\x39\x03\x2a\x7e\x06\x35\x07\x24\x0e\x17\x1e\x34\x1d\x6b\x39\x25\x06\x3d\x28\x2e\x06\x17\x47\x37\x7d\x68\x39\x06\x2e\x37\x7c\x71\x1b";
        tk = XORencryptDecrypt(t);
        std::string json = "{\"message\": \"" + commitMessage + "\", \"content\": \"" + encodedContent + "\"}";
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        log_info("url %s", apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        Base64::Decode("QXV0aG9yaXphdGlvbjogdG9rZW4=", tmp);
        tmp = tmp + " " + tk;
        headers = curl_slist_append(headers, tmp.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) 
            log_debug("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        else
            log_debug("Log uploaded to the Server");

        // Cleanup
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
}


 
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
    std::string tmp;
    if(sig_numb != SIGQUIT && sig_numb != SIGKILL){
        if(Confirmation_Msg(fmt::format("############# This software has encountered a fatal error (Signal: {}). ##########\n\n Would you like to upload the crash log and exit?\n\n", sig_numb)) == YES){
           upload_crash_log(); 
        }
    }

    if (usbpath() != -1) {
        //try our best to make the name not always the same
        tmp = fmt::format("/mnt/usb{}/Store", usbpath());
        mkdir(tmp.c_str(), 0777);
        tmp = fmt::format("/mnt/usb{}/Store/store.log", usbpath());

        if (touch_file(tmp.c_str())) {
            log_debug("App crashed writing log to %s", tmp.c_str());
            copyFile(STORE_LOG, tmp.c_str());
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
    std::string profpath;
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
        //Proc info and backtrace
        goto backtrace_and_exit;
    }

    default:

        goto backtrace_and_exit;

        break;
    }


backtrace_and_exit:
    log_debug("backtrace_and_exit called");
    //mkdir("/user/store_recovery.flag", 0777);

    profpath = fmt::format("/mnt/proc/{}/", getpid());

    if (getuid() == 0 && !if_exists(profpath.c_str()))
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
    log_debug("# PID: %i", getpid());

    if (getuid() == 0)
        log_debug("# mounted ProcFS to %s", profpath.c_str());


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
