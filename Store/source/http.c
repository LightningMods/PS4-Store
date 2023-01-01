#include <stdarg.h>
#include <stdlib.h> // calloc
#include <stdatomic.h>
#include "defines.h"
#include "net.h"
#include "GLES2_common.h"
#include "utils.h"
#include "jsmn.h"
#include <curl/curl.h>
#include <errno.h>

#define ORBIS_TRUE 1

atomic_ulong g_progress = 0;

int libnetMemId = 0xFF,
libsslCtxId = 0xFF,
libhttpCtxId = 0xFF;

static int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}


void DL_ERROR(const char* name, int statusCode, dl_arg_t *i)
{
    if (!i->is_threaded) 
        log_warn( "[StoreCore][HTTP] Download Failed with code HEX: %x Int: %i from Function %s url: %s", statusCode, statusCode, name, i->url);
    else 
        msgok(WARNING, "%s\n\nHEX: %x Int: %i\nfrom Function %s url: %s", getLangSTR(DL_FAILED_W),statusCode, statusCode, name, i->url);
}

int ini_dl_req(dl_arg_t *i)
{
    log_info( "i->url %s", i->url);
    //i->status = 500;

    char ua[100];
    sprintf(&ua[0], USER_AGENT"-0x%x", SysctlByName_get_sdk_version());
    log_debug("[StoreCore][HTTP] User Agent set to %s", &ua[0]);

    CURL* curl;
    CURLcode res;
    long http_code = 0;
    curl = curl_easy_init();
    if (curl)
    {
       FILE* devfile = fopen("/dev/null", "w");
       curl_easy_setopt(curl, CURLOPT_URL, i->url);
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
       curl_easy_setopt(curl, CURLOPT_USERAGENT, &ua[0]);
       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
       // Fail the request if the HTTP code returned is equal to or larger than 400
       curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
       curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
       curl_easy_setopt(curl, CURLOPT_NOBODY, 1l);
       curl_easy_setopt(curl, CURLOPT_HEADER, 1L); 
       if(devfile)
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, devfile); // write to /dev/null

       res = curl_easy_perform(curl);
       if(!res) {
         /* check the size */
         curl_off_t cl;
         res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
         if(!res) {
            log_info("Download size: %lu", cl);
            i->contentLength = cl;
         }
         fclose(devfile);
       }
       else{
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed with code %i from Function %s url: %s : error: %s", res, "ini_dl_req", i->url, curl_easy_strerror(res));
        fclose(devfile);
        curl_easy_cleanup(curl);
        return 404;
       }
    
       curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
       curl_easy_cleanup(curl);
    }
    else{
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed Function %s url: %s", "ini_dl_req", i->url);
        curl_easy_cleanup(curl);
        return 500;
    }
    if(i->status < 1) i->status = 0x1337;

    return http_code;
}

