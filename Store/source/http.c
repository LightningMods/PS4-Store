#include <stdarg.h>
#include <stdlib.h> // calloc
#include <stdatomic.h>


#define ORBIS_TRUE 1


#include "defines.h"
#include "Header.h"

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
    void *unused;
    bool is_threaded;
} dl_args;

#if defined (__ORBIS__)
int pingtest(int libnetMemId, int libhttpCtxId, const char* src)
{
//      int libnetMemId = 0;
        int libsslCtxId = 0;
        //int libhttpCtxId = 0;
        int ret;
        /* libnet */
        fprintf(INFO, "netinit\n");
        ret = sceNetInit();
        ret = sceNetPoolCreate(__FUNCTION__, NET_HEAP_SIZE, 0);
        libnetMemId = ret;

        fprintf(INFO, "sceSslInit\n");
        ret = sceSslInit(SSL_HEAP_SIZE);
        if (ret < 0) { fprintf(ERROR, "sceSslInit() error: 0x%08X\n", ret); }   
        libsslCtxId = ret;

        fprintf(INFO, "sceHttpInit\n");
        ret = sceHttpInit(libnetMemId, libsslCtxId, HTTP_HEAP_SIZE);
        if (ret < 0) { fprintf(INFO, "sceHttpInit() error: 0x%08X\n", ret); }
        libhttpCtxId = ret;

        char *dest = calloc(1, 4096);
        const char *url = strdup("http://10.0.0.2/");

//        download_file(libnetMemId, libhttpCtxId, url, dest);

fprintf(INFO, "%s\n", dest);

        fprintf(INFO, "done\n");

        return ret; // useless
}

