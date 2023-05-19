#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "ini.h"
#include <md5.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include "log.h"
#include "defines.h"
#include <orbis/SystemService.h>
#include <orbis/libkernel.h>

#include <dirent.h>
#include "GLES2_common.h"
#include "dialog.h"
#include <unordered_map>
#include <string>
#include <iostream>
//#include <sys/sysctl.h>

StoreOptions set;

extern std::unordered_map<std::string, std::string> stropts;
extern std::shared_ptr<layout_t> download_panel;
extern std::vector<std::string> download_panel_text;
//void* __stack_chk_guard = (void*)0xdeadbeef;

void __stack_chk_fail(void)
{
    log_info("Stack smashing detected.");
    msgok(FATAL, "Stack Smashing has been Detected");
}

extern bool sceAppInst_done;

bool unsafe_source = false;
int Lastlogcheck = 0;
static const char     *sizes[] = { "EiB", "PiB", "TiB", "GiB", "MiB", "KiB", "B" };
static const uint64_t  exbibytes = 1024ULL * 1024ULL * 1024ULL *
1024ULL * 1024ULL * 1024ULL;

size_t _orbisFile_lastopenFile_size;
// --------------------------------------------------------- buf_from_file ---
unsigned char *orbisFileGetFileContent( const char *filename )
{
    _orbisFile_lastopenFile_size = -1;

    FILE *file = fopen( filename, "rb" );
    if( !file )
        { printf( "Unable to open file \"%s\".\n", filename ); return 0; }

    fseek( file, 0, SEEK_END );
    size_t size = ftell( file );
    fseek(file, 0, SEEK_SET );

    unsigned char *buffer = (unsigned char *) malloc( (size +1) * sizeof(char) );
    fread( buffer, sizeof(char), size, file );
    buffer[size] = 0;
    _orbisFile_lastopenFile_size = size;
    fclose( file );

    return buffer;
}

int copyFile(const char* sourcefile, const char* destfile)
{
    int src = sceKernelOpen(sourcefile, 0x0000, 0);
    if (src > 0)
    {
        
        int out = sceKernelOpen(destfile, 0x0001 | 0x0200 | 0x0400, 0777);
        if (out > 0)
        {
            size_t bytes;
            char* buffer = (char*)malloc(65536);
            if (buffer != NULL)
            {
                while (0 < (bytes = sceKernelRead(src, buffer, 65536)))
                    sceKernelWrite(out, buffer, bytes);
                free(buffer);
            }
        }
        else
        {
            log_error("Could copy file: %s Reason: %x", destfile, out);
            sceKernelClose(src);
            return -1;
        }

        sceKernelClose(src);
        sceKernelClose(out);
        return 0;
    }
    else
    {
        log_error("Could copy file: %s Reason: %x", destfile, src);
        return -1;
    }
}

int64_t sys_dynlib_load_prx(char* prxPath, int* moduleID)
{
    return (int64_t)syscall4(594, prxPath, 0, moduleID, 0);
}

int64_t sys_dynlib_unload_prx(int64_t prxID)
{
    return (int64_t)syscall1(595, (void*)prxID);
}

void* prx_func_loader(const char* prx_path, const char* symbol) {

    void* addrp = NULL;
    log_info("Loading %s", prx_path);
     
    int libcmi = sceKernelLoadStartModule(prx_path, 0, NULL, 0, 0, 0);
    if(libcmi < 0){
        log_error("Error loading prx: 0x%X", libcmi);
        return addrp;
    }
    else
        log_debug("%s loaded successfully", prx_path);

    if(sceKernelDlsym(libcmi, symbol, &addrp) < 0){
        log_error("Symbol %s NOT Found", symbol);
    }
    else
        log_debug("Function %s | addr %p loaded successfully", symbol, addrp);

    return addrp;
}


int64_t sys_dynlib_dlsym(int64_t moduleHandle, const char* functionName, void* destFuncOffset)
{
    return (int64_t)syscall3(591, (void*)moduleHandle, (void*)functionName, destFuncOffset);
}

void ProgSetMessagewText(uint32_t prog, std::string fmt)
{
      
        int status = sceMsgDialogUpdateStatus();
		if (ORBIS_COMMON_DIALOG_STATUS_RUNNING == status) {
		   sceMsgDialogProgressBarSetValue(0, prog);
           sceMsgDialogProgressBarSetMsg(0, fmt.c_str());
	    }
}

/// timing
unsigned int get_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

bool touch_file(const char* destfile)
{
    int fd = open(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd > 0) {
        close(fd);
        return true; 
    }
    else
        return false;
}

#define SCE_KERNEL_EVF_WAITMODE_OR 0x02
unsigned char _bittest64(uint64_t *Base, uint64_t Offset)
{
  int old = 0;
  __asm__ __volatile__("btq %2,%1\n\tsbbl %0,%0 "
    :"=r" (old),"=m" ((*(volatile long long *) Base))
    :"Ir" (Offset));
  return (old != 0);
}


//extern "C" void sceKernelPollEventFlag(void* ef, uint32_t bits, uint32_t mode, uint64_t* res);
//extern "C" int sceKernelOpenEventFlag(void** ef, const char* name);
//extern "C" int sceKernelCloseEventFlag(void* ef);Fapi.php?


