
#include "utils.h"
#include <pl_ini.h>
#include <md5.h>

StoreOptions set,
            *get;

int Lastlogcheck = 0;
static const char     *sizes[] = { "EiB", "PiB", "TiB", "GiB", "MiB", "KiB", "B" };
static const uint64_t  exbibytes = 1024ULL * 1024ULL * 1024ULL *
1024ULL * 1024ULL * 1024ULL;

char* usbpath()
{
    int usb;
    static char usbbuf[100];
    usbbuf[0] = '\0';
    for(int x = 0; x <= 7; x++)
    {
        snprintf(usbbuf, sizeof(usbbuf), "/mnt/usb%i/.dirtest", x);
        usb = sceKernelOpen(usbbuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if(usb > 0)
        {
            sceKernelClose(usb);

            snprintf(usbbuf, 100, "/mnt/usb%i", x);

            klog("found usb = %s\n", usbbuf);
            return usbbuf;
        }
    }
    klog("FOUND NO USB\n");

    return "";
}

int MD5_hash_compare(const char* file1, const char* hash)
{
    unsigned char c[MD5_HASH_LENGTH];
    int i;
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
        sprintf(&md5_string[i*2], "%02x", (unsigned int)c[i]);
    }
        klog("FILE HASH: %s\n", md5_string);
    md5_string[32] = 0;

    if (strcmp(md5_string, hash) != 0) {
        return DIFFERENT_HASH;
    }

    klog("Input HASH: %s\n", hash);

    return SAME_HASH;
}

