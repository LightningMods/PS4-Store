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
//#include <sys/sysctl.h>

StoreOptions set,
            *get;

extern LangStrings* stropts;
extern layout_t* download_panel;
extern char* download_panel_text[];
void* __stack_chk_guard = (void*)0xdeadbeef;

void __stack_chk_fail(void)
{
    log_info("Stack smashing detected.");
    msgok(FATAL, "Stack Smashing has been Detected");
}

extern item_t* all_apps; // Installed_Apps
extern bool sceAppInst_done;


bool unsafe_source = false;
int Lastlogcheck = 0;
static const char     *sizes[] = { "EiB", "PiB", "TiB", "GiB", "MiB", "KiB", "B" };
static const uint64_t  exbibytes = 1024ULL * 1024ULL * 1024ULL *
1024ULL * 1024ULL * 1024ULL;


/*
     stdio helper for host

    provide orbisFileGetFileContent (old liborbisFile) to
    read file content in a buffer and set last filesize
*/

size_t _orbisFile_lastopenFile_size;
// --------------------------------------------------------- buf_from_file ---
unsigned char* orbisFileGetFileContent(const char* filename)
{
    _orbisFile_lastopenFile_size = -1;

    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        log_error("Unable to open file \"%s\".", filename); return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char* buffer = (unsigned char*)malloc((size + 1) * sizeof(char));
    fread(buffer, sizeof(char), size, file);
    buffer[size] = 0;
    _orbisFile_lastopenFile_size = size;
    fclose(file);

    return buffer;
}


int copyFile(char* sourcefile, char* destfile)
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

void ProgSetMessagewText(int prog, const char* fmt, ...)
{

        char buff[300];
        memset(&buff[0], 0, sizeof(buff));

        va_list args;
        va_start(args, fmt);
        vsnprintf(&buff[0], 299, fmt, args);
        va_end(args);

        sceMsgDialogProgressBarSetValue(0, prog);
        sceMsgDialogProgressBarSetMsg(0, buff);
}

/// timing
unsigned int get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

bool touch_file(char* destfile)
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


unsigned int sceShellCoreUtilIsUsbMassStorageMounted_func(unsigned int usb_index)
{
  uint64_t res;
  void* ef; 

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
    static char prev_section[50] = "";

    if (strcmp(section, prev_section)) {
        log_debug("%s[%s]", (prev_section[0] ? "\n" : ""), section);
        strncpy(prev_section, section, sizeof(prev_section));
        prev_section[sizeof(prev_section) - 1] = '\0';
    }
    log_debug("%s = %s", name, value);

    StoreOptions* set = (StoreOptions*)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("Settings", "CDN")) {
        strcpy(set->opt[CDN_URL], value);
    }
    else if (MATCH("Settings", "TTF_Font")) {
        strcpy(set->opt[FNT_PATH], value);
        if (strstr(value, "SCE-PS3-RD-R-LATIN.TTF") == NULL)
            Fnt_setting_enabled = true;

    }
    else if (MATCH("Settings", "temppath")) {
        strcpy(set->opt[TMP_PATH], value);
    }
    else if (MATCH("Settings", "auto_install")) {
        set->auto_install = atoi(value);
    }
    else if (MATCH("Settings", "Legacy_Install")) {
        set->Legacy_Install = atoi(value);
    }
#if BETA==1
    else if (MATCH("Settings", "BETA_KEY")) {
        strcpy(set->opt[BETA_KEY], value);
    }
#endif

    return 1;
}


