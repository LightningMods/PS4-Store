#include "defines.h"
#include "lang.h"
#include <unordered_map>


std::unordered_map<std::string, std::string> stropts;
std::vector<std::vector<std::string>> new_panel_text(LPANEL_Y, std::vector<std::string>(11));
std::vector<std::string> download_panel_text(3);
std::vector<std::string> option_panel_text(11);
std::vector<std::string> group_label(8);
extern const char* group_labels[];
extern const char* group_labels_non_pkg_zone[];

extern bool unsafe_source;
bool lang_is_initialized = false;
//
// https://github.com/Al-Azif/ps4-payload-guest/blob/main/src/Language.cpp
//
std::string Language_GetName(int m_Code)
{
    std::string s_Name;

    switch (m_Code)
    {
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
extern "C" int sceSystemServiceParamGetInt(int, int32_t *);

int32_t PS4GetLang()
{
    int32_t lang = 1;
#if defined(__ORBIS__)
    if (sceSystemServiceParamGetInt(1, &lang) != 0)
    {
        log_error("Unable to qet language code");
        return 0x1;
    }
#endif
    return lang;
}

std::string getLangSTR(Lang_STR str)
{
    if (lang_is_initialized)
    {
        if (str > LANG_NUM_OF_STRINGS)
            return stropts[lang_key[STR_NOT_FOUND]];
        else
            return stropts[lang_key[str]];
    }
    else
        return "ERROR: Lang has not loaded";
}

static int load_lang_ini(void *, const char *, const char *name,
                         const char *value, int)
{

    stropts.insert(std::pair<std::string, std::string>(name, value));
    //log_info("name %s value %s", name, value);

    return true;
}

void fill_menu_text() {
  //lang_key[STR_NOT_FOUND]
  if (stropts[lang_key[CANCEL]].empty()) {
    stropts[lang_key[SETTINGS_6]] = "Automatically install";
    //SETTINGS_5
    stropts[lang_key[AUTO_FAILURE_ERROR]] = "Show Install Progress is Required to be Disabled to Enable this Setting";
    stropts[lang_key[INSTALL_PROG_ERROR]] = "Auto Install is Required to be Disabled to Enable this Setting";

    stropts[lang_key[CANCEL]] = "Cancel";
    stropts[lang_key[DL_AND_IN]] = "Download and Install";
    stropts[lang_key[PAUSE_2]] = "Pause";
    stropts[lang_key[RESUME_2]] = "Resume";

    stropts[lang_key[SETTINGS_3]] = "Refresh Store Database";
    stropts[lang_key[SETTINGS_9]] = "Reset Store Settings";
    stropts[lang_key[APP_UPDATE_AVAIL]] = "Update Available";
    stropts[lang_key[UPDATE_NOW]] = "Update Now";
    stropts[lang_key[REINSTALL_APP]] = "Reinstall Latest";
  }
  // keep compitability with older store pkgs and unupdated langs
  if (stropts[lang_key[PRE_LOADING_CACHE]].empty()) {
    stropts[lang_key[SETTINGS_5]] = "Pre-load Cached Icons On Startup";
    stropts[lang_key[PRE_LOAD_SETTING]] = "Pre-load Cached Icons On Startup";
    stropts[lang_key[CHECKING_FOR_UPDATES]] = "Checking for updates...";
    stropts[lang_key[PRE_LOADING_CACHE]] = "pre-loading cached App icons...";
    stropts[lang_key[UPDATES_STILL_LOADING]] = "Updates are still being checked in the background";
    stropts[lang_key[SHOW_PROG]] = "Show Progress";
    stropts[lang_key[STAY_IN_BACKGROUND]] = "Stay in Background";
    stropts[lang_key[INSTALL_COMPLETE]] = "Installation Complete!";

  }

  new_panel_text[3][10] = getLangSTR(NUMB_OF_DL);
  new_panel_text[3][9] = getLangSTR(RDATE);
  new_panel_text[3][8] = getLangSTR(PV2);
  new_panel_text[4][0] = getLangSTR(PV2);
  new_panel_text[3][7] = getLangSTR(TYPE2);
  new_panel_text[4][1] = getLangSTR(AUTHOR2);
  new_panel_text[3][6] = getLangSTR(AUTHOR2);
  new_panel_text[3][5] = getLangSTR(SIZE2);
  new_panel_text[3][4] = getLangSTR(RSTARS);
  new_panel_text[3][3] = getLangSTR(VER);
  new_panel_text[3][2] = getLangSTR(PACKAGE2);
  new_panel_text[3][1] = getLangSTR(NAME2);
  new_panel_text[3][0] = getLangSTR(ID2);
  new_panel_text[1][2] = getLangSTR(FILTER_BY);
  new_panel_text[1][1] = getLangSTR(SORT_BY);
  new_panel_text[1][0] = getLangSTR(SEARCH);
  new_panel_text[0][6] = getLangSTR(SETTINGS);
  new_panel_text[0][5] = getLangSTR(UPDATES);
  new_panel_text[0][4] = getLangSTR(QUEUE);
  new_panel_text[0][3] = getLangSTR(RINSTALL);
  new_panel_text[0][2] = getLangSTR(STRG);
  new_panel_text[0][1] = getLangSTR(IAPPS);
  new_panel_text[0][0] = getLangSTR(SAPPS);
  download_panel_text[1] = getLangSTR(INSTALL2);
  download_panel_text[0] = getLangSTR(DL2);
  group_label[6] = getLangSTR(OTHER);
  group_label[5] = getLangSTR(UTLIITY);
  group_label[4] = getLangSTR(PLUGINS);
  group_label[3] = getLangSTR(MEDIA);
  group_label[2] = getLangSTR(EMU_ADDON);
  group_label[1] = getLangSTR(EMU);
  group_label[0] = getLangSTR(HB_GAME);
  option_panel_text[0] = getLangSTR(SETTINGS_1);
  option_panel_text[1] = getLangSTR(SETTINGS_2);
  option_panel_text[2] = getLangSTR(SETTINGS_3);
  option_panel_text[3] = getLangSTR(SETTINGS_4);
  option_panel_text[4] = getLangSTR(SETTINGS_5);
  option_panel_text[5] = getLangSTR(SETTINGS_6);
  option_panel_text[6] = getLangSTR(SETTINGS_7);
  option_panel_text[7] = getLangSTR(SETTINGS_8);
  option_panel_text[8] = getLangSTR(SETTINGS_9);
  option_panel_text[9] = getLangSTR(SETTINGS_10);
  download_panel_text[2] = getLangSTR(CANCEL);

  for (int i = 0; i < 7; i++) {
    if (unsafe_source)
        group_label[i] = group_labels_non_pkg_zone[i];
        
    //log_info("group_label[%d] = %s | unsafe_source %i", i, group_label[i].c_str(), unsafe_source);
  }


}
// OVERWRITE_SAVE
extern uint8_t lang_ini[];
extern int32_t lang_ini_sz;

#if defined(__ORBIS__)
bool load_embdded_eng()
{
    //mem should already be allocated as this is the last opt
    int fd = -1;
    if (stropts.empty()) {
        if (!if_exists("/user/app/NPXS39041/lang.ini")) {
            if ((fd = open("/user/app/NPXS39041/lang.ini", O_WRONLY | O_CREAT | O_TRUNC, 0777)) > 0 && fd != -1) {
                write(fd, lang_ini, lang_ini_sz);
                close(fd);
            }
            else
               return false;
        }

        int error = ini_parse("/user/app/NPXS39041/lang.ini", load_lang_ini, nullptr);
        if (error) {
            log_error("Bad config file (first error on line %d)!", error);
            return false;
        }
        else
            lang_is_initialized = true;
    }
    else
        return false;

    fill_menu_text();

    return true;
}
#else

#endif

bool LoadLangs(int LangCode)
{
    std::string dst;
    #ifdef __ORBIS__  
    dst = fmt::format("{0:}/{1:d}/lang.ini", LANG_DIR, LangCode);
    #else
    dst = asset_path("lang.ini");
    #endif
#ifndef TEST_INI_LANGS
    if (!lang_is_initialized)
#else
    if (1)
#endif
    {
        if(!if_exists(dst.c_str())){
            log_error("Lang file not found! %s", dst.c_str());
            return false;
        }
           
        int error = ini_parse(dst.c_str(), load_lang_ini, nullptr);
        if (error < 0)
        {
            log_error("Bad config file (first error on line %d)!", error);
            return false;
        }
        else
            lang_is_initialized = true;
    }

    fill_menu_text();
    return true;
}