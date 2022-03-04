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
#include "jsmn.h"
#include "md5.h"

#define SSL_HEAP_SIZE	(304 * 1024)
#define HTTP_HEAP_SIZE	(1024 * 1024)
#define NET_HEAP_SIZE   (1 * 1024 * 1024)
#define TEST_USER_AGENT	"Daemon/PS4"

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

#define DIFFERENT_HASH true
#define SAME_HASH false
#define UPDATE_NEEDED true
#define UPDATE_NOT_NEEDED false


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


void cleanup_net(int req, int connid, int tmpid)
{
    int ret;
    if (req > 0) {
        ret = sceHttpDeleteRequest(req);
        if (ret < 0) {
            log_error("[Download] sceHttpDeleteRequest(%i) error: 0x%08X\n", req, ret);
        }
    }
    if (connid > 0) {
        ret = sceHttpDeleteConnection(connid);
        if (ret < 0) {
            log_error("[Download] sceHttpDeleteConnection(%i) error: 0x%08X\n", connid, ret);
        }
    }
    if (tmpid > 0) {
        ret = sceHttpDeleteTemplate(tmpid);
        if (ret < 0) {
            log_error("[Download] sceHttpDeleteTemplate(%i) error: 0x%08X\n", tmpid, ret);
        }
    }
}

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



bool MD5_hash_compare(const char* file1, const char* hash)
{
    unsigned char c[MD5_HASH_LENGTH];
    FILE* f1 = fopen(file1, "rb");
    if (f1 == NULL)
        return true;
    MD5_CTX mdContext;

    int bytes = 0;
    unsigned char data[1024];

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, f1)) != 0)
        MD5_Update(&mdContext, data, bytes);
    MD5_Final(c, &mdContext);

    char md5_string[33] = { 0 };

    for (int i = 0; i < 16; ++i) {
        snprintf(&md5_string[i * 2], 32, "%02x", (unsigned int)c[i]);
    }
    log_info("FILE HASH: %s", md5_string);
    md5_string[32] = 0;

    if (strcmp(md5_string, hash) != 0) {
        return DIFFERENT_HASH;
    }

    log_info("Input HASH: %s", hash);
    fclose(f1);

    return SAME_HASH;
}


#define ORBIS_TRUE 1

struct dl_args {
    const char* src,
        * dst;
    int req,
        idx,    // thread index!
        connid,
        tmpid,
        status, // thread status
        g_idx;  // global item index in icon panel
    double      progress;
    uint64_t    contentLength;
} dl_args;


int libnetMemId = 0xFF,
libsslCtxId = 0xFF,
libhttpCtxId = 0xFF,
statusCode = 0xFF;

int  contentLengthType;
uint64_t contentLength;



void DL_ERROR(const char* name, int statusCode, struct dl_args* i)
{
    log_warn("Download Failed with code HEX: %x Int: %i from Function %s src: %s", statusCode, statusCode, name, i->src);
}


int ini_dl_req(struct dl_args* i)
{
    log_info("i->src %s", i->src);

    statusCode = sceHttpCreateTemplate(libhttpCtxId, TEST_USER_AGENT, 2, 1);
    if (statusCode < 0) {
        DL_ERROR("sceHttpCreateTemplate()", i->tmpid, i);
        goto error;
    }
    i->tmpid = statusCode;

    statusCode = sceHttpCreateConnectionWithURL(i->tmpid, i->src, ORBIS_TRUE);
    if (statusCode < 0)
    {
        DL_ERROR("sceHttpCreateConnectionWithURL()", statusCode, i);
        goto error;
    }
    i->connid = statusCode;
    // ret = sceHttpSendRequest(req, NULL, 0);
    statusCode = sceHttpCreateRequestWithURL(i->connid, 0, i->src, 0);
    if (statusCode < 0)
    {
        DL_ERROR("sceHttpCreateRequestWithURL()", statusCode, i);
        goto error;
    }
    i->req = statusCode;

    statusCode = sceHttpSendRequest(i->req, NULL, 0);
    if (statusCode < 0) {
        DL_ERROR("sceHttpSendRequest()", statusCode, i);
        goto error;
    }

    int ret = sceHttpGetStatusCode(i->req, &statusCode);
    if (ret < 0 || statusCode != 200) {
        DL_ERROR("sceHttpGetStatusCode()", statusCode, i);
        goto error; //fail silently (its for JSON Func)

    }

    log_info("[%s:%i] ----- statusCode: %i ---", __FUNCTION__, __LINE__, statusCode);


    ret = sceHttpGetResponseContentLength(i->req, &contentLengthType, &contentLength);
    if (ret < 0)
    {
        log_error("[%s:%i] ----- sceHttpGetContentLength() error: %i ---", __FUNCTION__, __LINE__, ret);
        goto error;
    }
    else
    {
        if (contentLengthType == 0)
        {
            i->contentLength = contentLength;
            log_info("[%s:%i] ----- Content-Length = %lu ---", __FUNCTION__, __LINE__, contentLength);
        }
        else { // for some reason COUNT and MD5 have no LENs but the app still loads the content fine???
            i->contentLength = 255;
            log_warn("Code 200 Success.. however no content len was reported by the server by default this app only downloads 8kbs ");
        }

        return statusCode;
    }

error:
    log_error("%s error: %d, 0x%x", __FUNCTION__, statusCode, statusCode);
    if (i->req > 0) {
        ret = sceHttpDeleteRequest(i->req);
        if (ret < 0) {
            log_info("sceHttpDeleteRequest() error: 0x%08X\n", ret);
        }
    }
    if (i->connid > 0) {
        ret = sceHttpDeleteConnection(i->connid);
        if (ret < 0) {
            log_info("sceHttpDeleteConnection() error: 0x%08X\n", ret);
        }
    }
    if (i->tmpid > 0) {
        ret = sceHttpDeleteTemplate(i->tmpid);
        if (ret < 0) {
            log_info("sceHttpDeleteTemplate() error: 0x%08X\n", ret);
        }
    }

    // failsafe
    if (statusCode < 1) statusCode = 0x1337;

    return statusCode;
}


