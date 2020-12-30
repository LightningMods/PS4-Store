#include <stdint.h>
#include <string.h>

#include <ps4sdk.h>
#include <debugnet.h>
#include <libkernel.h>

    ////// next
void next_test()
{
	int ret = sceAppInstUtilInitialize(NULL);
	ret = sceAppInstUtilAppInstallPkg(NULL);
	ret = sceAppInstUtilGetTitleIdFromPkg(NULL);
	ret = sceAppInstUtilCheckAppSystemVer(NULL);
	ret = sceAppInstUtilAppPrepareOverwritePkg(NULL);
	ret = sceAppInstUtilGetPrimaryAppSlot(NULL);
	ret = sceAppInstUtilAppUnInstall(NULL);
	ret = sceAppInstUtilAppGetSize(NULL);
	ret = sceBgftServiceInit(NULL);
	ret = sceBgftServiceIntDownloadRegisterTaskByStorageEx(NULL);
	ret = sceBgftServiceDownloadStartTask(NULL);
	//ret = sceSysmoduleLoadModuleInternal(0);
	//ret = sceSysmoduleUnloadModuleInternal(NULL);
}


int sceSysUtilSendSystemNotificationWithText(int messageType, char* message);
int notify(char *message)
{
//	int moduleId = sceKernelLoadStartModule("/system/common/lib/libSceSysUtil.sprx", 0, NULL, 0, 0, 0);

	int ret = sceSysUtilSendSystemNotificationWithText(222, message);

	return ret;
}

