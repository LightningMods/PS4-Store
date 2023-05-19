/*
    GLES2 scene

    my implementation of an interactive menu, from scratch.
    (shaders
     beware)

    2020-21, masterzorag & LM

    each time, 'l' stand for the current layout we are drawing, on each frame
*/

#include <stdio.h>
#include <string.h>

#include <time.h>
#ifdef __ORBIS__
#include "shaders.h"
#include <orbisPad.h>
#else
#include "pc_shaders.h"

//UNUSED
char* argvv[] = { "--refresh", NULL };

#endif
#include <signal.h>
#include <freetype-gl.h>
#include "defines.h"
#include "utils.h"

// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;
extern bool unsafe_source;
bool reboot_app = false;

int install_ret = -1, http_ret = -1;

extern std::string completeVersion;

bool install_done = false;

// the Settings
extern StoreOptions set,
* get;
 
// share from here resolution or other
vec2 resolution,
p1_pos;
vec3 offset;

/* normalized rgba colors */
vec4 sele, // default for selection (blue)
grey, // rect fg
white = (vec4)(1.),
col = (vec4){ .8164, .8164, .8125, 1. }, // text fg
t_sel;

extern double u_t;
extern int    selected_icon;
ivec4  menu_pos = (0),
rela_pos = (0);

extern texture_font_t* main_font, // small
* sub_font, // left panel
* titl_font;
extern vertex_buffer_t* text_buffer[4];
// posix threads args, for downloading
extern std::vector<dl_arg_t*> pt_info;
// available num of jsons
int json_c = 0;
std::shared_ptr<layout_t> active_p(nullptr),     // the one we control
                          icon_panel = std::make_shared<layout_t>(),
                          left_panel = std::make_shared<layout_t>(),
                          option_panel = std::make_shared<layout_t>(),
                          download_panel = std::make_shared<layout_t>(),
                          queue_panel = std::make_shared<layout_t>();
/*
    array for Special cases
*/
std::vector<item_t> games, groups;
/*
    auxiliary array of item_index_t:
    - shared for Search/Groups
    - the query to save search results;
*/
std::vector<item_idx_t> aux, q; // Search Query

// common title in different panels
GLuint fallback_t = 0;  // fallback texture


int GLES2_create_layout_from_json(std::shared_ptr<layout_t> &l)
{
    int   count = 0;

    count = SQL_Get_Count();

    int valid_tokens = sql_index_tokens(l, count);

    log_info("%d valid_tokens!", valid_tokens);

    return valid_tokens;
}

extern std::vector<std::string> option_panel_text;
extern std::vector<std::string> download_panel_text;
/*
    indexes item_t item_data
*/
int GLES2_create_layout_from_list_v2(std::shared_ptr<layout_t> &l)
{
    int i, j;

    log_info("GLES2_create_layout_from_list_v2()");

    for (i = 0; i < l->item_c; i++)
    {   // dynallocs for items
        if (l->item_d[i].token_d.empty())
        {
            l->item_d[i].token_c = 10; // minimum
            //l->item_d[i].token_d = (item_idx_t*)calloc(l->item_d[i].token_c, sizeof(item_idx_t));
            l->item_d[i].token_d.resize(l->item_d[i].token_c);
        }
        log_info("item %d", l->item_d[i].token_c);
        // iterate token_data
        for (j = 0; j < l->item_d[i].token_c; j++)
        {
            l->item_d[i].token_d[j].off = new_panel_text[i][j];
            if (l->item_d[i].token_d[j].off.empty())
            {
                l->item_d[i].token_d[j].len = -1; 
                break;
            } // stop here
            else
                l->item_d[i].token_d[j].len = l->item_d[i].token_d[j].off.size();
            //
            log_info("%s %d %d", l->item_d[i].token_d[j].off.c_str(), j ,l->item_d[i].token_d[j].len);
        }
        if (j < l->item_d[i].token_c)
        {   // shrink
            l->item_d[i].token_c = j;
           // l->item_d[i].token_d = (item_idx_t*)realloc(l->item_d[i].token_d,
             //   l->item_d[i].token_c * sizeof(item_idx_t));
             l->item_d[i].token_d.resize(l->item_d[i].token_c);
        }
    }
    return i;
}

