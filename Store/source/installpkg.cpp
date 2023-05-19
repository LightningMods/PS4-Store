#include "utils.h"
#include <stdarg.h>
#include <installpkg.h>
#include <stdbool.h>
#include <orbis/AppInstUtil.h>
#include "defines.h"
#include <pthread.h> 
#include <sfo.hpp>
#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>

#if __has_include("<byteswap.h>")
#include <byteswap.h>
#else
// Replacement function for byteswap.h's bswap_32
uint32_t bswap_32(uint32_t val) {
  val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0x00FF00FF );
  return (val << 16) | (val >> 16);
}
#endif

std::vector<uint8_t> readFile(std::string filename)
{
    // open the file:
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        log_info("Failed to open %s", filename.c_str());
        return std::vector<uint8_t>();
    }

    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // reserve capacity
    std::vector<uint8_t> vec;
    vec.reserve(fileSize);

    // read the data:
    vec.insert(vec.begin(),
               std::istream_iterator<uint8_t>(file),
               std::istream_iterator<uint8_t>());

    return vec;
}

extern std::vector<std::string> download_panel_text;
int PKG_ERROR(const char* name, int ret, dl_arg_t* args)
{
    if(!args)
        return ret;

    if(!args->is_threaded)
       msgok(WARNING,fmt::format("{}\nHEX: {}\nInt: {}\nFunction {}", getLangSTR(INSTALL_FAILED), ret, ret, name));

    log_error( "%s error: %x", name, ret);
    args->progress = 5.0;
    args->status = ret;

    return ret;
}

std::string normalize_version(const std::string &version) {
    std::istringstream iss(version);
    std::vector<int> parts;
    std::string part;
    while (std::getline(iss, part, '.')) {
        parts.push_back(std::stoi(part));
    }

    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            oss << '.';
        }
        oss << parts[i];
    }
    return oss.str();
}


bool GetInstalledVersion(std::string tid, std::string& version){
    
    std::string tmp = fmt::format("/system_data/priv/appmeta/{}/param.sfo", tid);
    if (!if_exists(tmp.c_str()))
         tmp = fmt::format("/system_data/priv/appmeta/external/{}/param.sfo", tid);

    //TODO make more effecient 
    std::vector<uint8_t> sfo_data = readFile(tmp);
    if(sfo_data.empty()){ 
       log_error("Could not read param.sfo");
       return false;
    }

    SfoReader sfo(sfo_data);
    version = sfo.GetValueFor<std::string>("VERSION");

    return true;
}
/* we use bgft heap menagement as init/fini as flatz already shown at 
 * https://github.com/flatz/ps4_remote_pkg_installer/blob/master/installer.c
 */

#define BGFT_HEAP_SIZE (1 * 1024 * 1024)

bool sceAppInst_done = false;
static bool   s_bgft_initialized = false;
static struct bgft_init_params  s_bgft_init_params;

void app_inst_util_fini(void) {
    if (!sceAppInst_done) {
        return;
    }

    sceAppInstUtilTerminate();
    sceAppInst_done = false;
}

bool bgft_init(void) {
    int ret;

    if (s_bgft_initialized) {
        goto done;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));
    {
        s_bgft_init_params.heapSize = BGFT_HEAP_SIZE;
        s_bgft_init_params.heap = (uint8_t*)malloc(s_bgft_init_params.heapSize);
        if (!s_bgft_init_params.heap) {
            log_debug( "No memory for BGFT heap.");
            goto err;
        }
        memset(s_bgft_init_params.heap, 0, s_bgft_init_params.heapSize);
    }

    ret = sceBgftServiceInit(&s_bgft_init_params);
    if (ret) {
        log_debug( "sceBgftInitialize failed: 0x%08X", ret);
        goto err_bgft_heap_free;
    }

    s_bgft_initialized = true;

done:
    return true;

err_bgft_heap_free:
    if (s_bgft_init_params.heap) {
        free(s_bgft_init_params.heap);
        s_bgft_init_params.heap = NULL;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));

err:
    s_bgft_initialized = false;

    return false;
}

void bgft_fini(void) {
    int ret;

    if (!s_bgft_initialized) {
        return;
    }

    ret = sceBgftServiceTerm();
    if (ret) {
        log_debug( "sceBgftServiceTerm failed: 0x%08X", ret);
    }

    if (s_bgft_init_params.heap) {
        free(s_bgft_init_params.heap);
        s_bgft_init_params.heap = NULL;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));

    s_bgft_initialized = false;
}

