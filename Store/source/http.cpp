#include <stdarg.h>
#include <stdlib.h> // calloc

#include "defines.h"
#include "net.h"
#include "GLES2_common.h"
#include "utils.h"
#include "jsmn.h"
#include <curl/curl.h>
#include <errno.h>
#include <curl/easy.h>

#define ORBIS_TRUE 1

std::atomic_ulong g_progress(0);

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


void DL_ERROR(const char* name, int statusCode, const dl_arg_t *i)
{
    if (i->is_threaded) 
        log_warn( "[StoreCore][HTTP] Download Failed with code HEX: %x Int: %i from Function %s url: %s", statusCode, statusCode, name, i->url.c_str());
    else 
       msgok(WARNING, fmt::format("{}\n\nHEX: {:x} Int: {}\nfrom Function {} url: {}", getLangSTR(DL_FAILED_W), statusCode, statusCode, name, i->url));
}

int ini_dl_req(dl_arg_t *i)
{
    log_info( "i->url %s", i->url.c_str());
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
       curl_easy_setopt(curl, CURLOPT_URL, i->url.c_str());
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
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed with code %i from Function %s url: %s : error: %s", res, "ini_dl_req", i->url.c_str(), curl_easy_strerror(res));
        fclose(devfile);
        curl_easy_cleanup(curl);
        return 404;
       }
    
       curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
       curl_easy_cleanup(curl);
    }
    else{
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed Function %s url: %s", "ini_dl_req", i->url.c_str());
        curl_easy_cleanup(curl);
        return 500;
    }
    if(i->status < 1) i->status = 0x1337;

    return http_code;
}

int progress_func(void* ptr, double TotalToDownload, double NowDownloaded, 
                    double TotalToUpload, double NowUploaded)
{
    // ensure that the file to be downloaded is not empty
    // because that would cause a division by zero error later on
    if (TotalToDownload <= 0.0) {
        log_info( "[StoreCore][HTTP] TotalToDownload <= 0.0");
        return 0;
    }

    // how wide you want the progress meter to be
    int totaldotz=40;
    double fractiondownloaded = NowDownloaded / TotalToDownload;
    // part of the progressmeter that's already "full"
    int dotz = (int) round(fractiondownloaded * totaldotz);

    // create the "meter"
    int ii=0;
    printf("%3.0f%% [",fractiondownloaded*100);
    // part  that's full already
    for ( ; ii < dotz;ii++) {
        printf("=");
    }
    // remaining part (spaces)
    for ( ; ii < totaldotz;ii++) {
        printf(" ");
    }
    // and back to line begin - do not forget the fflush to avoid output buffering problems!
    printf("]\r");
    fflush(stdout);
    // if you don't return 0, the transfer will be aborted - see the documentation
    return 0; 
}

static int my_progress_func(void *bar,
                     double t, /* dltotal */ 
                     double d, /* dlnow */ 
                     double ultotal,
                     double ulnow)
{
    dl_arg_t *perc = (dl_arg_t*)bar;
    //dl_arg_t* perc_ptr = static_cast<std::shared_ptr<dl_arg_t>*>(bar);
    //dl_arg_t& perc = **perc_ptr;

    switch (perc->status.load())
    {
    case CANCELED:
          //perc.status = READY;
          log_info("DL CANCELLED");
          return CURL_READFUNC_ABORT;
        break;
            case PAUSED:
            if (!perc->paused) {
                if (perc->curl_handle) {
                    curl_easy_pause(perc->curl_handle, CURLPAUSE_ALL);
                    perc->paused = true;
                    perc->running = false;
                    log_info("Download Paused");
                } else {
                    log_error("curl_easy_pause() failed: curl_handle is NULL");
                }
            }
            return 0;
        case RUNNING:
            if (!perc->running) {
                if (perc->curl_handle) {
                    curl_easy_pause(perc->curl_handle, CURLPAUSE_CONT);
                    perc->running = true;
                    perc->paused = false;
                }
            }
            break;
    default:
        break;
    }
    // log_info("status: %i", perc->status);

    if (perc->status == RUNNING) {
        // get current file pos for resuming
        // cURL dltotal gets reset when you resume so we do this
        perc->last_off = ftell(perc->dlfd);
        perc->progress =  (double)(((float)perc->last_off / perc->contentLength.load()) * 100.f);
        if (t > 0 && d > 0 && (int)d % MB(5) == 0)
            log_debug("[HTTP][cURL] thread reading data: %.2f / %.2f (%.2f%%) off: %ld" , d, t, perc->progress.load(), perc->last_off);

    }
  return 0;
}