// option
extern bool use_reflection;

// return index to selected item_data token_data
int get_item_index(std::shared_ptr<layout_t> &l)
{
    int
        // compute some indexes, first one of layout:
        i_paged = l->fieldsize.y * l->fieldsize.x
        * l->page_sel.x;
    // which icon is selected in field X*Y ?
    // go: new way have 'layout_t.f_sele'
    // which item is selected over all ?
    int idx = l->curr_item;

    /* Special cases: */

    // we have an auxiliary serch result item list to show
    if (!aux.empty())
        idx = aux[i_paged + l->f_sele
        + 1    // skip first reserved
        ].len; // read stored index
    return idx;
}

/*
    indexes item_idx_t* token_data in a single item_t item_data

    indexes means "just address all pointers, *zerocopy* !"
    so you have to take care to where those are pointing...
*/
int layout_fill_item_from_list(std::shared_ptr<layout_t> &l, std::vector<std::string> &i_list)
{
    int      i, count = 0;
    for (i = 0; i < l->item_c; i++) // iterate item_d
    {   // dynallocs each token_d
        if (l->item_d[i].token_d.empty())
        {
            l->item_d[i].token_c = 1; // minimum, one token_d
            //l->item_d[i].token_d = (item_idx_t*)calloc(l->item_d[i].token_c, sizeof(item_t));
            l->item_d[i].token_d.resize(l->item_d[i].token_c);
        }
        // we use just the first token_d
        if( i_list[i].empty() )
            continue;

        l->item_d[i].token_d[ID].off = i_list[i];
        if (!l->item_d[i].token_d[ID].off.empty())
        {
            //token->len = strlen( i_list[i] );
            count += 1;
            log_debug("%s, %s, %d", __FUNCTION__, l->item_d[i].token_d[ID].off.c_str(), count);
        }
        else
            l->item_d[i].token_d[ID].len = 0;
    }

    /* update */
    l->item_c = count;      // layout items count
    layout_update_fsize(l); // and the field_size

    return count;
}

// used for Groups
void recreate_item_t(std::vector<item_t> &i)
{  
    i.clear();
    i = analyze_item_t_v2(icon_panel->item_d, icon_panel->item_c);
}

//bool has_checked_already = false;
std::atomic_bool update_check_finised(false);
void *check_for_updates(void *) {
    std::string CDN = set.opt[CDN_URL];
    std::string tmp;
    vec4 col = (vec4){ .8164, .8164, .8125, 1. };
    vec2 pos = { 114, 504 };
    while (true) {
        //std::unique_lock<std::mutex> lock(mtx);
        log_info("Checking for updates..." );
         if ( check_store_from_url(CDN, MD5_HASH))
            log_info("[StoreCore] DB is already Up-to-date");
        else {
            log_info("[StoreCore] DB is Outdated!, Downloading the new one ...");

            tmp = fmt::format("{}/store.db", CDN);
            if (dl_from_url(tmp, SQL_STORE_DB) != 0)
                msgok(FATAL, getLangSTR(CACHE_FAILED));
            else
                log_info("[StoreCore] New DB Downloaded");

#ifdef __ORBIS__
      reboot_app = true; 
      raise(SIGQUIT);
#else
     execv("/proc/self/exe", argvv);
#endif
        }
        CheckforUpdates(ONLINE_CHECK_FOR_STARTUP);
         //has_checked_already = true;
        if(!update_check_finised.load()){
           left_panel2->mtx.lock();
           if(updates_counter.load() > 0 && active_p == left_panel2) // we counted at least one
           {
              left_panel2->vbo_s = ASK_REFRESH;
              log_info("ret: %d pen.x %.f pen.x %.f", updates_counter.load(), pos.x, pos.y);
              tmp = std::to_string(updates_counter.load());
              texture_font_load_glyphs( sub_font, tmp.c_str() );
              pos.x = 460 - tl;
              left_panel2->vbo.add_text(sub_font, tmp, col, pos);
           }
           update_check_finised = true;
           left_panel2->mtx.unlock();
        }


       sleep(5*60);
    }
    return nullptr;
}