unsigned int sceShellCoreUtilIsUsbMassStorageMounted_func(unsigned int usb_index)
{
  uint64_t res;
  OrbisKernelEventFlag ef; 

  if ( (unsigned int)sceKernelOpenEventFlag(&ef, "SceAutoMountUsbMass") )
    return 0;
  sceKernelPollEventFlag(ef, 0xFFFFFFF, SCE_KERNEL_EVF_WAITMODE_OR, &res);
  sceKernelCloseEventFlag(ef);

  return _bittest64(&res, usb_index);
}



unsigned int usbpath()
{
    unsigned int usb_index = -1;

    for (int i = 0; i < 8; i++){
        if (sceShellCoreUtilIsUsbMassStorageMounted_func((unsigned int)i)){
              usb_index = i;
              //log_info("[UTIL] USB %i is mounted, SceAutoMountUsbMass: %i", i, usb_number);
              break;
        }
    }

    return usb_index;
}


bool is_apputil_init()
{
    int ret = sceAppInstUtilInitialize();
    if (ret) {
        log_error( "sceAppInstUtilInitialize failed: 0x%08X", ret);
        return false;
    }
    else
        sceAppInst_done = true;

    return true;
}

bool Fnt_setting_enabled = false;

static int print_ini_info(void* user, const char* section, const char* name,
    const char* value, int idc_2)
{
    static std::string prev_section;
        if (section != prev_section) {
       // std::cout << (prev_section.empty() ? "" : "\n") << "[" << section << "]";
        prev_section = section;
    }
   // std::cout << name << " = " << value << std::endl;
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

        if (MATCH("Settings", "CDN")) {
            set.opt[CDN_URL] = value;
        }
        else if (MATCH("Settings", "TTF_Font")) {
            set.opt[FNT_PATH] = value;
            if ( set.opt[FNT_PATH].find("SCE-PS3-RD-R-LATIN.TTF") == std::string::npos)
                Fnt_setting_enabled = true;
        }
        else if (MATCH("Settings", "temppath")) {
            set.opt[TMP_PATH] = value;
        }
        else if (MATCH("Settings", "auto_install")) {
            set.auto_install = std::atoi(value);
        }
        else if (MATCH("Settings", "Legacy_Install")) {
            set.Legacy_Install = std::atoi(value);
        }
        else if (MATCH("Settings", "auto_load_cache")) {
            set.auto_load_cache = std::atoi(value);
        }
#if BETA==1
        else if (MATCH("Settings", "BETA_KEY")) {
            set.opt[BETA_KEY] = value;
        }
#endif
    

    return 1;
}

bool is_source_unsafe(const std::string& input) {
    const std::string main_api = "api.pkg-zone.com";
    const std::string beta_api = "pkg-zone.com/storage/store_beta";
#if BETA==1
    if(input.find(beta_api) != std::string::npos){
        set.opt[CDN_URL] = "https://api.pkg-zone.com";
        return false;
    }
#endif
    return input.find(main_api) == std::string::npos;
}