struct WriteCallbackData {
    FILE *file;
    std::atomic<int> &status;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    WriteCallbackData *data = static_cast<WriteCallbackData *>(stream);
    FILE *file = data->file;
    std::atomic<int> &status = data->status;
    //log_info("nmemb: %i",nmemb);

    if (file == NULL) {
        log_error("write_data: file pointer is NULL");
        return 0; // Return 0 to indicate an error
    }

    if (status.load() == PAUSED) {
        return CURL_WRITEFUNC_PAUSE;
    }
    //log_info("[HTTP][cURL] write_data status: %i", status->load());

    if (ptr != NULL) { // Check if ptr is NULL before calling fwrite
        size_t written = fwrite(ptr, size, nmemb, file);
        if (written != size * nmemb) {
            log_error("write_data: fwrite failed with error: %s", strerror(errno));
            return 0; // Return 0 to indicate an error
        }
        return written;
    } else {
        log_error("write_data: ptr is NULL");
        return 0; // Return 0 to indicate an error
    }
}
//int download_file(int libnetMemId, int libhttpCtxId, const char* src, const char* dst)
int start_routine(dl_arg_t* i)
{
    //std::unique_ptr<dl_arg_t> i(argument);
    //std::unique_ptr<dl_arg_t> i(static_cast<dl_arg_t*>(argument));
   // dl_arg_t* i = (dl_arg_t*)argument;

    log_info( "[Download] i->dst %s", i->dst.c_str());
    log_info( "[Download] i->url %s", i->url.c_str());
    char ua[100];
    char str[21];
    sprintf(&ua[0], USER_AGENT"-0x%x", SysctlByName_get_sdk_version());

    unlink(i->dst.c_str());
    CURLcode res;
    CURL* curl = NULL;
    curl = curl_easy_init();
    if (curl)
    {
       
        i->dlfd = fopen(i->dst.c_str(), "wb");
        if(!i->dlfd) {
           log_error("error opening file, ERROR: %s", strerror(errno));
           return errno;
        }
        WriteCallbackData data = {i->dlfd, i->status};
        i->curl_handle = curl;
        curl_easy_setopt(curl, CURLOPT_URL, i->url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, &ua[0]);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, i);
        // Fail the request if the HTTP code returned is equal to or larger than 400
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300); // Set the maximum time in seconds that you allow the transfer operation to take
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);      /* we want progress ... */
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);   /* to go here so we can detect a ^C within postgres */
        // Perform a file transfer synchronously.
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl), curl = NULL;
        if(res == CURLE_PARTIAL_FILE) {
Resume_loopback:
           log_info("CURLE_PARTIAL_FILE: Resuming @ %s / %s", calculateSize(i->last_off).c_str(), calculateSize(i->contentLength).c_str());
           i->last_off = ftell(i->dlfd);

           curl = curl_easy_init();
           if(curl) {
              i->curl_handle = curl;
              log_info("CURLE_PARTIAL_FILE: Initializing curl handle %p", i->curl_handle);

              curl_easy_setopt(curl, CURLOPT_URL, i->url.c_str());
              curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
              curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
              // Fail the request if the HTTP code returned is equal to or larger than 400
              curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
              curl_easy_setopt(curl, CURLOPT_USERAGENT, &ua[0]);
              curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
              curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, i);
              curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
              curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);      /* we want progress ... */
		      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);   /* to go here so we can detect a ^C within postgres */
               /*
                * Use RANGE method for resuming. We used to use RESUME_FROM_LARGE for this but some http servers
                * require us to always send the range request header. If we don't the server may provide different
                * content causing seeking to fail. Note that internally Curl will automatically handle this for FTP
                * so we don't need to worry about that here.
               */
              sprintf(str, "%ld-%ld", i->last_off, i->contentLength.load());
              log_debug("CURLE_PARTIAL_FILE: Range %s", str);
              curl_easy_setopt(curl, CURLOPT_RANGE, str);
              /* Perform the request, ERROR HANDLING BELOW */
              //i->contentLength = i->contentLength - i->last_off;
              res = curl_easy_perform(curl);
              curl_easy_cleanup(curl), curl = NULL;
              if(res == CURLE_PARTIAL_FILE) goto Resume_loopback;
           }
           else
             log_info("curl_easy_perform() failed: %d", curl);

        }
        if(res != CURLE_OK) {
            log_error( "[Download] curl_easy_perform() failed: %s", curl_easy_strerror(res));
            DL_ERROR("curl_easy_perform", res, i);
            if(i->dlfd)
               fclose(i->dlfd), i->dlfd = NULL;
            return res;
        }
        if(res == CURLE_OK){
          long http_code = 0;
          curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
          if((int)http_code > 299) {
            log_error( "[Download] HTTP error code: %d", http_code);
            DL_ERROR("HTTP error code", http_code, i);
            if(i->dlfd)
                fclose(i->dlfd), i->dlfd = NULL;

             return http_code;
          }
        }
    }

   if(i->dlfd)
       fclose(i->dlfd), i->dlfd = NULL;

    curl_off_t val;
    res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &val);
    if((CURLE_OK == res) && (val>0))
      log_info("Total download time: %lu.%06lu sec.", (unsigned long)(val / 1000000), (unsigned long)(val % 1000000));

    return 0;
}