// no page 0, start from 1
void GLES2_scene_init(int w, int h)
{
    resolution = (vec2){ w, h };
    // reset player pos
    p1_pos = (vec2)(0);
    // normalize colors
    sele = (vec4){ 29, 134, 240, 256 } / 256, // blue
        grey = (vec4){ 66,  66,  66, 256 } / 256; // fg

        /* first, scan folder, populate Installed_Apps */

        // build an array of indexes for each one entry there
#if defined(__ORBIS__)
    mkdir(APP_PATH("../"), 0777);
#else
    mkdir(APP_PATH(""), 0777);
#endif
    /* now, scan .json files, populate main icon panel */
    // try to compose any page, count available ones
    json_c = check_store_from_url("", COUNT);
    log_info("[StoreCore] %d available json!", json_c);

    // setup one for all missing, the fallback icon0
#if defined(__ORBIS__)
    if (!fallback_t)
    {
        if(!if_exists(asset_path("load.png"))){
           if (if_exists("/user/appmeta/NPXS39041/icon0.png"))
               fallback_t = load_png_asset_into_texture("/user/appmeta/NPXS39041/icon0.png");
           else
              fallback_t = load_png_asset_into_texture("/user/appmeta/external/NPXS39041/icon0.png");
        }
        else
          fallback_t = load_png_asset_into_texture(asset_path("load.png"));
    }
#else
    fallback_t = load_png_asset_into_texture(asset_path("load.png"));
#endif
    /* icon panel setup */
    //if (!icon_panel) icon_panel = calloc(1, sizeof(layout_t));
    // pos.xy, size.wh
    icon_panel->mtx.lock();
    icon_panel->bound_box = (vec4){ 680, 900,   1096, 664 };
    // five columns, three rows
    icon_panel->fieldsize = (ivec2){ 5, 3 };
    icon_panel->page_sel = (ivec2)(0);
    // malloc for max items
    icon_panel->item_c = json_c * icon_panel->fieldsize.x * icon_panel->fieldsize.y;
    icon_panel->item_d.resize(icon_panel->item_c); //= (item_t*)calloc(icon_panel->item_c, sizeof(item_t));
    // reset count, we refresh from ground
    icon_panel->item_c = 0;
    // create the first screen we will show
    GLES2_create_layout_from_json(icon_panel);

    log_info("layout->item_c: %d", icon_panel->item_c);
   
    // use first as reserved index to store count
    // copy as games array 
    games = icon_panel->item_d;
    games[0].token_c = icon_panel->item_c;
    // clean old buffer and swap with newly copy
    icon_panel->item_d.clear(), 
    icon_panel->item_d = games;
    
   // games.back().token_d.resize(TOTAL_NUM_OF_TOKENS);
    //for (int i = 0; i < TOTAL_NUM_OF_TOKENS; i++) 
      //   games.back().token_d.at(i).off = "test";


    log_info("game: %i back %i", games[0].token_c, games.size() - 1);

    // flag as to show it
    icon_panel->is_shown = 1;
    // set max_pages
    icon_panel->page_sel.y = icon_panel->item_c / (icon_panel->fieldsize.x * icon_panel->fieldsize.y);

//    for(const auto& game : icon_panel->item_d){
//        log_info("game: %s", game.token_d[0].off);
//    }

    layout_update_fsize(icon_panel);
    icon_panel->vbo_s = ASK_REFRESH;
    icon_panel->mtx.unlock();

    
    log_info(" Starting thread to check database updates ...");
    pthread_t update_thread;
    pthread_create(&update_thread, nullptr, check_for_updates, nullptr);
    log_info(" Thread started.");

    /* resort using custom comparision function */
    //qsort(icon_panel->item_d, icon_panel->item_c, sizeof(item_t), struct_cmp_by_token);

#define HAVE_ATLAS (0) /// main
#if HAVE_ATLAS
    create_atlas_from_tokens(icon_panel);
#endif

    /* main left panel OLD_WAY! */

#if 1
  //  if (!left_panel) left_panel = calloc(1, sizeof(layout_t));

    left_panel->bound_box = (vec4){ 0, 900,   500, 700 };
    left_panel->fieldsize = (ivec2){ 1, 9 };
    // by calloc curr page is 0!
    /* malloc for max pages, plus additional list  */
    left_panel->item_c = LPANEL_Y;   //sizeof(new_panel_text) / sizeof(new_panel_text[0][0]);
    left_panel->item_d.resize(left_panel->item_c + 1);// = (item_t*)calloc(left_panel->item_c + 1 /* Groups */, sizeof(item_t));
    // create the first page we will show
    GLES2_create_layout_from_list_v2(left_panel);
    // new_p indexes differently!
    left_panel->item_c = 10;
    log_info("layout->item_c: %d", left_panel->item_c);

    // build an array of indexes for each one Groups
    if (groups.empty()) 
        groups = analyze_item_t_v2(icon_panel->item_d, icon_panel->item_c);

    left_panel->item_d[LPANEL_Y].token_c = groups[0].token_c;
    log_info("groups->token_c: %d", groups[0].token_c);

    // flag as active
    left_panel->is_shown =
        left_panel->is_active = 1;
    // set max_pages
    left_panel->page_sel.y = LPANEL_Y;
    left_panel->vbo_s = ASK_REFRESH;
#endif
    {

        //if (!option_panel) option_panel = calloc(1, sizeof(layout_t));

        option_panel->bound_box.xy = icon_panel->bound_box.xy;
        option_panel->bound_box.zw = (vec2){ 980, 670 };
        option_panel->fieldsize = (ivec2){ 2,   5 };
        option_panel->page_sel.x = 0;
        // malloc for max items
        option_panel->item_c = 11;

        option_panel->item_d.resize(option_panel->item_c); //= (item_t*)calloc(option_panel->item_c, sizeof(item_t));
        // create the first screen we will show

        log_info("%p %i", &option_panel->item_d, option_panel->item_c);
        layout_fill_item_from_list(option_panel, option_panel_text);

        log_info("layout->item_c: %d", option_panel->item_c);
        // flag as active
        option_panel->is_shown =
            option_panel->is_active = 0;
        // set max_pages
        option_panel->page_sel.y = LPANEL_Y;
        option_panel->vbo_s = ASK_REFRESH;
    }

    {
       // if (!download_panel) download_panel = calloc(1, sizeof(layout_t));

        download_panel->bound_box = (vec4){ 500, 320,
                                              1024,  64 };
        download_panel->fieldsize = (ivec2){ 3, 1 };
        // malloc for max items
        download_panel->item_c = 2;//download_panel->fieldsize.x * download_panel->fieldsize.y;
        download_panel->item_d.resize(download_panel->fieldsize.x * download_panel->fieldsize.y); //= (item_t*)calloc(download_panel->fieldsize.x * download_panel->fieldsize.y, sizeof(item_t));
        // create the first screen we will show
        if (set.auto_install.load()) {
            download_panel->item_c--;
            download_panel_text[0] = getLangSTR(DL_AND_IN);
            download_panel_text[1] = getLangSTR(CANCEL);
        }
        layout_fill_item_from_list(download_panel, download_panel_text);

        log_info("layout->item_c: %d", download_panel->item_c);
        // flag as active
        download_panel->is_shown =
            download_panel->is_active = 0;
        // set max_pages
        download_panel->page_sel.y = download_panel->item_c / (download_panel->fieldsize.x * download_panel->fieldsize.y);
    }
    // force initial status
    menu_pos.z = ON_LEFT_PANEL;

    /* UI: menu */

    // init shaders for textures
    on_GLES2_Init_icons(w, h);
    // init shaders for lines and rects
    ORBIS_RenderFillRects_init(w, h);
    // init ttf fonts
    GLES2_fonts_from_ttf(set.opt[FNT_PATH].c_str());;
 // fragments shaders, effects etc
    pixelshader_init(w, h);
#if HAVE_ATLAS
    InitTextureAtlas(); // one from cache
#endif
}