bool LoadOptions()
{
    std::string buf;
    set.opt.clear();
    set.opt.resize(NUM_OF_STRINGS);

    bool no_error = true;
    int error = 1;
    unsigned int usb_num = usbpath();

    set.opt[CDN_URL] = "https://api.pkg-zone.com";
    set.auto_install = true;
    set.Legacy_Install = false;
    set.auto_load_cache = true;
    unsafe_source = false;
    set.opt[TMP_PATH] =  "/user/app/NPXS39041/downloads";
    set.opt[FNT_PATH] = "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF";

    /* Initialize INI structure */
    buf = fmt::format("/mnt/usb{}/settings.ini", usb_num);

    if (usb_num != -1)
    {
        if (!if_exists(buf.c_str()))
        {
            log_warn( "No INI on USB");
            if (if_exists("/user/app/NPXS39041/settings.ini")) {
                set.opt[INI_PATH] = "/user/app/NPXS39041/settings.ini";
                error = ini_parse("/user/app/NPXS39041/settings.ini", print_ini_info, NULL);
                if (error) log_error("Bad config file (first error on line %d)!\n", error);
            }

        } else {
            error = ini_parse(buf.c_str(), print_ini_info, NULL);
            if (error) log_error("Bad config file (first error on line %d)!\n", error);
            log_info( "Loading ini from USB");
            set.opt[INI_PATH] = fmt::format("/mnt/usb{}/settings.ini", usb_num);
        }
    }
    else if (!if_exists("/user/app/NPXS39041/settings.ini"))
    {
        log_error("CANT FIND INI"); no_error = false;
    } else {
        error = ini_parse("/user/app/NPXS39041/settings.ini", print_ini_info, NULL);
        if (error) log_error("Bad config file (first error on line %d)!\n", error);
        log_info( "Loading ini from APP DIR");
        set.opt[INI_PATH] = "/user/app/NPXS39041/settings.ini";
    }
    
    unsafe_source = is_source_unsafe(set.opt[CDN_URL]);

    uint32_t lang = PS4GetLang();
    if (error) 
        log_error("ERROR reading INI setting default vals");

#if TEST_INI_LANGS
    for (int i = 0; i < 29; i++)
    {
        if (!LoadLangs(i))
            log_debug("Failed to Load %s", Language_GetName(i));
        else
            log_debug("Successfully Loaded %s", Language_GetName(i));
        
    }
#endif
    //fallback lang
#ifdef OVERRIDE_LANG
    lang = OVERRIDE_LANG;
#else
    set.lang = lang;
#endif

    if (!LoadLangs(lang)) {
        if (!LoadLangs(0x01)) {
            log_debug("This PKG has no lang files... trying embedded");

            if(!load_embdded_eng())
               msgok(FATAL, "Failed to load Backup Lang...\nThe App is unable to continue");
            else
                log_debug("Loaded embdded ini lang file");
        }
        else
            log_debug("Loaded the backup, %s failed to load", Language_GetName(lang).c_str());
    }

    if (!Fnt_setting_enabled) {

        switch (lang) {
        case 0: //jAPN IS GREAT
            //strcpy(set.opt[FNT_PATH], "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansJP-Regular.ttf");
            set.opt[FNT_PATH] = "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansJP-Regular.ttf";
            break;
        case 9:///THIS IS FOR JOON, IF HE COMES BACK
            set.opt[FNT_PATH] = "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansKR-Regular.ttf";
            break;
        case 21:
        case 23:
        case 24:
        case 25:
        case 26:
            //strcpy(set.opt[FNT_PATH], "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/HelveticaWorld-Multi.ttf");
            set.opt[FNT_PATH] = "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/HelveticaWorld-Multi.ttf";
            break;
            //NO USERS HAS TRANSLATED THE FOLLOWING LANGS, SO I DELETED THEM OUT THE PKG TO SAVE SPACE
            /*
            case 10:
                strcpy(set.opt[FNT_PATH],"/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansTC-Regular.ttf");
                break;
            case 11:
                strcpy(set.opt[FNT_PATH],"/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansSC-Regular.ttf");
                break;
            case 27:
                strcpy(set.opt[FNT_PATH],"/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansThai-Regular.ttf");
                break;*/
        default:
            break;
        }
    }

    log_info( "set.opt[INI_PATH]: %s", set.opt[INI_PATH].c_str());
    log_info( "set.opt[USB_PATH]: %s", set.opt[USB_PATH].c_str());
    log_info( "set.opt[FNT_PATH]: %s", set.opt[FNT_PATH].c_str());
    log_info( "set.opt[TMP_PATH]: %s", set.opt[TMP_PATH].c_str());
    log_info( "set.opt[CDN_URL ]: %s", set.opt[CDN_URL].c_str());
    log_info( "set.auto_install   : %i", set.auto_install.load());
    log_info( "set.Legacy_Install.: %s", set.Legacy_Install.load() ? "ON" : "OFF");
    log_info( "set.Lang         : %s : %i", Language_GetName(lang).c_str(), set.lang);
    log_info( "set.auto_load_cache : %i", set.auto_load_cache);
    log_info( "Unsafe Sources    : %i", unsafe_source);

//strcpy(set.opt[CDN_URL], "https://api.pkg-zone.com");
   // strcpy(set.opt[CDN_URL], "https://api.pkg-zone.com");

    return no_error;
}
#include <iostream>
#include <fstream>
#include <sstream>

bool SaveOptions()
{  
    bool no_error = true;
    #if BETA==1
    set.opt[CDN_URL] = "https://api.pkg-zone.com/storage/store_beta";
    #endif

    std::ostringstream oss;
    oss << "[Settings]\nCDN=" << set.opt[CDN_URL]
        << "\nSecure_Boot=1\ntemppath=" << set.opt[TMP_PATH]
        #if BETA==1
        << "\nBETA_KEY=" << set.opt[BETA_KEY]
        #endif
        << "\nTTF_Font=" << set.opt[FNT_PATH]
        << "\nauto_install=" << set.auto_install.load()
        << "\nLegacy_install_prog=" << set.Legacy_Install.load()
        << "\nauto_load_cache=" << set.auto_load_cache;
    
    std::string buff = oss.str();

    int fd = open(set.opt[INI_PATH].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd >= 0)
    {
        write(fd, buff.c_str(), buff.size());
        close(fd);
    }
    else
        no_error = false;

    log_info("set->opt[INI_PATH]: %s", set.opt[INI_PATH].c_str());
    log_info("set->opt[USB_PATH]: %s", set.opt[USB_PATH].c_str());
    log_info("set->opt[TMP_PATH]: %s", set.opt[TMP_PATH].c_str());
    log_info("set->opt[FNT_PATH]: %.20s...", set.opt[FNT_PATH].c_str());
    log_info("set->opt[CDN_URL ]: %s", set.opt[CDN_URL ].c_str());
    log_info( "set.auto_load_cache : %i", set.auto_load_cache);
    log_info("set->auto_install   : %i", set.auto_install.load());
    log_info("set->Legacy_Install : %s", set.Legacy_Install.load() ? "ON" : "OFF");

    /* Load values */
    chmod(set.opt[INI_PATH].c_str(), 0777);

    return no_error;
}


static uint32_t sdkVersion = -1;
extern "C" int	sysctlbyname(const char *, void *, size_t *, const void *, size_t);
uint32_t SysctlByName_get_sdk_version(void)
{
    //cache the FW Version
    if (0 < sdkVersion) {
        size_t   len = 4;
        // sysKernelGetLowerLimitUpdVersion == machdep.lower_limit_upd_version
        // rewrite of sysKernelGetLowerLimitUpdVersion
        sysctlbyname("machdep.lower_limit_upd_version", &sdkVersion, &len, NULL, 0);
    }

    // FW Returned is in HEX
    return sdkVersion;
}
std::string calculateSize(uint64_t size)
{
    std::string result;
    uint64_t  multiplier = exbibytes;
    int i;

    for (i = 0; i < DIM(sizes); i++, multiplier /= 1024)
    {
        if (size < multiplier)
            continue;
        if (size % multiplier == 0)
            result = fmt::format("{0:} {1:}", size / multiplier, sizes[i]);
        else
            result = fmt::format("{0:.1f} {1:}", (float)(size / multiplier), sizes[i]);

        return result;
    }
    result = "0";
    return result;
}

