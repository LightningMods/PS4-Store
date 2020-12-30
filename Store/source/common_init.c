#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <orbislink.h>
#include <libkernel.h>
#include <sys/mman.h>
#include <ImeDialog.h>
#include <utils.h>
#include <MsgDialog.h>
#include <CommonDialog.h>
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

#include <orbislink.h>
extern OrbisGlobalConf globalConf;

int s_piglet_module = -1;
int s_shcomp_module = -1;
int JsonErr = 0,
    page;

StoreOptions set,
            *get;

/* XXX: patches below are given for Piglet module from 4.74 Devkit PUP */
static void pgl_patches_cb(void* arg, uint8_t* base, uint64_t size)
{
    /* Patch runtime compiler check */
    const uint8_t p_set_eax_to_1[] = {
        0x31, 0xC0, 0xFF, 0xC0, 0x90,
    };
    memcpy(base + 0x5451F, p_set_eax_to_1, sizeof(p_set_eax_to_1));
    /* Tell that runtime compiler exists */
    *(uint8_t*)(base + 0xB2DEC) = 0;
    *(uint8_t*)(base + 0xB2DED) = 0;
    *(uint8_t*)(base + 0xB2DEE) = 1;
    *(uint8_t*)(base + 0xB2E21) = 1;
    /* Inform Piglet that we have shader compiler module loaded */
    *(int32_t*)(base + 0xB2E24) = s_shcomp_module;
}

// reuses orbislink code from new liborbis (-lorbis)
int initGL_for_the_store(void)
{
    int ret = 0;

    get = &set; // address main get StoreOptions pointer

    // internals: net, user_service, system_service
    ret = loadModulesVanilla();

    // pad
    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PAD);
    if(ret) return -1;




    //Ime
    ret = sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG);
    if(ret) return -1;
    //MSGDIALOG
    ret = sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
    if(ret) return -1;


        ret =  sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL);
        if(ret) return -1;
	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NET);
        if(ret) return -1;
	
	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP);
        if(ret) return -1;

	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SSL);
        if(ret) return -1;


	ret = sceSysmoduleLoadModuleInternal(0x80000018);
        if(ret) return -1;
	
	ret = sceSysmoduleLoadModuleInternal(0x80000026);  // 0x80000026
        if(ret) return -1;
	
	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT);  // 0x80000026
        if(ret) return -1;
	
	ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_APPINSTUTIL);  // 0x80000026 0x80000037
        if(ret) return -1;
	

    sceCommonDialogInitialize();
    sceMsgDialogInitialize();


    ret = orbisPadInit();
    if(ret != 1) return -2;



    globalConf.confPad = orbisPadGetConf();

    // customs
    s_piglet_module = sceKernelLoadStartModule("/user/app/NPXS39041/pig.sprx",   0, NULL, 0, NULL, NULL);
    s_shcomp_module = sceKernelLoadStartModule("/user/app/NPXS39041/shacc.sprx", 0, NULL, 0, NULL, NULL);

    if(! patch_module("pig.sprx", &pgl_patches_cb, NULL, /*debugnetlevel*/3)) return -3;


     ret = LoadOptions(get);
     if(!ret)
         msgok(2, "Could NOT find/open the INI File");

    loadmsg("Downloading and Caching Website files....\n");

    page = 1; //first 
    while(JsonErr == 0)
    {
        JsonErr = getjson(page, get->StoreCDN);
        page++;
    }

    sceMsgDialogTerminate();

    // all fine.
    return 0;
}
