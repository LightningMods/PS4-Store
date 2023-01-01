#include "defines.h"
#include "lang.h"
#ifdef __ORBIS__
#include <orbis/SystemService.h>
int32_t PS4GetLang() {
    int32_t lang;

    if (sceSystemServiceParamGetInt(1, &lang) != 0) {
        log_error("Unable to qet language code");
        return 0x1;
    }

    return lang;
}
#endif

LangStrings* stropts;
extern char* new_panel_text[LPANEL_Y][11];
extern char* download_panel_text[3];
char* gm_p_text[5];
char* option_panel_text[11];
char* group_label[8];

const char* group_ltext_non_pkg_zone[] =
{   // 0 is reserved index for: (label, total count)
    "Base Games",
    "Patches",
    "DLC",
    "Themes",
    "Apps",
    "Unknown",
    "Other"
};
extern bool unsafe_source;
bool lang_is_initialized = false;
//
//https://github.com/Al-Azif/ps4-payload-guest/blob/main/src/Language.cpp
//
const char* Language_GetName(int m_Code) {
    char* s_Name;

    switch (m_Code) {
    case 0:
        s_Name = "Japanese";
        break;
    case 1:
        s_Name = "English (United States)";
        break;
    case 2:
        s_Name = "French (France)";
        break;
    case 3:
        s_Name = "Spanish (Spain)";
        break;
    case 4:
        s_Name = "German";
        break;
    case 5:
        s_Name = "Italian";
        break;
    case 6:
        s_Name = "Dutch";
        break;
    case 7:
        s_Name = "Portuguese (Portugal)";
        break;
    case 8:
        s_Name = "Russian";
        break;
    case 9:
        s_Name = "Korean";
        break;
    case 10:
        s_Name = "Chinese (Traditional)";
        break;
    case 11:
        s_Name = "Chinese (Simplified)";
        break;
    case 12:
        s_Name = "Finnish";
        break;
    case 13:
        s_Name = "Swedish";
        break;
    case 14:
        s_Name = "Danish";
        break;
    case 15:
        s_Name = "Norwegian";
        break;
    case 16:
        s_Name = "Polish";
        break;
    case 17:
        s_Name = "Portuguese (Brazil)";
        break;
    case 18:
        s_Name = "English (United Kingdom)";
        break;
    case 19:
        s_Name = "Turkish";
        break;
    case 20:
        s_Name = "Spanish (Latin America)";
        break;
    case 21:
        s_Name = "Arabic";
        break;
    case 22:
        s_Name = "French (Canada)";
        break;
    case 23:
        s_Name = "Czech";
        break;
    case 24:
        s_Name = "Hungarian";
        break;
    case 25:
        s_Name = "Greek";
        break;
    case 26:
        s_Name = "Romanian";
        break;
    case 27:
        s_Name = "Thai";
        break;
    case 28:
        s_Name = "Vietnamese";
        break;
    case 29:
        s_Name = "Indonesian";
        break;
    default:
        s_Name = "UNKNOWN";
        break;
    }

    return s_Name;
}

const char* getLangSTR(enum Lang_STR str)
{
    if (lang_is_initialized) {
        if (str > LANG_NUM_OF_STRINGS) {
            return stropts->strings[STR_NOT_FOUND];
        }
        else
        {
            if (stropts->strings[str] != NULL)
                return stropts->strings[str];
            else
                return stropts->strings[STR_NOT_FOUND];
        }
    }
    else
        return "ERROR: Lang has not loaded";
}


