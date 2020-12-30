


#include "defines.h"
#include <utils.h>
//extern void *(*libc_memset)(void *, int, size_t); ?

int libcmi = -1;
int (*jailbreak_me)(void);

int64_t sys_dynlib_load_prx(char* prxPath, int* moduleID)
{
	return (int64_t)syscall4(594, prxPath, 0, moduleID, 0);
}

int64_t sys_dynlib_unload_prx(int64_t prxID)
{
	return (int64_t)syscall1(595, (void*)prxID);
}


int64_t sys_dynlib_dlsym(int64_t moduleHandle, const char* functionName, void *destFuncOffset)
{
	return (int64_t)syscall3(591, (void*)moduleHandle, (void*)functionName, destFuncOffset);
}


OrbisPadConfig *confPad;
bool flag=true;

int    selected_icon;
double dt, u_t = 0;
struct timeval  t1, t2;

#define DELTA  (123) // dpad delta movement

void updateController()
{
    unsigned int buttons=0;
    int ret = orbisPadUpdate();

    sceKernelUsleep(5000);

//    ScePadData *padDataCurrent;
//ret=scePadReadState(confPad->padHandle,confPad->padDataCurrent);

//fprintf(INFO, "dpad:%d, %d\n", confPad->padDataCurrent->lx, confPad->padDataCurrent->ly);
/*
lx : 0-255 l-r
ly : 0-255 u-d
*/
    if(ret==0)
    {
        if(orbisPadGetButtonPressed(ORBISPAD_L2|ORBISPAD_R2) || orbisPadGetButtonHold(ORBISPAD_L2|ORBISPAD_R2))
        {
            printf("Combo L2R2 pressed\n");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L2|ORBISPAD_R2);
            orbisPadSetCurrentButtonsPressed(buttons);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L1|ORBISPAD_R1) )
        {
            printf("Combo L1R1 pressed\n");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L1|ORBISPAD_R1);
            orbisPadSetCurrentButtonsPressed(buttons);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L1|ORBISPAD_R2) || orbisPadGetButtonHold(ORBISPAD_L1|ORBISPAD_R2))
        {
            printf("Combo L1R2 pressed\n");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L1|ORBISPAD_R2);
            orbisPadSetCurrentButtonsPressed(buttons);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L2|ORBISPAD_R1) || orbisPadGetButtonHold(ORBISPAD_L2|ORBISPAD_R1) )
        {
            printf("Combo L2R1 pressed\n");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L2|ORBISPAD_R1);
            orbisPadSetCurrentButtonsPressed(buttons);
        }

        if(orbisPadGetButtonPressed(ORBISPAD_UP)//) || orbisPadGetButtonHold(ORBISPAD_UP))
        || confPad->padDataCurrent->ly < (127 - DELTA))
        {
            fprintf(INFO, "Up pressed\n");
            GLES2_scene_on_pressed_button(111);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_DOWN)// || orbisPadGetButtonHold(ORBISPAD_DOWN))
        || confPad->padDataCurrent->ly > (127 + DELTA))
        {
            fprintf(INFO, "Down pressed\n");
            GLES2_scene_on_pressed_button(116);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_RIGHT)// || orbisPadGetButtonHold(ORBISPAD_RIGHT))
        || confPad->padDataCurrent->lx > (127 + DELTA))
        {
            fprintf(INFO, "Right pressed\n");
            GLES2_scene_on_pressed_button(114);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_LEFT)// || orbisPadGetButtonHold(ORBISPAD_LEFT))
        || confPad->padDataCurrent->lx < (127 - DELTA))
        {
            fprintf(INFO, "Left pressed\n");
            GLES2_scene_on_pressed_button(113);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_TRIANGLE))
        {
            fprintf(INFO, "Triangle pressed exit\n");
            GLES2_scene_on_pressed_button(28);
            //flag=0;  // exit app
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_CIRCLE))
        {
            fprintf(INFO, "Circle pressed\n");
            GLES2_scene_on_pressed_button(54);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_CROSS))
        {
            fprintf(INFO, "Cross pressed\n");
            GLES2_scene_on_pressed_button(53);
            //notify("Notify test:\nPassed\n");
        }

        if(orbisPadGetButtonPressed(ORBISPAD_SQUARE))
        {
            fprintf(INFO, "Square pressed\n");
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L1))
        {
            fprintf(INFO, "L1 pressed\n");
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L2))
        {
            fprintf(INFO, "L2 pressed\n");
        }
        if(orbisPadGetButtonPressed(ORBISPAD_R1))
        {
            fprintf(INFO, "R1 pressed\n");
            int mId, hcId;

            int ret = pingtest(mId, hcId, "https://10.0.0.2");
            fprintf(INFO, "pingtest ret:%d\n", ret);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_R2))
        {
            fprintf(INFO, "R2 pressed\n");

            uint64_t numb;
            uint32_t ret = sceKernelGetCpuTemperature(&numb);
//            printf( "sceKernelGetCpuTemperature returned %i\n", ret);
            fprintf(INFO, "TEMP %d°C\n", numb);
        }
    }
}

void finishApp()
{
    //orbisAudioFinish();
    //orbisKeyboardFinish();
    //orbisGlFinish();
    orbisPadFinish();
    orbisNfsFinish();
}