int LoadOptions(StoreOptions *set)
{
    for(int i=0; i< NUM_OF_STRINGS; i++)
    {   // dynalloc and zerofill
        set->opt[ i ] = calloc(256, sizeof(char));
    }

    /* Initialize INI structure */
    pl_ini_file file;
    char buff[256];
    if (strstr(usbpath(), "/mnt/usb"))
    {
        snprintf(buff, 256, "%s/settings.ini", usbpath());

        if (sceKernelOpen(buff, 0x0000, 0000) < 0)
        {
            klog("No INI on USB\n");
            if (sceKernelOpen("/user/app/NPXS39041/settings.ini", 0x0000, 0000) > 0)
                sprintf(set->opt[INI_PATH], "%s", "/user/app/NPXS39041/settings.ini");
            else
                return -1;
        } else {
            pl_ini_load(&file, buff);
            klog("Loading ini from USB\n");
            sprintf(set->opt[INI_PATH], "%s", buff);
        }
    }
    else
    if (sceKernelOpen("/user/app/NPXS39041/settings.ini", 0x0000, 0000) < 0)
    {
        klog("CANT FIND INI\n"); return -1;
    } else {
        pl_ini_load( &file, "/user/app/NPXS39041/settings.ini");
        klog("Loading ini from APP DIR\n");
        sprintf(set->opt[INI_PATH], "%s", "/user/app/NPXS39041/settings.ini");
    }

    /* Read the file */
    pl_ini_load(&file, set->opt[INI_PATH]);
    /* Load values */
    if(strstr(usbpath(), "/mnt/usb"))
        sprintf(set->opt[USB_PATH], "%s", usbpath());

    pl_ini_get_string(&file, "Settings", "CDN",      "http://api.pkg-zone.com",
                                 set->opt[CDN_URL],  256);
    pl_ini_get_string(&file, "Settings", "temppath", "/user/app/",
                                 set->opt[TMP_PATH], 256);
    pl_ini_get_string(&file, "Settings", "TTF_Font", "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF",
                                 set->opt[FNT_PATH], 256);
    // get an int
    set->StoreOnUSB = pl_ini_get_int(&file, "Settings", "StoreOnUSB", 0);

    klog("set->opt[INI_PATH]: %s\n", set->opt[INI_PATH]);
    klog("set->opt[USB_PATH]: %s\n", set->opt[USB_PATH]);
    klog("set->opt[FNT_PATH]: %s\n", set->opt[FNT_PATH]);
    klog("set->opt[TMP_PATH]: %s\n", set->opt[TMP_PATH]);
    klog("set->opt[CDN_URL ]: %s\n", set->opt[CDN_URL]);
    klog("set->StoreOnUSB   : %i\n", set->StoreOnUSB);

    /* Clean up */
    pl_ini_destroy(&file);

    return 1;
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

int SaveOptions(StoreOptions *set)
{
    /* Initialize INI structure */
    pl_ini_file file;
    int ret = 0;

    /* Read the file */
    if (ret = sceKernelOpen(set->opt[INI_PATH], 0x000, 0x0000) < 0) {
        pl_ini_create(&file);
        klog("INI NOT AVAIL Creating... \n");
    }
    else
       pl_ini_load(&file, set->opt[INI_PATH]);

    klog("set->opt[INI_PATH] ret %x: %s\n", ret, set->opt[INI_PATH]);

    klog("set->opt[USB_PATH]: %s\n", set->opt[USB_PATH]);
    klog("set->opt[TMP_PATH]: %s\n", set->opt[TMP_PATH]);
    klog("set->opt[FNT_PATH]: %.20s...\n", set->opt[FNT_PATH]);
    klog("set->opt[CDN_URL ]: %s\n", set->opt[CDN_URL ]);
    klog("set->StoreOnUSB   : %i\n", set->StoreOnUSB);

    /* Load values */
    pl_ini_set_string(&file, "Settings", "CDN",        set->opt[CDN_URL ]);
    pl_ini_set_string(&file, "Settings", "temppath",   set->opt[TMP_PATH]);
    pl_ini_set_string(&file, "Settings", "TTF_Font",   set->opt[FNT_PATH]);
    pl_ini_set_int   (&file, "Settings", "StoreOnUSB", set->StoreOnUSB);

    ret = pl_ini_save(&file, set->opt[INI_PATH]);
    chmod(set->opt[INI_PATH], 0777);

    /* Clean up */
    pl_ini_destroy(&file);

    return ret;
}


uint32_t SysctlByName_get_sdk_version(void)
{
    uint32_t sdkVersion = -1;
    size_t   len = 4;
    int      ret = sysctlbyname("kern.sdk_version", &sdkVersion, &len, NULL, 0);
//  printf("kern.sdk_version ret:%d, 0x%08X\n", ret, sdkVersion);
    return sdkVersion;
}

char *calculateSize(uint64_t size)
{
    char     *result = (char *)malloc(sizeof(char) * 20);
    uint64_t  multiplier = exbibytes;
    int i;

    for (i = 0; i < DIM(sizes); i++, multiplier /= 1024)
    {
        if (size < multiplier)
            continue;
        if (size % multiplier == 0)
            sprintf(result, "%" PRIu64 " %s", size / multiplier, sizes[i]);
        else
            sprintf(result, "%.1f %s", (float)size / multiplier, sizes[i]);
        return result;
    }
    strcpy(result, "0");
    return result;
}



void CalcAppsize(char *path)
{
    FILE *fp = NULL;
    long off;

    fp = fopen(path, "r");
    if (fp == NULL)
        printf("failed to fopen %s\n", path);
            
    printf("DEBUG: app fd = %s", path);

    if (fseek(fp, 0, SEEK_END) == -1)
        printf("DEBUG: failed to fseek %s\n", path);
            
    off = ftell(fp);
    if (off == (long)-1)
        printf("DEBUG: failed to ftell %s\n", path);
        
    
    printf("[*] fseek_filesize - file: %s, size: %ld\n", path, off);

    if (fclose(fp) != 0)
        printf("DEBUG: failed to fclose %s\n", path);

    
    if(off)
        checkedsize = calculateSize(off);
    else
        checkedsize = NULL;

}
//////////////////////////////////////////////////////




void CheckLogSize()
{


    CalcAppsize(STORE_LOG);

    printf("[LOG CHECK] Size  = %s\n", checkedsize);

    if (strstr(checkedsize, "GiB") != NULL)
               unlink(STORE_LOG);
        else
          printf("[LOG CHECK] File Is NOT 1 => GiB, Skipping Deletion\n");
    
}




void die(char* message)
{
    fprintf(ERROR, message);
    sceSystemServiceLoadExec("invaild", 0);
}

int msgok(int level, char* format, ...)
{
    if(strlen(format) > 300) return 0x1337;

    int ret = 0;
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    sceMsgDialogTerminate();


    char buff[300];
    char buffer[300];
    memset(&buff[0], 0,  sizeof(buff));


    va_list args;
    va_start(args, format);
    vsprintf(buff, format, args);
    va_end(args);

    switch (level)
    {
    case NORMAL:
        strcpy(buffer, buff);
        break;
    case FATAL:
        sprintf(buffer, "Fatal Error\n\n %s \n\n Please Close the Program \n after Pressing 'OK'", buff);
        break;
    case WARNING:
        sprintf(buffer, "Warning\n\n %s", &buff[0]);            
        break;
    }

    strcpy(buffer, buff);
    klog(buffer);

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
           klog("MsgD failed\n"); ret = -1; // this ret is outer if!
    
        
    

    OrbisCommonDialogStatus stat;

    while (1)
    {
        stat = sceMsgDialogUpdateStatus();
        if (stat == ORBIS_COMMON_DIALOG_STATUS_FINISHED)
        {
            OrbisMsgDialogResult result;
            memset(&result, 0, sizeof(result));

            if (0 > sceMsgDialogGetResult(&result))
                         klog("MsgD failed\n"); ret = -2;

            sceMsgDialogTerminate();
            break;
        }
    }


    switch (level)
    {
    case FATAL:
        die(&buff[0]);
        break;
    }

        klog("DEBUG: ret code %i\n", ret);
    return ret;
}

int loadmsg(char* format, ...)
{
    if(strlen(format) > 1024) return 0x1337;

    sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);

    sceMsgDialogInitialize();

    char buff[1024];
    memset(&buff[0], 0, sizeof(buff));

    va_list args;
    va_start(args, format);
    vsprintf(&buff[0], format, args);
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

    while (1) {
        stat = sceMsgDialogUpdateStatus();
        if (stat == ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
            break;
        }
    }

    return 0;
}

void klog(const char *format, ...)  
{
    if(strlen(format) < 300)
    {
        char buff[300];
        memset(&buff[0], 0, sizeof(buff));

        va_list args;
        va_start(args, format);
        vsprintf(&buff[0], format, args);
        va_end(args);

        printf("%s", buff);
         
        int fd = sceKernelOpen(STORE_LOG, O_WRONLY | O_CREAT | O_APPEND, 0777);
        if (fd >= 0)
        {
            if(Lastlogcheck == 10) {
                CheckLogSize(); Lastlogcheck = 0; }
            else
                Lastlogcheck++;
                 
            sceKernelWrite(fd, buff, strlen(buff));
            sceKernelClose(fd);
        }


    }
    else
        printf("DEBUG: input is too large!\n");

}

int getjson(int Pagenumb, char* cdn)
{
    char buff[300];
    char destbuf[300];

    snprintf(buff, 300, "%s/homebrew-page%i.json", cdn, Pagenumb);
    snprintf(destbuf, 300, "/user/app/NPXS39041/homebrew-page%i.json", Pagenumb);
    printf("%s", buff);

    return dl_from_url(buff, destbuf, false);
}