int my_progress_func(void *bar,
                     double t, /* dltotal */ 
                     double d, /* dlnow */ 
                     double ultotal,
                     double ulnow)
{
    dl_arg_t *perc = (dl_arg_t*)bar;

    switch (perc->status)
    {
    case CANCELED:
          //perc->status = READY;
          return CURL_READFUNC_ABORT;
        break;
    case PAUSED:
        curl_easy_pause(perc->curl_handle, CURLPAUSE_ALL);
        break;
    case RUNNING:
        curl_easy_pause(perc->curl_handle, CURLPAUSE_CONT);
        break;
    default:
        break;
    }

    if (t > 0 && perc->status == RUNNING) {
        // get current file pos for resuming
        // cURL dltotal gets reset when you resume so we do this
        perc->last_off = ftell(perc->dlfd);
        perc->progress = (double)(((float)perc->last_off / perc->contentLength) * 100.f);
        if (t > 0 && d > 0 && (int)d % MB(5) == 0)
            log_debug("[HTTP][cURL] thread reading data: %.2f / %.2f (%.2f%%) off: %ld" , d, t, perc->progress, perc->last_off);

    }
  return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    //log_info("size: %i nmemb: %i ptr: %p stream: %p", size, nmemb, ptr, stream);
    size_t written = fwrite(ptr, size, nmemb, stream);
    
    return written;
}
//int download_file(int libnetMemId, int libhttpCtxId, const char* src, const char* dst)
int start_routine(void *argument)
{
     dl_arg_t *i = (dl_arg_t*) argument;
    
    log_info( "[Download] i->dst %s", i->dst);
    log_info( "[Download] i->url %s", i->url);
    char ua[100];
    char str[21];
    sprintf(&ua[0], USER_AGENT"-0x%x", SysctlByName_get_sdk_version());

    unlink(i->dst);
    CURLcode res;
    CURL* curl = NULL;
    curl = curl_easy_init();
    if (curl)
    {
        i->curl_handle = curl;
        i->dlfd = fopen(i->dst, "wb");
        if(!i->dlfd) {
           log_error("error opening file, ERROR: %s", strerror(errno));
           return errno;
        }
        curl_easy_setopt(curl, CURLOPT_URL, i->url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, &ua[0]);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, i);
        // Fail the request if the HTTP code returned is equal to or larger than 400
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, i->dlfd);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);      /* we want progress ... */
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);   /* to go here so we can detect a ^C within postgres */
        // Perform a file transfer synchronously.
        res = curl_easy_perform(curl);
        if(res == CURLE_PARTIAL_FILE) {
Resume_loopback:
           curl_easy_cleanup(curl);
           log_info("CURLE_PARTIAL_FILE: Resuming @ %s / %s", calculateSize(i->last_off), calculateSize(i->contentLength));
           i->last_off = ftell(i->dlfd);

           curl = curl_easy_init();
           if(curl) {
              i->curl_handle = curl;
              log_info("CURLE_PARTIAL_FILE: Initializing curl handle %x", i->curl_handle);

              curl_easy_setopt(curl, CURLOPT_URL, i->url);
              curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
              curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
              // Fail the request if the HTTP code returned is equal to or larger than 400
              curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
              curl_easy_setopt(curl, CURLOPT_USERAGENT, &ua[0]);
              curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
              curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, i);
              curl_easy_setopt(curl, CURLOPT_WRITEDATA, i->dlfd);
              curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);      /* we want progress ... */
		      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);   /* to go here so we can detect a ^C within postgres */
               /*
                * Use RANGE method for resuming. We used to use RESUME_FROM_LARGE for this but some http servers
                * require us to always send the range request header. If we don't the server may provide different
                * content causing seeking to fail. Note that internally Curl will automatically handle this for FTP
                * so we don't need to worry about that here.
               */
              sprintf(str, "%ld-%ld", i->last_off, i->contentLength);
              log_debug("CURLE_PARTIAL_FILE: Range %s", str);
              curl_easy_setopt(curl, CURLOPT_RANGE, str);
              /* Perform the request, ERROR HANDLING BELOW */
              //i->contentLength = i->contentLength - i->last_off;
              res = curl_easy_perform(curl);
              if(res == CURLE_PARTIAL_FILE) goto Resume_loopback;
           }
           else
             log_info("curl_easy_perform() failed: %d", curl);

        }
        if(res != CURLE_OK) {
            log_error( "[Download] curl_easy_perform() failed: %s", curl_easy_strerror(res));
            DL_ERROR("curl_easy_perform", res, i);
            curl_easy_cleanup(curl);
            if(i->dlfd)
               fclose(i->dlfd), i->dlfd = NULL;
            return res;
        }
        if(res == CURLE_OK){
          long http_code = 0;
          curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
          if((int)http_code < 200 || (int)http_code > 299) {
             log_error( "[Download] HTTP error code: %d", http_code);
             DL_ERROR("HTTP error code", http_code, i);
             curl_easy_cleanup(curl);
             if(i->dlfd)
                fclose(i->dlfd), i->dlfd = NULL;
             return http_code;
          }
        }
    }

   if(i->dlfd)
       fclose(i->dlfd), i->dlfd = NULL;
       
   if(!i->is_threaded){
      free((void*)i->dst), i->dst = NULL;
      free((void*)i->url), i->url = NULL;
      free((void*)i), i = NULL;
   }
    if(curl)
       curl_easy_cleanup(curl);

    curl_off_t val;
    res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &val);
    if((CURLE_OK == res) && (val>0))
      log_info("Total download time: %lu.%06lu sec.\n", (unsigned long)(val / 1000000), (unsigned long)(val % 1000000));

    // leave httpCtx, sslCtx, NetMemId valids to be reused !
    return 0;
}


#include <pthread.h>

// the _v2 version is used in download panel
int dl_from_url(const char *url_, const char *dst_, dl_arg_t* arg, bool is_threaded)
{
    dl_arg_t* args = arg;
    if(args == NULL){
       args = (dl_arg_t*)malloc(sizeof(dl_arg_t));
       args->url = strdup(url_);
       args->dst = strdup(dst_);
       args->req = -1;
       args->status = 0;
       args->is_threaded = is_threaded;
    }
    else
      args->is_threaded = is_threaded;

    args->dlfd = NULL;
    return start_routine((void*)args);
}


int jsoneq(const char *json, jsmntok_t *tok, const char *s);

#define BUFFER_SIZE (0x10000) /* 256kB */

struct write_result {
char *data;
int pos;
};

static size_t curl_write( void *ptr, size_t size, size_t nmemb, void *stream) {

struct write_result *result = (struct write_result *)stream;

/* Will we overflow on this write? */
if(result->pos + size * nmemb >= BUFFER_SIZE - 1) {
   log_error("curl error: too small buffer\n");
   return 0;
}

/* Copy curl's stream buffer into our own buffer */
memcpy(result->data + result->pos, ptr, size * nmemb);

/* Advance the position */
result->pos += size * nmemb;

return size * nmemb;

}