int netInit(void)
{

    int libnetMemId;
    int ret;
    /* libnet */
    ret = sceNetInit();
    ret = sceNetPoolCreate("simple", NET_HEAP_SIZE, 0);
    libnetMemId = ret;

    return libnetMemId;
}


#include <pthread.h>


void Network_Init()
{
    int ret = netInit();
    if (ret < 0) { log_info("netInit() error: 0x%08X", ret); }
    libnetMemId = ret;

    ret = sceHttpInit(libnetMemId, libsslCtxId, HTTP_HEAP_SIZE);
    if (ret < 0) { log_info("sceHttpInit() error: 0x%08X", ret); }
    libhttpCtxId = ret;
}

int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}
char* check_from_url(const char* url_)
{
    char  RES_STR[255];
    char  JSON[255];
    int total_read = 0, lprog = 0;

    memset(JSON, 0, sizeof(JSON));

    //avoid Pool error
    if (libhttpCtxId == 0xFF || libnetMemId == 0xFF) Network_Init();

    dl_args.src = url_;
    dl_args.req = -1;

    int r = ini_dl_req(&dl_args);
    log_info("status = %i", r);
    if (r == 200)
    {

        while (1)
        {
            int read = sceHttpReadData(dl_args.req, JSON, sizeof(JSON));
            if (read < 0) break;
            if (read == 0) break;

            total_read += read;

            int prog = (uint32_t)(((float)total_read / dl_args.contentLength) * 100.f);

            if (prog != lprog) {      // speeds it up,  the printfs are fucking slow 
                log_info("reading data, %lub / %lub (%u%%)", total_read, dl_args.contentLength, prog);
            }
            lprog = prog;

            if (total_read >= dl_args.contentLength || prog >= 100) {   //          reset
                prog = 100; break;//runn = 0; // stop
            }
        }
    }
    else
        return NULL;
    
    log_info("[StoreCore] Update Response: %s", JSON);

    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */

    jsmn_init(&p);
    r = jsmn_parse(&p, JSON, strlen(JSON), t, sizeof(t) / sizeof(t[0]));
    if (r  < 0) {
        log_error("[StoreCore] could not parse Update json");
        cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);
        return NULL;
    }
    

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        log_error("[StoreCore] JSON Update Object not found");
        cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);
        return NULL;
    }

    for (int i = 1; i < r; i++) {
        if (jsoneq(JSON, &t[i], "hash") == 0) {
            snprintf(RES_STR, 255, "%.*s", t[i + 1].end - t[i + 1].start, JSON + t[i + 1].start);
            cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);
            return strdup(RES_STR);
        }
        else
            log_error("[StoreCore] Update format not found");
    }//
    return NULL;
}


bool check_update_from_url(const char* url)
{
    char  http_req[300];
    char  dst_path[300];
    snprintf(http_req, 300, "%s/api.php?check_update=true", url);
    snprintf(dst_path, 300, "/user/app/NPXS39041/homebrew.elf");
    char* result = check_from_url(http_req);

    if (result != NULL) {
        bool ret = MD5_hash_compare(dst_path, result);
        free(result);
        log_info("[StoreCore] Update Status: %s", ret ? "UPDATE_REQUIRED": "NO_UPDATE");
        return ret;
    }
    else
        log_error("[StoreCore] Failed to get response from Update Server");
     

    return false;
}


bool full_init()
{
    
    // pad
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PAD)) return false;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYSUTIL)) return false;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL)) return false;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NET)) return false;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP)) return false;
    if (sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SSL)) return false;

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

int sceSysUtilSendSystemNotificationWithText(int messageType, char* message);

void notify(char* message)
{
    log_info(message);
    sceKernelLoadStartModule("/system/common/lib/libSceSysUtil.sprx", 0, NULL, 0, 0, 0);
    if (sceSysUtilSendSystemNotificationWithText != NULL)
        sceSysUtilSendSystemNotificationWithText(222, message);
    else
        log_warn("sceSysUtilSendSystemNotificationWithText NULL");
}  