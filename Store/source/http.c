#include <stdarg.h>
#include <stdlib.h> // calloc
#include <stdatomic.h>
#include <jsmn.h>
#include "defines.h"
#define ORBIS_TRUE 1

struct dl_args {
    const char *src,
               *dst;
    int req,
        idx,    // thread index!
        connid,
        tmpid,
        status, // thread status
        g_idx;  // global item index in icon panel
    double      progress;
    uint64_t    contentLength; 
    void *unused; // for cross compat with _v2
    bool is_threaded;
} dl_args;

atomic_ulong g_progress = 0;

int libnetMemId  = 0xFF,
    libsslCtxId  = 0xFF,
    libhttpCtxId = 0xFF,
    statusCode   = 0xFF;

int  contentLengthType;
uint64_t contentLength;



void DL_ERROR(const char* name, int statusCode, struct dl_args *i)
{
    if (!i->is_threaded) 
        log_warn( "[StoreCore][HTTP] Download Failed with code HEX: %x Int: %i from Function %s src: %s", statusCode, statusCode, name, i->src);
    else 
        msgok(WARNING, "%s\n\nHEX: %x Int: %i\nfrom Function %s src: %s", getLangSTR(DL_FAILED_W),statusCode, statusCode, name, i->src);
       
    
}

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