std::string checkedsize;

uint64_t CalcAppsize(std::string path)
{
    FILE *fp = NULL;
    uint64_t off;
    checkedsize.clear();

    fp = fopen(path.c_str(), "r");
    if (fp == NULL) return 0;
            
    if (fseek(fp, 0, SEEK_END) == -1) goto cleanup;
            
    off = ftell(fp);
    if (off == (long)-1) goto cleanup;

    if (fclose(fp) != 0) goto cleanup;
    
    if(off) { checkedsize = calculateSize(off); return off; }
    else    {  return 0; }

cleanup:
    if (fp != NULL)
        fclose(fp);

    return 0;

}
//////////////////////////////////////////////////////

void die(const char* message)
{
    log_fatal( message);
    sceSystemServiceLoadExec("INVALID", 0);
}

void msgok(enum MSG_DIALOG level, std::string format)
{
    std::string buf;
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    sceMsgDialogTerminate();


    switch (level)
    {
    case NORMAL:
        log_info(format.c_str());
        buf = format;
        break;
    case FATAL:
        log_fatal(format.c_str());
        buf = fmt::format("{}\n {}\n\n {}", getLangSTR(FATAL_ERROR), format, getLangSTR(PRESS_OK_CLOSE));

        break;
    case WARNING:
        log_warn(format.c_str());
        buf = fmt::format("{}\n\n {}", getLangSTR(WARNING2), format);
        break;
    }


    sceMsgDialogInitialize();
    OrbisMsgDialogParam param;
    OrbisMsgDialogParamInitialize(&param);
    param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

    OrbisMsgDialogUserMessageParam userMsgParam;
    memset(&userMsgParam, 0, sizeof(userMsgParam));
    userMsgParam.msg        = buf.c_str();
    userMsgParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_OK;
    param.userMsgParam      = &userMsgParam;

    if (sceMsgDialogOpen(&param) < 0)
           log_fatal( "MsgD failed");
    
        
    

    OrbisCommonDialogStatus stat;

    while (1)
    {
        stat = sceMsgDialogUpdateStatus();
        if (stat == ORBIS_COMMON_DIALOG_STATUS_FINISHED)
        {
            OrbisMsgDialogResult result;
            memset(&result, 0, sizeof(result));

            if (0 > sceMsgDialogGetResult(&result))
                         log_fatal( "MsgD failed"); 

            sceMsgDialogTerminate();
            break;
        }
    }


    switch (level)
    {
    case FATAL:
        die(format.c_str());
        break;

    default:
        break;
    }

    return;
}

void loadmsg(std::string format)
{
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    sceMsgDialogInitialize();

    OrbisMsgDialogButtonsParam buttonsParam;
    OrbisMsgDialogUserMessageParam messageParam;
    OrbisMsgDialogParam dialogParam;

    OrbisMsgDialogParamInitialize(&dialogParam);

    memset(&buttonsParam, 0x00, sizeof(buttonsParam));
    memset(&messageParam, 0x00, sizeof(messageParam));

    dialogParam.userMsgParam = &messageParam;
    dialogParam.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

    messageParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT;
    messageParam.msg        = format.c_str();

    sceMsgDialogOpen(&dialogParam);

    OrbisCommonDialogStatus stat;

    while (1)
    {
        stat = sceMsgDialogUpdateStatus();
        if (stat == ORBIS_COMMON_DIALOG_STATUS_RUNNING) 
             break;
        
    }

    return;
}

static int convert_to_utf16(const char* utf8, uint16_t* utf16, uint32_t available)
{
    int count = 0;
    while (*utf8)
    {
        uint8_t ch = (uint8_t)*utf8++;
        uint32_t code;
        uint32_t extra;

        if (ch < 0x80)
        {
            code = ch;
            extra = 0;
        }
        else if ((ch & 0xe0) == 0xc0)
        {
            code = ch & 31;
            extra = 1;
        }
        else if ((ch & 0xf0) == 0xe0)
        {
            code = ch & 15;
            extra = 2;
        }
        else
        {
            // TODO: this assumes there won't be invalid utf8 codepoints
            code = ch & 7;
            extra = 3;
        }

        for (uint32_t i=0; i<extra; i++)
        {
            uint8_t next = (uint8_t)*utf8++;
            if (next == 0 || (next & 0xc0) != 0x80)
            {
                goto utf16_end;
            }
            code = (code << 6) | (next & 0x3f);
        }

        if (code < 0xd800 || code >= 0xe000)
        {
            if (available < 1) goto utf16_end;
            utf16[count++] = (uint16_t)code;
            available--;
        }
        else // surrogate pair
        {
            if (available < 2) goto utf16_end;
            code -= 0x10000;
            utf16[count++] = 0xd800 | (code >> 10);
            utf16[count++] = 0xdc00 | (code & 0x3ff);
            available -= 2;
        }
    }

utf16_end:
    utf16[count]=0;
    return count;
}

