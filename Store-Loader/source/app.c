#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <MsgDialog.h>
#include "Header.h"


void logshit(char* format, ...)
{
	char* buff[1024];
	memset(buff, 0, 1024);

	va_list args;
	va_start(args, format);
	vsprintf(buff, format, args);
	va_end(args);

	sceKernelDebugOutText(DGB_CHANNEL_TTYL, buff);

	int fd = sceKernelOpen("/user/app/NPXS39041/logs/loader.log", O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd >= 0)
	{
		sceKernelWrite(fd, buff, strlen(buff));
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



 int32_t netInit(void)
{

	int libnetMemId;
	int ret;
	/* libnet */
	ret = sceNetInit();
	ret = sceNetPoolCreate("simple", NET_HEAP_SIZE, 0);
	libnetMemId = ret;

	return libnetMemId;
}

