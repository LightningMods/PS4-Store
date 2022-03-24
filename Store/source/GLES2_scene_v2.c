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
#include "shaders.h"

#if defined(__ORBIS__)
    #include <orbisPad.h>
    extern OrbisPadConfig *confPad;
    #include <installpkg.h>

#endif

#include <freetype-gl.h>
// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;

#include "defines.h"
#include "GLES2_common.h"

#include "jsmn.h"

int jsoneq(const char *json, jsmntok_t *tok, const char *s);

#include "utils.h"

int install_ret = -1, http_ret = -1;

extern const unsigned char completeVersion[];

bool install_done = false;

// the Settings
extern StoreOptions set,
                   *get;

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
int struct_cmp_by_token(const void *a, const void *b)
{
    item_t *ia = (item_t *)a;
    item_t *ib = (item_t *)b;
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
     white = (vec4) ( 1. ),
     col   = (vec4) { .8164, .8164, .8125, 1. }, // text fg
     t_sel;

extern double u_t;
extern int    selected_icon;
       ivec4  menu_pos = (0),
              rela_pos = (0);

extern texture_font_t  *main_font, // small
                        *sub_font, // left panel
                       *titl_font;
extern vertex_buffer_t *text_buffer[4];
// posix threads args, for downloading
extern dl_arg_t        *pt_info;
// available num of jsons
int json_c = 0;

layout_t *active_p     = NULL, // the one we control
           *icon_panel = NULL,
           *left_panel = NULL,
         *option_panel = NULL,
       *download_panel = NULL,
          *queue_panel = NULL;
/*
    array for Special cases
*/
item_t* games = NULL,
* groups = NULL,
* all_apps = NULL;
/*
    auxiliary array of item_index_t:
    - shared for Search/Groups
    - the query to save search results;
*/
item_idx_t *aux = NULL,
           *q   = NULL; // Search Query

// common title in different panels
GLuint fallback_t = 0;  // fallback texture

// unused
void GLES2_destroy_layout(layout_t *l)
{
    if(!l) return;

//    clean_textures(l);

    item_idx_t *token_d = NULL;
    for(int i = 0; i < l->item_c; i++)
    {
        token_d = &l->item_d[i].token_d[0];
        for(int j = 0; j < NUM_OF_USER_TOKENS; j++)
                        // < token_c
        {
            if(token_d[j].off) free(token_d[j].off);
        }
        if(token_d) free(token_d);
    }
    if(l->item_d) free(l->item_d);

    if(l->vbo) vertex_buffer_delete(l->vbo), l->vbo = NULL;

    if(l) free(l), l = NULL;
}


int GLES2_create_layout_from_json(layout_t *l)
{
    char  json_file[128];
    void *p;
    int   count = 0;

    count = SQL_Get_Count();

    int valid_tokens = sql_index_tokens(l, count);

    log_info( "%d valid_tokens!", valid_tokens);

    return valid_tokens;
}

extern const char *option_panel_text[];
char* new_panel_text[LPANEL_Y][11];
char* download_panel_text[3];
/*
    indexes item_t item_data
*/
int GLES2_create_layout_from_list_v2(layout_t *l, const char **i_list)
{
    int i, j;
    item_idx_t *token = NULL;

    for(i = 0; i < l->item_c; i++)
    {   // dynallocs for items
        if(!l->item_d[i].token_d)
        {
            l->item_d[i].token_c = 10; // minimum
            l->item_d[i].token_d = calloc(l->item_d[i].token_c, sizeof(item_idx_t));
        }
        // iterate token_data
        for(j = 0; j < l->item_d[i].token_c; j++)
        {
            token      = &l->item_d[i].token_d[j];
            token->off = new_panel_text[i][j];
            if(!token->off)
            { token->len = -1; break; } // stop here
            else
                token->len = strlen( token->off );
            //
            log_info( "%s %d", l->item_d[i].token_d[j].off,
                            l->item_d[i].token_d[j].len);
        }
        if(j < l->item_d[i].token_c)
        {   // shrink
            l->item_d[i].token_c = j;
            l->item_d[i].token_d = realloc(l->item_d[i].token_d,
                                           l->item_d[i].token_c * sizeof(item_idx_t)); }
    }
    return i;
}

// option
extern bool use_reflection,
            use_pixelshader;


// testing new way to detect a view
bool is_Installed_Apps = false;

bool is_Installed_Apps_view(void)
{
    if( !left_panel->is_active
    &&   left_panel->page_sel.x == 0
    &&   left_panel->item_sel.y == 1 ) return true;

    return false;
}

// return index to selected item_data token_data
int get_item_index(layout_t *l)
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
    if(aux)
        idx = aux[ i_paged + l->f_sele
                 + 1    // skip first reserved
                 ].len; // read stored index
    return idx;
}

