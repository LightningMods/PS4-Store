#include "Header.h"
#include <stdarg.h>

#define ORBIS_TRUE 1
#include <MsgDialog.h>

int msgok(char* format, ...)
{
        int ret = 0;
	sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

	sceMsgDialogTerminate();

	char* buff[1024];
	char buffer[1000];
	memset(buff, 0, 1024);

	va_list args;
	va_start(args, format);
	vsprintf(buff, format, args);
	va_end(args);

	strcpy(buffer, buff);

	logshit(buff);

	sceMsgDialogInitialize();
	OrbisMsgDialogParam param;
	OrbisMsgDialogParamInitialize(&param);
	param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

	OrbisMsgDialogUserMessageParam userMsgParam;
	memset(&userMsgParam, 0, sizeof(userMsgParam));
	userMsgParam.msg = buffer;
	userMsgParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_OK;
	param.userMsgParam = &userMsgParam;

	if (sceMsgDialogOpen(&param) < 0)
           klog("MsgD failed\n"); ret = -1;	
	

	OrbisCommonDialogStatus stat;

	while (1)
	{
		stat = sceMsgDialogUpdateStatus();
		if (stat == ORBIS_COMMON_DIALOG_STATUS_FINISHED)
		{
			OrbisMsgDialogResult result;
			memset(&result, 0, sizeof(result));

			if (0 > sceMsgDialogGetResult(&result))
                         klog("MsgD failed\n"); ret = -2;

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

	char* buff[1024];
	memset(buff, 0, 1024);

	va_list args;
	va_start(args, format);
	vsprintf(buff, format, args);
	va_end(args);

	// int32_t	ret = 0;_sceCommonDialogBaseParamInit

	OrbisMsgDialogButtonsParam buttonsParam;
	OrbisMsgDialogUserMessageParam messageParam;
	OrbisMsgDialogParam dialogParam;

	OrbisMsgDialogParamInitialize(&dialogParam);

	memset(&buttonsParam, 0x00, sizeof(buttonsParam));
	memset(&messageParam, 0x00, sizeof(messageParam));

	dialogParam.userMsgParam = &messageParam;
	dialogParam.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

	messageParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT;

	messageParam.msg = buff;

	sceMsgDialogOpen(&dialogParam);

	return 0;
}




int pingtest(int libnetMemId, int libhttpCtxId, const char* src)
{
	int ret;
	int contentLengthType;
	int res;
	uint64_t contentLength;
	int32_t  statusCode;

	int tpl = sceHttpCreateTemplate(libhttpCtxId, TEST_USER_AGENT, 1, 1);
	if (tpl < 0)
		goto clean;

	ret = sceHttpCreateConnectionWithURL(tpl, src, ORBIS_TRUE);
	if (ret < 0)
	{
		logshit("sceHttpCreateConnectionWithURL() error: %i\n", ret);
		goto clean;
	}
	else
		logshit("[STORE_GL_Loader:%s:%i] ----- sceHttpCreateConnectionWithURL() Success ---\n", __FUNCTION__, __LINE__);

	int conn = ret;

	ret = sceHttpCreateRequestWithURL(conn, 0, src, 0);
	if (ret < 0)
	{
		logshit("sceHttpCreateRequestWithURL() error: %i\n", ret);
		goto clean;
	}
	int req = ret;

	ret = sceHttpSendRequest(req, NULL, 0);
	if (ret < 0)
		printf("ERROR: %i", ret);


clean:
	sceHttpDeleteRequest(req);
	sceHttpDeleteConnection(conn);
	sceHttpDeleteTemplate(tpl);

	return ret;
}

int32_t progress_steps = 10;
int32_t total_progress = 0;


int progstart(char* msg)
{


	int32_t ret, _sceCommonDialogBaseParamInit = 0;

	ret = sceMsgDialogTerminate();

	klog("sceMsgDialogTerminate = %i\n", ret);

	ret = sceMsgDialogInitialize();

	klog("sceMsgDialogInitialize = %i\n", ret);


	OrbisMsgDialogParam dialogParam;
	OrbisMsgDialogParamInitialize(&dialogParam);
	dialogParam.mode = 2;

	OrbisMsgDialogProgressBarParam  progBarParam;
	memset(&progBarParam, 0, sizeof(OrbisMsgDialogProgressBarParam));

	dialogParam.progBarParam = &progBarParam;
	dialogParam.progBarParam->barType = 0;
	dialogParam.progBarParam->msg = msg;

	OrbisCommonDialogStatus stat;

	sceMsgDialogOpen(&dialogParam);

	return ret;
}


int download_file(int libnetMemId, int libhttpCtxId, const char* src, const char* dst)
{
	int ret;
	int contentLengthType;
	int res;
	uint64_t contentLength;
	int 	 statusCode;

	int tpl = sceHttpCreateTemplate(libhttpCtxId, TEST_USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
	if (tpl < 0)
		goto error;


	ret = sceHttpCreateConnectionWithURL(tpl, src, ORBIS_TRUE);
	if (ret < 0)
	{
		logshit("sceHttpCreateConnectionWithURL() error: %i\n", ret);
		goto error;
	}
	int conn = ret;
	// ret = sceHttpSendRequest(req, NULL, 0);
	ret = sceHttpCreateRequestWithURL(conn, 0, src, 0);
	if (ret < 0)
	{
		logshit("sceHttpCreateRequestWithURL() error: %i\n", ret);
		goto error;
	}
	int req = ret;

	ret = sceHttpSendRequest(req, NULL, 0);
	if (ret < 0)
	{
		{
			goto error;
		}
	}

	ret = sceHttpGetStatusCode(req, &statusCode);
	if (ret < 0 || statusCode != 200)
	{
		goto error;
	}

	logshit("[STORE_GL_Loader:%s:%i] ----- statusCode: %i ---\n", __FUNCTION__, __LINE__, statusCode);

	unsigned char buf[4096] = {0};

	long long length = 0;
	ret = sceHttpGetResponseContentLength(req, &contentLengthType, &contentLength);
	if (ret < 0)
	{
		logshit("[STORE_GL_Loader:%s:%i] ----- sceHttpGetContentLength() error: %i ---\n", __FUNCTION__, __LINE__, ret);
		//msgok("Connection Error\n\n The PS4 has Reported the Error 0x%08X\n\n Join Our Discord for Support", ret);
		goto error;
	}
	else
	{
		if (contentLengthType == ORBIS_HTTP_CONTENTLEN_EXIST)
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- Content-Length = %lu ---\n", __FUNCTION__, __LINE__, contentLength);
		}
	}
	if (statusCode == 200)
	{
		char buf[1024];

		progstart("start");

		int fd = sceKernelOpen(dst, O_WRONLY | O_CREAT, 0777);
		// fchmod(fd, 777);
		int total_read = 0;
		if (fd < 0)
		{
			return fd;
		}

		while (1)
		{
			int read = sceHttpReadData(req, buf, sizeof(buf));
			if (read < 0)
			{
				return read;
			}
			if (read == 0)
				break;
			ret = sceKernelWrite(fd, buf, read);
			if (ret < 0 || ret != read)
			{
				if (ret < 0)
					return ret;
				return -1;
			}
			total_read += read;

			uint32_t g_progress = (uint32_t)(((float)total_read / contentLength) * 100.f);
			int status = sceMsgDialogUpdateStatus();
			if (ORBIS_COMMON_DIALOG_STATUS_RUNNING == status)
			{
				sprintf(buf, "Downloading...\n\n %s\n Size: %lld", dst, contentLength);
				sceMsgDialogProgressBarSetValue(0, g_progress);

				sceMsgDialogProgressBarSetMsg(0, buf);

			}
		}
		ret = sceKernelClose(fd);

		goto clean;
	}
	else
	{
		statusCode = 0xDEADC0DE;
		goto error;
	}

error:
	logshit("connection errror\n");

	logshit("[STORE_GL_Loader:%s:%i] ----- status code = %i ---\n", __FUNCTION__, __LINE__, statusCode);

clean:
	sceHttpDeleteRequest(req);
	sceHttpDeleteConnection(conn);
	sceHttpDeleteTemplate(tpl);



	sceMsgDialogTerminate();


	return statusCode;
}