int sceAppInstUtilAppExists(const char* tid, int* flag);

bool app_inst_util_is_exists(const char* title_id, bool* exists) {
    int flag;

    if (!title_id) return false;

    if (!sceAppInst_done) {
        log_debug("Starting app_inst_util_init..");
        if (!app_inst_util_init()) {
            log_error("app_inst_util_init has failed...");
            return false;
        }
    }

    int ret = sceAppInstUtilAppExists(title_id, &flag);
    if (ret) {
        log_error("sceAppInstUtilAppExists failed: 0x%08X\n", ret);
        return false;
    }

    if (exists) *exists = flag;

    return true;
}

/* sample ends */
/* install package wrapper:
   init, (install), then clean AppInstUtil and BGFT
   for next install */
extern bool Download_icons;

bool pkg_is_patch(const char* src_dest) {

    // Read PKG header
    struct pkg_header hdr;
    static const uint8_t magic[] = { '\x7F', 'C', 'N', 'T' };
    // Open path
    int pkg = sceKernelOpen(src_dest, O_RDONLY, 0);
    if (pkg < 0) return false;

    sceKernelLseek(pkg, 0, SEEK_SET);
    sceKernelRead(pkg, &hdr, sizeof(struct pkg_header));

    sceKernelClose(pkg);

    if (memcmp(hdr.magic, magic, sizeof(magic)) != 0) {
        log_error("PKG Format is wrong");
        return false;
    }

    unsigned int flags = BE32(hdr.content_flags);

    if (flags & PKG_CONTENT_FLAGS_FIRST_PATCH || flags & PKG_CONTENT_FLAGS_SUBSEQUENT_PATCH || 
        flags & PKG_CONTENT_FLAGS_DELTA_PATCH || flags & PKG_CONTENT_FLAGS_CUMULATIVE_PATCH) 
    {
        return true;
    }

    return false;
}

void *install_prog(void* argument)
{
    struct install_args* args = (install_args*) argument;
    SceBgftTaskProgress progress_info;
    bool is_threaded = args->is_thread;

    log_info("Starting PKG Install");
    args->l->progress = 0.;
    args->l->status = INSTALLING_APP;
    // trigger refresh of Queue active count
    left_panel2->vbo_s = ASK_REFRESH;

    while (args->l->progress < 99)
    {
        memset(&progress_info, 0, sizeof(progress_info));

        int ret = sceBgftServiceDownloadGetProgress(args->task_id, &progress_info);
        if (ret) {
          // PKG_ERROR("sceBgftDownloadGetProgress", ret);
           args->l->progress = 0.;
           args->l->status = ret;
           goto clean;
        }

        if (progress_info.transferred > 0 && progress_info.error_result != 0) {
             args->l->progress = 0.;
             args->l->status = progress_info.error_result;
             goto clean;
        }

        args->l->progress = (uint32_t)(((float)progress_info.transferred / progress_info.length) * 100.f);


        if (progress_info.transferred % (4096 * 128) == 0)
             log_debug("%s, Install_Thread: reading data, %lub / %lub (%%%lf) ERR: %i", __FUNCTION__, progress_info.transferred, progress_info.length,  args->l->progress.load(), progress_info.error_result);

    }

    if (progress_info.error_result != 0) {
            args->l->progress = 0.;
            args->l->status = progress_info.error_result;
            log_error("Installation of %s has failed with code 0x%x", args->title_id.c_str(), progress_info.error_result);
    }
    else{
       args->l->status = COMPLETED;
    }
    
clean:
    log_info("Finalizing Memory...");
    log_info("Deleting PKG %s...", args->path.c_str());
    icon_panel->mtx.lock();
    download_panel->mtx.lock();
    if(!games.empty()){ // reset gaame update status to latest status
        games[args->l->g_idx].interruptible = false;
        if(games[args->l->g_idx].update_status.load() == UPDATE_FOUND){
         if(updates_counter.load() > 0){
            updates_counter--;
         }

        games[args->l->g_idx].update_status = NO_UPDATE;
       }
       download_panel->item_d[0].token_d[0].off =  download_panel_text[0] = getLangSTR(REINSTALL_APP);
    }
    download_panel->mtx.unlock();
    icon_panel->mtx.unlock();
    args->l->g_idx = -1;
   
    delete args;

    // trigger refresh of Queue active count
    left_panel2->vbo_s = ASK_REFRESH;
    log_info("Set Status: Ready");

    if(is_threaded)
       pthread_exit(NULL);

    return NULL;
}
bool app_inst_util_init(void) {
    int ret;

    if (sceAppInst_done) {
        goto done;
    }

    ret = sceAppInstUtilInitialize();
    if (ret) {
        log_debug( "sceAppInstUtilInitialize failed: 0x%08X", ret);
        goto err;
    }

    sceAppInst_done = true;

done:
    return true;

err:
    sceAppInst_done = false;

    return false;
}