/*
    indexes item_idx_t* token_data in a single item_t item_data

    indexes means "just address all pointers, *zerocopy* !"
    so you have to take care to where those are pointing...
*/
int layout_fill_item_from_list(layout_t* l, const char** i_list)
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
void recreate_item_t(item_t **i)
{
    if( *i )
    {
        log_info("%s destroy %p, %p, %p", __FUNCTION__, i, *i, **i);
        for(int j = 0; j < i[0]->token_c + 1; j++)
        {
            log_info("destroy %i %p %d", j, &groups[j].token_d, groups[j].token_c);
        }
        free(*i), *i = NULL;
    }
    // build an array of index for Groups
    if( ! *i )
          *i = analyze_item_t_v2(icon_panel->item_d, icon_panel->item_c);
}

// no page 0, start from 1
void GLES2_scene_init( int w, int h )
{
    resolution = (vec2) { w, h };
    // reset player pos
    p1_pos = (vec2) (0);
    // normalize colors
    sele   = (vec4) {  29, 134, 240, 256 } / 256, // blue
    grey   = (vec4) {  66,  66,  66, 256 } / 256; // fg

    /* first, scan folder, populate Installed_Apps */

    // build an array of indexes for each one entry there
    mkdir("/user/app/", 0777);
    log_info("Searching for Apps");
    if (!all_apps) all_apps = index_items_from_dir("/user/app", "/mnt/ext0/user/app");
    /* now, scan .json files, populate main icon panel */
    // try to compose any page, count available ones
    json_c =  check_store_from_url(NULL, COUNT);
    log_info( "[StoreCore] %d available json!", json_c);

    // setup one for all missing, the fallback icon0
    if (!fallback_t)
    {
        if(if_exists("/user/appmeta/NPXS39041/icon0.png"))
          fallback_t = load_png_asset_into_texture("/user/appmeta/NPXS39041/icon0.png");
        else
          fallback_t = load_png_asset_into_texture("/user/appmeta/external/NPXS39041/icon0.png");
    }

    /* icon panel setup */
    if( ! icon_panel ) icon_panel = calloc(1, sizeof(layout_t));
    // pos.xy, size.wh
    icon_panel->bound_box =  (vec4) { 680, 900,   1096, 664 };
    // five columns, three rows
    icon_panel->fieldsize = (ivec2) { 5, 3 };
    icon_panel->page_sel  = (ivec2) (0);
    // malloc for max items
    icon_panel->item_c    = json_c * icon_panel->fieldsize.x * icon_panel->fieldsize.y;
    icon_panel->item_d    = calloc(icon_panel->item_c, sizeof(item_t));
    // reset count, we refresh from ground
    icon_panel->item_c    = 0;
    // create the first screen we will show
    GLES2_create_layout_from_json(icon_panel);

    log_info( "layout->item_c: %d", icon_panel->item_c);
    // shrink
    icon_panel->item_d    = realloc(icon_panel->item_d, icon_panel->item_c * sizeof(item_t));

    // dynalloc games array
    if( ! games ) games = calloc(icon_panel->item_c +1 /* 1st reserved */, sizeof(item_t));
    // use first as reserved index to store count
    games->token_c = icon_panel->item_c;
    // copy as games array 
    memcpy(&games[ 1 ], icon_panel->item_d, icon_panel->item_c * sizeof(item_t));
    // clean old buffer and swap with newly copy
    free(icon_panel->item_d), icon_panel->item_d = &games[1];

    // flag as to show it
    icon_panel->is_shown  = 1;
    // set max_pages
    icon_panel->page_sel.y = icon_panel->item_c / (icon_panel->fieldsize.x * icon_panel->fieldsize.y);

    layout_update_fsize(icon_panel);
    icon_panel->vbo_s = ASK_REFRESH;

    /* resort using custom comparision function */
    //qsort(icon_panel->item_d, icon_panel->item_c, sizeof(item_t), struct_cmp_by_token);

    /* now check for badges: scan items for Installed_Apps */
    scan_for_badges(icon_panel, all_apps);

#define HAVE_ATLAS (0) /// main
#if HAVE_ATLAS
    create_atlas_from_tokens(icon_panel);
#endif

    /* main left panel OLD_WAY! */

#if 1
    if( ! left_panel ) left_panel = calloc(1, sizeof(layout_t));

    left_panel->bound_box =  (vec4) { 0, 900,   500, 700 };
    left_panel->fieldsize = (ivec2) { 1, 9 };
    // by calloc curr page is 0!
    /* malloc for max pages, plus additional list  */
    left_panel->item_c    = LPANEL_Y;   //sizeof(new_panel_text) / sizeof(new_panel_text[0][0]);
    left_panel->item_d    = calloc(left_panel->item_c + 1 /* Groups */, sizeof(item_t));
    // create the first page we will show
    GLES2_create_layout_from_list_v2(left_panel, &new_panel_text[0][0]);
    // new_p indexes differently!
    left_panel->item_c = 10;
    log_info( "layout->item_c: %d", left_panel->item_c);

    // build an array of indexes for each one Groups
    if( ! groups ) groups = analyze_item_t_v2(icon_panel->item_d, icon_panel->item_c);

    left_panel->item_d[LPANEL_Y].token_c = groups->token_c;
    log_info( "groups->token_c: %d", groups->token_c);

    // flag as active
    left_panel->is_shown  =
    left_panel->is_active = 1;
    // set max_pages
    left_panel->page_sel.y = LPANEL_Y;
    left_panel->vbo_s = ASK_REFRESH;
#endif
  {
    if( ! option_panel ) option_panel = calloc(1, sizeof(layout_t));

    option_panel->bound_box.xy = icon_panel->bound_box.xy;
    option_panel->bound_box.zw =  (vec2) { 980, 670 };
    option_panel->fieldsize    = (ivec2) {   2,   5 };
    option_panel->page_sel.x   = 0;
    // malloc for max items
    option_panel->item_c = 11;
    option_panel->item_d = calloc(option_panel->item_c, sizeof(item_t));
    // create the first screen we will show
    layout_fill_item_from_list(option_panel, &option_panel_text[0]);

    log_info( "layout->item_c: %d", option_panel->item_c);
    // flag as active
    option_panel->is_shown  =
    option_panel->is_active = 0;
    // set max_pages
    option_panel->page_sel.y = LPANEL_Y;
    option_panel->vbo_s = ASK_REFRESH;
  }

  {
    if( ! download_panel ) download_panel = calloc(1, sizeof(layout_t));

    download_panel->bound_box  =  (vec4) { 500, 320,
                                          1024,  64 };
    download_panel->fieldsize  = (ivec2) { 3, 1 };
    // malloc for max items
    download_panel->item_c     = download_panel->fieldsize.x * download_panel->fieldsize.y;
    download_panel->item_d     = calloc(download_panel->item_c, sizeof(item_t));
    // create the first screen we will show
    layout_fill_item_from_list(download_panel, &download_panel_text[0]);

    log_info( "layout->item_c: %d", download_panel->item_c);
    // flag as active
    download_panel->is_shown   =
    download_panel->is_active  = 0;
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
    GLES2_fonts_from_ttf(get->opt[ FNT_PATH ]);
    // init ani_notify()
    GLES2_ani_init(w, h);
    // fragments shaders, effects etc
    pixelshader_init(w, h);
    // badges
    GLES2_Init_badge();
    // coverflow alike, two parts for now
    InitScene_4(w, h);
    InitScene_5(w, h);

#if HAVE_ATLAS
    InitTextureAtlas(); // one from cache
#endif
}


void GLES2_UpdateVboForLayout(layout_t *l)
{
    if(l)
    {   // skip until we ask to refresh VBO
        if(l->vbo_s < ASK_REFRESH) return;

        if(l->vbo) vertex_buffer_delete(l->vbo), l->vbo = NULL;

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
void GLES2_scene_render(void)
{
    // update the time
    on_GLES2_Update (u_t);
    on_GLES2_Update1(u_t);
    GLES2_ani_update(u_t);

    switch(menu_pos.z) // as view
    {
        // the two sided panels view
        case ON_LEFT_PANEL :
        case ON_MAIN_SCREEN:
            GLES2_render_paged_list(0); // new way: left_panel2
            GLES2_render_icon_list(0);  // new way: icon_panel clone
            break;
        // download item view
        case ON_ITEM_INFO:
            GLES2_render_download_panel(); // new way
            break;
        // Ready to install view
        case ON_INSTALL:
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
       case ON_FILEMANAGER:
          //GLES2_render_filemanager(0);
            break;
       case ON_ITEMzFLOW:
            DrawScene_4();
            break;
        default: 
            break;
    }

    //show_dumped_frame();

    GLES2_Draw_sysinfo();

    GLES2_Draw_common_texts();

    GLES2_ani_draw();

    // ...

    layout_refresh_VBOs();
}


// just used in download panel and Settings
void X_action_dispatch(int action, layout_t *l)
{
    char tmp[1024];
    // selected item from active panel
    log_info( "%s execute %d -> '%s'", __FUNCTION__, action, l->item_d[ action ].token_d[0].off);


    /* actually on Setting we jump there... */
    if(l == option_panel) goto actions_for_Settings;

    char *trigger = l->item_d[ action ].token_d[0].off;

    int label = NAME;
    switch(action)
    {
        case 0: label = PACKAGE; break;
        default:                 break;
    }
    // which item is selected by icon_panel ?
    int idx       = get_item_index(icon_panel);
    // address the selected item
    item_idx_t *t = &icon_panel->item_d[ idx ].token_d[0];
    // selected item from index in icon_panel, print token_data label
    log_info( "item:%d, %s '%s'", idx,
                                        trigger,
                                     // icon_panel->item_d[ idx ].token_d[ label ].off,
                                        t[ label ].off);
    // default
    //"/user/app/temp.pkg"
    // avoid same donwload path
    snprintf(&tmp[0], 255, "%s/%s.pkg", get->opt[ TMP_PATH ], t[ ID ].off);
    log_info(tmp);

    dl_arg_t *ta = NULL;
    int x = thread_find_by_item( idx );
    if( x > -1 ) // a thread is operating on item
    {
        ta = &pt_info[ x ];
        log_info( "thread[%d] is handling %d, status:%d", x, idx, ta->status);
        //action = -1; // disable, skip action
    }

    switch(action)
    {   /* Donwload */
        case 0: {
            // a thread is downloading item, skip
            if(x > -1) return;
            char API_BUF[300];
            //New Download API

            snprintf(API_BUF, sizeof(API_BUF), "%s/download.php?tid=%s", get->opt[CDN_URL], t[ID].off);
            if (strcmp(t[ID].off, STORE_TID) != NULL)
            {
                log_info("t[label].off %s", t[label].off);
                    //Check for the Legacy INI Setting, its a Bool, and check to be Sure its not trying to download a direct DL (.pkg)
                    if (strstr(t[label].off, "pkg-zone.com") != NULL )
                        http_ret = dl_from_url_v2(API_BUF, &tmp[0], t); //Download from New API
                    else
                        http_ret = dl_from_url_v2(t[label].off, &tmp[0], t); //Download from Legacy
            }
            else
                msgok(WARNING, getLangSTR(NOT_ON_APP));

        } break;
        /* Install */
        case 1: {
            if(ta
            && ta->status == COMPLETED)
            {
                log_info("pkginstall(%s,%i,%i)", tmp, ta->contentLength, get->Show_install_prog);
                // install
                pkginstall(&tmp[0], ta->contentLength, get->Show_install_prog);

                // clean thread args for next job
                if(ta->dst) free((void*)ta->dst), ta->dst = NULL;
                memset(ta, 0, sizeof(dl_arg_t));
                ta->g_idx  = -1;
                ta->status = READY;
            }
        } break;
        default:
            log_info("more");
          break;
    }
    return;

actions_for_Settings:

    switch(action)
    {   // use sceKbd to edit related field
        case CDN_SETTING : 
        case TMP__SETTING:
        case INI_SETTING:
        case FNT__SETTING:
        
            if(action == INI_PATH)
                 msgok(WARNING, getLangSTR(NEW_INI));

            log_warn("%p, %s", l, get->opt[action]);
            snprintf(&tmp[0], 1024, "%s", StoreKeyboard(l->item_d[action].token_d[0].off, get->opt[action]));
            // clean string, works for LF, CR, CRLF, LFCR, ...
            tmp[strcspn(tmp, "\r")] = 0;

            // check for valid result
            if (strlen(tmp) <= 1 ) {
                msgok(NORMAL, getLangSTR(STR_TOO_LONG));  goto error;
            }

            if (strstr(tmp, "ps4h3x") != NULL)
            {
                msgok(WARNING, "Fuck off you Tatto ke sudagar, also tell your mom i said hi :)");
                goto error;
            }

            if (action == INI_PATH && !touch_file(tmp) || action != CDN_URL && !if_exists(tmp))
            {
                msgok(NORMAL, getLangSTR(INVAL_PATH));
                goto error;
            }

            if((action == TMP_PATH && strstr(tmp, "pkg") != NULL) || strstr(tmp, "/data") != NULL)
            {
                msgok(NORMAL, getLangSTR(PKG_SUF)); goto error;
            }

            // validate result (CDN)
            if(action == CDN_URL && strstr(tmp, "://") == NULL )
            {
                msgok(NORMAL, getLangSTR(INVAL_CDN));  goto error;
            }

            // update 
            log_info("Options: %i Before: %.30s ->  After:  %.30s", action, get->opt[action], tmp);

            // store
            snprintf(get->opt[action], 255, "%s", tmp);

            log_info("get->opt[%i]:  %.20s", action, get->opt[action]);

            // try to load custom font
            if(action == FNT_PATH) {
                if(strstr(get->opt[action],".ttf") != NULL)
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
        case STORE_USB_SETTING: {//STORE_ON_USB
            if (get->StoreOnUSB)
            {
                get->StoreOnUSB = 0;
                snprintf(get->opt[TMP_PATH], 255, "%s", "/user/app/NPXS39041/downloads");
            }
            else {
                get->StoreOnUSB = 1;
                snprintf(get->opt[TMP_PATH], 255, "%s", "/mnt/usb0");
            }
            break;
        }

        case CLEAR_CACHE_SETTING: { // clear cached images
            loadmsg(getLangSTR(CLEARING_CACHE));
            unlink(STORE_LOG);
            FILE* fp = fopen(STORE_LOG, "w");
            log_add_fp(fp, LOG_DEBUG);
            log_info("Settings -> Clear Cached images and content");

            if (rmtree("/user/app/NPXS39041/covers") && rmtree("/user/app/NPXS39041/storedata") && rmtree("/user/app/NPXS39041/pages"))
                msgok(NORMAL, getLangSTR(CACHE_CLEARED));
            else
                msgok(WARNING, getLangSTR(CACHE_FAILED));

            mkdir("/user/app/NPXS39041/covers", 0777); 
            mkdir("/user/app/NPXS39041/pages", 0777);
            mkdir("/user/app/NPXS39041/storedata", 0777);

            break;
        }

        case SHOW_INSTALL_PROG: {
            get->Show_install_prog = (get->Show_install_prog) ? false : true;
            log_info("SHOW_INSTALL_PROG: %d", get->Show_install_prog);
            break;
        }
        case USE_PIXELSHADER_SETTING: {
            use_pixelshader = (use_pixelshader) ? false : true;
            log_info("use_pixelshader: %d", use_pixelshader);
            break;
        }
        case HOME_MENU_SETTING: {
            //
            if (is_connected_app) {
                get->HomeMenu_Redirection = (get->HomeMenu_Redirection) ? false : true;
                log_info("Turning Home menu %s", get->HomeMenu_Redirection ? "ON (ItemzFlow)" : "OFF (Orbis)");

                uint8_t* IPC_BUFFER = malloc(100);
                int error = INVALID, wait = INVALID;

                if (get->HomeMenu_Redirection)
                {
                    error = IPCSendCommand(ENABLE_HOME_REDIRECT, IPC_BUFFER);
                    if (error == NO_ERROR)
                        log_debug("HOME MENU REDIRECT IS ENABLED");
                }
                else
                {
                    error = IPCSendCommand(DISABLE_HOME_REDIRECT, IPC_BUFFER);
                    if (error == NO_ERROR)
                        log_debug("HOME MENU REDIRECT IS DISABLED");
                }
                free(IPC_BUFFER);
            }
            else
                msgok(WARNING, getLangSTR(ITEMZ_FEATURE_DISABLED));

            break;
        }
        case SAVE_SETTINGS: {//SAVE_OPTIONS:
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
