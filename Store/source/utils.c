#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include <pl_ini.h>
#include <md5.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include "log.h"
#include "defines.h"
#include <user_mem.h> 

StoreOptions set,
            *get;

extern LangStrings* stropts;

void* __stack_chk_guard = (void*)0xdeadbeef;

void __stack_chk_fail(void)
{
    log_info("Stack smashing detected.");
    msgok(FATAL, "Stack Smashing has been Detected");
}

extern item_t* all_apps; // Installed_Apps


bool sceAppInst_done = false;
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



static bool touch_file(char* destfile)
{
    int fd = open(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd > 0) {
        close(fd);
        return true; 
    }
    else
        return false;
}

char* usbpath()
{
    static char usbbuf[100];
    usbbuf[0] = '\0';
    for(int x = 0; x <= 7; x++)
    {
        snprintf(usbbuf, sizeof(usbbuf), "/mnt/usb%i/.dirtest", x);
        if(touch_file(usbbuf))
        {
            snprintf(usbbuf, 100, "/mnt/usb%i", x);

            log_info("found usb. = %s", usbbuf);
            return usbbuf;
        }
    }
    log_warn( "FOUND NO USB");

    return "";
}

bool MD5_hash_compare(const char* file1, const char* hash)
{
    unsigned char c[MD5_HASH_LENGTH];
    FILE* f1 = fopen(file1, "rb");
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

    if (strcmp(md5_string, hash) != 0) {
        return DIFFERENT_HASH;
    }

    log_info( "Input HASH: %s", hash);
    fclose(f1);

    return SAME_HASH;
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


bool app_inst_util_uninstall_patch(const char* title_id, int* error) {
    int ret;

    if(!sceAppInst_done)
    {
        bool res = is_apputil_init();
        if (!res)
        {  *error = 0xDEADBEEF;
           return false;
        }
    }
    if (!title_id) {
        ret = 0x80020016;
        if (error) {
            *error = ret;
        }
        goto err;
    }

    ret = sceAppInstUtilAppUnInstallPat(title_id);
    if (ret)
    {
        if (error) {
            *error = ret;
        }
        log_error( "sceAppInstUtilAppUnInstallPat failed: 0x%08X", ret);
        goto err;
    }

    return true;

err:
    return false;
}


bool app_inst_util_uninstall_game(const char *title_id, int *error)
{
    int ret;

    if (strlen(title_id) != 9)
    {
        *error = 0xBADBAD;
        return false;
    }
    if(!sceAppInst_done)
    {
        bool res = is_apputil_init();
        if (!res)
        {  *error = 0xDEADBEEF;
           return false;
        }    
    }

    if (!title_id)
    {
        ret = 0x80020016;
        if (error)
        {
            *error = ret;
        }
        goto err;
    }

    ret = sceAppInstUtilAppUnInstall(title_id);
    if (ret)
    {
        if (error)
        {
            *error = ret;
        }
        log_error( "sceAppInstUtilAppUnInstall failed: 0x%08X", ret);
        goto err;
    }

    return true;

err:
    return false;
}

int app_inst_util_get_size(const char* title_id) {
    uint64_t size = 0;

    if (!sceAppInst_done) {
        log_debug("Starting app_inst_util_init..");
        if (!app_inst_util_init()) {
            log_error("app_inst_util_init has failed...");
            return size;
        }
    }

    int ret = sceAppInstUtilAppGetSize(title_id, &size);
    if (ret)
        log_error("sceAppInstUtilAppGetSize failed: 0x%08X", ret);
    else
        log_info("Size: %s", calculateSize(size));
   
    return B2GB(size);
}

bool Fnt_setting_enabled = false;

static int print_ini_info(void* user, const char* section, const char* name,
    const char* value)
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

    if (MATCH("Settings", "Daemon_on_start")) {
        set->Daemon_on_start = atoi(value);
    }
    else if (MATCH("Settings", "CDN")) {
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
    else if (MATCH("Settings", "Legacy")) {
        set->Legacy = atoi(value);
    }
    else if (MATCH("Settings", "StoreOnUSB")) {
        set->StoreOnUSB = atoi(value);
    }
    else if (MATCH("Settings", "HomeMenu_Redirection")) {
        set->HomeMenu_Redirection = atoi(value);
    }
    else if (MATCH("Settings", "Show_install_prog")) {
        set->Show_install_prog = atoi(value);
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

    for (int i = 0; i < NUM_OF_STRINGS; i++) {
        if (set->opt[i] != NULL)
            free(set->opt[i]);
    }
    for(int i=0; i< NUM_OF_STRINGS; i++)
    {   // dynalloc and zerofill
        set->opt[ i ] = calloc(256, sizeof(char));
    }

    strcpy(set->opt[CDN_URL], "http://api.pkg-zone.com");
    set->Daemon_on_start = true;
    set->Legacy = false;
    set->StoreOnUSB = false;
    set->HomeMenu_Redirection = false;
    set->Show_install_prog = true;
    strcpy(set->opt[TMP_PATH], "/user/app/NPXS39041/downloads");
    strcpy(set->opt[FNT_PATH], "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF");

    /* Initialize INI structure */
    char buff[257];
    if (strstr(usbpath(), "/mnt/usb"))
    {
        snprintf(buff, 256, "%s/settings.ini", usbpath());

        if (!if_exists(buff))
        {
            log_warn( "No INI on USB");
            if (if_exists("/user/app/NPXS39041/settings.ini"))
                snprintf(set->opt[INI_PATH], 255, "%s", "/user/app/NPXS39041/settings.ini");
        } else {
            error = ini_parse(buff, print_ini_info, set);
            if (error) log_error("Bad config file (first error on line %d)!\n", error);
            log_info( "Loading ini from USB");
            snprintf(set->opt[INI_PATH], 255, "%s", buff);
        }
    }
    else if (!if_exists("/user/app/NPXS39041/settings.ini"))
    {
        log_error("CANT FIND INI"); no_error = false;
    } else {
        error = ini_parse("/user/app/NPXS39041/settings.ini", print_ini_info, set);
        if (error) log_error("Bad config file (first error on line %d)!\n", error);
        log_info( "Loading ini from APP DIR");
        
        snprintf(set->opt[INI_PATH], 255, "%s", "/user/app/NPXS39041/settings.ini");
    }
    
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
    log_info( "set->legacy       : %i", set->Legacy);
    log_info( "set->Daemon_...   : %s", set->Daemon_on_start ? "ON" : "OFF");
    log_info( "set->HomeMen...   : %s", set->HomeMenu_Redirection ? "ItemzFlow (ON)" : "Orbis (OFF)");
    log_info( "set->StoreOnUSB   : %i", set->StoreOnUSB);
    log_info( "set->Install_prog.: %s", set->Show_install_prog ? "ON" : "OFF");
    log_info( "set->Lang         : %s : %i", Language_GetName(lang), set->lang);



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

    char* buff[4084];
    memset(buff, 0, 4084);
    snprintf(buff, 4083, "[Settings]\nCDN=%s\nSecure_Boot=1\ntemppath=%s\nTTF_Font=%s\nStoreOnUSB=%i\nShow_install_prog=%i\nHomeMenu_Redirection=%i\nDaemon_on_start=%i\nLegacy=%i\n", set->opt[CDN_URL], set->opt[TMP_PATH], set->opt[FNT_PATH], set->StoreOnUSB, set->Show_install_prog, set->HomeMenu_Redirection, set->Daemon_on_start, set->Legacy);

    int fd = open(set->opt[INI_PATH], O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd >= 0)
    {
        write(fd, buff, strlen(buff));
        close(fd);
    }
    else
        no_error = false;

    log_info( "set->opt[INI_PATH]: %s", set->opt[INI_PATH]);
    log_info( "set->opt[USB_PATH]: %s", set->opt[USB_PATH]);
    log_info( "set->opt[TMP_PATH]: %s", set->opt[TMP_PATH]);
    log_info( "set->opt[FNT_PATH]: %.20s...", set->opt[FNT_PATH]);
    log_info( "set->opt[CDN_URL ]: %s", set->opt[CDN_URL ]);
    log_info( "set->StoreOnUSB   : %i", set->StoreOnUSB);
    log_info( "set->HomeMen...   : %s", set->HomeMenu_Redirection ? "ItemzFlow (ON)" : "Orbis (OFF)");
    log_info( "set->Install_prog : %s", set->Show_install_prog ? "ON" : "OFF");


    /* Load values */
    chmod(set->opt[INI_PATH], 0777);

    return no_error;
}

uint32_t sdkVersion = -1;

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

char *calculateSize(uint64_t size)
{
    char     *result = (char *)malloc(sizeof(char) * 32);
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
    strcpy(result, "0");
    return result;
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

void msgok(enum MSG_DIALOG level, char* format, ...)
{
    if(strlen(format) > 301) return;

    int ret = 0;
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

void loadmsg(char* format, ...)
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
        snprintf(&storebuffer[0], 1023, "%s", initialTextBuffer);
    //converts the multibyte string src to a wide-character string starting at dest.
    mbstowcs(inputTextBuffer, storebuffer, strlen(storebuffer) + 1);
    // use custom title
    if (Title)
        snprintf(&titl[0], 1023, "%s", Title);
    else // default
        snprintf(&titl[0], 1023, "%s", "Store Keyboard");
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
//
int check_download_counter(StoreOptions* set, char* title_id)
{

    char  http_req[300];
    char* result;
    int count;


    snprintf(http_req, 300, "%s/download.php?tid=%s&check=true", set->opt[CDN_URL], title_id);

    result = check_from_url(http_req, DL_COUNTER, false);

    if (result != NULL)
        count = atoi(result);
    else
        msgok(FATAL, "Dev Error 6066");

    free(result);

    log_debug( "%s Number_of_DL:%d", __FUNCTION__, count);

    return count;
}



// 
int check_store_from_url(int page_number, char* cdn, enum CHECK_OPTS opt)
{
    char  http_req[300];
    char  dst_path[300];
    char *result = NULL;

    switch(opt)
    {
        case MD5_HASH:
        {
            snprintf(http_req, 300, "%s/api.php?page=%i&check_hash=true", cdn, page_number);
            snprintf(dst_path, 300, "/user/app/NPXS39041/pages/homebrew-page%i.json", page_number);

            if ((result = check_from_url(http_req, MD5_HASH, false)) != NULL && MD5_hash_compare(dst_path, result) == SAME_HASH)
            {
                free(result); return 1;
            }
            else
            {
                if(result != NULL)
                   free(result); 
                
                return 0;
            }
        } break;

        case COUNT:
        {
            snprintf(http_req, 300, "%s/api.php?count=true", cdn);

            int pages = 0;
            if ((result = check_from_url(http_req, COUNT, false)) != NULL)
            {
                log_info("result %s", result);
                if (atoi(result) < STORE_MAX_LIMIT_PAGES)
                    pages = (atoi(result) + PAGE_SIZE - 1) / PAGE_SIZE;
                else
                    msgok(FATAL, getLangSTR(EXCEED_LIMITS));

                if (pages <= 0)
                    msgok(FATAL, getLangSTR(ZERO_ITEMS));

                free(result);

                log_debug("counted pages: %d", pages);
            }
            else
                msgok(FATAL, getLangSTR(COUNT_NULL));
          
            return pages;
        } break;

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
    return 0;   
}

int getjson(int Pagenumb, char* cdn, bool legacy)
{
    char http_req[300];
    char destbuf [300];

    snprintf(destbuf, 300, "/user/app/NPXS39041/pages/homebrew-page%i.json", Pagenumb);

    if(legacy == true)
        snprintf(http_req, 300, "%s/homebrew-page%i.json", cdn, Pagenumb);
    else
    {
        snprintf(http_req, 300, "%s/api.php?page=%i", cdn, Pagenumb);
        if (if_exists(destbuf))
        {
            log_info("page %i exists", Pagenumb);
            if (!check_store_from_url(Pagenumb, get->opt[CDN_URL], MD5_HASH))
            {
                unlink(destbuf);
                if( dl_from_url(http_req, destbuf, false) )
                {
                    msgok(FATAL, "%s: %i: %s",getLangSTR(DL_FAILED_W), Pagenumb, get->opt[CDN_URL]);
                    return -1;
                }
            }
            else
            {
                log_info("HASH IS THE SAME"); return 0;
            }
        }
        else
        {
            if( dl_from_url(http_req, destbuf, false) )
            {
                msgok(FATAL, "%s: %i From: %s", getLangSTR(DL_ERROR_PAGE),Pagenumb, get->opt[CDN_URL]);
                return -1;
            }
        }
    }   
    log_info("%s %s", __FUNCTION__, http_req);

    return dl_from_url(http_req, destbuf, false);
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


bool IS_ERROR(uint32_t a1)
{
    return a1 & 0x80000000;
}

//            sceSystemServiceKillApp();

uint32_t Launch_App(char* TITLE_ID, bool silent) {

    uint32_t sys_res = -1;

    int libcmi = sceKernelLoadStartModule("/system/common/lib/libSceSystemService.sprx", 0, NULL, 0, 0, 0);
    if (libcmi > 0)
    {
        log_info("Starting action Launch_Game_opt");


        OrbisUserServiceLoginUserIdList userIdList;

        log_info("ret %x", sceUserServiceGetLoginUserIdList(&userIdList));

        for (int i = 0; i < 4; i++)
        {
            if (userIdList.userId[i] != 0xFF)
            {
                log_info("[%i] User ID 0x%x", i, userIdList.userId[i]);
            }
        }


        LncAppParam param;
        param.sz = sizeof(LncAppParam);

        if (userIdList.userId[0] != 0xFF)
            param.user_id = userIdList.userId[0];
        else if (userIdList.userId[1] != 0xFF)
            param.user_id = userIdList.userId[1];

        param.app_opt = 0;
        param.crash_report = 0;
        param.check_flag = SkipSystemUpdateCheck;

        log_info("l1 %x", sceLncUtilInitialize());
        

        sys_res = sceLncUtilLaunchApp(TITLE_ID, 0, &param);
        if (IS_ERROR(sys_res))
        {
            if (!silent) {
                log_info("Switch 0x%x", sys_res);
                switch (sys_res) {
                case SCE_LNC_ERROR_APP_NOT_FOUND: {
                    if(strstr(DEBUG_SETTINGS_TID, TITLE_ID) == NULL)
                       msgok(WARNING, getLangSTR(APP_NOT_FOUND));

                    break;
                }
                case SCE_LNC_UTIL_ERROR_ALREADY_RUNNING: {
                    msgok(WARNING, getLangSTR(APP_OPENED));
                    break;
                }
                case SCE_LNC_UTIL_ERROR_APPHOME_EBOOTBIN_NOT_FOUND: {
                    msgok(WARNING, getLangSTR(MISSING_EBOOT));
                    break;
                }
                case SCE_LNC_UTIL_ERROR_APPHOME_PARAMSFO_NOT_FOUND: {
                    msgok(WARNING, getLangSTR(MISSING_SFO));
                    break;
                }
                case SCE_LNC_UTIL_ERROR_NO_SFOKEY_IN_APP_INFO: {
                    msgok(WARNING, getLangSTR(CORRUPT_SFO));
                    break;
                }
                case SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_KILL_NEEDED: {
                    log_debug("ALREADY RUNNING KILL NEEDED");
                    break;
                }
                case SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_SUSPEND_NEEDED: {
                    log_debug("ALREADY RUNNING SUSPEND NEEDED");
                    break;
                }
                case SCE_LNC_UTIL_ERROR_SETUP_FS_SANDBOX: {
                    msgok(WARNING, getLangSTR(APP_UNL));
                    break;
                }
                case SCE_LNC_UTIL_ERROR_INVALID_TITLE_ID: {
                    msgok(WARNING, getLangSTR(ID_NOT_VAILD));
                    break;
                }

                default: {
                    msgok(WARNING, "%s: 0x%x", getLangSTR(LAUNCH_ERROR),sys_res);
                    break;
                }
                }
            }
        }

        log_info("launch ret 0x%x", sys_res);

    }
    else {
        if(!silent)
            msgok(WARNING, "%s 0x%X", getLangSTR(LAUNCH_ERROR),libcmi);
    }

    return sys_res;
}

extern struct retry_t* cf_tex;

extern int total_pages;


void build_iovec(struct iovec** iov, int* iovlen, const char* name, const void* val, size_t len) {
    int i;

    if (*iovlen < 0)
        return;

    i = *iovlen;
    *iov = (struct iovec*)realloc((void*)(*iov), sizeof(struct iovec) * (i + 2));
    if (*iov == NULL) {
        *iovlen = -1;
        return;
    }

    (*iov)[i].iov_base = strdup(name);
    (*iov)[i].iov_len = strlen(name) + 1;
    ++i;

    (*iov)[i].iov_base = (void*)val;
    if (len == (size_t)-1) {
        if (val != NULL)
            len = strlen((const char*)val) + 1;
        else
            len = 0;
    }
    (*iov)[i].iov_len = (int)len;

    *iovlen = ++i;
}


int mountfs(const char* device, const char* mountpoint, const char* fstype, const char* mode, uint64_t flags)
{
    struct iovec* iov = NULL;
    int iovlen = 0;
    int ret;

    build_iovec(&iov, &iovlen, "fstype", fstype, -1);
    build_iovec(&iov, &iovlen, "fspath", mountpoint, -1);
    build_iovec(&iov, &iovlen, "from", device, -1);
    build_iovec(&iov, &iovlen, "large", "yes", -1);
    build_iovec(&iov, &iovlen, "timezone", "static", -1);
    build_iovec(&iov, &iovlen, "async", "", -1);
    build_iovec(&iov, &iovlen, "ignoreacl", "", -1);

    if (mode) {
        build_iovec(&iov, &iovlen, "dirmask", mode, -1);
        build_iovec(&iov, &iovlen, "mask", mode, -1);
    }

    log_info("##^  [I] Mounting %s \"%s\" to \"%s\" \n", fstype, device, mountpoint);
    ret = nmount(iov, iovlen, flags);
    if (ret < 0) {
        log_info("##^  [E] Failed: %d (errno: %d).\n", ret, errno);
        goto error;
    }
    else {
        log_info("##^  [I] Success.\n");
    }

error:
    return ret;
}


void print_memory()
{
    SceLibcMallocManagedSize ManagedSize;
    SCE_LIBC_INIT_MALLOC_MANAGED_SIZE(ManagedSize);
    malloc_stats_fast(&ManagedSize);

   log_info("[StoreCore][MEM] CurrentSystemSize: %s, CurrentInUseSize: %s", calculateSize(ManagedSize.currentSystemSize), calculateSize(ManagedSize.currentInuseSize));
   log_info("[StoreCore][MEM] MaxSystemSize: %s, MaxInUseSize: %s", calculateSize(ManagedSize.maxSystemSize), calculateSize(ManagedSize.maxInuseSize));

}


int progstart(char* msg)
{

    int ret = 0;
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    ret = sceMsgDialogTerminate();

    log_info("sceMsgDialogTerminate = %i", ret);

    ret = sceMsgDialogInitialize();

    log_info("sceMsgDialogInitialize = %i", ret);


    OrbisMsgDialogParam dialogParam;
    OrbisMsgDialogParamInitialize(&dialogParam);
    dialogParam.mode = 2;

    OrbisMsgDialogProgressBarParam  progBarParam;
    memset(&progBarParam, 0, sizeof(OrbisMsgDialogProgressBarParam));

    dialogParam.progBarParam = &progBarParam;
    dialogParam.progBarParam->barType = 0;
    dialogParam.progBarParam->msg = msg;

    sceMsgDialogOpen(&dialogParam);

    return ret;
}

int item_t_order_by_alphabet(const void* a1, const void* b1)
{
    item_t* a = (item_t*)a1;
    item_t* b = (item_t*)b1;

    if (b->token_d[NAME].off == NULL || b->token_d[NAME].off == NULL) return 1;

    //log_info("|First: %s | Second: %s |", a->token_d[NAME].off, b->token_d[NAME].off);

    return strcmp(a->token_d[NAME].off, b->token_d[NAME].off);
}

static int item_t_order_by_tid(const item_t* a, const item_t* b)
{
    if (a->token_d[ID].off == NULL || b->token_d[ID].off == NULL) return 1;

    //log_info("|First: %s | Second: %s |", a->token_d[ID].off, b->token_d[ID].off);

    return strcmp(a->token_d[ID].off, b->token_d[ID].off);
    /* strcmp functions works exactly as expected from comparison function */
}

void print_Apps_Array(item_t* b, int arrSize) {
    for (int i = 0; i <= arrSize; i++) {
        if (b[i].token_d[ID].off != NULL)
            log_info("b[%i].token_d[%i].off ptr: %p", i, ID, b[i].token_d[ID].off);
        if (b[i].token_d[NAME].off != NULL)
             log_info("b[%i].token_d[%i].off ptr: %p", i, NAME, b[i].token_d[NAME].off);
        if (b[i].token_d[VERSION].off != NULL)
             log_info("b[%i].token_d[%i].off ptr: %p", i, VERSION, b[i].token_d[VERSION].off);
    }
}
void delete_apps_array(item_t* b, int arrSize) {
    //free only what we know is not NULL
    if (b != NULL) {
        for (int i = 0; i <= arrSize; i++) {
            for (int j = 0; j < NUM_OF_USER_TOKENS; j++) {
                // log_info("Index %i, index_t %i", i, j);
                if (b[i].token_d[j].off != NULL) {
                    log_info("b[%i].token_d[%i].off %p", i, j, b[i].token_d[j].off);
                    free(b[i].token_d[j].off);
                }
            }
        }
        //free struct 
        free(b);
    }
}

void refresh_apps_for_cf(enum SORT_APPS_BY op)
{
    int before = all_apps[0].token_c;
    loadmsg(getLangSTR(RELOAD_LIST));
    log_debug("Reloading Installed Apps before: %i", before);
    //leak not so much
    //print_Apps_Array(all_apps, before);
    delete_apps_array(all_apps, before);

    all_apps = index_items_from_dir("/user/app", "/mnt/ext0/user/app");
    log_info("=== %i", all_apps[0].token_c);

    switch (op) {
        case SORT_TID: {
        qsort(all_apps + 1, all_apps[0].token_c, sizeof(item_t), &item_t_order_by_tid);
        break;
        }
        case SORT_ALPHABET: {
        qsort(all_apps + 1, all_apps[0].token_c, sizeof(item_t), &item_t_order_by_alphabet);
        break;
        }
    }

    log_info("=== %i", all_apps[0].token_c);
    print_Apps_Array(all_apps, all_apps[0].token_c);
    InitScene_5(ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);
    InitScene_4(ATTR_ORBISGL_WIDTH, ATTR_ORBISGL_HEIGHT);
    for (int i = 1; i < all_apps[0].token_c + 1; i++) {
        check_tex_for_reload(i);
        check_n_load_textures(i);
    }

    sceMsgDialogTerminate();
    log_debug("Done reloading # of App: %i, # of Apps added/removed: %i", all_apps[0].token_c, all_apps[0].token_c - before);
}
