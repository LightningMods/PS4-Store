#include "defines.h"
#include <utils.h>
#include <signal.h>
#include <orbisGl.h>
#include <errno.h>
#include <orbis/SystemService.h>
int libcmi = -1;
int32_t crash_test_2(void* pCommon)
{
  log_debug("crash_test_2");
  return 0;
}
int32_t (*sceCoredumpRegisterCoredumpHandler)(
	int32_t (*handler)(void *),
	size_t stackSize,
	void *pCommon
)= NULL;
void debugNetPrintf(int level, char* format, ...) {

}
unsigned char * orbisNfsGetFileContent(const char *filename){
    log_info("fake orbisNfsGetFileContent called()\n");
    return NULL;
}
OrbisGlConfig* GlConf;
OrbisPadConfig *confPad;
bool flag=false;

int    selected_icon;
double dt, u_t = 0;
struct timeval  t1, t2;
const char *launch_query = NULL;

#define DELTA  (123) // dpad delta movement

int* (*crash_test)();
static int (*jailbreak_me)(void) = NULL;

void updateController()
{
    unsigned int buttons=0;
    int ret = orbisPadUpdate();
    if(ret==0)
    {
        if(orbisPadGetButtonPressed(ORBISPAD_L2|ORBISPAD_R2) || orbisPadGetButtonHold(ORBISPAD_L2|ORBISPAD_R2))
        {
            log_info("Combo L2R2 pressed");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L2|ORBISPAD_R2);
            orbisPadSetCurrentButtonsPressed(buttons);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L1|ORBISPAD_R1) )
        {
            log_info("Combo L1R1 pressed");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L1|ORBISPAD_R1);
            orbisPadSetCurrentButtonsPressed(buttons);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L1|ORBISPAD_R2) || orbisPadGetButtonHold(ORBISPAD_L1|ORBISPAD_R2))
        {
            log_info("Combo L1R2 pressed");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L1|ORBISPAD_R2);
            orbisPadSetCurrentButtonsPressed(buttons);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L2|ORBISPAD_R1) || orbisPadGetButtonHold(ORBISPAD_L2|ORBISPAD_R1) )
        {
            log_info("Combo L2R1 pressed");
            buttons=orbisPadGetCurrentButtonsPressed();
            buttons&= ~(ORBISPAD_L2|ORBISPAD_R1);
            orbisPadSetCurrentButtonsPressed(buttons);
        }

        if(orbisPadGetButtonPressed(ORBISPAD_UP))
        {
            
            log_info( "Up pressed");
            GLES2_scene_on_pressed_button(111);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_DOWN))
        {
            log_info( "Down pressed");

            GLES2_scene_on_pressed_button(116);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_RIGHT))
        {
            log_info( "Right pressed");
            GLES2_scene_on_pressed_button(114);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_LEFT))
        {
            log_info( "Left pressed");
            GLES2_scene_on_pressed_button(113);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_TRIANGLE))
        {
            log_info( "Triangle pressed exit");
            GLES2_scene_on_pressed_button(28);
            //flag=0;  // exit app
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_CIRCLE))
        {
            log_info( "Circle pressed");
            GLES2_scene_on_pressed_button(54);
        }
        else
        if(orbisPadGetButtonPressed(ORBISPAD_CROSS))
        {
            log_info( "Cross pressed");
            GLES2_scene_on_pressed_button(53);
        }

        if(orbisPadGetButtonPressed(ORBISPAD_SQUARE))
        {

            log_info( "Square pressed");
            GLES2_scene_on_pressed_button(SQU);
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L1))
        {
            log_info( "L1 pressed");
            trigger_dump_frame();
        }
        if(orbisPadGetButtonPressed(ORBISPAD_L2))
        {

            log_info( "L2 pressed");
            int libcmi = sceKernelLoadStartModule("/system/common/lib/libkernel_sys.sprx", 0, NULL, 0, 0, 0);
            sceKernelDlsym(libcmi, "sceCoredumpRegisterCoredumpHandler", (void**)&sceCoredumpRegisterCoredumpHandler);
            if(sceCoredumpRegisterCoredumpHandler)
               sceCoredumpRegisterCoredumpHandler(crash_test_2, 16384, NULL);
            else
               log_info("sceCoredumpRegisterCoredumpHandler not found");
        }
        if(orbisPadGetButtonPressed(ORBISPAD_R1))
        {
           
                
        }
        if(orbisPadGetButtonPressed(ORBISPAD_R2))
        {
        }
        if (orbisPadGetButtonPressed(ORBISPAD_OPTIONS))
        {
            log_info("Exit Called");
            //crash_test();
            raise(SIGQUIT);
        }
    }
}

