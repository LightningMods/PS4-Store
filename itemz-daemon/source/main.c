#include "defines.h"
#include <utils.h>
#include <sys/signal.h>
#include "log.h"
#include <ps4sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/socket.h>

bool IS_ERROR(uint32_t a1)
{
    return a1 & 0x80000000;
}

bool is_enabled = false;
void PS_Button_loop()
{
    OrbisUserServiceInitializeParams params;
    memset(&params, 0, sizeof(params));
    params.priority = 256;

    log_info("[Daemon] sceUserServiceInitialize %x", sceUserServiceInitialize(&params));

    OrbisUserServiceLoginUserIdList userIdList;

    log_info("[Daemon] sceUserServiceGetLoginUserIdList %x", sceUserServiceGetLoginUserIdList(&userIdList));

    for (int i = 0; i < 4; i++) {
        if (userIdList.userId[i] != -1) {
            log_info("[Daemon][%i] User ID 0x%x", i, userIdList.userId[i]);
        }
    }

    int ret = scePadInit();
    if (ret < 0)
       log_info("[Daemon] %s scePadInit return error 0x%8x", __FUNCTION__, ret);
    

    log_info("[Daemon] scePadSetProcessPrivilege %x", scePadSetProcessPrivilege(1));


    int pad = scePadOpen(userIdList.userId[0], 0, 0, NULL);
    if (pad < 0)
      log_info("[Daemon] %s scePadOpen return error 0x%8x", __FUNCTION__, pad);
    else
       log_info("[Daemon] Opened Pad");


    ScePadData data;

    log_info("[Daemon] l1 %x", sceLncUtilInitialize());


    log_info("[Daemon] Main Loop Started");
    int i = 0;

    while(pad > 0)
    {
        if (!isRestMode() && is_enabled)
        {
            //get sample size
            scePadReadState(pad, &data);
            
            if (data.buttons & PS_BUTTON) {//PS_BUTTON from RE

                log_info("[Daemon] PS BUTTON Was Pressed & intercepted");
                log_info("[Daemon] Redirecting Home Menu to ITEMzFlow");

                LncAppParam param;
                param.sz = sizeof(LncAppParam);

                if (userIdList.userId[0] != 0xFF)
                    param.user_id = userIdList.userId[0];
                else if (userIdList.userId[1] != 0xFF)
                    param.user_id = userIdList.userId[1];

                param.app_opt = 0;
                param.crash_report = 0;
                param.check_flag = SkipSystemUpdateCheck;
                log_info("[Daemon] Home menu Boot Option SkipSystemUpdateCheck Applied");

                uint32_t sys_res = sceLncUtilLaunchApp(HOME_TITLE_ID, 0, &param);
                if (IS_ERROR(sys_res)) {
                    if (sys_res == SCE_LNC_UTIL_ERROR_ALREADY_RUNNING || sys_res == SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_SUSPEND_NEEDED || sys_res == SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_KILL_NEEDED) {
                        log_info("[Daemon] ITEMzFlow Already running, resuming App....");
                        log_info("[Daemon] Redirect Successful");
                    }
                    else
                        log_info("[Daemon] NPXS39041 launch ret 0x%x", sys_res);
                }
                else
                    log_info("[Daemon] Redirect Successful");

            }
        }


        usleep(15000);

    }
}

#define VERSION_MAJOR 1
#define VERSION_MINOR 01



int main(int argc, char* argv[])
{
   ScePthread thread;
   // internals: net, user_service, system_service
   loadModulesVanilla();

   if (!jailbreak("/app0/Media/jb.prx"))  goto exit;

   if(!full_init()) goto exit;

   log_info("[Daemon] Registering Daemon...");

   sceSystemServiceRegisterDaemon();

   log_info("[Daemon] Starting Network loop Thread...");

   scePthreadCreate(&thread, NULL, network_loop, NULL, "network_loop_thread");

   log_info("[Daemon] Starting main PS Button loop...");

   PS_Button_loop();

   log_info("[Daemon] WTF the Loop eneded???");

       
exit:    
   log_info("[Daemon] Exiting");

   if (!rejail())
       log_info("[Daemon] rejail failed");

   sceKernelIccSetBuzzer(2);
   return sceSystemServiceLoadExec("exit", 0);
}