void GLES2_UpdateVboForLayout(std::shared_ptr<layout_t> &l)
{
    if (l)
    {   // skip until we ask to refresh VBO
        if (l->vbo_s < ASK_REFRESH) return;

        l->vbo.clear();

        l->vbo_s = EMPTY;
    }
}



// not used
#if _DEBUG && 0
void _prt() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    unsigned long long millisecondsSinceEpoch =
        (unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000;

    log_info("timestamp %llu", millisecondsSinceEpoch);
}
#endif



/*
    this is the main renderloop
    note: don't need to be cleared or swapped any frames !

    this one is drawing each rendered frame!
*/
bool query_ran = false;
void GLES2_scene_render(const char* query)
{
    // update the time
    bool is_tid = false;
    on_GLES2_Update(u_t);
    on_GLES2_Update1(u_t);

    switch (menu_pos.z) // as view
    {
        // the two sided panels view
    case ON_LEFT_PANEL:
    case ON_MAIN_SCREEN:
        GLES2_render_paged_list(0); // new way: left_panel2
        GLES2_render_icon_list(0);  // new way: icon_panel clone
        if(!query_ran && query && icon_panel){
            for(auto &t: icon_panel->item_d){

                if(t.token_d.empty() || t.token_d.size() <  NAME || t.token_d[NAME].off.empty() || t.token_d[ID].off.empty())
                   continue;

                if (strcmp(t.token_d[ID].off.c_str(), query) == 0) {
                   Install_View(icon_panel, t.token_d[ID].off.c_str(), ID);
                   is_tid = true;
                   break;
                }
           }
           if(!is_tid)
               Install_View(icon_panel, query, NAME);

           query_ran = true;
        }
        break;
        // download item view
    case ON_ITEM_INFO:
        GLES2_render_download_panel(); // new way
        break;
        // Ready to install view
    case ON_INSTALL:

        if (set.auto_install.load()) 
            GLES2_render_queue(queue_panel, 1337);
        else
            GLES2_render_queue(queue_panel, 9);

        GLES2_render_paged_list(0); // new way
        break;
        // Queue view
    case ON_QUEUE:
        GLES2_render_queue(queue_panel, 0);
        GLES2_render_paged_list(0); // new way
        break;
    case ON_SETTINGS:
        GLES2_render_layout_v2(option_panel, 0);
        GLES2_render_paged_list(0); // new way
        break;
    default:
        break;
    }

    //show_dumped_frame();

    GLES2_Draw_sysinfo();

    GLES2_Draw_common_texts();

    layout_refresh_VBOs();
}