int pingtest2(int libnetMemId, int libhttpCtxId, const char* src)
{
    int ret;
    int contentLengthType;
    int res;
    uint64_t contentLength;
    int32_t  statusCode;

int libsslCtxId;
size_t poolsize = 512;
    ret = sceNetCtlInit();
fprintf(INFO, "sceNetCtlInit(): %i\n", ret);

    ret = sceNetPoolCreate(__FUNCTION__, 4 * 1024, 0);
    fprintf(INFO, "sceNetPoolCreate(): %i\n", ret);
    if (ret < 0) {
        fprintf(ERROR, "sceNetPoolCreate() failed (0x%x)\n", ret);
    }
    libnetMemId = ret;

    libsslCtxId = sceSslInit(libnetMemId);
    fprintf(INFO, "sceSslInit(): %i\n", libsslCtxId);

    const char *hostname = strdup("google.com");
    int rid   = -1;
    int memid = -1;

    ret = sceHttpInit(libnetMemId, libsslCtxId, poolsize);
    fprintf(INFO, "sceHttpInit(): %i\n", ret);

/////////
    int tpl = sceHttpCreateTemplate(libhttpCtxId, TEST_USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
    fprintf(INFO, "sceHttpCreateTemplate(): %i\n", tpl);
    if (tpl < 0)
    {
        return tpl;
    }
    sceHttpsDisableOption(tpl, 0x01);

    ret = sceHttpCreateConnectionWithURL(tpl, src, ORBIS_TRUE);
    if (ret < 0)
    {
        fprintf(ERROR, "sceHttpCreateConnectionWithURL() error: %i\n", ret);
    }
    else
        fprintf(INFO, "[%s:%i] ----- PingTest CreateConnection() Success ---\n", __FUNCTION__, __LINE__);

    int conn = ret;

    ret = sceHttpCreateRequestWithURL(conn, 0, src, 0);
    if (ret < 0)
    {
        fprintf(ERROR, "sceHttpCreateRequestWithURL() error: %i\n", ret);
    }
    int req = ret;

    ret = sceHttpSendRequest(req, NULL, 0);
    if (ret < 0)
    {
        if (ret == 0x804101E2)
        {
            fprintf(ERROR, "ping failed");
        }
    }
    return ret;
}
#endif

atomic_ulong g_progress = 0;

int libnetMemId = 0xFF,
    libsslCtxId = 0xFF,
    libhttpCtxId = 0xFF,
    statusCode = 0xFF;

int ret,
contentLengthType,
res;
uint64_t contentLength;


void DL_ERROR(const char* name, int statusCode, struct dl_args *i)
{
    if (!i->is_threaded) {
        printf("this isnt supposed happen\n");
    }
    else {
        msgok(3, "Download Failed with code\nHEX: %x\n Int: %i\n\nfrom Function %s\n\n src: %s\n", statusCode, statusCode, name, i->src);
       // fprintf(ERROR, "%s error: %i\n", statusCode);
    }
}

int ini_dl_req(struct dl_args *i)
{
    klog("i->src %s\n", i->src);

    i->tmpid = sceHttpCreateTemplate(libhttpCtxId, TEST_USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
    if (i->tmpid < 0) {
        DL_ERROR("sceHttpCreateTemplate()", statusCode, i);
        goto error;
    }

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
    if (ret < 0) {
        DL_ERROR("sceHttpSendRequest()", statusCode, i);
        goto error;
    }

    ret = sceHttpGetStatusCode(i->req, &statusCode);
    if (ret < 0 || statusCode != 200)
    {
        if(statusCode == 404)
        {
            if(i->is_threaded)
                    DL_ERROR("sceHttpGetStatusCode()", statusCode, i);
            else
                goto error; //fail silently (its for JSON Func)

        }
        else
           DL_ERROR("sceHttpGetStatusCode()", statusCode, i);
        goto error;
    }

    fprintf(INFO, "[%s:%i] ----- statusCode: %i ---\n", __FUNCTION__, __LINE__, statusCode);


    long long length = 0;
    ret = sceHttpGetResponseContentLength(i->req, &contentLengthType, &contentLength);
    if (ret < 0)
    {
        fprintf(ERROR, "[%s:%i] ----- sceHttpGetContentLength() error: %i ---\n", __FUNCTION__, __LINE__, ret);
        goto error;
    }
    else
    {
        if (contentLengthType == ORBIS_HTTP_CONTENTLEN_EXIST)
        {
            i->contentLength = contentLength;
            fprintf(DEBUG, "[%s:%i] ----- Content-Length = %lu ---\n", __FUNCTION__, __LINE__, contentLength);
            return statusCode;
        }
    }

error:
    fprintf(ERROR, "connection error\n");

    //failsafe
    if (statusCode <= 0)
        statusCode = 0x1337;

    return statusCode;
}

//int download_file(int libnetMemId, int libhttpCtxId, const char* src, const char* dst)
void *start_routine(void *argument)
{
    struct dl_args *i = (struct dl_args*) argument;

    int ret = -1;
    klog("i->dst %s\n", i->dst);
    klog("i->url %s\n", i->src);
    klog("i->req %x\n", i->req);

    unsigned char buf[8000] = { 0 };

    unlink(i->dst);

    int fd = sceKernelOpen(i->dst, O_WRONLY | O_CREAT, 0777);
    // fchmod(fd, 777);
    int total_read = 0, last=0;
    if (fd < 0)
        return fd;

    int lprog = 0;
    while (1)
    {
        int read = sceHttpReadData(i->req, buf, sizeof(buf));
        if (read <  0) 
            return read;
        if (read == 0) 
            break;
        ret = sceKernelWrite(fd, buf, read);
        if (ret < 0 || ret != read)
        {
            if (ret < 0) return ret;
            return -1;
        }
        total_read += read;

        int prog  = (uint32_t)(((float)total_read / i->contentLength) * 100.f);

        if (prog != lprog) {      // speeds it up,  the printfs are fucking slow 
            fprintf(DEBUG, "reading data, %lub / %lub (%u%%)\n", total_read, i->contentLength, prog);
            g_progress = prog;
        }
        lprog = prog;
      
        if(total_read >= i->contentLength || prog >=  100)  {   //          reset
            prog = 100; break;//runn = 0; // stop
        }
    }
    ret  = sceKernelClose(fd);

cleanup:
    if(i->req)
       sceHttpDeleteRequest(i->req);
    if(i->connid)
       sceHttpDeleteConnection(i->connid);
    if(i->tmpid)
       sceHttpDeleteTemplate(i->tmpid);

    // leave httpCtx, sslCtx, NetMemId valids to be reused !

    return ret;
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
    if (ret < 0) { klog("netInit() error: 0x%08X\n", ret); }
    libnetMemId = ret;

    ret = sceHttpInit(libnetMemId, libsslCtxId, HTTP_HEAP_SIZE);
    if (ret < 0) { klog("sceHttpInit() error: 0x%08X\n", ret); }
    libhttpCtxId = ret;
#endif
}

// the _v2 version is used in download panel
int dl_from_url(const char *url_, const char *dst_, bool is_threaded)
{
    //avoid Pool error
    if(libhttpCtxId == 0xFF || libnetMemId == 0xFF)
           Network_Init();

    dl_args.src = url_;
    dl_args.dst = dst_;
    dl_args.req = -1;
    dl_args.is_threaded = is_threaded;

    ret = ini_dl_req(&dl_args);
    if (ret == 200)
    {
        if (is_threaded)
        {
            pthread_t thread = 0;
            ret = pthread_create(&thread, NULL, start_routine, &dl_args);
            //scanf("%s", message); // block until input
            fprintf(DEBUG, "pthread_create for %s, ret:%d\n", url_, ret);

            ani_notify("Download thread start");
        }
        else
            ret = start_routine(&dl_args);
    }

    return ret;
}