static bool initAppGl()
{
    int ret = orbisGlInit(ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);
    if(ret>0)
    {
        glViewport(0, 0, ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);
        ret=glGetError();
        if(ret)
        {
            fprintf(INFO, "[%s] glViewport failed: 0x%08X\n",__FUNCTION__,ret);
            return false;
        }
//      glClearColor(0.f, 0.f, 1.f, 1.f);          // blue RGBA
        glClearColor( 0.1211, 0.1211, 0.1211, 1.); // background color

        ret=glGetError();
        if(ret)
        {
            fprintf(INFO, "[%s] glClearColor failed: 0x%08X\n",__FUNCTION__,ret);
            return false;
        }
        return true;
    }
    return false;
}

bool initApp()
{
    //orbisNfsInit(NFSEXPORT);
    //orbisFileInit();
//  int ret=initOrbisLinkApp();

    sceSystemServiceHideSplashScreen();


    confPad = orbisPadGetConf(); 

    if( ! initAppGl() ) return false;

    return true;
}


/// for timing, fps
#define WEN  (2048)
unsigned int frame   = 1,
             time_ms = 0;

int main(int argc, char *argv[])
{
    StoreOptions set;
    int ret = sceKernelIccSetBuzzer(1);

    sleep(2);
    /*
        prepare pad and piglet modules in advance,
        read /data/orbislink/orbislink_config.ini for debugnet

        (no nfs, audio init yet...)
    */
    //debugNetInit("10.0.0.2", 18198, 3);

    // to access /user and notify we need full privileges
    //escalate_priv();

sys_dynlib_load_prx("/app0/Media/jb.prx", &libcmi);

ret = sys_dynlib_dlsym(libcmi, "jailbreak_me", &jailbreak_me);
if (!ret)
{
    klog("jailbreak_me resolved from PRX\n");
   
    if(ret = jailbreak_me() != 0)
    goto error;
}
else
    goto error; 


    /* load some required modules */
#if defined (USE_NFS)
    ret = initOrbisLinkAppVanillaGl();

    /* my setup */
//  ret = orbisNfsInit("nfs://10.0.0.2/hostapp");
    ret = orbisNfsInit("nfs://10.0.0.2/Archive/PS4-work/orbisdev/orbisdev-samples/linux_es2goodness");
//  ret = orbisNfsInit("nfs://192.168.2.61/home/alfa/NFS/hostapp");

    fprintf(INFO, "orbisNfsInit return: %x\n", ret);
    sleep(1);
#else
    if(ret = initGL_for_the_store() < 0)  
          goto error;
         

#endif
    if(!ret)
       sceKernelIccSetBuzzer(1);

    // init some libraries
    flag = initApp();

    ret = sceSysmoduleLoadModule(0x009A);  // internal FreeType, libSceFreeTypeOl

    int moduleId = sceKernelLoadStartModule("/system/common/lib/libSceSysUtil.sprx", 0, NULL, 0, 0, 0);

    // more test
    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL);
    fprintf(INFO, "%s SCE_SYSMODULE_INTERNAL_NETCTL: 0x%08X\n",__FUNCTION__, ret);

    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP);
    fprintf(INFO, "%s SCE_SYSMODULE_INTERNAL_HTTP: 0x%08X\n",__FUNCTION__, ret);

    ret = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SSL);
    fprintf(INFO, "%s SCE_SYSMODULE_INTERNAL_SSL: 0x%08X\n",__FUNCTION__, ret);

    // feedback
    notify("Here we are!");

    /* init GLES2 stuff */

    // demo-font.c init
    es2init_text( ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT );
    // ES_UI
    GLES2_scene_init( ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT );

    /// reset timers
    time_ms = get_time_ms();
    gettimeofday(&t1, NULL);
    t2 = t1;

    /// enter main render loop
    while(flag)
    {
        updateController();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ret = glGetError();
        if(ret) {
            fprintf(INFO, "[ORBIS_GL] glClear failed: 0x%08X\n", ret);
            //goto err;
        }

        /// draw

        // freetype demo-font.c, renders text just from init_
        //render_text(); 
        // ES_UI
        GLES2_scene_render();

        /// get timing, fps
        if( ! (frame %WEN) )
        {
            unsigned int now = get_time_ms();
            fprintf(INFO, "frame: %d, took: %ums, fps: %.3f\n", frame, now - time_ms,
                                               ((double)WEN / (double)(now - time_ms) * 1000.f));
            time_ms = now;
        }
        frame++;

        if(1) // update for the GLES uniform time
        {
            gettimeofday( &t2, NULL );
            // calculate delta time
            dt = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
            //printf("dt = %0.4f\n", dt);
            t1 = t2;
            // update total time
            u_t += dt;
        }

        orbisGlSwapBuffers();  /// flip frame

        sceKernelUsleep(10);  // haha, control your rendering properly , use delta time for animations not wait 
    }

error:
   sceKernelIccSetBuzzer(2); 
   msgok(FATAL, "App has Died with exit code %i", ret);

    // destructors
    ORBIS_RenderFillRects_fini();
    on_GLES2_Final();
    pixelshader_fini();
    finishOrbisLinkApp();

    exit(EXIT_SUCCESS);
}