static void actions_for_settings(int action, std::shared_ptr<layout_t> &l){
    char tmp[512] = { 0 };
    switch (action)
    {   // use sceKbd to edit related field
    case CDN_SETTING:
    case TMP_SETTING:
    case INI_SETTING:

        if (action == INI_PATH)
            msgok(WARNING, getLangSTR(NEW_INI));

        log_warn(" %s", set.opt[action].c_str());
        
        if(!Keyboard(l->item_d[action].token_d[0].off.c_str(), set.opt[action].c_str(), &tmp[0], (action == CDN_SETTING) ? true : false)) break;
        // clean string, works for LF, CR, CRLF, LFCR, ...
        tmp[strcspn(tmp, "\r")] = 0;

        // check for valid result
        if (strlen(tmp) <= 1) {
            msgok(NORMAL, getLangSTR(STR_TOO_LONG));  goto error;
        }

        if (action == INI_PATH && (!touch_file(tmp) || action != CDN_URL) && !if_exists(tmp))
        {
            msgok(NORMAL, getLangSTR(INVAL_PATH));
            goto error;
        }

        if ((action == TMP_PATH && strstr(tmp, "pkg") != NULL) || strstr(tmp, "/data") != NULL)
        {
            msgok(NORMAL, getLangSTR(PKG_SUF)); goto error;
        }

        // validate result (CDN)
        if (action == CDN_URL)
        {
            //log_info("sssssss");
            loadmsg(getLangSTR(SEARCHING));
            if(dl_from_url(fmt::format("{}/store.db", tmp), "/user/app/NPXS39041/store_downloaded.db") != 0){
                msgok(NORMAL, getLangSTR(INVAL_CDN));
                unlink("/user/app/NPXS39041/store_downloaded.db");
                goto error;
            }
            sceMsgDialogTerminate();
        }

        //log_info("sssssss");

        // update 
        log_info("Options: %i Before: %.30s ->  After:  %.30s", action, set.opt[action].c_str(), tmp);

        // store
        set.opt[action] = tmp;

        log_info("set.opt[%i]:  %.20s", action, set.opt[action].c_str());

        // try to load custom font
        if (action == FNT_PATH) {
            if (set.opt[action].find(".ttf") != std::string::npos)
                GLES2_fonts_from_ttf(set.opt[action].c_str());
            else {
                msgok(NORMAL, getLangSTR(INVAL_TTF));  goto error;
            }
        }

        break;

    error:
        log_info("ERROR: entered %.20s", tmp);
        break;

        // execute action, set flags
    case AUTO_INSTALL_SETTING: {//STORE_ON_USB

        if (!set.auto_install.load() && set.Legacy_Install.load()) {
            msgok(WARNING, getLangSTR(AUTO_FAILURE_ERROR));
            break;
        }
        set.auto_install = (set.auto_install.load()) ? false : true;
        log_info("auto_install: %d", set.auto_install.load());

        if (set.auto_install.load()) {
            download_panel->item_c--;
            download_panel_text[0] = getLangSTR(DL_AND_IN);
            download_panel_text[1] = getLangSTR(CANCEL);
        }
        else {
            download_panel->item_c++;
            download_panel_text[0] = getLangSTR(DL2);
            download_panel_text[1] = getLangSTR(INSTALL2);
            download_panel_text[2] = getLangSTR(CANCEL);
        }

        layout_fill_item_from_list(download_panel, download_panel_text);

        break;
    }

    case CLEAR_CACHE_SETTING: { // clear cached images
        loadmsg(getLangSTR(CLEARING_CACHE));
        unlink(STORE_LOG);
        FILE* fp = fopen(STORE_LOG, "w");
        if(fp)
            log_add_fp(fp, LOG_DEBUG);
            
        log_info("Settings -> Clear Cached images and content");

        if (rmtree("/user/app/NPXS39041/storedata"))
            msgok(NORMAL, getLangSTR(CACHE_CLEARED));
        else
            msgok(WARNING, getLangSTR(CACHE_FAILED));

        mkdir("/user/app/NPXS39041/storedata", 0777);

        break;
    }

    case LEGACY_INSTALL_PROG: {
        if (set.auto_install.load() && !set.Legacy_Install.load()) {
            msgok(WARNING, getLangSTR(INSTALL_PROG_ERROR));
            break;
        }
        set.Legacy_Install = (set.Legacy_Install.load()) ? false : true;
        log_info("Legacy_Install: %d", set.Legacy_Install.load());
        break;
    }
    case LOAD_CACHE_ICONS: {
        set.auto_load_cache = (set.auto_load_cache) ? false : true;
        log_info("auto_load_cache: %d", set.auto_load_cache);
        SaveOptions();

        break;
    }
    case RESET_SETTING: {
        log_info("Settings -> Reset");

        set.opt[CDN_URL] = "https://api.pkg-zone.com";
        set.opt[TMP_PATH] = "/user/app/NPXS39041/downloads";
        set.opt[FNT_PATH] = "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF";
        
        set.auto_install = true;
        set.Legacy_Install = false;

        goto save;
        break;
    }
    case REFRESH_DB_SETTING: {
#ifdef __ORBIS__
      reboot_app = true; 
      raise(SIGQUIT);
#else
     execv("/proc/self/exe", argvv);
#endif
        break;
    }
    case SAVE_SETTINGS: {//SAVE_OPTIONS:
save:
        log_info("%d", action);
        if (!SaveOptions())
            msgok(WARNING, getLangSTR(SAVE_ERROR));
        else
        {
            msgok(NORMAL, getLangSTR(SAVE_SUCCESS));
            log_info("Settings saved to %s", set.opt[INI_PATH].c_str());
            // reload settings
            LoadOptions();
        }
        break;
    }
    }
    GLES2_Refresh_for_settings();
}