static int convert_from_utf16(const uint16_t* utf16, char* utf8, uint32_t size)
{
    int count = 0;
    while (*utf16)
    {
        uint32_t code;
        uint16_t ch = *utf16++;
        if (ch < 0xd800 || ch >= 0xe000)
        {
            code = ch;
        }
        else // surrogate pair
        {
            uint16_t ch2 = *utf16++;
            if (ch < 0xdc00 || ch > 0xe000 || ch2 < 0xd800 || ch2 > 0xdc00)
            {
                goto utf8_end;
            }
            code = 0x10000 + ((ch & 0x03FF) << 10) + (ch2 & 0x03FF);
        }

        if (code < 0x80)
        {
            if (size < 1) goto utf8_end;
            utf8[count++] = (char)code;
            size--;
        }
        else if (code < 0x800)
        {
            if (size < 2) goto utf8_end;
            utf8[count++] = (char)(0xc0 | (code >> 6));
            utf8[count++] = (char)(0x80 | (code & 0x3f));
            size -= 2;
        }
        else if (code < 0x10000)
        {
            if (size < 3) goto utf8_end;
            utf8[count++] = (char)(0xe0 | (code >> 12));
            utf8[count++] = (char)(0x80 | ((code >> 6) & 0x3f));
            utf8[count++] = (char)(0x80 | (code & 0x3f));
            size -= 3;
        }
        else
        {
            if (size < 4) goto utf8_end;
            utf8[count++] = (char)(0xf0 | (code >> 18));
            utf8[count++] = (char)(0x80 | ((code >> 12) & 0x3f));
            utf8[count++] = (char)(0x80 | ((code >> 6) & 0x3f));
            utf8[count++] = (char)(0x80 | (code & 0x3f));
            size -= 4;
        }
    }

utf8_end:
    utf8[count]=0;
    return count;
}

#define SCE_IME_DIALOG_MAX_TEXT_LENGTH 512
static uint16_t inputTextBuffer[SCE_IME_DIALOG_MAX_TEXT_LENGTH+1];
static uint16_t input_ime_title[SCE_IME_DIALOG_MAX_TEXT_LENGTH];

bool Keyboard(const char* Title, const char* initialTextBuffer, char* out_buffer, bool is_url)
{
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG);

    log_info("Keyboard(%s, %s, %p, %s)", Title, initialTextBuffer, out_buffer, is_url ? "true" : "false");

    if (initialTextBuffer && strlen(initialTextBuffer) > 254) return false;

    memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
    memset(input_ime_title, 0, sizeof(input_ime_title));


    if (initialTextBuffer) {
       strncpy(out_buffer, initialTextBuffer, 254);
       convert_to_utf16(initialTextBuffer, inputTextBuffer, sizeof(inputTextBuffer));
    }
    if(Title)
       convert_to_utf16(Title, input_ime_title, sizeof(input_ime_title));

    OrbisImeDialogParam param;
    memset(&param, 0, sizeof(OrbisImeDialogParam));
   // msgok(NORMAL, "title: %ls inputTextBuffer: %ls", &title[0], &inputTextBuffer[0]);

    param.maxTextLength = 254;
    param.inputTextBuffer = (wchar_t*) inputTextBuffer;
    param.title = (wchar_t*) input_ime_title;
    param.userId = 0xFE;
    param.type = is_url ? ORBIS_IME_TYPE_URL : ORBIS_IME_TYPE_BASIC_LATIN;
    param.enterLabel = ORBIS_IME_ENTER_LABEL_DEFAULT;

    sceImeDialogInit(&param, NULL);

    int status;
    while (1)
    {
        status = sceImeDialogGetStatus();

        if (status == ORBIS_IME_DIALOG_STATUS_FINISHED)
        {
            OrbisImeDialogResult result;
            memset(&result, 0, sizeof(OrbisImeDialogResult));
            sceImeDialogGetResult(&result);
            log_info("status %i, endstatus %i ", status, result.endstatus);

            if (result.endstatus == ORBIS_IME_DIALOG_END_STATUS_USER_CANCELED){
                log_info("User Cancelled this Keyboard Session");
            }
            else if (result.endstatus == ORBIS_IME_DIALOG_END_STATUS_OK)
            {
                log_info("User Entered text.. Entering into buffer");
                //PS4_wcstombs(out_buffer, &inputTextBuffer[0], sizeof(inputTextBuffer));
                convert_from_utf16(inputTextBuffer, out_buffer, sizeof(inputTextBuffer));
                sceImeDialogTerm();
                return true;
            }
            goto Finished;
        }
        else if (status == ORBIS_IME_DIALOG_STATUS_NONE) goto Finished;
    }

Finished:
    sceImeDialogTerm();
    return false;
}
//

int check_download_counter(std::string title_id)
{
    std::string http_req;
    std::string results;
    int count;

    http_req = set.opt[CDN_URL] + "/download.php?tid=" + title_id + "&check=true";

    results = check_from_url(http_req, DL_COUNTER, false);
    if (!results.empty())
        count = std::stoi(results);
    else
        count = -1;

    log_debug("%s Number_of_DL:%d", __FUNCTION__, count);
    return count;
}


// 
#include <memory>
#include <string>
#include <vector>

