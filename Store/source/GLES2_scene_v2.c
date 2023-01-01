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
#include <stdatomic.h>
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
#include "GLES2_common.h"
#include "utils.h"

// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;
extern bool unsafe_source;

int install_ret = -1, http_ret = -1;

extern const unsigned char completeVersion[];

bool install_done = false;

// the Settings
extern StoreOptions set,
* get;

// related indexes from json.h enum
// for sorting entries
unsigned char cmp_token = 0,
sort_patterns[10] = { 0, 1, 4, 5, 9, 10, 11, 12, 13, 16 };

/* set comparison token */
void set_cmp_token(const int index)
{    //cmp_token = index;
    cmp_token = sort_patterns[index];
}

/* qsort struct comparison function (C-string field) */
int struct_cmp_by_token(const void* a, const void* b)
{
    item_t* ia = (item_t*)a;
    item_t* ib = (item_t*)b;
    return strcmp(ia->token_d[cmp_token].off, ib->token_d[cmp_token].off);
    /* strcmp functions works exactly as expected from comparison function */
}


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
extern dl_arg_t* pt_info;
// available num of jsons
int json_c = 0;

layout_t* active_p = NULL, // the one we control
* icon_panel = NULL,
* left_panel = NULL,
* option_panel = NULL,
* download_panel = NULL,
* queue_panel = NULL;
/*
    array for Special cases
*/
item_t* games = NULL,
* groups = NULL;
/*
    auxiliary array of item_index_t:
    - shared for Search/Groups
    - the query to save search results;
*/
item_idx_t* aux = NULL,
* q = NULL; // Search Query

// common title in different panels
GLuint fallback_t = 0;  // fallback texture

void GLES2_destroy_layout(layout_t* l)
{
    if (!l) return;

    //    clean_textures(l);

    item_idx_t* token_d = NULL;
    for (int i = 0; i < l->item_c; i++)
    {
        token_d = &l->item_d[i].token_d[0];
        for (int j = 0; j < NUM_OF_USER_TOKENS; j++)
            // < token_c
        {
            if (token_d[j].off) free(token_d[j].off), token_d[j].off = NULL;
        }
        if (token_d) free(token_d), token_d = NULL;
    }
    if (l->item_d) free(l->item_d), l->item_d = NULL;;

    if (l->vbo) vertex_buffer_delete(l->vbo), l->vbo = NULL;

    if (l) free(l), l = NULL;
}



int GLES2_create_layout_from_json(layout_t* l)
{
    int   count = 0;

    count = SQL_Get_Count();

    int valid_tokens = sql_index_tokens(l, count);

    log_info("%d valid_tokens!", valid_tokens);

    return valid_tokens;
}

extern const char* option_panel_text[];
char* new_panel_text[LPANEL_Y][11];
char* download_panel_text[3];
/*
    indexes item_t item_data
*/
int GLES2_create_layout_from_list_v2(layout_t* l, const char** i_list)
{
    int i, j;
    item_idx_t* token = NULL;

    for (i = 0; i < l->item_c; i++)
    {   // dynallocs for items
        if (!l->item_d[i].token_d)
        {
            l->item_d[i].token_c = 10; // minimum
            l->item_d[i].token_d = calloc(l->item_d[i].token_c, sizeof(item_idx_t));
        }
        // iterate token_data
        for (j = 0; j < l->item_d[i].token_c; j++)
        {
            token = &l->item_d[i].token_d[j];
            token->off = new_panel_text[i][j];
            if (!token->off)
            {
                token->len = -1; break;
            } // stop here
            else
                token->len = strlen(token->off);
            //
            log_info("%s %d", l->item_d[i].token_d[j].off,
                l->item_d[i].token_d[j].len);
        }
        if (j < l->item_d[i].token_c)
        {   // shrink
            l->item_d[i].token_c = j;
            l->item_d[i].token_d = realloc(l->item_d[i].token_d,
                l->item_d[i].token_c * sizeof(item_idx_t));
        }
    }
    return i;
}