char *check_from_url(const char *url_, enum CHECK_OPTS opt, bool silent)
{
    char  RES_STR[255];
    char  JSON[BUFFER_SIZE];
    char  ua[100];
    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */
    CURL* curl;
    CURLcode res;
    long http_code = 0;
    
    memset(JSON, 0, sizeof(JSON));


    sprintf(&ua[0], USER_AGENT"-0x%x", SysctlByName_get_sdk_version());
    curl = curl_easy_init();
    if (curl)
    {
        
       struct write_result write_result = {
        .data = &JSON[0],
        .pos = 0
       };
       curl_easy_setopt(curl, CURLOPT_URL, url_);
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
       curl_easy_setopt(curl, CURLOPT_USERAGENT, &ua[0]);
       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
       curl_easy_setopt(curl, CURLOPT_NOPROGRESS,   1l);
       curl_easy_setopt(curl, CURLOPT_NOBODY, 0l);
       curl_easy_setopt(curl, CURLOPT_HEADER, 0L); 
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

       res = curl_easy_perform(curl);
       if(!res) {
          log_info("RESPONSE: %s", JSON);
       }
       else{
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed with code %i from Function %s url: %s : error: %s", res, "ini_dl_req", url_, curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        if (!silent) msgok(FATAL, getLangSTR(SERVER_DIS));
       }
    
       curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
       if(http_code != 200){
          curl_easy_cleanup(curl);
          if (!silent) msgok(FATAL, getLangSTR(SERVER_DIS));
       }
       curl_easy_cleanup(curl);
    }
    else{
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed Function %s url: %s", "ini_dl_req", url_);
        curl_easy_cleanup(curl);
        if (!silent) msgok(FATAL, getLangSTR(SERVER_DIS));
    }

#if BETA==1
    if (opt == BETA_CHECK)
        return r;
#endif

    jsmn_init(&p);
    int r = jsmn_parse(&p, JSON, strlen(JSON), t,  sizeof(t) / sizeof(t[0]));
    if (r < 0) {

        log_info("[StoreCore] Error Buffer: %s", JSON);
        if(!silent)
          msgok(FATAL,"%s: %d", getLangSTR(FAILED_TO_PARSE),r);

        return NULL;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {

        log_info("[StoreCore] Error Buffer: %s", JSON);
        if (!silent)    
          msgok(FATAL, getLangSTR(OBJ_EXPECTED));

        return NULL;
    }

     for (int i = 1; i < r; i++) {
        if (jsoneq(JSON, &t[i], "hash") == 0 || jsoneq(JSON, &t[i], "number_of_downloads") == 0) {
             snprintf(RES_STR, 255, "%.*s", t[i + 1].end - t[i + 1].start, JSON + t[i + 1].start);
             return strdup(RES_STR);
         }
     }//

    return NULL;
}

bool pingtest(const char* server)
{
    log_info( "Ping requested for: %s", server);
    loadmsg(getLangSTR(SEARCHING));
    //i->status = 500;

    char tmp[100];
    sprintf(&tmp[0], "%s/store.db", server);

    CURL* curl;
    CURLcode res = CURLE_OK;
    curl = curl_easy_init();
    if (curl)
    {
       FILE* devfile = fopen("/dev/null", "w");
       curl_easy_setopt(curl, CURLOPT_URL, &tmp[0]);
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	   sprintf(&tmp[0], USER_AGENT"-0x%x", SysctlByName_get_sdk_version());
       log_info("[StoreCore][HTTP] User Agent set to %s", &tmp[0]);
       curl_easy_setopt(curl, CURLOPT_USERAGENT, &tmp[0]);
       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
       curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	   // Fail the request if the HTTP code returned is equal to or larger than 400
       curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
       curl_easy_setopt(curl, CURLOPT_NOBODY, 1l);
       curl_easy_setopt(curl, CURLOPT_HEADER, 1L); 
       if(devfile)
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, devfile); // write to /dev/null

       curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
       curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
       res = curl_easy_perform(curl);
       if(!res) {
         /* check the size */
         curl_off_t cl;
         res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
         if(!res) {
            log_info("Download size: %lu", cl);
         }
         fclose(devfile);
         curl_easy_cleanup(curl);
       }
       else{
          log_info( "[StoreCore][HTTP] curl_easy_perform() failed with code %i from Function ini_dl_req url: %s : error: %s", res, server, curl_easy_strerror(res));
          fclose(devfile);
          curl_easy_cleanup(curl);
          sceMsgDialogTerminate();
          return false;
       }
    }
    else{
        log_info( "[StoreCore][HTTP] curl_easy_perform() failed Function ini_dl_req url: %s", server);
        curl_easy_cleanup(curl);
        sceMsgDialogTerminate();
        return false;
    }

    sceMsgDialogTerminate();
    return true;
}