// just used in download panel and Settings
void X_action_dispatch(int action, std::shared_ptr<layout_t> &l)
{
    /* actually on Setting we jump there... */
    if (l == option_panel){
        actions_for_settings(action, l);
        return;
    };

    std::string tmp;
    // selected item from active panel
    log_info("%s execute %d -> '%s'", __FUNCTION__, action, l->item_d[action].token_d[0].off.c_str());


    int label = NAME;
    switch (action)
    {
    case 0: label = PACKAGE; break;
    default:                 break;
    }
    // which item is selected by icon_panel ?
    int idx = get_item_index(icon_panel);
    // address the selected item
    // selected item from index in icon_panel, print token_data label
    log_info("item:%d, %s '%s'", idx,
         l->item_d[action].token_d[0].off.c_str(),
        // icon_panel->item_d[ idx ].token_d[ label ].off,
        icon_panel->item_d[idx].token_d[label].off.c_str());
    // default
    //"/user/app/temp.pkg"
    // avoid same donwload path
   // snprintf(&tmp[0], 255, "%s/%s.pkg", set.opt[TMP_PATH], t[ID].off);
    tmp = fmt::format("{}/{}.pkg", set.opt[TMP_PATH],  icon_panel->item_d[idx].token_d[ID].off);
    log_info(tmp.c_str());

    dl_arg_t *ta = NULL;
    int x = thread_find_by_item(idx);
    if (x > -1) // a thread is operating on item
    {
        ta = pt_info[x];
        log_info("thread[%d] is handling %d, status:%d", x, idx, ta->status.load());
        //action = -1; // disable, skip action
    }

    switch (action)
    {   /* Donwload */
    case 0: {
        if(ta && ta->status == RUNNING)
        {
            ta->status = PAUSED;
            download_panel_text[0] = getLangSTR(RESUME_2);
            goto draw;
        }
        else if(ta && ta->status == PAUSED)
        {
            ta->status = RUNNING;
            download_panel_text[0] = getLangSTR(PAUSE_2);
            goto draw;
        }
        else if(ta && ta->status == COMPLETED) goto install;
        // a thread is downloading item, skip
        if (x > -1) return;
        //New Download API
        if (!unsafe_source){
           icon_panel->item_d[idx].token_d[label].off = fmt::format("{}/download.php?tid={}", set.opt[CDN_URL], icon_panel->item_d[idx].token_d[ID].off);
        }

        if (icon_panel->item_d[idx].token_d[ID].off != STORE_TID)
        {
            log_info("t[label].off %s",icon_panel->item_d[idx].token_d[label].off.c_str());
            download_panel->item_c++;
            download_panel_text[0] = getLangSTR(PAUSE_2);
            loadmsg(getLangSTR(DL_CACHE));
            http_ret = dl_from_url_v2(icon_panel->item_d[idx].token_d[label].off, tmp, icon_panel->item_d[idx].token_d); //Download from Legacy
            if(http_ret == 200)
                icon_panel->item_d[idx].interruptible = true;
            sceMsgDialogTerminate();
        }
        else
            msgok(WARNING, getLangSTR(NOT_ON_APP));

draw:
        layout_fill_item_from_list(download_panel, download_panel_text);

    } break;
        /* Install */
    case 1: {
        if (set.auto_install.load())
            goto opt_3;
install:
        if (ta
            && ta->status == COMPLETED)
        {
            #ifdef __ORBIS__
            ta->is_threaded = false;
            pkginstall(tmp.c_str(), ta, set.auto_install.load());
            #else
            log_info("install %s", tmp.c_str());
            // clean thread args for next job
           // if(ta->dst) free((void*)ta->dst), ta->dst = NULL;
            ta->dst.clear();
            ta->g_idx  = -1;
            ta->status = READY;
            log_info("Set Status: Ready");     
            #endif       
            icon_panel->item_d[icon_panel->curr_item].interruptible = false;


        }

    } break;
    case 2: /*option 3*/ {
    opt_3:
        if (ta 
             && ta->status > READY)
        {
            unlink(ta->dst.c_str());
            //if(ta->dst) free((void*)ta->dst), ta->dst = NULL;
            ta->dst.clear();
            //memset(ta, 0, sizeof(dl_arg_t));
            ta->g_idx  = -1;
            ta->status = CANCELED;
            icon_panel->item_d[icon_panel->curr_item].interruptible = false;
            
            log_info("Set Status: CANCELED");
        }
        break;
    }
    default:
        log_info("more");
        break;
    }
}

void O_action_dispatch(void)
{
    refresh_atlas();
}
