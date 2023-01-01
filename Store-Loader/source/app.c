#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <MsgDialog.h>
#include "Header.h"
#include "lang.h"
#include <curl/curl.h>

#define USER_AGENT	"StoreHAX/GL"


void logshit(char* format, ...)
{
	char buff[1024];
	memset(&buff[0], 0, sizeof buff);

	va_list args;
	va_start(args, format);
	vsprintf(&buff[0], format, args);
	va_end(args);

	sceKernelDebugOutText(DGB_CHANNEL_TTYL, &buff[0]);

	int fd = sceKernelOpen("/user/app/NPXS39041/logs/loader.log", O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd >= 0)
	{
		sceKernelWrite(fd, &buff[0], strlen(&buff[0]));
		sceKernelClose(fd);
	}
}

void init_STOREGL_modules()
{
	
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_SYSTEM_SERVICE\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_USER_SERVICE);
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_USER_SERVICE\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL);
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_NETCTL\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NET);
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_NET\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP);
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_HTTP\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SSL);
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_SSL\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYS_CORE);
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_SYS_CORE\n");
	sceSysmoduleLoadModuleInternal(0x80000018);
	logshit("[DEBUG] Started Internal Module 0x80000018\n");
	sceSysmoduleLoadModuleInternal(0x80000026);  // 0x80000026
	logshit("[DEBUG] Started Internal Module libSceSysUtil_SYSMODULE_INTERNAL_NETCTL\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT);  // 0x80000026
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_BGFT\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_APPINSTUTIL);  // 0x80000026 0x80000037
	logshit("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_APPINSTUTIL\n");
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
}


uint32_t sdkVersion = -1;
int	sysctlbyname(const char *, void *, size_t *, const void *, size_t);
uint32_t SysctlByName_get_sdk_version(void)
{
    //cache the FW Version
    if (0 < sdkVersion) {
        size_t   len = 4;
        // sysKernelGetLowerLimitUpdVersion == machdep.lower_limit_upd_version
        // rewrite of sysKernelGetLowerLimitUpdVersion
        sysctlbyname("machdep.lower_limit_upd_version", &sdkVersion, &len, NULL, 0);
    }

    // FW Returned is in HEX
    return sdkVersion;
}
 

#define ORBIS_TRUE 1

int msgok(char* format, ...)
{
    int ret = 0;
	sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

	sceMsgDialogTerminate();

	char buff[1024];
	memset(&buff[0], 0, 1024);

	va_list args;
	va_start(args, format);
	vsprintf(&buff[0], format, args);
	va_end(args);

	logshit(&buff[0]);

	sceMsgDialogInitialize();
	OrbisMsgDialogParam param;
	OrbisMsgDialogParamInitialize(&param);
	param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

	OrbisMsgDialogUserMessageParam userMsgParam;
	memset(&userMsgParam, 0, sizeof(userMsgParam));
	userMsgParam.msg = &buff[0];
	userMsgParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_OK;
	param.userMsgParam = &userMsgParam;

	ret = sceMsgDialogOpen(&param);
	
	OrbisCommonDialogStatus stat;

	while (1)
	{
		stat = sceMsgDialogUpdateStatus();
		if (stat == ORBIS_COMMON_DIALOG_STATUS_FINISHED)
		{
			OrbisMsgDialogResult result;
			memset(&result, 0, sizeof(result));

			ret = sceMsgDialogGetResult(&result);

			sceMsgDialogTerminate();
			break;
		}
	}

	return ret;
}

int loadmsg(char* format, ...)
{
	sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

	sceMsgDialogInitialize();

	char buff[1024];
	memset(&buff[0], 0, 1024);

	va_list args;
	va_start(args, format);
	vsprintf(&buff[0], format, args);
	va_end(args);

	OrbisMsgDialogButtonsParam buttonsParam;
	OrbisMsgDialogUserMessageParam messageParam;
	OrbisMsgDialogParam dialogParam;

	OrbisMsgDialogParamInitialize(&dialogParam);

	memset(&buttonsParam, 0x00, sizeof(buttonsParam));
	memset(&messageParam, 0x00, sizeof(messageParam));

	dialogParam.userMsgParam = &messageParam;
	dialogParam.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

	messageParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT;

	messageParam.msg = &buff[0];

	sceMsgDialogOpen(&dialogParam);

	return 0;
}