int check_store_from_url(const std::string cdn, CHECK_OPTS opt)
{
    std::string http_req;
    std::string result;

    switch (opt)
    {
        case MD5_HASH:
        {
            if (!if_exists(SQL_STORE_DB))
                return false;

            return (result = check_from_url(cdn + "/api.php?db_check_hash=true", MD5_HASH, false), !result.empty() && MD5_hash_compare(SQL_STORE_DB, result.c_str()));
        }

        break;

        case COUNT:
        {
            int pages = 0;
            int count = SQL_Get_Count();
            if (count > 0)
            {
                pages = (count + STORE_PAGE_SIZE - 1) / STORE_PAGE_SIZE;
                if (pages > STORE_MAX_LIMIT_PAGES)
                    msgok(FATAL, getLangSTR(EXCEED_LIMITS));

                log_debug("counted pages: %d", pages);
            }
            else
                msgok(FATAL, getLangSTR(ZERO_ITEMS));

            return pages;
        }
        break;

#if BETA == 1 //http://api.staging.pkg-zone.com/key/beta/check?key=KEY

        case BETA_CHECK:
        {
            std::string url = "https://api.pkg-zone.com/key/beta/check?key=" + set.opt[BETA_KEY];
            log_info("FULL URL: %s", url.c_str());
            return std::stoi(check_from_url(url, BETA_CHECK, false));
        }
#endif

        default:
            break;
    }
    return false;
}


bool rmtree(const char path[]) {

    struct dirent* de;
    char fname[300];
    DIR* dr = opendir(path);
    if (dr == NULL)
    {
        log_debug("No file or directory found: %s", path);
        return false;
    }
    while ((de = readdir(dr)) != NULL)
    {
        int ret = -1;
        struct stat statbuf;
        snprintf(fname,299, "%s/%s", path, de->d_name);
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        if (!stat(fname, &statbuf))
        {
            if (S_ISDIR(statbuf.st_mode))
            {
                log_debug("removing dir: %s Err: %d", fname, ret = unlinkat(dirfd(dr), fname, AT_REMOVEDIR));
                if (ret != 0)
                {
                    rmtree(fname);
                    log_debug("Err: %d", ret = unlinkat(dirfd(dr), fname, AT_REMOVEDIR));
                }
            }
            else
            {
                log_debug("Removing file: %s, Err: %d", fname, unlink(fname));
            }
        }
    }
    closedir(dr);

    return true;
}

extern "C" int malloc_stats_fast(OrbisMallocManagedSize *ManagedSize);

void debugNetPrintf(int level, const char *format, ...){
    log_info("debugNetPrintf: %s", format);
}

void print_memory()
{
   size_t sz = 0;
   OrbisMallocManagedSize ManagedSize;
   SCE_LIBC_INIT_MALLOC_MANAGED_SIZE(ManagedSize);
   malloc_stats_fast(&ManagedSize);
   sceKernelAvailableFlexibleMemorySize(&sz);
   double dfp_fmem = (1. - (double)sz / (double)0x8000000) * 100.;
/*	unsigned short sz;
	unsigned short ver;
	unsigned int reserv;
	size_t maxSysSz;
	size_t curSysSz;
	size_t maxUseSz;
	size_t curUseSz;*/
   log_info("[MEM] CurrentSystemSize: %s, CurrentInUseSize: %s", calculateSize(ManagedSize.curSysSz).c_str(), calculateSize(ManagedSize.curUseSz).c_str());
   log_info("[MEM] MaxSystemSize: %s, MaxInUseSize: %s", calculateSize(ManagedSize.maxSysSz).c_str(), calculateSize(ManagedSize.maxUseSz).c_str());
   log_info("[VRAM] VRAM_Available: %s, AmountUsed: %.2f%%",calculateSize(sz).c_str(), dfp_fmem);

}

bool MD5_hash_compare(const char* file1, const char* hash)
{
    unsigned char c[MD5_HASH_LENGTH];
    FILE* f1 = fopen(file1, "rb");
    if (!f1) {
        log_error("Could not open file: %s", file1);
        return false;
    }
    MD5_CTX mdContext;

    int bytes = 0;
    unsigned char data[1024];

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, f1)) != 0)
        MD5_Update(&mdContext, data, bytes);
    MD5_Final(c, &mdContext);

    char md5_string[33] = {0};

    for(int i = 0; i < 16; ++i) {
        snprintf(&md5_string[i*2], 32, "%02x", (unsigned int)c[i]);
    }
    log_info( "FILE HASH: %s", md5_string);
    md5_string[32] = 0;
    fclose(f1);

    if (strcmp(md5_string, hash) != 0) {
        return false;
    }

    log_info( "Input HASH: %s", hash);

    return true;
}

int progstart(std::string format)
{

    
    int ret = 0;

    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
    sceMsgDialogInitialize();

    OrbisMsgDialogParam dialogParam;
    OrbisMsgDialogParamInitialize(&dialogParam);
    dialogParam.mode = ORBIS_MSG_DIALOG_MODE_PROGRESS_BAR;

    OrbisMsgDialogProgressBarParam  progBarParam;
    memset(&progBarParam, 0, sizeof(OrbisMsgDialogProgressBarParam));

    dialogParam.progBarParam = &progBarParam;
    dialogParam.progBarParam->barType = ORBIS_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE;
    dialogParam.progBarParam->msg = format.c_str();

    sceMsgDialogOpen(&dialogParam);

    return ret;
}


bool init_curl(){
   
    CURLcode ress = curl_global_init(CURL_GLOBAL_SSL);
    if(ress < 0){
        log_info("curl_global_init failed: %s", curl_easy_strerror(ress));
        return false;
    }
    else{
        log_info("curl_version | %s", curl_version());
        return true;
    }

    return false;
}