// option
extern bool use_reflection;

// return index to selected item_data token_data
int get_item_index(layout_t* l)
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
    if (aux)
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
int layout_fill_item_from_list(layout_t* l, char** i_list)
{
    int      i, count = 0;
    item_idx_t* token = NULL;


    for (i = 0; i < l->item_c; i++) // iterate item_d
    {   // dynallocs each token_d
        if (!l->item_d[i].token_d)
        {
            l->item_d[i].token_c = 1; // minimum, one token_d
            l->item_d[i].token_d = calloc(l->item_d[i].token_c, sizeof(item_t));
        }
        // we use just the first token_d
        token = &l->item_d[i].token_d[0];

        token->off = i_list[i];
        if (token->off)
        {
            //token->len = strlen( i_list[i] );
            count += 1;
            log_debug("%s, %s, %d", __FUNCTION__, token->off, count);
        }
        else
            token->len = 0;
    }

    /* update */
    l->item_c = count;      // layout items count
    layout_update_fsize(l); // and the field_size

    return count;
}

// used for Groups
void recreate_item_t(item_t** i)
{
    if (*i)
    {
        log_info("%s destroy %p, %p, %p", __FUNCTION__, i, *i, **i);
        for (int j = 0; j < i[0]->token_c + 1; j++)
        {
            log_info("destroy %i %p %d", j, &groups[j].token_d, groups[j].token_c);
        }
        free(*i), * i = NULL;
    }
    // build an array of index for Groups
    if (!*i)
        *i = analyze_item_t_v2(icon_panel->item_d, icon_panel->item_c);
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
    json_c = check_store_from_url(NULL, COUNT);
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
    if (!icon_panel) icon_panel = calloc(1, sizeof(layout_t));
    // pos.xy, size.wh
    icon_panel->bound_box = (vec4){ 680, 900,   1096, 664 };
    // five columns, three rows
    icon_panel->fieldsize = (ivec2){ 5, 3 };
    icon_panel->page_sel = (ivec2)(0);
    // malloc for max items
    icon_panel->item_c = json_c * icon_panel->fieldsize.x * icon_panel->fieldsize.y;
    icon_panel->item_d = calloc(icon_panel->item_c, sizeof(item_t));
    // reset count, we refresh from ground
    icon_panel->item_c = 0;
    // create the first screen we will show
    GLES2_create_layout_from_json(icon_panel);

    log_info("layout->item_c: %d", icon_panel->item_c);
    // shrink
    icon_panel->item_d = realloc(icon_panel->item_d, icon_panel->item_c * sizeof(item_t));

    // dynalloc games array
    if (!games) games = calloc(icon_panel->item_c + 1 /* 1st reserved */, sizeof(item_t));
    // use first as reserved index to store count
    games->token_c = icon_panel->item_c;
    // copy as games array 
    memcpy(&games[1], icon_panel->item_d, icon_panel->item_c * sizeof(item_t));
    // clean old buffer and swap with newly copy
    free(icon_panel->item_d), icon_panel->item_d = &games[1];

    // flag as to show it
    icon_panel->is_shown = 1;
    // set max_pages
    icon_panel->page_sel.y = icon_panel->item_c / (icon_panel->fieldsize.x * icon_panel->fieldsize.y);

    layout_update_fsize(icon_panel);
    icon_panel->vbo_s = ASK_REFRESH;

    /* resort using custom comparision function */
    //qsort(icon_panel->item_d, icon_panel->item_c, sizeof(item_t), struct_cmp_by_token);

#define HAVE_ATLAS (0) /// main
#if HAVE_ATLAS
    create_atlas_from_tokens(icon_panel);
#endif

    /* main left panel OLD_WAY! */

#if 1
    if (!left_panel) left_panel = calloc(1, sizeof(layout_t));

    left_panel->bound_box = (vec4){ 0, 900,   500, 700 };
    left_panel->fieldsize = (ivec2){ 1, 9 };
    // by calloc curr page is 0!
    /* malloc for max pages, plus additional list  */
    left_panel->item_c = LPANEL_Y;   //sizeof(new_panel_text) / sizeof(new_panel_text[0][0]);
    left_panel->item_d = calloc(left_panel->item_c + 1 /* Groups */, sizeof(item_t));
    // create the first page we will show
    GLES2_create_layout_from_list_v2(left_panel, (const char**)&new_panel_text[0][0]);
    // new_p indexes differently!
    left_panel->item_c = 10;
    log_info("layout->item_c: %d", left_panel->item_c);

    // build an array of indexes for each one Groups
    if (!groups) groups = analyze_item_t_v2(icon_panel->item_d, icon_panel->item_c);

    left_panel->item_d[LPANEL_Y].token_c = groups->token_c;
    log_info("groups->token_c: %d", groups->token_c);

    // flag as active
    left_panel->is_shown =
        left_panel->is_active = 1;
    // set max_pages
    left_panel->page_sel.y = LPANEL_Y;
    left_panel->vbo_s = ASK_REFRESH;
#endif
    {

        if (!option_panel) option_panel = calloc(1, sizeof(layout_t));

        option_panel->bound_box.xy = icon_panel->bound_box.xy;
        option_panel->bound_box.zw = (vec2){ 980, 670 };
        option_panel->fieldsize = (ivec2){ 2,   5 };
        option_panel->page_sel.x = 0;
        // malloc for max items
        option_panel->item_c = 11;

        option_panel->item_d = calloc(option_panel->item_c, sizeof(item_t));
        // create the first screen we will show

        log_info("%p %p %i", option_panel->item_c, option_panel->item_d, option_panel->item_c);
        layout_fill_item_from_list(option_panel, (char**)&option_panel_text[0]);

        log_info("layout->item_c: %d", option_panel->item_c);
        // flag as active
        option_panel->is_shown =
            option_panel->is_active = 0;
        // set max_pages
        option_panel->page_sel.y = LPANEL_Y;
        option_panel->vbo_s = ASK_REFRESH;
    }

    {
        if (!download_panel) download_panel = calloc(1, sizeof(layout_t));

        download_panel->bound_box = (vec4){ 500, 320,
                                              1024,  64 };
        download_panel->fieldsize = (ivec2){ 3, 1 };
        // malloc for max items
        download_panel->item_c = 2;//download_panel->fieldsize.x * download_panel->fieldsize.y;
        download_panel->item_d = calloc(download_panel->fieldsize.x * download_panel->fieldsize.y, sizeof(item_t));
        // create the first screen we will show
        if (get->auto_install) {
            download_panel->item_c--;
            download_panel_text[0] = (char*)getLangSTR(DL_AND_IN);
            download_panel_text[1] = (char*)getLangSTR(CANCEL);
        }
        layout_fill_item_from_list(download_panel, &download_panel_text[0]);

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
    GLES2_fonts_from_ttf(get->opt[FNT_PATH]);
 // fragments shaders, effects etc
    pixelshader_init(w, h);
#if HAVE_ATLAS
    InitTextureAtlas(); // one from cache
#endif
}


void GLES2_UpdateVboForLayout(layout_t* l)
{
    if (l)
    {   // skip until we ask to refresh VBO
        if (l->vbo_s < ASK_REFRESH) return;

        if (l->vbo) vertex_buffer_delete(l->vbo), l->vbo = NULL;

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
    bool is_author = false;
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
           for (int i = 0; i < icon_panel->item_c; i++) {
               if (strcmp(icon_panel->item_d[i].token_d[ID].off, query) == 0) {
                   Install_View(icon_panel, query, ID);
                   is_author = true;
                   break;
               }
           }
           if(!is_author)
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

        if (get->auto_install) 
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

bool reboot_app = false;
// just used in download panel and Settings
void X_action_dispatch(int action, layout_t* l)
{
    char tmp[1024];
    // selected item from active panel
    log_info("%s execute %d -> '%s'", __FUNCTION__, action, l->item_d[action].token_d[0].off);


    /* actually on Setting we jump there... */
    if (l == option_panel) goto actions_for_Settings;

    char* trigger = l->item_d[action].token_d[0].off;

    int label = NAME;
    switch (action)
    {
    case 0: label = PACKAGE; break;
    default:                 break;
    }
    // which item is selected by icon_panel ?
    int idx = get_item_index(icon_panel);
    // address the selected item
    item_idx_t* t = &icon_panel->item_d[idx].token_d[0];
    // selected item from index in icon_panel, print token_data label
    log_info("item:%d, %s '%s'", idx,
        trigger,
        // icon_panel->item_d[ idx ].token_d[ label ].off,
        t[label].off);
    // default
    //"/user/app/temp.pkg"
    // avoid same donwload path
    snprintf(&tmp[0], 255, "%s/%s.pkg", get->opt[TMP_PATH], t[ID].off);
    log_info(tmp);

    dl_arg_t* ta = NULL;
    int x = thread_find_by_item(idx);
    if (x > -1) // a thread is operating on item
    {
        ta = &pt_info[x];
        log_info("thread[%d] is handling %d, status:%d", x, idx, ta->status);
        //action = -1; // disable, skip action
    }

    switch (action)
    {   /* Donwload */
    case 0: {
        if(ta && ta->status == RUNNING)
        {
            ta->status = PAUSED;
            download_panel_text[0] = (char*)getLangSTR(RESUME_2);
            goto draw;
        }
        else if(ta && ta->status == PAUSED)
        {
            ta->status = RUNNING;
            download_panel_text[0] = (char*)getLangSTR(PAUSE_2);
            goto draw;
        }
        else if(ta && ta->status == COMPLETED) goto install;
        // a thread is downloading item, skip
        if (x > -1) return;
        //New Download API
        if (!unsafe_source){
           sqlite3_free(t[label].off);
           t[label].off = sqlite3_mprintf("%s/download.php?tid=%s", get->opt[CDN_URL], t[ID].off);
        }

        if (strcmp(t[ID].off, STORE_TID) != 0)
        {
            log_info("t[label].off %s",t[label].off);
            download_panel->item_c++;
            download_panel_text[0] = (char*)getLangSTR(PAUSE_2);
            loadmsg(getLangSTR(DL_CACHE));
            http_ret = dl_from_url_v2(t[label].off, &tmp[0], t); //Download from Legacy
            if(http_ret == 200)
                icon_panel->item_d[idx].interuptable = true;
            sceMsgDialogTerminate();
        }
        else
            msgok(WARNING, getLangSTR(NOT_ON_APP));

draw:
        layout_fill_item_from_list(download_panel, &download_panel_text[0]);

    } break;
        /* Install */
    case 1: {
        if (get->auto_install)
            goto opt_3;
install:
        if (ta
            && ta->status == COMPLETED)
        {
            #ifdef __ORBIS__
            pkginstall(&tmp[0], ta, get->auto_install);
            #else
            log_info("install %s", &tmp[0]);
            // clean thread args for next job
            if(ta->dst) free((void*)ta->dst), ta->dst = NULL;
            memset(ta, 0, sizeof(dl_arg_t));
            ta->g_idx  = -1;
            ta->status = READY;
            log_info("Set Status: Ready");     
            #endif       
            icon_panel->item_d[icon_panel->curr_item].interuptable = false;


        }

    } break;
    case 2: /*option 3*/ {
    opt_3:
        if (ta 
             && ta->status > READY)
        {
            unlink(ta->dst);
            if(ta->dst) free((void*)ta->dst), ta->dst = NULL;
            memset(ta, 0, sizeof(dl_arg_t));
            ta->g_idx  = -1;
            ta->status = CANCELED;
            icon_panel->item_d[icon_panel->curr_item].interuptable = false;
            
            log_info("Set Status: CANCELED");
        }
        break;
    }
    default:
        log_info("more");
        break;
    }
    return;

actions_for_Settings:

    switch (action)
    {   // use sceKbd to edit related field
    case CDN_SETTING:
    case TMP_SETTING:
    case INI_SETTING:
    case FNT_SETTING:

        if (action == INI_PATH)
            msgok(WARNING, getLangSTR(NEW_INI));

        log_warn("%p, %s", l, get->opt[action]);
        
        if(!Keyboard(l->item_d[action].token_d[0].off, get->opt[action], &tmp[0], (action == CDN_SETTING) ? true : false)) break;
        // clean string, works for LF, CR, CRLF, LFCR, ...
        tmp[strcspn(tmp, "\r")] = 0;

        // check for valid result
        if (strlen(tmp) <= 1) {
            msgok(NORMAL, getLangSTR(STR_TOO_LONG));  goto error;
        }

        if (strstr(tmp, "ps4h3x") != NULL || strstr(tmp, "rooted.gq") != NULL)
        {
            msgok(WARNING, "Fuck off you Tatto ke sudagar, also tell your mom i said hi :)");
            goto error;
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
        if (action == CDN_URL && !pingtest(tmp))
        {
            msgok(NORMAL, getLangSTR(INVAL_CDN));  goto error;
        }

        // update 
        log_info("Options: %i Before: %.30s ->  After:  %.30s", action, get->opt[action], tmp);

        // store
        snprintf(get->opt[action], 255, "%s", tmp);

        log_info("get->opt[%i]:  %.20s", action, get->opt[action]);

        // try to load custom font
        if (action == FNT_PATH) {
            if (strstr(get->opt[action], ".ttf") != NULL)
                GLES2_fonts_from_ttf(get->opt[action]);
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

        if (!get->auto_install && get->Legacy_Install) {
            msgok(WARNING, getLangSTR(AUTO_FAILURE_ERROR));
            break;
        }
        get->auto_install = (get->auto_install) ? false : true;
        log_info("auto_install: %d", get->auto_install);

        if (get->auto_install) {
            download_panel->item_c--;
            download_panel_text[0] = (char*)getLangSTR(DL_AND_IN);
            download_panel_text[1] = (char*)getLangSTR(CANCEL);
        }
        else {
            download_panel->item_c++;
            download_panel_text[0] = (char*)getLangSTR(DL2);
            download_panel_text[1] = (char*)getLangSTR(INSTALL2);
            download_panel_text[2] = (char*)getLangSTR(CANCEL);
        }

        layout_fill_item_from_list(download_panel, &download_panel_text[0]);

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
        if (get->auto_install && !get->Legacy_Install) {
            msgok(WARNING, getLangSTR(INSTALL_PROG_ERROR));
            break;
        }
        get->Legacy_Install = (get->Legacy_Install) ? false : true;
        log_info("Legacy_Install: %d", get->Legacy_Install);
        break;
    }
    case RESET_SETTING: {
        log_info("Settings -> Reset");

        strcpy(get->opt[CDN_URL], "https://api.pkg-zone.com");
        strcpy(get->opt[TMP_PATH], "/user/app/NPXS39041/downloads");
        strcpy(get->opt[FNT_PATH], "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF");
        get->auto_install = true;
        get->Legacy_Install = false;

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
        log_info("%p, %d", l, action);
        if (!SaveOptions(get))
            msgok(WARNING, getLangSTR(SAVE_ERROR));
        else
        {
            msgok(NORMAL, getLangSTR(SAVE_SUCCESS));
            log_info("Settings saved to %s", get->opt[INI_PATH]);
            // reload settings
            LoadOptions(get);
        }
        break;
    }
    }
    GLES2_Refresh_for_settings();
    return;
}

void O_action_dispatch(void)
{
    refresh_atlas();
}