uint32_t pkginstall(const char *fullpath, dl_arg_t* ta, bool Auto_install)
{
    char title_id[16];
    int  is_app, ret = -1;
    int  task_id = -1;
    char buffer[255];

    ta->status = INSTALLING_APP;
    ta->progress = 0.0f;

    if( if_exists(fullpath) )
    {
      if (sceAppInst_done) {
          log_info("Initializing AppInstUtil...");

          if (!app_inst_util_init())
              return PKG_ERROR("AppInstUtil", ret, ta);
      }
        
        log_info("Initializing BGFT...");
        if (!bgft_init()) {
            return PKG_ERROR("BGFT_initialization", ret, ta);
        }

        ret = sceAppInstUtilGetTitleIdFromPkg(fullpath, title_id, &is_app);
        if (ret) 
            return PKG_ERROR("sceAppInstUtilGetTitleIdFromPkg", ret, ta);


        snprintf(buffer, 254, "%s via Store", title_id);
        log_info( "%s", buffer);
        struct bgft_download_param_ex download_params;
        memset(&download_params, 0, sizeof(download_params));
        download_params.param.entitlement_type = 5;
        download_params.param.id = "";
        download_params.param.content_url = fullpath;
        download_params.param.content_name = buffer;
        download_params.param.icon_path = "/update/fakepic.png";
        download_params.param.playgo_scenario_id = "0";
        download_params.param.option = BGFT_TASK_OPTION_INVISIBLE;

        download_params.slot = 0;

    retry:
        log_info("%s 1", __FUNCTION__);
        ret = sceBgftServiceIntDownloadRegisterTaskByStorageEx(&download_params, &task_id);
        if(ret == 0x80990088 || ret == 0x80990015)
        {
            ret = sceAppInstUtilAppUnInstall(&title_id[0]);
            if(ret != 0)
               return PKG_ERROR("sceAppInstUtilAppUnInstall", ret, ta);

            goto retry;

        }
        else if(ret) 
            return PKG_ERROR("sceBgftServiceIntDownloadRegisterTaskByStorageEx", ret, ta);
        

        log_info("Task ID(s): 0x%08X", task_id);

        ret = sceBgftServiceDownloadStartTask(task_id);
        if(ret) 
            return PKG_ERROR("sceBgftDownloadStartTask", ret, ta);
    }
    else//bgft_download_get_task_progress
        return PKG_ERROR("no file at", ret, ta);


    struct install_args* args = new install_args;
    args->title_id = title_id;
    args->task_id = task_id;
    args->l = ta;
    args->path = fullpath;
    args->is_thread = !Auto_install;
    args->delete_pkg = true; //STORE DOWNLOADS ONLY

     //Both Auto install and Show install progress will set status to INSTALL_APP
    if (Auto_install){ //Auto Install (is being called by the download thread)
        install_prog((void*)args);
    }
    else if (set.Legacy_Install.load()){ //is "Show Install Progess" enabled, if so make a thread
        pthread_t thread = 0;
        ret = pthread_create(&thread, NULL, install_prog, (void*)args);
        log_debug("pthread_create for %x, ret:%d", task_id, ret);
    }
    else { // Default, Auto install and Show install progress are disabled
          //  so we let the PS4 INSTALL it in the Background, 
        ret = sceBgftServiceDownloadStartTask(args->task_id);
        if (ret) {
            return PKG_ERROR("sceBgftServiceDownloadStartTask", ret, ta);
        }
        else { // too bad we cant delete the file with this option
            if(icon_panel && !icon_panel->item_d[args->l->g_idx].token_d[ID].off.empty()){
               icon_panel->item_d[ta->g_idx].interruptible = false;
               icon_panel->item_d[ta->g_idx].update_status = NO_UPDATE;
               
               download_panel->item_d[0].token_d[0].off =  download_panel_text[0] = getLangSTR(REINSTALL_APP);
            }
            ta->g_idx = -1;
            ta->status = READY;
            layout_refresh_VBOs();

            log_info("package successfully started in the background");
        }

    }

    log_info( "%s(%s) done.", __FUNCTION__, fullpath);
    ta->dst.clear();

    return 0;
}