// Finds the param.sfo's offset inside a PS4 PKG file
// https://github.com/hippie68/sfo/blob/main/sfo.c#L732
static int read_sfo_from_pkg(const char* pkg_path, uint8_t** sfo_buffer, size_t* sfo_size) {
	FILE* file;
	uint32_t pkg_table_offset;
	uint32_t pkg_file_count;
	pkg_table_entry_t pkg_table_entry;

	file = fopen(pkg_path, "rb");
	if (!file)
		return (-1);

  	fread(&pkg_file_count, sizeof(uint32_t), 1, file);
	if (pkg_file_count != PKG_MAGIC)
	{
        log_error("read_sfo_from_pkg: Invalid PKG file");
		fclose(file);
		return (-1);
	}

	fseek(file, 0x00C, SEEK_SET);
	fread(&pkg_file_count, sizeof(uint32_t), 1, file);
	fseek(file, 0x018, SEEK_SET);
	fread(&pkg_table_offset, sizeof(uint32_t), 1, file);

	pkg_file_count = ES32(pkg_file_count);
	pkg_table_offset = ES32(pkg_table_offset);
	fseek(file, pkg_table_offset, SEEK_SET);

	for (int i = 0; i < pkg_file_count; i++) {
		fread(&pkg_table_entry, sizeof (pkg_table_entry_t), 1, file);

		// param.sfo ID
		if (pkg_table_entry.id == 1048576) {
			*sfo_size = ES32(pkg_table_entry.size);
			*sfo_buffer = (uint8_t*)malloc(*sfo_size);

			fseek(file, ES32(pkg_table_entry.offset), SEEK_SET);
			fread(*sfo_buffer, *sfo_size, 1, file);
			fclose(file);

			return 0;
		}
	}

	log_info("Could not find a param.sfo file inside the PKG.");
	fclose(file);
	return(-1);
}

int get_ver_from_pkg(const char *file_path) {
	int ret;
	uint8_t *sfo = NULL;
	size_t sfo_size = 0;
	sfo_header_t *header = NULL;
	sfo_index_table_t *index_table = NULL;
	sfo_context_param_t *param = NULL;

	ret = 0;
	if ((ret = read_sfo_from_pkg(file_path, &sfo, &sfo_size)) < 0)
		goto error;

	if (sfo_size < sizeof(sfo_header_t)) {
		ret = -1;
		goto error;
	}

	header = (sfo_header_t *)sfo;
	if (header->magic != SFO_MAGIC) {
		ret = -1;
		goto error;
	}

	for (size_t i = 0; i < header->num_entries; ++i) {
		index_table = (sfo_index_table_t *)(sfo + sizeof(sfo_header_t) + i * sizeof(sfo_index_table_t));
		if(strcmp((char *)(sfo + header->key_table_offset + index_table->key_offset), "APP_VER") == 0) {
            param = (sfo_context_param_t *)(sfo + header->data_table_offset + index_table->data_offset);
            ret = atoi((const char*)param->value);
            break;
        }
	}

	free(sfo);

error:
	return ret;
}

extern std::atomic_bool update_check_finised;

