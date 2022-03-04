#include "defines.h"
#include "lang.h"

LangStrings* stropts;
extern char* new_panel_text[LPANEL_Y][11];
extern char* download_panel_text[3];
char* gm_p_text[5];
char* option_panel_text[11];
char* group_label[8];
bool lang_is_initialized = false;
//
//https://github.com/Al-Azif/ps4-payload-guest/blob/main/src/Language.cpp
//
char* Language_GetName(int m_Code) {
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

int32_t PS4GetLang() {
    int32_t lang;

    if (sceSystemServiceParamGetInt(1, &lang) != 0) {
        log_error("Unable to qet language code");
        return 0x1;
    }

    return lang;
}

char* getLangSTR(enum Lang_STR str)
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

    if (MATCH("STORE", "STARTING_DUMPER")) {
        snprintf(set->strings[STARTING_DUMPER], 255, "%s", value);
    }
    else if (MATCH("STORE", "DL_COVERS")) {
        snprintf(set->strings[DL_COVERS], 255, "%s", value);
    }
    else if (MATCH("STORE", "TRYING_TO_DL")) {
        snprintf(set->strings[TRYING_TO_DL2], 255, "%s", value);
    }
    else if (MATCH("STORE", "BETA_LOGGED_IN")) {
    snprintf(set->strings[BETA_LOGGED_IN], 255, "%s", value);
    }
    else if (MATCH("STORE", "BETA_REVOKED")) {
    snprintf(set->strings[BETA_REVOKED], 255, "%s", value);
    }
    if (MATCH("STORE", "WAITING_FOR_DAEMON")) {
        snprintf(set->strings[WAITING_FOR_DAEMON], 255, "%s", value);//
    }
    else if (MATCH("STORE", "SEARCHING")) {
        snprintf(set->strings[SEARCHING], 255, "%s", value);//
    }
    else if (MATCH("STORE", "RELOAD_LIST")) {
        snprintf(set->strings[RELOAD_LIST], 255, "%s", value);
    }
    if (MATCH("STORE", "LAUNCH_ERROR")) {
        snprintf(set->strings[LAUNCH_ERROR], 255, "%s", value);
    }
    else if (MATCH("STORE", "ID_NOT_VAILD")) {
        snprintf(set->strings[ID_NOT_VAILD], 255, "%s", value);
    }
    else if (MATCH("STORE", "ID_NOT_VAILD")) {
        snprintf(set->strings[ID_NOT_VAILD], 255, "%s", value);
    }
    if (MATCH("STORE", "APP_OPENED")) {
        snprintf(set->strings[APP_OPENED], 255, "%s", value);
    }
    else if (MATCH("STORE", "APP_NOT_FOUND")) {
        snprintf(set->strings[APP_NOT_FOUND], 255, "%s", value);
    }
    else if (MATCH("STORE", "ERROR_DL_ASSETS")) {
        snprintf(set->strings[ERROR_DL_ASSETS], 255, "%s", value);
    }
    if (MATCH("STORE", "DL_ERROR_PAGE")) {
        snprintf(set->strings[DL_ERROR_PAGE], 255, "%s", value);
    }
    else if (MATCH("STORE", "COUNT_NULL")) {
        snprintf(set->strings[COUNT_NULL], 255, "%s", value);
    }
    else if (MATCH("STORE", "ZERO_ITEMS")) {
        snprintf(set->strings[ZERO_ITEMS], 255, "%s", value);
    }
    if (MATCH("STORE", "EXCEED_LIMITS")) {
        snprintf(set->strings[EXCEED_LIMITS], 255, "%s", value);
    }
    else if (MATCH("STORE", "DUMP_FAILED")) {
        snprintf(set->strings[DUMP_FAILED], 255, "%s", value);
    }
    else if (MATCH("STORE", "COMPLETE_WO_ERRORS")) {
        snprintf(set->strings[COMPLETE_WO_ERRORS], 255, "%s", value);
    }
    if (MATCH("STORE", "DUMP_OF")) {
        snprintf(set->strings[DUMP_OF], 255, "%s", value);
    }
    else if (MATCH("STORE", "FATAL_DUMP_ERROR")) {
        snprintf(set->strings[FATAL_DUMP_ERROR], 255, "%s", value);
    }
    else if (MATCH("STORE", "PKG_TEAM")) {
        snprintf(set->strings[PKG_TEAM], 255, "%s", value);
    }
    if (MATCH("STORE", "APP_DIED")) {
        snprintf(set->strings[APP_DIED], 255, "%s", value);
    }
    else if (MATCH("STORE", "CANT_OPEN")) {
        snprintf(set->strings[CANT_OPEN], 255, "%s", value);
    }
    else if (MATCH("STORE", "OBJ_EXPECTED")) {
        snprintf(set->strings[OBJ_EXPECTED], 255, "%s", value);
    }
    if (MATCH("STORE", "FAILED_TO_PARSE")) {
        snprintf(set->strings[FAILED_TO_PARSE], 255, "%s", value);
    }
    else if (MATCH("STORE", "SERVER_DIS")) {
        snprintf(set->strings[SERVER_DIS], 255, "%s", value);
    }
    else if (MATCH("STORE", "DL_FAILED2")) {
        snprintf(set->strings[DL_FAILED2], 255, "%s", value);
    }
    if (MATCH("STORE", "SAVE_SUCCESS")) {
        snprintf(set->strings[SAVE_SUCCESS], 255, "%s", value);
    }
    else if (MATCH("STORE", "SAVE_ERROR")) {
        snprintf(set->strings[SAVE_ERROR], 255, "%s", value);
    }
    else if (MATCH("STORE", "ITEMZ_FEATURE_DISABLED")) {
        snprintf(set->strings[ITEMZ_FEATURE_DISABLED], 255, "%s", value);
    }
    if (MATCH("STORE", "CACHE_FAILED")) {
        snprintf(set->strings[CACHE_FAILED], 255, "%s", value);
    }
    else if (MATCH("STORE", "CLEARING_CACHE")) {
        snprintf(set->strings[CLEARING_CACHE], 255, "%s", value);
    }
    else if (MATCH("STORE", "CACHE_CLEARED")) {
        snprintf(set->strings[CACHE_CLEARED], 255, "%s", value);
    }
    if (MATCH("STORE", "INVAL_TTF")) {
        snprintf(set->strings[INVAL_TTF], 255, "%s", value);
    }
    else if (MATCH("STORE", "INVAL_CDN")) {
        snprintf(set->strings[INVAL_CDN], 255, "%s", value);
    }
    else if (MATCH("STORE", "PKG_SUF")) {
        snprintf(set->strings[PKG_SUF], 255, "%s", value);
    }
    if (MATCH("STORE", "INVAL_PATH")) {
        snprintf(set->strings[INVAL_PATH], 255, "%s", value);
    }
    else if (MATCH("STORE", "STR_TOO_LONG")) {
        snprintf(set->strings[STR_TOO_LONG], 255, "%s", value);
    }
    else if (MATCH("STORE", "NEW_INI")) {
        snprintf(set->strings[NEW_INI], 255, "%s", value);
    }
    if (MATCH("STORE", "NOT_ON_APP")) {
        snprintf(set->strings[NOT_ON_APP], 255, "%s", value);
    }
    else if (MATCH("STORE", "DL_FAILED_W")) {
        snprintf(set->strings[DL_FAILED_W], 255, "%s", value);
    }
    else if (MATCH("STORE", "DOWNLOADING_COVERS")) {
        snprintf(set->strings[DOWNLOADING_COVERS], 255, "%s", value);
    }
    else if (MATCH("STORE", "STR_NOT_FOUND")) {
        snprintf(set->strings[STR_NOT_FOUND], 255, "%s", value);
    }
    if (MATCH("STORE", "MORE_APPS")) {
        snprintf(set->strings[MORE_APPS], 255, "%s", value);
    }
    else if (MATCH("STORE", "APP_REQ")) {
        snprintf(set->strings[APP_REQ], 255, "%s", value);
    }
    else if (MATCH("STORE", "RELOAD_IAPPS")) {
        snprintf(set->strings[RELOAD_IAPPS], 255, "%s", value);
    }
    if (MATCH("STORE", "COMING_SOON")) {
        snprintf(set->strings[COMING_SOON], 255, "%s", value);
    }
    else if (MATCH("STORE", "UNINSTALL_FAILED")) {
        snprintf(set->strings[UNINSTALL_FAILED], 255, "%s", value);
    }
    else if (MATCH("STORE", "UNINSTALL_SUCCESS")) {
        snprintf(set->strings[UNINSTALL_SUCCESS], 255, "%s", value);
    }
    if (MATCH("STORE", "UNINSTAL_UPDATE_FAILED")) {
        snprintf(set->strings[UNINSTAL_UPDATE_FAILED], 255, "%s", value);
    }
    else if (MATCH("STORE", "UNINSTALL_UPDATE")) {
        snprintf(set->strings[UNINSTALL_UPDATE], 255, "%s", value);
    }
    else if (MATCH("STORE", "DUMP")) {
        snprintf(set->strings[DUMP], 255, "%s", value);
    }
    if (MATCH("STORE", "NO_USB")) {
        snprintf(set->strings[NO_USB], 255, "%s", value);
    }
    else if (MATCH("STORE", "PIG_FAIL")) {
        snprintf(set->strings[PIG_FAIL], 255, "%s", value);
    }
    else if (MATCH("STORE", "DL_CACHE")) {
        snprintf(set->strings[DL_CACHE], 255, "%s", value);
    }
    if (MATCH("STORE", "DAEMON_OFF")) {
        snprintf(set->strings[DAEMON_OFF], 255, "%s", value);
    }
    else if (MATCH("STORE", "FAILED_DAEMON")) {
        snprintf(set->strings[FAILED_DAEMON], 255, "%s", value);
    }
    else if (MATCH("STORE", "INI_ERROR")) {
        snprintf(set->strings[INI_ERROR], 255, "%s", value);
    }
    if (MATCH("STORE", "SWITCH_TO_EM")) {
        snprintf(set->strings[SWITCH_TO_EM], 255, "%s", value);
    }
    else if (MATCH("STORE", "FAILED_TTF")) {
        snprintf(set->strings[FAILED_TTF], 255, "%s", value);
    }
    else if (MATCH("STORE", "WARNING")) {
        snprintf(set->strings[WARNING2], 255, "%s", value);
    }
    if (MATCH("STORE", "PRESS_OK_CLOSE")) {
        snprintf(set->strings[PRESS_OK_CLOSE], 255, "%s", value);
    }
    else if (MATCH("STORE", "FATAL_ERROR")) {
        snprintf(set->strings[FATAL_ERROR], 255, "%s", value);
    }
    else if (MATCH("STORE", "FAILED_W_CODE")) {
        snprintf(set->strings[FAILED_W_CODE], 255, "%s", value);
    }
    if (MATCH("STORE", "COMPLETE")) {
        snprintf(set->strings[COMPLETE], 255, "%s", value);
    }
    else if (MATCH("STORE", "INSTALL_OF")) {
        snprintf(set->strings[INSTALL_OF], 255, "%s", value);
    }
    else if (MATCH("STORE", "TIP1")) {
        snprintf(set->strings[TIP1], 255, "%s", value);
    }
    if (MATCH("STORE", "INSTALL_ONGOING")) {
        snprintf(set->strings[INSTALL_ONGOING], 255, "%s", value);
    }
    else if (MATCH("STORE", "INSTALL_FAILED")) {
        snprintf(set->strings[INSTALL_FAILED], 255, "%s", value);
    }
    else if (MATCH("STORE", "NUMB_OF_DL")) {
        snprintf(set->strings[NUMB_OF_DL], 255, "%s", value);
        new_panel_text[3][10] = set->strings[NUMB_OF_DL];
    }
    if (MATCH("STORE", "RDATE")) {
        snprintf(set->strings[RDATE], 255, "%s", value);
        new_panel_text[3][9] = set->strings[RDATE];
    }
    else if (MATCH("STORE", "PV")) {
        snprintf(set->strings[PV2], 255, "%s", value);
        new_panel_text[3][8] = set->strings[PV2];
        new_panel_text[4][0] = set->strings[PV2];
    }
    else if (MATCH("STORE", "TYPE")) {
        snprintf(set->strings[TYPE2], 255, "%s", value);
        new_panel_text[3][7] = set->strings[TYPE2];

    }
    if (MATCH("STORE", "AUTHOR")) {
        snprintf(set->strings[AUTHOR2], 255, "%s", value);
        new_panel_text[4][1] = set->strings[AUTHOR2];
        new_panel_text[3][6] = set->strings[AUTHOR2];
    }
    else if (MATCH("STORE", "SIZE")) {
        snprintf(set->strings[SIZE2], 255, "%s", value);
        new_panel_text[3][5] = set->strings[SIZE2];
    }
    else if (MATCH("STORE", "RSTARS")) {
        snprintf(set->strings[RSTARS], 255, "%s", value);
        new_panel_text[3][4] = set->strings[RSTARS];
    }
    if (MATCH("STORE", "VER")) {
        snprintf(set->strings[VER], 255, "%s", value);
        new_panel_text[3][3] = set->strings[VER];
    }
    else if (MATCH("STORE", "PACKAGE")) {
        snprintf(set->strings[PACKAGE2], 255, "%s", value);
        new_panel_text[3][2] = set->strings[PACKAGE2];
    }
    else if (MATCH("STORE", "NAME")) {
        snprintf(set->strings[NAME2], 255, "%s", value);
        new_panel_text[3][1] = set->strings[NAME2];
    }
    if (MATCH("STORE", "ID")) {
        snprintf(set->strings[ID2], 255, "%s", value);
        new_panel_text[3][0] = set->strings[ID2];
    }
    else if (MATCH("STORE", "FILTER_BY")) {
        snprintf(set->strings[FILTER_BY], 255, "%s", value);
        new_panel_text[1][2] = set->strings[FILTER_BY];
    }
    else if (MATCH("STORE", "SORT_BY")) {
        snprintf(set->strings[SORT_BY], 255, "%s", value);
        new_panel_text[1][1] = set->strings[SORT_BY];

    }
    if (MATCH("STORE", "SEARCH")) {
        snprintf(set->strings[SEARCH], 255, "%s", value);
        new_panel_text[1][0] = set->strings[SEARCH];
    }
    else if (MATCH("STORE", "SETTINGS")) {
        snprintf(set->strings[SETTINGS], 255, "%s", value);
        new_panel_text[0][6] = set->strings[SETTINGS];
    }
    else if (MATCH("STORE", "UPDATES")) {
        snprintf(set->strings[UPDATES], 255, "%s", value);
        new_panel_text[0][5] = set->strings[UPDATES];
    }
    if (MATCH("STORE", "QUEUE")) {
        snprintf(set->strings[QUEUE], 255, "%s", value);
        new_panel_text[0][4] = set->strings[QUEUE];
    }
    else if (MATCH("STORE", "RINSTALL")) {
        snprintf(set->strings[RINSTALL], 255, "%s", value);
        new_panel_text[0][3] = set->strings[RINSTALL];
    }
    else if (MATCH("STORE", "STRG")) {
        snprintf(set->strings[STRG], 255, "%s", value);
        new_panel_text[0][2] = set->strings[STRG];
    }
    if (MATCH("STORE", "IAPPS")) {
        snprintf(set->strings[IAPPS], 255, "%s", value);
        new_panel_text[0][1] = set->strings[IAPPS];
    }
    else if (MATCH("STORE", "SAPPS")) {
        snprintf(set->strings[SAPPS], 255, "%s", value);
        new_panel_text[0][0] = set->strings[SAPPS];
    }
    else if (MATCH("STORE", "INSTALLING")) {
        snprintf(set->strings[INSTALLING], 255, "%s", value);
    }
    if (MATCH("STORE", "MORE")) {
        snprintf(set->strings[MORE], 255, "%s", value);
        download_panel_text[2] = set->strings[MORE];
    }
    else if (MATCH("STORE", "INSTALL")) {
        snprintf(set->strings[INSTALL2], 255, "%s", value);
        download_panel_text[1] = set->strings[INSTALL2];

    }
    else if (MATCH("STORE", "DL")) {
        snprintf(set->strings[DL2], 255, "%s", value);
        download_panel_text[0] = set->strings[DL2];
    }
    if (MATCH("STORE", "OTHER")) {
        snprintf(set->strings[OTHER], 255, "%s", value);
        group_label[6] = set->strings[OTHER];
    }
    else if (MATCH("STORE", "UTLIITY")) {
        snprintf(set->strings[UTLIITY], 255, "%s", value);
        group_label[5] = set->strings[UTLIITY];
    }
    else if (MATCH("STORE", "PLUGINS")) {
        snprintf(set->strings[PLUGINS], 255, "%s", value);
        group_label[4] = set->strings[PLUGINS];

    }
    if (MATCH("STORE", "MEDIA")) {
        snprintf(set->strings[MEDIA], 255, "%s", value);
        group_label[3] = set->strings[MEDIA];

    }
    else if (MATCH("STORE", "EMU_ADDON")) {
        snprintf(set->strings[EMU_ADDON], 255, "%s", value);
        group_label[2] = set->strings[EMU_ADDON];
    }
    else if (MATCH("STORE", "EMU")) {
        snprintf(set->strings[EMU], 255, "%s", value);
        group_label[1] = set->strings[EMU];
    }
    if (MATCH("STORE", "HB_GAME")) {
        snprintf(set->strings[HB_GAME], 255, "%s", value);
        group_label[0] = set->strings[HB_GAME];
    }
    else if (MATCH("STORE", "SYS_VER")) {
        snprintf(set->strings[SYS_VER], 255, "%s", value);
    }
    else if (MATCH("STORE", "STORE_VER")) {
        snprintf(set->strings[STORE_VER], 255, "%s", value);
    }
    if (MATCH("STORE", "STORAGE")) {
        snprintf(set->strings[STORAGE], 255, "%s", value);
    }
    else if (MATCH("STORE", "PAGE")) {
        snprintf(set->strings[PAGE2], 255, "%s", value);
    }
    else if (MATCH("STORE", "DOWNLOADING")) {
        snprintf(set->strings[DOWNLOADING], 255, "%s", value);
    }
    else if (MATCH("LOADER", "UPDATE_REQ")) {
        snprintf(set->strings[UPDATE_REQ], 255, "%s", value);
    }
    if (MATCH("STORE", "DL_CANCELLED")) {
        snprintf(set->strings[DL_CANCELLED], 255, "%s", value);
    }
    else if (MATCH("STORE", "DL_COMPLETE")) {
        snprintf(set->strings[DL_COMPLETE], 255, "%s", value);
    }
    else if (MATCH("STORE", "DL_ERROR")) {
        snprintf(set->strings[DL_ERROR2], 255, "%s", value);
    }
    if (MATCH("STORE", "FALSE")) {
        snprintf(set->strings[FALSE2], 255, "%s", value);
    }
    else if (MATCH("STORE", "TRUE")) {
        snprintf(set->strings[TRUE2], 255, "%s", value);
    }
    else if (MATCH("STORE", "OFF")) {
        snprintf(set->strings[OFF2], 255, "%s", value);
    }
    if (MATCH("STORE", "ON")) {
        snprintf(set->strings[ON2], 255, "%s", value);
    }
    else if (MATCH("STORE", "MISSING_SFO")) {
        snprintf(set->strings[MISSING_SFO], 255, "%s", value);
    }
    else if (MATCH("STORE", "MISSING_EBOOT")) {
        snprintf(set->strings[MISSING_EBOOT], 255, "%s", value);
    }
    if (MATCH("STORE", "RESORT")) {
        snprintf(set->strings[RESORT], 255, "%s", value);
    }
    else if (MATCH("STORE", "ITEMZ_SETTINGS")) {
        snprintf(set->strings[ITEMZ_SETTINGS], 255, "%s", value);
    }
    else if (MATCH("STORE", "SORT_OPTS")) {
        snprintf(set->strings[SORT_OPTS], 255, "%s", value);
    }
    else if (MATCH("STORE", "INERNET_REQ")) {
        snprintf(set->strings[INERNET_REQ], 255, "%s", value);
    }
    else if (MATCH("STORE", "CORRUPT_SFO")) {
        snprintf(set->strings[CORRUPT_SFO], 255, "%s", value);
    }
    if (MATCH("STORE", "LG")) {
        snprintf(set->strings[LG], 255, "%s", value);
        gm_p_text[0] = set->strings[LG];
    }
    else if (MATCH("STORE", "DG")) {
        snprintf(set->strings[DG], 255, "%s", value);
        gm_p_text[1] = set->strings[DG];
    }
    else if (MATCH("STORE", "UU")) {
        snprintf(set->strings[UU], 255, "%s", value);
        gm_p_text[2] = set->strings[UU];
    }
    else if (MATCH("STORE", "UG")) {
        snprintf(set->strings[UG], 255, "%s", value);
        gm_p_text[3] = set->strings[UG];
    }
    else if (MATCH("STORE", "TRAINERS")) {
        snprintf(set->strings[TRAINERS], 255, "%s", value);
        gm_p_text[4] = set->strings[TRAINERS];
    }
    else if (MATCH("STORE", "SETTINGS_1")) {
        snprintf(set->strings[SETTINGS_1], 255, "%s", value);
        option_panel_text[0] = set->strings[SETTINGS_1];
    }
    else if (MATCH("STORE", "SETTINGS_2")) {
        snprintf(set->strings[SETTINGS_2], 255, "%s", value);
        option_panel_text[1] = set->strings[SETTINGS_2];
    }
    else if (MATCH("STORE", "SETTINGS_3")) {
        snprintf(set->strings[SETTINGS_3], 255, "%s", value);
        option_panel_text[2] = set->strings[SETTINGS_3];
    }
    else if (MATCH("STORE", "SETTINGS_4")) {
        snprintf(set->strings[SETTINGS_4], 255, "%s", value);
        option_panel_text[3] = set->strings[SETTINGS_4];
    }
    else if (MATCH("STORE", "SETTINGS_5")) {
        snprintf(set->strings[SETTINGS_5], 255, "%s", value);
        option_panel_text[4] = set->strings[SETTINGS_5];
    }
    else if (MATCH("STORE", "SETTINGS_6")) {
        snprintf(set->strings[SETTINGS_6], 255, "%s", value);
        option_panel_text[5] = set->strings[SETTINGS_6];
    }
    else if (MATCH("STORE", "SETTINGS_7")) {
        snprintf(set->strings[SETTINGS_7], 255, "%s", value);
        option_panel_text[6] = set->strings[SETTINGS_7];
    }
    else if (MATCH("STORE", "SETTINGS_8")) {
        snprintf(set->strings[SETTINGS_8], 255, "%s", value);
        option_panel_text[7] = set->strings[SETTINGS_8];
    }
    else if (MATCH("STORE", "SETTINGS_9")) {
        snprintf(set->strings[SETTINGS_9], 255, "%s", value);
        option_panel_text[8] = set->strings[SETTINGS_9];
    }
    else if (MATCH("STORE", "SETTINGS_10")) {
        snprintf(set->strings[SETTINGS_10], 255, "%s", value);
        option_panel_text[9] = set->strings[SETTINGS_10];
    }
    return 1;

}

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
                stropts->strings[i] = calloc(256, sizeof(char));
                if (stropts->strings[i] == NULL) {
                    msgok(WARNING, "Failed to load Language(s)... Failed to allocate stropts->strings[%i]", i);
                    return false;
                }
            }
            char tmp[255];
            snprintf(tmp, 254, LANG_DIR, LangCode);

            int error = ini_parse(tmp, load_lang_ini, stropts);
            if (error) {
                log_error("Bad config file (first error on line %d)!\n", error);
                return false;
            }
            else
                lang_is_initialized = true;
        }
        else
            return false;
    }
	return true;
}