int ini_dl_req(struct dl_args *i)
{
    log_info( "i->src %s", i->src);
    char USER_AGENT[100];
    sprintf(USER_AGENT, TEST_USER_AGENT"-0x%x", SysctlByName_get_sdk_version());
    log_debug("[StoreCore][HTTP] User Agent set to %s", USER_AGENT);
    statusCode = sceHttpCreateTemplate(libhttpCtxId, USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
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

    log_info("[StoreCore][HTTP] RecvTimeout: %x", sceHttpSetRecvTimeOut(i->tmpid, 1999999999));
    log_info("[StoreCore][HTTP] Timeout: %x", sceHttpSetSendTimeOut(i->tmpid, 1999999999));

    statusCode = sceHttpSendRequest(i->req, NULL, 0);
    if (statusCode < 0) {
        DL_ERROR("sceHttpSendRequest()", statusCode, i);
        goto error;
    }

    int ret = sceHttpGetStatusCode(i->req, &statusCode);
    if (ret < 0 || statusCode != 200){
        DL_ERROR("sceHttpGetStatusCode()", statusCode, i);
        goto error; //fail silently (its for JSON Func)
        
    }

    log_info( "[%s:%i] ----- statusCode: %i ---", __FUNCTION__, __LINE__, statusCode);


    ret = sceHttpGetResponseContentLength(i->req, &contentLengthType, &contentLength);
    if (ret < 0)
    {
        log_error( "[%s:%i] ----- sceHttpGetContentLength() error: %i ---", __FUNCTION__, __LINE__, ret);
        goto error;
    }
    else
    {
        if (contentLengthType == ORBIS_HTTP_CONTENTLEN_EXIST)
        {
            i->contentLength = contentLength;
            log_info("[%s:%i] ----- Content-Length = %lu ---", __FUNCTION__, __LINE__, contentLength);
        }
        else { // for some reason COUNT and MD5 have no LENs but the app still loads the content fine???
            i->contentLength = 0x10000;
            log_warn("Code 200 Success.. however no content len was reported by the server by default this app only downloads 1MB ");
        }

        return statusCode;
    }

error:
    log_error( "%s error: %d, 0x%x", __FUNCTION__, statusCode, statusCode);
    cleanup_net(i->req, i->connid, i->tmpid);

    // failsafe
    if(statusCode < 1) statusCode = 0x1337;

    return statusCode;
}

//int download_file(int libnetMemId, int libhttpCtxId, const char* src, const char* dst)
void *start_routine(void *argument)
{
    struct dl_args *i = (struct dl_args*) argument;

    int ret = -1;
    int total_read = 0;
    log_error( "[Download] i->dst %s", i->dst);
    log_error( "[Download] i->url %s", i->src);
    log_error( "[Download] i->req %x", i->req);

    unsigned char buf[10000] = { 0 };

    unlink(i->dst);

    int fd = sceKernelOpen(i->dst, O_WRONLY | O_CREAT, 0777);
    log_debug("[Download] fd %i", fd);
    if(fd < 0) 
        goto cleanup;

    while (1)
    {
        int read = sceHttpReadData(i->req, buf, sizeof(buf));
        if (read < 0) { ret = (void*)read; log_error("[Download] read < 0"); goto cleanup; }
        if (read == 0) { log_error("[Download] read == 0");  goto cleanup; }

        ret = sceKernelWrite(fd, buf, read);
        if (ret < 0 || ret != read) {
            log_error("[Download] ret < 0 || ret != read");
            goto cleanup;
        }
        
        total_read += read;

        int prog = (uint32_t)(((float)total_read / i->contentLength) * 100.f);

        if(total_read >= i->contentLength || prog >=  100)  {   //          reset
            prog = 100; break;//runn = 0; // stop
        }
    }

cleanup:
    cleanup_net(i->req, i->connid, i->tmpid);
    if (fd > 0) {
        ret = sceKernelClose(fd);
        if (ret < 0) {
            log_error("[Download] sceKernelClose(%i) error: 0x%08X\n", fd, ret);
        }
    }

    // leave httpCtx, sslCtx, NetMemId valids to be reused !
    return (void*)ret;
}

#if defined (__ORBIS__)
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
#endif

#include <pthread.h>


void Network_Init()
{
#if defined (__ORBIS__)
    int ret = netInit();
    if (ret < 0) { log_info( "[StoreCore][NET_OPS] netInit() error: 0x%08X", ret); }
    libnetMemId = ret;

    ret = sceHttpInit(libnetMemId, libsslCtxId, HTTP_HEAP_SIZE);
    if (ret < 0) { log_info( "[StoreCore][NET_OPS] sceHttpInit() error: 0x%08X", ret); }
    libhttpCtxId = ret;
#endif
}

// the _v2 version is used in download panel
int dl_from_url(const char *url_, const char *dst_, bool is_threaded)
{
    // avoid Pool error
    if(libhttpCtxId == 0xFF || libnetMemId == 0xFF) Network_Init();

    dl_args.src = url_;
    dl_args.dst = dst_;
    dl_args.req = -1;
    dl_args.is_threaded = is_threaded;

    int ret = ini_dl_req(&dl_args);
    if(ret == 200)
    {   // passed
        if(is_threaded)
        {
            pthread_t thread = 0;
            ret = pthread_create(&thread, NULL, start_routine, &dl_args);
            log_debug( "[StoreCore][NET_OPS] pthread_create for %s, ret:%d", url_, ret);
        }
        else
        {
            start_routine(&dl_args);
            // flag done
            ret = 0;
        }
    }
    else // on error (404, or something different)
    {
        log_error( "[StoreCore][NET_OPS] %s, ret:%d", __FUNCTION__, ret);
    }

    return ret;
}

#include "jsmn.h"
#include "json.h"
int jsoneq(const char *json, jsmntok_t *tok, const char *s);


char *check_from_url(const char *url_, enum CHECK_OPTS opt, bool silent)
{
    char  RES_STR[255];
    char  JSON[255];
    int total_read = 0, lprog = 0;
    
    memset(JSON, 0, sizeof(JSON));

    //avoid Pool error
    if(libhttpCtxId == 0xFF || libnetMemId == 0xFF) Network_Init();

    dl_args.src = url_;
    dl_args.req = -1;

    int r = ini_dl_req(&dl_args);
    log_info( "status = %i", r);

#if BETA==1
    if (opt == BETA_CHECK)
        return r;
#endif
    if(r  == 200)
    {  
        while (1)
        {   
            uint64_t read = sceHttpReadData(dl_args.req, JSON, sizeof(JSON));
            if (read < 0) return NULL;
            if (read == 0) break;

            total_read += read;

            int prog  = (double)(((float)total_read / dl_args.contentLength) * 100.f);

            if (prog != lprog) {      // speeds it up,  the printfs are fucking slow 
                log_info( "[StoreCore][HTTP] reading data: %lub / %lub (%u%%)", total_read, dl_args.contentLength, prog);
                g_progress = prog;
            }
            lprog = prog;

            if(total_read >= dl_args.contentLength || prog >=  100)  {   //          reset
                prog = 100; break;//runn = 0; // stop
            }
        }
    }
    else {
        if (!silent)
           msgok(FATAL, getLangSTR(SERVER_DIS));
    }
    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */

    jsmn_init(&p);
    r = jsmn_parse(&p, JSON, strlen(JSON), t,  sizeof(t) / sizeof(t[0]));
    if (r < 0) {

        log_info("[StoreCore] Error Buffer: %s", JSON);
        if(!silent)
          msgok(FATAL,"%s: %d", getLangSTR(FAILED_TO_PARSE),r);

        cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);
        return NULL;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {

        log_info("[StoreCore] Error Buffer: %s", JSON);
        if (!silent)    
          msgok(FATAL, getLangSTR(OBJ_EXPECTED));
        cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);

        return NULL;
    }

    if(opt == MD5_HASH)
    {
        log_info( "[StoreCore] MD5 Hash: %s", JSON);

        for (int i = 1; i < r; i++) {
            if (jsoneq(JSON, &t[i], "hash") == 0) {
                snprintf(RES_STR, 255, "%.*s", t[i + 1].end - t[i + 1].start, JSON + t[i + 1].start);
                cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);
                return strdup(RES_STR);
            }
        }//
    }
    else if(opt == COUNT)
    {
        log_info( "COUNT_JSON = %s", JSON);

        for (int i = 1; i < r; i++) {
            if (jsoneq(JSON, &t[i], "number_rows") == 0) {
                snprintf(RES_STR, 255, "%.*s", t[i + 1].end - t[i + 1].start, JSON + t[i + 1].start);
                cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);
                return strdup(RES_STR);
            }

        }//

    }
    else if (opt == DL_COUNTER)
    {
        log_info("DL_COUNTER_JSON = %s", JSON);

        for (int i = 1; i < r; i++) {
            if (jsoneq(JSON, &t[i], "number_of_downloads") == 0) {
                snprintf(RES_STR, 255, "%.*s", t[i + 1].end - t[i + 1].start, JSON + t[i + 1].start);
                cleanup_net(dl_args.req, dl_args.connid, dl_args.tmpid);
                return strdup(RES_STR);
            }

        }//

    }

    return NULL;
}