void CheckUpdate(const char* tid, item_t &li){

    if((li.interruptible &&  li.update_status != (update_ret)UPDATE_NOT_CHECKED)
    || (!li.interruptible && li.update_status != (update_ret)UPDATE_NOT_CHECKED)){
        //log_info("interruptible %i, update_s %i", li->interruptible, li->update_status); 
        return;
    }

    std::string app_path, ver;
    bool app_exists = false;

#ifdef __ORBIS__
    app_inst_util_is_exists(tid, &app_exists);
    if(!app_exists){
        log_info("App not installed, skipping check");
        li.update_status = APP_NOT_INSTALLED;
        goto skip;
    }
    app_exists = if_exists(fmt::format("/user/app/{}/app.pkg", tid).c_str());
    if(!app_exists && li.token_d[APPTYPE].off != "Theme" && li.token_d[APPTYPE].off != "theme" && li.token_d[APPTYPE].off != "DLC"){
        app_exists = if_exists(fmt::format("/mnt/ext0/user/app/{}/app.pkg", tid).c_str());
        if(!app_exists)
           log_error("wtf??? where it is installed? Apptype %s", li.token_d[APPTYPE].off.c_str());
        else
          app_path = fmt::format("/mnt/ext0/user/app/{}/app.pkg", tid);
    }
    else
      app_path = fmt::format("/user/app/{}/app.pkg", tid);

    if(li.token_d[APPTYPE].off == "Theme" || li.token_d[APPTYPE].off == "theme" 
    || li.token_d[APPTYPE].off == "DLC" || li.token_d[APPTYPE].off == "Other")
       app_exists = true;

#endif

    if(!unsafe_source && app_exists)
    {
        update_ret ret =  (update_ret)UPDATE_NOT_CHECKED;
        if(li.update_status == (update_ret)UPDATE_NOT_CHECKED){
           // loadmsg(getLangSTR(DL_CACHE));
            // REPLACE WITH MD5 HASH IN DB
            if(!li.token_d[VERSION].off.empty()){
               //log_info(" checking md5 hash %s", li.token_d[PKG_MD5_HASH].off.c_str());
               if( li.token_d[APPTYPE].off == "Media" || li.token_d[APPTYPE].off == "media"){
                   ret = NO_UPDATE;
                   li.update_status = ret;
                   goto skip;
               }
               else if((li.token_d[PKG_MD5_HASH].off.empty() || CalcAppsize(app_path) > MB(300) || li.token_d[APPTYPE].off == "Theme" || li.token_d[APPTYPE].off == "theme" 
               ||  li.token_d[APPTYPE].off == "DLC" || li.token_d[APPTYPE].off == "Other") && GetInstalledVersion(li.token_d[ID].off, ver)){
                  if(normalize_version(li.token_d[VERSION].off) != normalize_version(ver)){
                    log_debug("version from db %s but installed version is %s", ver.c_str(), li.token_d[VERSION].off.c_str());
                    ret = UPDATE_FOUND;
                    li.update_status = ret;
                  }
                  else{
                        ret = NO_UPDATE;
                        li.update_status = ret;
                        log_info("No update found for %s [%s vs %s], skipping", tid, ver.c_str(), li.token_d[VERSION].off.c_str());
                        goto skip;
                    }
               }
               else if(!li.token_d[PKG_MD5_HASH].off.empty() && !MD5_hash_compare(app_path.c_str(), li.token_d[PKG_MD5_HASH].off.c_str())){
                  ret = UPDATE_FOUND;
                  li.update_status = ret;
               }
               else{
                  ret = NO_UPDATE;
                  li.update_status = ret;
                  log_info("No update found for %s, skipping", tid);
                  goto skip;
               }
            }
            log_info("sceStoreApiCheckUpdate %i %s", ret, tid);
            if (ret == UPDATE_FOUND){
                updates_counter++;
                if(!update_check_finised.load() && active_p == left_panel2){
                   left_panel2->mtx.lock();
                   if(updates_counter.load() > 0) // we counted at least one
                   {
                     vec4 col = (vec4){ .8164, .8164, .8125, 1. };
                     vec2 pos = { 114, 504 };
                     //log_info("ret: %d pen.x %.f pen.x %.f", updates_counter.load(), pos.x, pos.y);
                     texture_font_load_glyphs( sub_font, std::to_string(updates_counter.load()).c_str() );
                     pos.x = 460 - tl;
                     left_panel2->vbo.add_text(sub_font, std::to_string(updates_counter.load()), col, pos);
                     left_panel2->vbo_s = ASK_REFRESH;
                   }
                   left_panel2->mtx.unlock();
                }
            }
            //sceMsgDialogTerminate();
        }
        else{
            ret = li.update_status;                
            if (ret == UPDATE_FOUND){
                 download_panel->item_d[0].token_d[0].off = download_panel_text[0] = getLangSTR(UPDATE_NOW);
            }
            else if (ret == NO_UPDATE){
                //log_info("No Update Found for %s", li->token_d[ ID ].off);
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = getLangSTR(REINSTALL_APP);
            }
           /// log_info("app_exists %i, download_panel_text[0] %s, ret %i", app_exists, download_panel_text[0], ret);
        }  

       // li.update_status = ret;
    }
    else{
        skip:
        if(!app_exists && (download_panel_text[0] == getLangSTR(REINSTALL_APP) || download_panel_text[0] == getLangSTR(UPDATE_NOW))){
            log_info("app_exists %i, download_panel_text[0] %s", app_exists, download_panel_text.at(0).c_str());
            if(set.auto_install.load()){
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = getLangSTR(DL_AND_IN);
            }
            else{
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = getLangSTR(DL2);
            }
        }
    }
}


int Confirmation_Msg(std::string msg)
{

    sceMsgDialogTerminate();
    //ds

    sceMsgDialogInitialize();
    OrbisMsgDialogParam param;
    OrbisMsgDialogParamInitialize(&param);
    param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

    OrbisMsgDialogUserMessageParam userMsgParam;
    memset(&userMsgParam, 0, sizeof(userMsgParam));
    userMsgParam.msg = msg.c_str();
    userMsgParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO;
    param.userMsgParam = &userMsgParam;
    // cv   
    if (0 < sceMsgDialogOpen(&param))
        return NO;

    OrbisCommonDialogStatus stat;
    //
    while (1) {
        stat = sceMsgDialogUpdateStatus();
        if (stat == ORBIS_COMMON_DIALOG_STATUS_FINISHED) {

            OrbisMsgDialogResult result;
            memset(&result, 0, sizeof(result));

            sceMsgDialogGetResult(&result);

            return result.buttonId;
        }
    }
    //c
    return NO;
}

int options_dialog(std::string msg, std::string op1, std::string op2)
{

    sceMsgDialogTerminate();
    //ds

    sceMsgDialogInitialize();
    OrbisMsgDialogParam param;
    OrbisMsgDialogButtonsParam		buttonsParam;
    OrbisMsgDialogParamInitialize(&param);
    param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

    OrbisMsgDialogUserMessageParam userMsgParam;
    memset(&userMsgParam, 0, sizeof(userMsgParam));
    userMsgParam.msg = msg.c_str();
    userMsgParam.buttonsParam = &buttonsParam;
    userMsgParam.buttonsParam->msg1 = op1.c_str();
    userMsgParam.buttonsParam->msg2 = op2.c_str();
    userMsgParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS;
    param.userMsgParam = &userMsgParam;
    // cv   
    if (0 < sceMsgDialogOpen(&param))
        return 1;

    OrbisCommonDialogStatus stat;
    //
    while (1) {
        stat = sceMsgDialogUpdateStatus();
        if (stat == ORBIS_COMMON_DIALOG_STATUS_FINISHED) {

            OrbisMsgDialogResult result;
            memset(&result, 0, sizeof(result));

            sceMsgDialogGetResult(&result);

            log_info("result.buttonId %i", result.buttonId);

            return result.buttonId;
        }
    }
    //c
    return 2;
}