int pingtest(const char* server)
{
    logshit( "server %s\n", server);
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
       logshit("[StoreCore][HTTP] User Agent set to %s\n", &tmp[0]);
       curl_easy_setopt(curl, CURLOPT_USERAGENT, &tmp[0]);
       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
       curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	   // Fail the request if the HTTP code returned is equal to or larger than 400
       curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
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
            logshit("Download size: %lu\n", cl);
         }
         fclose(devfile);
       }
       else{
        logshit( "[StoreCore][HTTP] curl_easy_perform() failed with code %i from Function ini_dl_req url: %s : error: %s\n", res, server, curl_easy_strerror(res));
        fclose(devfile);
        curl_easy_cleanup(curl);
        return res;
       }
       curl_easy_cleanup(curl);
    }
    else{
        logshit( "[StoreCore][HTTP] curl_easy_perform() failed Function ini_dl_req url: %s\n", server);
        curl_easy_cleanup(curl);
        return res;
    }

    return 0;
}

int progstart(char* msg)
{


	int32_t ret = 0;

	ret = sceMsgDialogTerminate();

	logshit("sceMsgDialogTerminate = %i\n", ret);

	ret = sceMsgDialogInitialize();

	logshit("sceMsgDialogInitialize = %i\n", ret);


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
/* follow the CURLOPT_XFERINFOFUNCTION callback definition */
static int update_progress(void *p, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)
{
    logshit( "Download: %lld / %lld\n", dlnow, dltotal);
	char buf[1024];
	uint32_t g_progress = (uint32_t)(((float)dlnow / dltotal) * 100.f);
	int status = sceMsgDialogUpdateStatus();
	if (ORBIS_COMMON_DIALOG_STATUS_RUNNING == status)
	{
		sprintf(buf, "%s...\n\n Size: %lu", getLangSTR(DOWNLOADING_UPDATE), dltotal);
		sceMsgDialogProgressBarSetValue(0, g_progress);
		sceMsgDialogProgressBarSetMsg(0, buf);
	}
    return 0;
}


long download_file(const char* src, const char* dst)
{
	CURL *curl = NULL;
	long httpresponsecode = 0;
	
    char ua[100];
    sprintf(&ua[0], USER_AGENT"-0x%x", SysctlByName_get_sdk_version());

    unlink(dst);
	FILE* imageFD = fopen(dst, "wb");
	if(!imageFD){
		logshit( "[StoreCore][HTTP] Failed to open file %s\n", dst);
		return 500;
	}
	
	CURLcode res = CURLE_OK;
	if (!curl) curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, src);
		// Set user agent string
		curl_easy_setopt(curl, CURLOPT_USERAGENT, &ua[0]);
		// not sure how to use this when enabled
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		// not sure how to use this when enabled
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		// Set SSL VERSION to TLS 1.2
		curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
		// Set timeout for the connection to build
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
		// Follow redirects (?)
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		// The function that will be used to write the data 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
		// The data filedescriptor which will be written to
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, imageFD);
        // maximum number of redirects allowed
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20L);
        // Fail the request if the HTTP code returned is equal to or larger than 400
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        // request using SSL for the FTP transfer if available
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);

        /* pass the struct pointer into the xferinfo function */
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &update_progress);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

		// Perform the request
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
			curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpresponsecode);
			msgok("%s : HTTP Code: %i\n", curl_easy_strerror(res), httpresponsecode);
			logshit("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

			fclose(imageFD);
	        curl_easy_cleanup(curl);
			return 500;
			
		}else{
    		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpresponsecode);
			logshit("OK [%d]\n", httpresponsecode);
		}

	}else{
		logshit("NO CURL curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		return 500;
	}

	// close filedescriptor
	fclose(imageFD);
	// cleanup
	curl_easy_cleanup(curl);
    logshit("returning %lu\n", httpresponsecode);
	return httpresponsecode;
	
}