#include <pthread.h>

// the _v2 version is used in download panel
int dl_from_url(std::string url_, std::string dst_)
{
    std::shared_ptr<dl_arg_t> args = std::make_shared<dl_arg_t>();
    args->url = url_;
    args->dst = dst_;
    args->req = -1;
    args->status = 0;
    args->is_threaded = true;

    args->dlfd = NULL;
    int  ret = start_routine(args.get());
    log_info("ret %d", ret);

    return ret;
}

int dl_from_url_v3_threaded(std::string url_, std::string dst_, dl_arg_t *arg)
{
    arg->url = url_;
    arg->dst = dst_;
    arg->req = -1;
    arg->status = RUNNING;
    arg->is_threaded = true;

    int  ret = start_routine(arg);
    log_info("ret %d", ret);

    return ret;
}


int jsoneq(const char *json, jsmntok_t *tok, const char *s);

#define BUFFER_SIZE (0x10000) /* 256kB */
struct WriteResult {
    std::string data;
    int pos;
};

static size_t curl_write(void *ptr, size_t size, size_t nmemb, void *stream) {
    WriteResult *result = static_cast<WriteResult *>(stream);

    size_t dataSize = size * nmemb;
    result->data.append(static_cast<char *>(ptr), dataSize);
    result->pos += dataSize;

    return dataSize;
}


std::string check_from_url(const std::string &url_, enum CHECK_OPTS opt, bool silent)
{ 
    std::string json, ua;
    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */
    CURL* curl;
    CURLcode res;
    long http_code = 0;

    ua = fmt::format(USER_AGENT"-{:#x}", SysctlByName_get_sdk_version());
    curl = curl_easy_init();
    if (curl)
    {
        
       WriteResult write_result{};
       curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
       curl_easy_setopt(curl, CURLOPT_USERAGENT, ua.c_str());
       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
       curl_easy_setopt(curl, CURLOPT_NOPROGRESS,   1l);
       curl_easy_setopt(curl, CURLOPT_NOBODY, 0l);
       curl_easy_setopt(curl, CURLOPT_HEADER, 0L); 
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

       res = curl_easy_perform(curl);
       if(!res) {
          json = write_result.data;
          log_info("RESPONSE: %s", json.c_str());

       }
       else{
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed with code %i from Function %s url: %s : error: %s", res, "ini_dl_req", url_.c_str(), curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        if (!silent) msgok(FATAL, getLangSTR(SERVER_DIS));
       }
    
       curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
       if(http_code != 200){
          curl_easy_cleanup(curl);
#if BETA==1
        if (opt == BETA_CHECK)
             return std::to_string(http_code);
#endif
          if (!silent) msgok(FATAL, getLangSTR(SERVER_DIS));
       }
       curl_easy_cleanup(curl);
    }
    else{
        log_warn( "[StoreCore][HTTP] curl_easy_perform() failed Function %s url: %s", "ini_dl_req", url_.c_str());
        curl_easy_cleanup(curl);
        if (!silent) msgok(FATAL, getLangSTR(SERVER_DIS));
    }

#if BETA==1
    if (opt == BETA_CHECK)
        return std::to_string(http_code);
#endif

    jsmn_init(&p);
    int r = jsmn_parse(&p, json.c_str(), json.length(), t,  sizeof(t) / sizeof(t[0]));
    if (r < 0) {

        log_info("[StoreCore] Error Buffer: %s", json.c_str());
        if(!silent)
          msgok(FATAL,fmt::format("{}: {}", getLangSTR(FAILED_TO_PARSE),r));

        return std::string();
    }

    /* Assume the top-level element is an object */
   if (r < 1 || t[0].type != JSMN_OBJECT) {
        log_info("[StoreCore] Error Buffer: %s", json.c_str());
        if (!silent) msgok(FATAL, getLangSTR(OBJ_EXPECTED));
        return std::string();
    }

    std::string result_str;
    for (int i = 1; i < r; i++) {
        if (jsoneq(json.c_str(), &t[i], "hash") == 0 || jsoneq(json.c_str(), &t[i], "number_of_downloads") == 0) {
            result_str.assign(json.c_str() + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            return result_str;
        }
    }

    return std::string();
}