bool LoadOptions(StoreOptions *set)
{
    bool no_error = true;
    int error = 1;
    unsigned int usb_num = usbpath();

    for (int i = 0; i < NUM_OF_STRINGS; i++) {
        if (set->opt[i] != NULL)
            free(set->opt[i]);
    }
    for(int i=0; i< NUM_OF_STRINGS; i++)
    {   // dynalloc and zerofill
        set->opt[ i ] = calloc(256, sizeof(char));
    }

    strcpy(set->opt[CDN_URL], "https://api.pkg-zone.com");
    set->auto_install = true;
    set->Legacy_Install = false;
    strcpy(set->opt[TMP_PATH], "/user/app/NPXS39041/downloads");
    strcpy(set->opt[FNT_PATH], "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF");

    /* Initialize INI structure */
    char buff[257];
    sprintf(&buff[0], "/mnt/usb%d/settings.ini", usb_num);

    if (usb_num != -1)
    {
        if (!if_exists(&buff[0]))
        {
            log_warn( "No INI on USB");
            if (if_exists("/user/app/NPXS39041/settings.ini")) {
                snprintf(set->opt[INI_PATH], sizeof(buff), "%s", "/user/app/NPXS39041/settings.ini");
                error = ini_parse("/user/app/NPXS39041/settings.ini", print_ini_info, set);
                if (error) log_error("Bad config file (first error on line %d)!\n", error);
            }

        } else {
            error = ini_parse(&buff[0], print_ini_info, set);
            if (error) log_error("Bad config file (first error on line %d)!\n", error);
            log_info( "Loading ini from USB");
            snprintf(set->opt[INI_PATH], sizeof(buff), "%s", &buff[0]);
        }
    }
    else if (!if_exists("/user/app/NPXS39041/settings.ini"))
    {
        log_error("CANT FIND INI"); no_error = false;
    } else {
        error = ini_parse("/user/app/NPXS39041/settings.ini", print_ini_info, set);
        if (error) log_error("Bad config file (first error on line %d)!\n", error);
        log_info( "Loading ini from APP DIR");
        
        snprintf(set->opt[INI_PATH], sizeof(buff), "%s", "/user/app/NPXS39041/settings.ini");
    }
    
    if(strstr(set->opt[CDN_URL], "pkg-zone.com") == NULL)
       unsafe_source = true;

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
    set->lang = lang;
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
            log_debug("Loaded the backup, %s failed to load", Language_GetName(lang));
    }

    if (!Fnt_setting_enabled) {

        switch (lang) {
        case 0: //jAPN IS GREAT
            strcpy(set->opt[FNT_PATH], "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansJP-Regular.ttf");
            break;
        case 9:///THIS IS FOR JOON, IF HE COMES BACK
            strcpy(set->opt[FNT_PATH], "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansKR-Regular.ttf");
            break;
        case 21:
        case 23:
        case 24:
        case 25:
        case 26:
            strcpy(set->opt[FNT_PATH], "/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/HelveticaWorld-Multi.ttf");
            break;
            //NO USERS HAS TRANSLATED THE FOLLOWING LANGS, SO I DELETED THEM OUT THE PKG TO SAVE SPACE
            /*
            case 10:
                strcpy(set->opt[FNT_PATH],"/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansTC-Regular.ttf");
                break;
            case 11:
                strcpy(set->opt[FNT_PATH],"/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansSC-Regular.ttf");
                break;
            case 27:
                strcpy(set->opt[FNT_PATH],"/mnt/sandbox/pfsmnt/NPXS39041-app0/assets/fonts/NotoSansThai-Regular.ttf");
                break;*/
        default:
            break;
        }
    }
    //

    log_info( "set->opt[INI_PATH]: %s", set->opt[INI_PATH]);
    log_info( "set->opt[USB_PATH]: %s", set->opt[USB_PATH]);
    log_info( "set->opt[FNT_PATH]: %s", set->opt[FNT_PATH]);
    log_info( "set->opt[TMP_PATH]: %s", set->opt[TMP_PATH]);
    log_info( "set->opt[CDN_URL ]: %s", set->opt[CDN_URL]);
    log_info( "set->auto_install   : %i", set->auto_install);
    log_info( "set->Legacy_Install.: %s", set->Legacy_Install ? "ON" : "OFF");
    log_info( "set->Lang         : %s : %i", Language_GetName(lang), set->lang);
    log_info( "Unsafe Sources    : %i", unsafe_source);