void finishApp()
{
    orbisPadFinish();
}

static bool initAppGl()
{
    int ret = orbisGlInit(ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);
    if (ret > 0)
    {
        glViewport(0, 0, ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);
        ret = glGetError();
        if (ret)
        {
            log_info("[%s] glViewport failed: 0x%08X", __FUNCTION__, ret);
            return false;
        }
        //      glClearColor(0.f, 0.f, 1.f, 1.f);          // blue RGBA
        glClearColor(0.1211, 0.1211, 0.1211, 1.); // background color

        ret = glGetError();
        if (ret)
        {
            log_info("[%s] glClearColor failed: 0x%08X", __FUNCTION__, ret);
            return false;
        }
        return true;
    }
    return false;
}

bool initApp()
{
#if defined (USE_NFS)
    orbisNfsInit(NFSEXPORT);
    orbisFileInit();
    int ret = initOrbisLinkApp();
#endif

    sceSystemServiceHideSplashScreen();

    confPad = orbisPadGetConf();

    if (!initAppGl()) return false;

    return true;
}


/// for timing, fps
#define WEN  (2048)
unsigned int frame = 1,
time_ms = 0;
int main(int argc, char* argv[])
{

    int ret = -1;
    /*
        prepare pad and piglet modules in advance,
        read /data/orbislink/orbislink_config.ini for debugnet
        (no nfs, audio init yet...)
    */
#if defined (USE_NFS)
    debugNetInit("10.0.0.2", 18198, 3);

    ret = initOrbisLinkAppVanillaGl();

    /* some nfs setup */
    ret = orbisNfsInit("nfs://10.0.0.2/Archive/PS4-work/orbisdev/orbisdev-samples/linux_es2goodness");

    log_info("orbisNfsInit return: %x", ret);
    sleep(1);
#else

    /* load custom .prx, resolve and call a function */
    int libcmi = sceKernelLoadStartModule("/app0/Media/jb.prx", 0, NULL, 0, 0, 0);
    ret = sceKernelDlsym(libcmi, "jailbreak_me", (void**)&jailbreak_me);
    if (ret >= 0)
    {
        log_info("jailbreak_me resolved from PRX");
        if ((ret = jailbreak_me() != 0)) goto error;
    }
    else
        goto error;

    
    if (initGL_for_the_store(false, -1) == INIT_FAILED) goto error;
    
#endif
    if(argv[1] != NULL){
       launch_query = argv[1];
       log_info("Launched With Request: \"%s\"", launch_query);
    }

    // init some libraries
    flag = initApp();

    // feedback
    log_info("Here we are!");

    /* init GLES2 stuff */
    // demo-font.c init
    es2init_text(ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);
    // ES_UI
    GLES2_scene_init(ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);

    /// reset timers
    time_ms = get_time_ms();
    gettimeofday(&t1, NULL);
    t2 = t1;

    /// enter main render loop
    while (flag)
    {
        //skip the first frame 
        //so theres no race between 
        //the countroller update func and the GLES UI
        if (frame != 1) updateController();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ret = glGetError();
            if (ret) {
                log_info("[ORBIS_GL] glClear failed: 0x%08X", ret);
                //goto err;
            }


            // ES_UI
           GLES2_scene_render(launch_query);

            if (!(frame % WEN))
            {
                //ItemzCore funcs
                double const_1000 = 1000.0f;
                uint64_t now = get_time_ms();
                uint64_t frame_time = now - time_ms;
                double time_to_draw = frame_time / const_1000;
                double framerate = const_1000 / time_to_draw;
                log_info("--------------------------------------------------");
                log_info("[Render] time: %lums FPS: %.3lf", frame_time, framerate);
                print_memory();
                //reset Frame so we dont overflow later in release
                frame = 1;
                log_info("---------------------------------------------------");
                time_ms = now;
            }
            frame++;

            if (1) // update for the GLES uniform time
            {
                gettimeofday(&t2, NULL);
                // calculate delta time
                dt = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;

                t1 = t2;
                // update total time
                u_t += dt;
            }
            orbisGlSwapBuffers();  /// flip frame


            dump_frame();
            //fallback in case a loading dialog is still running
            if (sceMsgDialogUpdateStatus() == ORBIS_COMMON_DIALOG_STATUS_RUNNING)  sceMsgDialogTerminate();
        
    }

error:
    sceKernelIccSetBuzzer(2);
    msgok(FATAL, "Returned %i", ret);

    // destructors
    ORBIS_RenderFillRects_fini();
    on_GLES2_Final();

    exit(EXIT_SUCCESS);
}