static int load_lang_ini(void* user, const char* section, const char* name,
   const char* value, int lineno)
{
 
    LangStrings* set = (LangStrings*)user;

    //log_debug("%s %i", name, lineno);

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("STORE", "TRYING_TO_DL")) {
        snprintf(set->strings[TRYING_TO_DL2], 128, "%s", value);
    }
    else if (MATCH("STORE", "BETA_LOGGED_IN")) {
    snprintf(set->strings[BETA_LOGGED_IN], 128, "%s", value);
    }
    else if (MATCH("STORE", "BETA_REVOKED")) {
    snprintf(set->strings[BETA_REVOKED], 128, "%s", value);
    }
    if (MATCH("STORE", "WAITING_FOR_DAEMON")) {
        snprintf(set->strings[WAITING_FOR_DAEMON], 128, "%s", value);//
    }
    else if (MATCH("STORE", "SEARCHING")) {
        snprintf(set->strings[SEARCHING], 128, "%s", value);//
    }
    else if (MATCH("STORE", "ERROR_DL_ASSETS")) {
        snprintf(set->strings[ERROR_DL_ASSETS], 128, "%s", value);
    }
    if (MATCH("STORE", "DL_ERROR_PAGE")) {
        snprintf(set->strings[DL_ERROR_PAGE], 128, "%s", value);
    }
    else if (MATCH("STORE", "COUNT_NULL")) {
        snprintf(set->strings[COUNT_NULL], 128, "%s", value);
    }
    else if (MATCH("STORE", "ZERO_ITEMS")) {
        snprintf(set->strings[ZERO_ITEMS], 128, "%s", value);
    }
    if (MATCH("STORE", "EXCEED_LIMITS")) {
        snprintf(set->strings[EXCEED_LIMITS], 128, "%s", value);
    }
    else if (MATCH("STORE", "PKG_TEAM")) {
        snprintf(set->strings[PKG_TEAM], 128, "%s", value);
    }
    if (MATCH("STORE", "APP_DIED")) {
        snprintf(set->strings[APP_DIED], 128, "%s", value);
    }
    else if (MATCH("STORE", "CANT_OPEN")) {
        snprintf(set->strings[CANT_OPEN], 128, "%s", value);
    }
    else if (MATCH("STORE", "OBJ_EXPECTED")) {
        snprintf(set->strings[OBJ_EXPECTED], 128, "%s", value);
    }
    if (MATCH("STORE", "FAILED_TO_PARSE")) {
        snprintf(set->strings[FAILED_TO_PARSE], 128, "%s", value);
    }
    else if (MATCH("STORE", "SERVER_DIS")) {
        snprintf(set->strings[SERVER_DIS], 128, "%s", value);
    }
    else if (MATCH("STORE", "DL_FAILED2")) {
        snprintf(set->strings[DL_FAILED2], 128, "%s", value);
    }
    if (MATCH("STORE", "SAVE_SUCCESS")) {
        snprintf(set->strings[SAVE_SUCCESS], 128, "%s", value);
    }
    else if (MATCH("STORE", "SAVE_ERROR")) {
        snprintf(set->strings[SAVE_ERROR], 128, "%s", value);
    }
    else if (MATCH("STORE", "ITEMZ_FEATURE_DISABLED")) {
        snprintf(set->strings[ITEMZ_FEATURE_DISABLED], 128, "%s", value);
    }
    if (MATCH("STORE", "CACHE_FAILED")) {
        snprintf(set->strings[CACHE_FAILED], 128, "%s", value);
    }
    else if (MATCH("STORE", "CLEARING_CACHE")) {
        snprintf(set->strings[CLEARING_CACHE], 128, "%s", value);
    }
    else if (MATCH("STORE", "CACHE_CLEARED")) {
        snprintf(set->strings[CACHE_CLEARED], 128, "%s", value);
    }
    if (MATCH("STORE", "INVAL_TTF")) {
        snprintf(set->strings[INVAL_TTF], 128, "%s", value);
    }
    else if (MATCH("STORE", "INVAL_CDN")) {
        snprintf(set->strings[INVAL_CDN], 128, "%s", value);
    }
    else if (MATCH("STORE", "PKG_SUF")) {
        snprintf(set->strings[PKG_SUF], 128, "%s", value);
    }
    if (MATCH("STORE", "INVAL_PATH")) {
        snprintf(set->strings[INVAL_PATH], 128, "%s", value);
    }
    else if (MATCH("STORE", "STR_TOO_LONG")) {
        snprintf(set->strings[STR_TOO_LONG], 128, "%s", value);
    }
    else if (MATCH("STORE", "NEW_INI")) {
        snprintf(set->strings[NEW_INI], 128, "%s", value);
    }
    if (MATCH("STORE", "NOT_ON_APP")) {
        snprintf(set->strings[NOT_ON_APP], 128, "%s", value);
    }
    else if (MATCH("STORE", "DL_FAILED_W")) {
        snprintf(set->strings[DL_FAILED_W], 128, "%s", value);
    }
    else if (MATCH("STORE", "STR_NOT_FOUND")) {
        snprintf(set->strings[STR_NOT_FOUND], 128, "%s", value);
    }
    else if (MATCH("STORE", "UNINSTALL_FAILED")) {
        snprintf(set->strings[UNINSTALL_FAILED], 128, "%s", value);
    }
    else if (MATCH("STORE", "UNINSTALL_SUCCESS")) {
        snprintf(set->strings[UNINSTALL_SUCCESS], 128, "%s", value);
    }
    if (MATCH("STORE", "UNINSTAL_UPDATE_FAILED")) {
        snprintf(set->strings[UNINSTAL_UPDATE_FAILED], 128, "%s", value);
    }
    else if (MATCH("STORE", "UNINSTALL_UPDATE")) {
        snprintf(set->strings[UNINSTALL_UPDATE], 128, "%s", value);
    }
    else if (MATCH("STORE", "DUMP")) {
        snprintf(set->strings[DUMP], 128, "%s", value);
    }
    if (MATCH("STORE", "NO_USB")) {
        snprintf(set->strings[NO_USB], 128, "%s", value);
    }
    else if (MATCH("STORE", "PIG_FAIL")) {
        snprintf(set->strings[PIG_FAIL], 128, "%s", value);
    }
    else if (MATCH("STORE", "DL_CACHE")) {
        snprintf(set->strings[DL_CACHE], 128, "%s", value);
    }
    else if (MATCH("STORE", "INI_ERROR")) {
        snprintf(set->strings[INI_ERROR], 128, "%s", value);
    }
    if (MATCH("STORE", "SWITCH_TO_EM")) {
        snprintf(set->strings[SWITCH_TO_EM], 128, "%s", value);
    }
    else if (MATCH("STORE", "FAILED_TTF")) {
        snprintf(set->strings[FAILED_TTF], 128, "%s", value);
    }
    else if (MATCH("STORE", "WARNING")) {
        snprintf(set->strings[WARNING2], 128, "%s", value);
    }
    if (MATCH("STORE", "PRESS_OK_CLOSE")) {
        snprintf(set->strings[PRESS_OK_CLOSE], 128, "%s", value);
    }
    else if (MATCH("STORE", "FATAL_ERROR")) {
        snprintf(set->strings[FATAL_ERROR], 128, "%s", value);
    }
    else if (MATCH("STORE", "FAILED_W_CODE")) {
        snprintf(set->strings[FAILED_W_CODE], 128, "%s", value);
    }
    if (MATCH("STORE", "COMPLETE")) {
        snprintf(set->strings[COMPLETE], 128, "%s", value);
    }
    else if (MATCH("STORE", "INSTALL_OF")) {
        snprintf(set->strings[INSTALL_OF], 128, "%s", value);
    }
    else if (MATCH("STORE", "TIP1")) {
        snprintf(set->strings[TIP1], 128, "%s", value);
    }
    if (MATCH("STORE", "INSTALL_ONGOING")) {
        snprintf(set->strings[INSTALL_ONGOING], 128, "%s", value);
    }
    else if (MATCH("STORE", "INSTALL_FAILED")) {
        snprintf(set->strings[INSTALL_FAILED], 128, "%s", value);
    }
    else if (MATCH("STORE", "NUMB_OF_DL")) {
        snprintf(set->strings[NUMB_OF_DL], 128, "%s", value);
        new_panel_text[3][10] = set->strings[NUMB_OF_DL];
    }
    if (MATCH("STORE", "RDATE")) {
        snprintf(set->strings[RDATE], 128, "%s", value);
        new_panel_text[3][9] = set->strings[RDATE];
    }
    else if (MATCH("STORE", "PV")) {
        snprintf(set->strings[PV2], 128, "%s", value);
        new_panel_text[3][8] = set->strings[PV2];
        new_panel_text[4][0] = set->strings[PV2];
    }
    else if (MATCH("STORE", "TYPE")) {
        snprintf(set->strings[TYPE2], 128, "%s", value);
        new_panel_text[3][7] = set->strings[TYPE2];

    }
    if (MATCH("STORE", "AUTHOR")) {
        snprintf(set->strings[AUTHOR2], 128, "%s", value);
        new_panel_text[4][1] = set->strings[AUTHOR2];
        new_panel_text[3][6] = set->strings[AUTHOR2];
    }
    else if (MATCH("STORE", "SIZE")) {
        snprintf(set->strings[SIZE2], 128, "%s", value);
        new_panel_text[3][5] = set->strings[SIZE2];
    }
    else if (MATCH("STORE", "RSTARS")) {
        snprintf(set->strings[RSTARS], 128, "%s", value);
        new_panel_text[3][4] = set->strings[RSTARS];
    }
    if (MATCH("STORE", "VER")) {
        snprintf(set->strings[VER], 128, "%s", value);
        new_panel_text[3][3] = set->strings[VER];
    }
    else if (MATCH("STORE", "PACKAGE")) {
        snprintf(set->strings[PACKAGE2], 128, "%s", value);
        new_panel_text[3][2] = set->strings[PACKAGE2];
    }
    else if (MATCH("STORE", "NAME")) {
        snprintf(set->strings[NAME2], 128, "%s", value);
        new_panel_text[3][1] = set->strings[NAME2];
    }
    if (MATCH("STORE", "ID")) {
        snprintf(set->strings[ID2], 128, "%s", value);
        new_panel_text[3][0] = set->strings[ID2];
    }
    else if (MATCH("STORE", "FILTER_BY")) {
        snprintf(set->strings[FILTER_BY], 128, "%s", value);
        new_panel_text[1][2] = set->strings[FILTER_BY];
    }
    else if (MATCH("STORE", "SORT_BY")) {
        snprintf(set->strings[SORT_BY], 128, "%s", value);
        new_panel_text[1][1] = set->strings[SORT_BY];

    }
    if (MATCH("STORE", "SEARCH")) {
        snprintf(set->strings[SEARCH], 128, "%s", value);
        new_panel_text[1][0] = set->strings[SEARCH];
    }
    else if (MATCH("STORE", "SETTINGS")) {
        snprintf(set->strings[SETTINGS], 128, "%s", value);
        new_panel_text[0][6] = set->strings[SETTINGS];
    }
    else if (MATCH("STORE", "UPDATES")) {
        snprintf(set->strings[UPDATES], 128, "%s", value);
        new_panel_text[0][5] = set->strings[UPDATES];
    }
    if (MATCH("STORE", "QUEUE")) {
        snprintf(set->strings[QUEUE], 128, "%s", value);
        new_panel_text[0][4] = set->strings[QUEUE];
    }
    else if (MATCH("STORE", "RINSTALL")) {
        snprintf(set->strings[RINSTALL], 128, "%s", value);
        new_panel_text[0][3] = set->strings[RINSTALL];
    }
    else if (MATCH("STORE", "STRG")) {
        snprintf(set->strings[STRG], 128, "%s", value);
        new_panel_text[0][2] = set->strings[STRG];
    }
    if (MATCH("STORE", "IAPPS")) {
        snprintf(set->strings[IAPPS], 128, "%s", value);
        new_panel_text[0][1] = set->strings[IAPPS];
    }
    else if (MATCH("STORE", "SAPPS")) {
        snprintf(set->strings[SAPPS], 128, "%s", value);
        new_panel_text[0][0] = set->strings[SAPPS];
    }
    else if (MATCH("STORE", "INSTALLING")) {
        snprintf(set->strings[INSTALLING], 128, "%s", value);
    }
    if (MATCH("STORE", "MORE")) {
        snprintf(set->strings[MORE], 128, "%s", value);
    }
    else if (MATCH("STORE", "INSTALL")) {
        snprintf(set->strings[INSTALL2], 128, "%s", value);
        download_panel_text[1] = set->strings[INSTALL2];

    }
    else if (MATCH("STORE", "DL")) {
        snprintf(set->strings[DL2], 128, "%s", value);
        download_panel_text[0] = set->strings[DL2];
    }
    if (MATCH("STORE", "OTHER")) {
        snprintf(set->strings[OTHER], 128, "%s", value);
        group_label[6] = set->strings[OTHER];
    }
    else if (MATCH("STORE", "UTLIITY")) {
        snprintf(set->strings[UTLIITY], 128, "%s", value);
        group_label[5] = set->strings[UTLIITY];
    }
    else if (MATCH("STORE", "PLUGINS")) {
        snprintf(set->strings[PLUGINS], 128, "%s", value);
        group_label[4] = set->strings[PLUGINS];

    }
    if (MATCH("STORE", "MEDIA")) {
        snprintf(set->strings[MEDIA], 128, "%s", value);
        group_label[3] = set->strings[MEDIA];

    }
    else if (MATCH("STORE", "EMU_ADDON")) {
        snprintf(set->strings[EMU_ADDON], 128, "%s", value);
        group_label[2] = set->strings[EMU_ADDON];
    }
    else if (MATCH("STORE", "EMU")) {
        snprintf(set->strings[EMU], 128, "%s", value);
        group_label[1] = set->strings[EMU];
    }
    if (MATCH("STORE", "HB_GAME")) {
        snprintf(set->strings[HB_GAME], 128, "%s", value);
        group_label[0] = set->strings[HB_GAME];
    }
    else if (MATCH("STORE", "SYS_VER")) {
        snprintf(set->strings[SYS_VER], 128, "%s", value);
    }
    else if (MATCH("STORE", "STORE_VER")) {
        snprintf(set->strings[STORE_VER], 128, "%s", value);
    }
    if (MATCH("STORE", "STORAGE")) {
        snprintf(set->strings[STORAGE], 128, "%s", value);
    }
    else if (MATCH("STORE", "PAGE")) {
        snprintf(set->strings[PAGE2], 128, "%s", value);
    }
    else if (MATCH("STORE", "DOWNLOADING")) {
        snprintf(set->strings[DOWNLOADING], 128, "%s", value);
    }
    else if (MATCH("LOADER", "UPDATE_REQ")) {
        snprintf(set->strings[UPDATE_REQ], 128, "%s", value);
    }
    if (MATCH("STORE", "DL_CANCELLED")) {
        snprintf(set->strings[DL_CANCELLED], 128, "%s", value);
    }
    else if (MATCH("STORE", "DL_COMPLETE")) {
        snprintf(set->strings[DL_COMPLETE], 128, "%s", value);
    }
    else if (MATCH("STORE", "DL_ERROR")) {
        snprintf(set->strings[DL_ERROR2], 128, "%s", value);
    }
    if (MATCH("STORE", "FALSE")) {
        snprintf(set->strings[FALSE2], 128, "%s", value);
    }
    else if (MATCH("STORE", "TRUE")) {
        snprintf(set->strings[TRUE2], 128, "%s", value);
    }
    else if (MATCH("STORE", "OFF")) {
        snprintf(set->strings[OFF2], 128, "%s", value);
    }
    if (MATCH("STORE", "ON")) {
        snprintf(set->strings[ON2], 128, "%s", value);
    }
    else if (MATCH("STORE", "INERNET_REQ")) {
        snprintf(set->strings[INERNET_REQ], 128, "%s", value);
    }
    else if (MATCH("STORE", "SETTINGS_1")) {
        snprintf(set->strings[SETTINGS_1], 128, "%s", value);
        option_panel_text[0] = set->strings[SETTINGS_1];
    }
    else if (MATCH("STORE", "SETTINGS_2")) {
        snprintf(set->strings[SETTINGS_2], 128, "%s", value);
        option_panel_text[1] = set->strings[SETTINGS_2];
    }
    else if (MATCH("STORE", "SETTINGS_3")) {
        snprintf(set->strings[SETTINGS_3], 128, "%s", value);
        option_panel_text[2] = set->strings[SETTINGS_3];
    }
    else if (MATCH("STORE", "SETTINGS_4")) {
        snprintf(set->strings[SETTINGS_4], 128, "%s", value);
        option_panel_text[3] = set->strings[SETTINGS_4];
    }
    else if (MATCH("STORE", "SETTINGS_5")) {
        snprintf(set->strings[SETTINGS_5], 128, "%s", value);
        option_panel_text[4] = set->strings[SETTINGS_5];
    }
    else if (MATCH("STORE", "SETTINGS_6")) {
        snprintf(set->strings[SETTINGS_6], 128, "%s", value);
        option_panel_text[5] = set->strings[SETTINGS_6];
    }
    else if (MATCH("STORE", "SETTINGS_7")) {
        snprintf(set->strings[SETTINGS_7], 128, "%s", value);
        option_panel_text[6] = set->strings[SETTINGS_7];
    }
    else if (MATCH("STORE", "SETTINGS_8")) {
        snprintf(set->strings[SETTINGS_8], 128, "%s", value);
        option_panel_text[7] = set->strings[SETTINGS_8];
    }
    else if (MATCH("STORE", "SETTINGS_9")) {
        snprintf(set->strings[SETTINGS_9], 128, "%s", value);
        option_panel_text[8] = set->strings[SETTINGS_9];
    }
    else if (MATCH("STORE", "SETTINGS_10")) {
        snprintf(set->strings[SETTINGS_10], 128, "%s", value);
        option_panel_text[9] = set->strings[SETTINGS_10];
    }
    else if (MATCH("STORE", "AUTO_FAILURE_ERROR")) {
        snprintf(set->strings[AUTO_FAILURE_ERROR], 128, "%s", value);
    }
    else if (MATCH("STORE", "INSTALL_PROG_ERROR")) {
        snprintf(set->strings[INSTALL_PROG_ERROR], 128, "%s", value);
    }
    else if (MATCH("STORE", "CANCEL")) {
        snprintf(set->strings[CANCEL], 128, "%s", value);
        download_panel_text[2] = set->strings[CANCEL];
    }
    else if (MATCH("STORE", "DL_AND_IN")) {
        snprintf(set->strings[DL_AND_IN], 128, "%s", value);
    }
    else if (MATCH("STORE", "PAUSE_2")){
        snprintf(set->strings[PAUSE_2], 128, "%s", value);
    }
    else if (MATCH("STORE", "RESUME_2")){
        snprintf(set->strings[RESUME_2], 128, "%s", value);
    }
     else if (MATCH("STORE", "APP_UPDATE_AVAIL")){
        snprintf(set->strings[APP_UPDATE_AVAIL], 128, "%s", value);
    }
    else if (MATCH("STORE", "UPDATE_NOW")){
        snprintf(set->strings[UPDATE_NOW], 128, "%s", value);
    }
    else if(MATCH("STORE", "REINSTALL_APP")){
        snprintf(set->strings[REINSTALL_APP], 128, "%s", value);
    }

    return 1;

}
#ifdef __ORBIS__
extern uint8_t lang_ini[];
extern int32_t lang_ini_sz;

