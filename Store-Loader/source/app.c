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

	printf(buff);

	int fd = sceKernelOpen("/data/Loader_Logs.txt", O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd >= 0)
	{
		sceKernelWrite(fd, buff, strlen(buff));
		sceKernelClose(fd);
	}
}

void init_STOREGL_modules()
{
	
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_SYSTEM_SERVICE\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_USER_SERVICE);
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_USER_SERVICE\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL);
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_NETCTL\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NET);
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_NET\n");
	//sceKernelLoadStartModule("/app0/Media/libSceHttp.sprx", 0, 0, 0, 0, 0);
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP);
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_HTTP\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SSL);
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_SSL\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYS_CORE);
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_SYS_CORE\n");
	sceSysmoduleLoadModuleInternal(0x80000018);
	klog("[DEBUG] Started Internal Module 0x80000018\n");
	sceSysmoduleLoadModuleInternal(0x80000026);  // 0x80000026
	klog("[DEBUG] Started Internal Module libSceSysUtil_SYSMODULE_INTERNAL_NETCTL\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT);  // 0x80000026
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_BGFT\n");
	sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_APPINSTUTIL);  // 0x80000026 0x80000037
	klog("[DEBUG] Started Internal Module SCE_SYSMODULE_INTERNAL_APPINSTUTIL\n");
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