//strcpy(set->opt[CDN_URL], "https://api.pkg-zone.com");
   // strcpy(set->opt[CDN_URL], "https://api.pkg-zone.com");

    return no_error;
}


char* cutoff(const char* str, int from, int to)
{
    if (from >= to)
        return  NULL;

    char* cut = calloc(sizeof(char), (to - from) + 1);
    char* begin = cut;
    if (!cut)
        return  NULL;

    const char* fromit = str + from;
    const char* toit = str + to;
    (void)toit;
    memcpy(cut, fromit, to);
    return begin;
}

bool SaveOptions(StoreOptions *set)
{  
    bool no_error = true;

    char buff[4084];
    memset(&buff[0], 0, 4084);
    snprintf(&buff[0], 4083, "[Settings]\nCDN=%s\nSecure_Boot=1\ntemppath=%s\nTTF_Font=%s\nauto_install=%i\nLegacy_Install=%i\n", set->opt[CDN_URL], set->opt[TMP_PATH], set->opt[FNT_PATH], set->auto_install, set->Legacy_Install);

    int fd = open(set->opt[INI_PATH], O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd >= 0)
    {
        write(fd, &buff[0], strlen((const char*)&buff[0]));
        close(fd);
    }
    else
        no_error = false;

    log_info( "set->opt[INI_PATH]: %s", set->opt[INI_PATH]);
    log_info( "set->opt[USB_PATH]: %s", set->opt[USB_PATH]);
    log_info( "set->opt[TMP_PATH]: %s", set->opt[TMP_PATH]);
    log_info( "set->opt[FNT_PATH]: %.20s...", set->opt[FNT_PATH]);
    log_info( "set->opt[CDN_URL ]: %s", set->opt[CDN_URL ]);
    log_info( "set->auto_install   : %i", set->auto_install);
    log_info( "set->Install_prog : %s", set->Legacy_Install ? "ON" : "OFF");


    /* Load values */
    chmod(set->opt[INI_PATH], 0777);

    return no_error;
}

static uint32_t sdkVersion = -1;
int	sysctlbyname(const char *, void *, size_t *, const void *, size_t);
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
char     result[40];
char *calculateSize(uint64_t size)
{
    uint64_t  multiplier = exbibytes;
    int i;

    for (i = 0; i < DIM(sizes); i++, multiplier /= 1024)
    {
        if (size < multiplier)
            continue;
        if (size % multiplier == 0)
            snprintf(result, sizeof(char) * 31, "%" PRIu64 " %s", size / multiplier, sizes[i]);
        else
            snprintf(result, sizeof(char) * 31, "%.1f %s", (float)size / multiplier, sizes[i]);
        return result;
    }
    strcpy(&result[0], "0");
    return &result[0];
}


static char *checkedsize = NULL;

long CalcAppsize(char *path)
{
    FILE *fp = NULL;
    long off;

    fp = fopen(path, "r");
    if (fp == NULL) return 0;
            
    if (fseek(fp, 0, SEEK_END) == -1) goto cleanup;
            
    off = ftell(fp);
    if (off == (long)-1) goto cleanup;

    if (fclose(fp) != 0) goto cleanup;
    
    if(off) { checkedsize = calculateSize(off); return off; }
    else    { checkedsize = NULL; return 0; }

cleanup:
    if (fp != NULL)
        fclose(fp);

    return 0;

}
//////////////////////////////////////////////////////




void CheckLogSize()
{
    CalcAppsize(STORE_LOG);

    log_info("[LOG CHECK] Size  = %s", checkedsize);

    if (strstr(checkedsize, "GiB") != NULL)
        unlink(STORE_LOG);
    else
        log_info("[LOG CHECK] File Is NOT 1 => GiB, Skipping Deletion");

    if(checkedsize) free(checkedsize), checkedsize = NULL;
}




void die(char* message)
{
    log_fatal( message);
    sceSystemServiceLoadExec("INVALID", 0);
}