bool load_embdded_eng()
{
    //mem should already be allocated as this is the last opt
    int fd = -1;
    if (stropts != NULL) {
        if (!if_exists("/user/app/NPXS39041/lang.ini")) {
            if ((fd = open("/user/app/NPXS39041/lang.ini", O_WRONLY | O_CREAT | O_TRUNC, 0777)) > 0 && fd != -1) {
                write(fd, lang_ini, lang_ini_sz);
                close(fd);
            }
            else
               return false;
        }

        int error = ini_parse("/user/app/NPXS39041/lang.ini", load_lang_ini, stropts);
        if (error) {
            log_error("Bad config file (first error on line %d)!\n", error);
            return false;
        }
        else
            lang_is_initialized = true;
    }
    else
        return false;

    return true;
}
#endif
bool LoadLangs(int LangCode)
{
#ifndef TEST_INI_LANGS
    if (!lang_is_initialized)
#else
    if (1)
#endif
    {
        //dont leak or realloc on fail
        if (stropts == NULL) 
            stropts = (LangStrings*)malloc(sizeof(LangStrings));

        if (stropts != NULL) {
            for (int i = 0; i < LANG_NUM_OF_STRINGS; i++) {
                stropts->strings[i] = calloc(128, sizeof(char));
                if (stropts->strings[i] == NULL) {
                    msgok(WARNING, "Failed to load Language(s)... Failed to allocate stropts->strings[%i]", i);
                    return false;
                }
            }
            char tmp[128];
            #ifdef __ORBIS__
            snprintf(tmp, 128, LANG_DIR, LangCode);
            #else
            strcpy(tmp, asset_path("lang.ini"));
            #endif

            int error = ini_parse(tmp, load_lang_ini, stropts);
            if (error) {
                log_error("Bad config file (first error on line %d)!\n", error);
                return false;
            }
            else
                lang_is_initialized = true;

            if(unsafe_source){
               for(int i = 0; i < 7; i++){
                   strcpy(group_label[i], group_ltext_non_pkg_zone[i]);
                   //log_info("%i", i);
                   //log_info("Setting group_label[%d] to %s", i, group_ltext_non_pkg_zone[i]);
               }
            }
            //DONT BREAK COMPATIBILITY WITH OLDER STORE PKGS
            //THAT DONT HAVE THESE STRINGS
            if(strlen(stropts->strings[CANCEL]) < 3){
               //UPDATE 2.3, NEW SETTING
               strcpy(stropts->strings[SETTINGS_6], "Automatically install");
               strcpy(stropts->strings[AUTO_FAILURE_ERROR], "Show Install Progress is Required to be Disabled to Enable this Setting");
               strcpy(stropts->strings[INSTALL_PROG_ERROR], "Auto Install is Required to be Disabled to Enable this Setting");

               //UPDATE 2.4, NEW 2 SETTINGS
               strcpy(stropts->strings[CANCEL], "Cancel");
               strcpy(stropts->strings[DL_AND_IN], "Download and Install");
               strcpy(stropts->strings[PAUSE_2], "Pause");
               strcpy(stropts->strings[RESUME_2], "Resume");

               strcpy(stropts->strings[SETTINGS_3], "Refresh Store Database");
               strcpy(stropts->strings[SETTINGS_9], "Reset Store Settings");
               strcpy(stropts->strings[APP_UPDATE_AVAIL], "Update Available");
               strcpy(stropts->strings[UPDATE_NOW], "Update Now");
               strcpy(stropts->strings[REINSTALL_APP], "Reinstall Latest");
               download_panel_text[2] = stropts->strings[CANCEL];

            }
        }
        else
            return false;
    }
	return true;
}