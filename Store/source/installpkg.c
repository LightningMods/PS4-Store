
#include "Header.h"
#include <stdarg.h>
#include <stdlib.h> // calloc
#include <installpkg.h>
#include <stdbool.h>

#define ORBIS_TRUE 1

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <libkernel.h>
#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO

#else // on linux

#include <stdio.h>
#define  debugNetPrintf  fprintf
#define  ERROR           stderr
#define  DEBUG           stdout
#define  INFO            stdout

#endif

bool EALREADY = false;



int PKG_ERROR(const char* name, int ret)
{
    msgok(3, "Install Failed with code\nHEX: %x\n Int: %i\n\nfrom Function %s\n\n\n", ret, ret, name);
    fprintf(ERROR, "%s error: %i\n", ret);

    return ret;
}


uint8_t pkginstall(const char* path)
{
	static const uint8_t magic[] = { '\x7F', 'C', 'N', 'T' };
    char title_id[16];
	int is_app, ret, bgft_mid = 0;
	int task_id = -1;
	char buffer[255];


	//sceKernelClose(fd);
	int fd = sceKernelOpen(path, 0000, 0x0000);
	if (fd >= 0) {
		sceKernelClose(fd);

   
		struct bgft_init_params init_params;
		memset(&init_params, 0, sizeof(init_params));
		{
			init_params.size = 0x100000;
			init_params.mem = malloc(init_params.size);
			if (!init_params.mem) 
				return PKG_ERROR("init_params", ret);

			
			memset(init_params.mem, 0, init_params.size);
		}


	if(!EALREADY)
	{


		ret = sceAppInstUtilInitialize();
		if (ret)
			return PKG_ERROR("sceAppInstUtilInitialize", ret);
		


		 ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT);
		 if(ret!=0)
               return PKG_ERROR("sceSysmoduleLoadModuleInternal", ret);


		ret = sceBgftServiceInit(&init_params);
		if (ret && ret  != 0x80990001) 
		   return PKG_ERROR("sceBgftInitialize", ret);

		EALREADY = true;
	}



		ret = sceAppInstUtilGetTitleIdFromPkg(path, title_id, &is_app);
		if (ret) 
			return PKG_ERROR("sceAppInstUtilGetTitleIdFromPkg", ret);


		sprintf(buffer, "%s via Store", title_id);
		klog("%s\n", buffer);




		struct bgft_download_param_ex download_params;
		memset(&download_params, 0, sizeof(download_params));
		download_params.param.entitlement_type = 5;
		download_params.param.id = "";
		download_params.param.content_url = path;
		download_params.param.content_name = buffer;
		download_params.param.icon_path = "/update/fakepic.png";
		download_params.param.playgo_scenario_id = "0";
		download_params.param.option = BGFT_TASK_OPTION_DELETE_AFTER_UPLOAD;
		download_params.slot = 0;



	retry:

		
		ret = sceBgftServiceIntDownloadRegisterTaskByStorageEx(&download_params, &task_id);
		if (ret == 0x80990088)
		{
	
			ret = sceAppInstUtilAppUnInstall(&title_id);
			if(ret!=0)
				return PKG_ERROR("sceAppInstUtilAppUnInstall", ret);

			goto retry;
		}
		else if (ret) 
	           return PKG_ERROR("sceBgftServiceIntDownloadRegisterTaskByStorageEx", ret);



		printf("Task ID(s): 0x%08X", task_id);


		ret = sceBgftServiceDownloadStartTask(task_id);
		if (ret) 
			return PKG_ERROR("sceBgftDownloadStartTask", ret);



	}
	else
		return PKG_ERROR("sceKernelOpen", fd);





	return 0;

}