void msgok(enum MSG_DIALOG level, const char* format, ...)
{
    if(strlen(format) > 301) return;

    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    sceMsgDialogTerminate();


    char buff[300];
    char buffer[300];
    memset(&buff[0], 0,  sizeof(buff));


    va_list args;
    va_start(args, format);
    vsnprintf(buff, 299, format, args);
    va_end(args);



    switch (level)
    {
    case NORMAL:
        log_info(buff);
        strcpy(buffer, buff);
        break;
    case FATAL:
        log_fatal( buff);
        snprintf(buffer, 299, "%s %s\n\n %s", getLangSTR(FATAL_ERROR),buff, getLangSTR(PRESS_OK_CLOSE));
        break;
    case WARNING:
        log_warn( buff);
        snprintf(buffer, 299, "%s %s",getLangSTR(WARNING2),&buff[0]);
        break;
    }


    sceMsgDialogInitialize();
    OrbisMsgDialogParam param;
    OrbisMsgDialogParamInitialize(&param);
    param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

    OrbisMsgDialogUserMessageParam userMsgParam;
    memset(&userMsgParam, 0, sizeof(userMsgParam));
    userMsgParam.msg        = buffer;
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
        die(&buff[0]);
        break;

    default:
        break;
    }

    return;
}

void loadmsg(const char* format, ...)
{
    if(strlen(format) > 1024) return;

    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    sceMsgDialogInitialize();

    char buff[1024];
    memset(&buff[0], 0, sizeof(buff));

    va_list args;
    va_start(args, format);
    vsnprintf(&buff[0], 1023, format, args);
    va_end(args);

    // int32_t  ret = 0;_sceCommonDialogBaseParamInit

    OrbisMsgDialogButtonsParam buttonsParam;
    OrbisMsgDialogUserMessageParam messageParam;
    OrbisMsgDialogParam dialogParam;

    OrbisMsgDialogParamInitialize(&dialogParam);

    memset(&buttonsParam, 0x00, sizeof(buttonsParam));
    memset(&messageParam, 0x00, sizeof(messageParam));

    dialogParam.userMsgParam = &messageParam;
    dialogParam.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;

    messageParam.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT;
    messageParam.msg        = &buff[0];

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

bool Keyboard(const char* Title, const char* initialTextBuffer, char* out_buffer, bool is_url)
{
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG);

    wchar_t title[100];
    wchar_t inputTextBuffer[255];
    char    titl[100];

    log_info("Keyboard(%s, %s, %p, %s)", Title, initialTextBuffer, out_buffer, is_url ? "true" : "false");

    if (initialTextBuffer && strlen(initialTextBuffer) > 254) return false;

    memset(&inputTextBuffer[0], 0, sizeof(inputTextBuffer));
    memset(&titl[0], 0, sizeof(titl));
    memset(&title[0], 0, sizeof(title));

    size_t (*PS4_wcstombs)(char* dest, const wchar_t* src, size_t max) = prx_func_loader("libSceLibcInternal.sprx", "wcstombs");
    size_t (*PS4_mbstowcs)(wchar_t* dest, const char* src, size_t max) = prx_func_loader("libSceLibcInternal.sprx", "mbstowcs");
    if(!PS4_wcstombs || !PS4_mbstowcs) {
        log_fatal("wcstombs or mbstowcs not found");
        return false;
    }

    if (initialTextBuffer) strncpy(out_buffer, initialTextBuffer, 254);
    //converts the multibyte string src to a wide-character string starting at dest.
    PS4_mbstowcs(&inputTextBuffer[0], out_buffer, strlen(out_buffer));
    // use custom title
    strncpy(&titl[0], Title ? Title : "Store Keyboard", sizeof titl - 1);
    //converts the multibyte string src to a wide-character string starting at dest.
    PS4_mbstowcs(&title[0], &titl[0], strlen(&titl[0]));

    OrbisImeDialogParam param;
    memset(&param, 0, sizeof(OrbisImeDialogParam));
   // msgok(NORMAL, "title: %ls inputTextBuffer: %ls", &title[0], &inputTextBuffer[0]);

    param.maxTextLength = 254;
    param.inputTextBuffer = &inputTextBuffer[0];
    param.title = &title[0];
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
                PS4_wcstombs(out_buffer, &inputTextBuffer[0], sizeof(inputTextBuffer));
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
int check_download_counter(StoreOptions* set, char* title_id)
{
    char  http_req[300];
    char* result;
    int count;

    snprintf(http_req, 300, "%s/download.php?tid=%s&check=true", set->opt[CDN_URL], title_id);

    result = check_from_url(http_req, DL_COUNTER, false);
    if (result){
        count = atoi(result);
        free(result);
    }
    else
        count = -1;

    log_debug( "%s Number_of_DL: %d", __FUNCTION__, count);
    return count;
}



// 
int check_store_from_url(char* cdn, enum CHECK_OPTS opt)
{
    char  http_req[300];
    char *result = NULL;

    switch(opt)
    {
        case MD5_HASH:
        {
            if (!if_exists(SQL_STORE_DB))
                return 0;

            snprintf(http_req, 299, "%s/api.php?db_check_hash=true", cdn);

            if ((result = check_from_url(http_req, MD5_HASH, false)) != NULL && result && MD5_hash_compare(SQL_STORE_DB, result) == SAME_HASH)
            {
                if(result) free(result); 
                return 1;
            }
            else
            {
                if(result) free(result); 
                return 0;
            }
            break;
        } 

        case COUNT:
        {
            int pages = 0;
            int count = SQL_Get_Count();
            if (count > 0)
            {
                pages = (count + STORE_PAGE_SIZE - 1) / STORE_PAGE_SIZE;
                if (pages >  STORE_MAX_LIMIT_PAGES)
                    msgok(FATAL, getLangSTR(EXCEED_LIMITS));

                log_debug("counted pages: %d", pages);
            }
            else
                msgok(FATAL, getLangSTR(ZERO_ITEMS));
          
            return pages;
        } 

#if BETA==1 //http://api.staging.pkg-zone.com/key/beta/check?key=KEY
     
        case BETA_CHECK: {
            char url[255];
            snprintf(url, 254, "http://api.staging.pkg-zone.com/key/beta/check?key=%s", get->opt[BETA_KEY]);
            log_info("FULL URL: %s", url); 
            return check_from_url(url, BETA_CHECK, false); 
            break;
        }
#endif

        default: break;
    }
    return -1;   
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

int malloc_stats_fast(OrbisMallocManagedSize *ManagedSize);

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
   log_info("[MEM] CurrentSystemSize: %s, CurrentInUseSize: %s", calculateSize(ManagedSize.curSysSz), calculateSize(ManagedSize.curUseSz));
   log_info("[MEM] MaxSystemSize: %s, MaxInUseSize: %s", calculateSize(ManagedSize.maxSysSz), calculateSize(ManagedSize.maxUseSz));
   log_info("[VRAM] VRAM_Available: %s, AmountUsed: %.2f%%",calculateSize(sz), dfp_fmem);

}


int progstart(char* format, ...)
{

    
    int ret = 0;

    char buff[1024];

    memset(buff, 0, 1024);

    va_list args;
    va_start(args, format);
    vsprintf(&buff[0], format, args);
    va_end(args);

    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
    sceMsgDialogInitialize();

    OrbisMsgDialogParam dialogParam;
    OrbisMsgDialogParamInitialize(&dialogParam);
    dialogParam.mode = 2;

    OrbisMsgDialogProgressBarParam  progBarParam;
    memset(&progBarParam, 0, sizeof(OrbisMsgDialogProgressBarParam));

    dialogParam.progBarParam = &progBarParam;
    dialogParam.progBarParam->barType = 0;
    dialogParam.progBarParam->msg = &buff[0];

    sceMsgDialogOpen(&dialogParam);

    return ret;
}

bool init_curl(){
   
    int ress = curl_global_init(CURL_GLOBAL_SSL);
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

void CheckUpdate(const char* tid, item_t *li){

    bool app_exists = false;
    if(li->interuptable && 
    li->update_status != UPDATE_NOT_CHECKED){
        //log_info("interuptable %i, update_s %i", li->interuptable, li->update_status); 
        return;
    }

    char buff[256];
    sprintf(buff, "/user/app/%s/app.pkg", tid);
    if(CalcAppsize(buff) > MB(500)){
        log_info("App is bigger than 500MB, skipping check");
        goto skip;
    }
    if(li->update_status != UPDATE_FOUND &&
     li->update_status != NO_UPDATE)
      app_inst_util_is_exists(tid, &app_exists);
    else
        app_exists = true;

    if(!unsafe_source && app_exists)
    {
        update_ret ret = UPDATE_NOT_CHECKED;
        if(li->update_status == UPDATE_NOT_CHECKED){
            loadmsg(getLangSTR(DL_CACHE));
            ret = sceStoreApiCheckUpdate(tid);
            sceMsgDialogTerminate();
        }
        else{
            ret = li->update_status;                
            if (ret == UPDATE_FOUND){
                 download_panel->item_d[0].token_d[0].off = download_panel_text[0] = (char*)getLangSTR(UPDATE_NOW);
            }
            else if (ret == NO_UPDATE){
                //log_info("No Update Found for %s", li->token_d[ ID ].off);
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = (char*)getLangSTR(REINSTALL_APP);
            }
           /// log_info("app_exists %i, download_panel_text[0] %s, ret %i", app_exists, download_panel_text[0], ret);

        }   li->update_status = ret;
    }
    else{
        skip:
        if(!app_exists && (download_panel_text[0] == (char*)getLangSTR(REINSTALL_APP) 
        || download_panel_text[0] == (char*)getLangSTR(UPDATE_NOW))){
            log_info("app_exists %i, download_panel_text[0] %s", app_exists, download_panel_text[0]);
            if(get->auto_install){
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = (char*)getLangSTR(DL_AND_IN);
            }
            else{
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = (char*)getLangSTR(DL2);
            }
        }
    }
}




wchar_t inputTextBuffer[70];
static char storebuffer[70];

char* StoreKeyboard(const char* Title, char* initialTextBuffer)
{
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG);

    wchar_t title[100];
    char    titl[100];

    if (initialTextBuffer
        && strlen(initialTextBuffer) > 1023) return "Too Long";

    memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
    memset(&storebuffer[0], 0, sizeof(storebuffer));

    if (initialTextBuffer)
        snprintf(&storebuffer[0], 69, "%s", initialTextBuffer);
    //converts the multibyte string src to a wide-character string starting at dest.
    mbstowcs(inputTextBuffer, storebuffer, strlen(storebuffer) + 1);
    // use custom title
    if (Title)
        snprintf(&titl[0], 69, "%s", Title);
    else // default
        snprintf(&titl[0], 69, "%s", "Store Keyboard");
    //converts the multibyte string src to a wide-character string starting at dest.
    mbstowcs(title, titl, strlen(titl) + 1);

    OrbisImeDialogParam param;
    memset(&param, 0, sizeof(OrbisImeDialogParam));

    param.maxTextLength = 1023;
    param.inputTextBuffer = inputTextBuffer;
    param.title = title;
    param.userId = 0xFE;
    param.type = ORBIS_IME_TYPE_BASIC_LATIN;
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

            if (result.endstatus == ORBIS_IME_DIALOG_END_STATUS_USER_CANCELED)
                goto Finished;

            if (result.endstatus == ORBIS_IME_DIALOG_END_STATUS_OK)
            {
                log_info("status %i, endstatus %i ", status, result.endstatus);
                wcstombs(&storebuffer[0], inputTextBuffer, 70);
                goto Finished;
            }
        }

        if (status == ORBIS_IME_DIALOG_STATUS_NONE) goto Finished;
    }

Finished:
    sceImeDialogTerm();
    return storebuffer;
}