/*
    GLES2_layout.c

    part of the GLES2 playground: draw a basic UI
    holding a selector to trigger some action, etc

    _from now on 'l' stands for 'layout'_

    2020, 2021, masterzorag
*/

#include "defines.h"
#include "GLES2_common.h"
#include "utils.h"
#include "shaders.h"
#include <stdatomic.h>
#include <pthread.h>
// store-clone / v2
layout_t* left_panel2 = NULL;
extern vec2 resolution;

/* update field_size for layout: eventually reduce
   field_size to match remaining item count */
void layout_update_fsize(layout_t *l)
{
    l->f_size = l->fieldsize.x * l->fieldsize.y;
    int count = l->item_c;
    // check for aux
    if(l == icon_panel
    && aux
    && aux->len) count = aux->len;

    if(l != left_panel2) 
    {   // minus paged field_size items
        count -= l->f_size * l->page_sel.x;
    }
    // reduce field_size
    if( count < l->f_size )
    {
        l->f_size = count;
        l->vbo_s  = ASK_REFRESH;
    }
}

/* each keypress updates linear and vector indexes */
void layout_update_sele(layout_t *l, int movement)
{
    // unconditionally ask to refresh
    l->vbo_s = ASK_REFRESH;

    if(l == icon_panel && 
    l->item_sel.x == 0 && movement == -1)
    {
        // drop aux
        aux = NULL;
        // then point back to Games and its count
        l->item_c =  games->token_c;
        l->item_d = &games[1];
        l->curr_item = 0;
        layout_update_fsize(l);
        // switch active
        active_p = left_panel2;
        menu_pos = ON_LEFT_PANEL;
        return;
    }

    // update all related to this one!
    int idx = l->curr_item + movement;

    if(idx < 0) idx = l->item_c -1, l->vbo_s = ASK_REFRESH;
    if(idx > l->item_c -1) idx = 0, l->vbo_s = ASK_REFRESH;
    // update vector and linear index
    l->item_sel  = (ivec2) { idx % l->fieldsize.x,
                             idx / l->fieldsize.x };
    l->curr_item = idx;
    l->f_size    = l->fieldsize.x * l->fieldsize.y;

    if(l->f_size)
    {
        l->f_sele = idx % l->f_size;
        // update pages count
        if(l != left_panel2)
        {
            l->page_sel = (ivec2) { idx / l->f_size,
                              l->item_c / l->f_size };
        }
    }
    else // guard the div by zero
    {
        l->f_sele   = 0,
        l->page_sel = (ivec2) (0);
    }
    layout_update_fsize(l);
}

// v2, for migration
void layout_set_active(layout_t *l)
{
    active_p = l;
}

// f_rect helper: scale by outline border, in px
vec4 frect_add_border(vec4 *in, int border)
{   /* increase the size of rectangle by applying some
       border pixels offset to each of two rect points */
    vec4 r = *in;
    vec2 o = (vec2) ( border ) / resolution *2;
     r.xy -= o,
     r.zw += o;
    // normalized rectangle (.xyzw) as (p1.xy, p2.xy)
    return r;
}

char selected_text[64];
bool is_sort_panel = false;
/* default way to deal with text, for layout_t */
static void layout_compose_text(layout_t *l, int idx, vec2 *pen, bool save_text)
{
#if 0
    log_info("%s %p, %d", __FUNCTION__, l, save_text);
#endif

    if( save_text )
        memset(&selected_text[0], 0, sizeof(selected_text));

    char tmp[64],
        *data   = NULL,
        *format = "%d: %p";
    // default indexing, item_t* AOS
    item_t  *li = &l->item_d[ idx ];

    ///XXX
    if(li->token_d[0].len < 0)
        log_debug( "%s %d", __FUNCTION__, idx);

    if(li && li->token_d) // we have token_data?
    {
        data = li->token_d[0].off;
        if(data)
        {
            format = "%s"; // default/fallback one

            if(l == icon_panel) // just the selected showing NAME, ID
            {
                data = li->token_d[ NAME ].off;
                //
                if( save_text )
                    snprintf(&selected_text[0], 63, "%s", data);
                // cut on more than N characters
                if(li->token_d[ NAME ].len > 18)
                    snprintf(&tmp[0], 22, "%.18s...", data);
                else
                    snprintf(&tmp[0], 63, format, data);

                // we draw 2 lines and use a smaller font
                vec2 bk = *pen;
                pen->y += 20;  add_text( l->vbo, main_font, &tmp[0], &white, pen);
                // second line, ID
                   data = li->token_d[ ID ].off;
                   snprintf(&tmp[0], 63, format, data);
                   *pen = bk;  add_text( l->vbo, main_font, &tmp[0], &white, pen);
                return;
            }
            else
            if(l == left_panel2)
            {        
                snprintf(&tmp[0], 63, format, data);
                //
                if (l->page_sel.x != 3)
                    is_sort_panel = false;

                if( save_text )
                    snprintf(&selected_text[0], 63, "%s", tmp);
                if(l->page_sel.x == 0) // on main page
                {
                    /* draw Installed_Apps, Groups, Ready_to_install, Queue total count numbers */
                    int ret        = 0,
                        req_status = 0;
                    // one item label per line
                    switch( idx )
                    {
                        case 1:  ret =   number_of_iapps(APP_PATH("../"));  break; // Installed_Apps has first reserved
                        case 0:  ret =   games->token_c;  break; 
                        case 2:  ret =  groups->token_c;  break; // Groups
                        case 3: { 
                            if (get->auto_install)
                               req_status = INSTALLING_APP;
                            else
                                req_status = COMPLETED;

                            break;
                        }
                        case 4:  req_status = RUNNING;  break; // Queue
                    }
                    // if requested, count threads
                    if(req_status) {
                        ret = thread_count_by_status( req_status );
                        if(idx == 4)
                            ret += thread_count_by_status( PAUSED );
                    }

                    if(ret > 0) // we counted at least one
                    {
                        add_text( l->vbo, sub_font, &tmp[0], &col, pen);
                        snprintf(&tmp[0], 63, "%d", ret);
                        texture_font_load_glyphs( sub_font, &tmp[0] );
                        pen->x = 460 - tl;
                    }
                }
                if(l->page_sel.x == 2) // on Group page
                {   // get Group label
                    data = groups[ idx +1 ].token_d[0].off;
                    snprintf(&tmp[0], 63, format, data);
                    add_text( l->vbo, sub_font, &tmp[0], &col, pen);
                    //
                    if( save_text )
                        snprintf(&selected_text[0], 63, "%s", tmp);
                    // get Group count
                    snprintf(&tmp[0], 63, "%d", groups[ idx +1 ].token_c);
                    texture_font_load_glyphs( sub_font, &tmp[0] );
                    pen->x = 460 - tl;
                    is_sort_panel = false;
                }
                // 3 Sort_by uses default
                if(l->page_sel.x == 3) // on Sot_by page
                {
                    is_sort_panel = true;
                    snprintf(&tmp[0], 63, format, new_panel_text[ 3 ][ idx ]);
                    log_info("new_panel_text[ 3 ][ %i ], %s", idx, new_panel_text[3][idx]);
                    if( save_text )
                        snprintf(&selected_text[0], 63, "%s", tmp);
                }

                if(l->page_sel.x == 4) // uses token_data from just 1st
                {
                }

                if(l->page_sel.x == 5) // on Filter_by page, draw items count
                {
                    add_text( l->vbo, sub_font, &tmp[0], &col, pen);
                    if( save_text )
                         snprintf(&selected_text[0], 63, "%s", tmp);
                    // read item count from token data
                    snprintf(&tmp[0], 63, "%d", l->item_d[ 0 ].token_d[ idx +1 ].len);
                    texture_font_load_glyphs( sub_font, &tmp[0] );
                    pen->x = 460 - tl;
                }
            }
            else
            if(l == option_panel)
            {
                snprintf(&tmp[0], 63, format, data);
                // append to vbo
                vec2 tp    = *pen;
                     tp.y +=  60.;
                add_text( l->vbo, sub_font, &tmp[0], &col, &tp);
                if( save_text )
                     snprintf(&selected_text[0], 63, "%s", tmp);
                // zerofill for second line
                memset(&tmp[0], 0, sizeof(tmp));
                // get the option value
                if(idx < NUM_OF_STRINGS && idx != REFRESH_DB_SETTING)
                {   // shorten any entry longer than n chars
                    if(strlen(get->opt[ idx ]) > 40) format = "%.40s...";

                    snprintf(&tmp[0], 63, format, get->opt[ idx ]);
                }
                else
                {   // to extend for more options
                    switch(idx)
                    {
                        case AUTO_INSTALL_SETTING: snprintf(&tmp[0], 63, format, get->auto_install ? getLangSTR(ON2) : getLangSTR(OFF2)); break;
                        case REFRESH_DB_SETTING: break;
                        case LEGACY_INSTALL_PROG: snprintf(&tmp[0], 63, format, get->Legacy_Install  ? getLangSTR(ON2) : getLangSTR(OFF2)); break;
                        default: break;
                    }
                }
            }
            else // default/fallback one
            {
                snprintf(&tmp[0], 63, format, data);
                if( save_text )
                     snprintf(&selected_text[0], 63, "%s", tmp);
            }
        }
    }
    else // fallback  // format = "%d %p";
    {
        snprintf(&tmp[0], 63, format, idx, data);
        if( save_text )
             snprintf(&selected_text[0], 63, "%s", tmp);
    }

    // on Settings change color and font for value
    if(l == option_panel)
    {
        vec4 c = col * .75f;
        add_text( l->vbo, main_font, &tmp[0], &c, pen);
    }
    else // default
       add_text( l->vbo, sub_font, &tmp[0], &col, pen);
}


/* also, different panel have a different selector */
static void GLES2_render_selector(layout_t *l, vec4 *rect)
{
    vec4 r = *rect;

    if(l == left_panel2)
    {   // gles render the selected rectangle
        ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
        // shrink frect LtoR to paint rightmost blue selector
        r.x = r.z - .0075f;
        ORBIS_RenderFillRects(USE_COLOR, &sele, &r, 1);
    }
    if(l == icon_panel)
    {   // draw the foreground rect, for text: .yw flips Y
        r.w -= ( 150. + 24. ) / resolution.y *2;
        ORBIS_RenderFillRects(USE_COLOR, &sele, &r, 1);
    }
    if(l == download_panel)
    {   // gles render the selected rectangle
        ORBIS_RenderFillRects(USE_COLOR, &sele, &r, 1);
    }
    if(l == option_panel)
    {   // gles render the selected rectangle
        ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
        // gles render the glowing blue box
        ORBIS_RenderDrawBox(USE_UTIME, &sele, &r);
    }    
}
#if 0
void glDeleteTextures1(int unused, GLuint* text) {
    log_info("fallback_t tex: %i, curr tex %i", fallback_t, *text);
    glDeleteTextures(1, text), text = 0;
}
#endif
static bool reset_tex_slot(int idx)
{
    item_t *i = &icon_panel->item_d[ idx ];

    if(i->texture  > GL_NULL
    && i->texture != fallback_t 
    && i->png_is_loaded) // keep icon0 template
    {   // discard icon0.png
//      log_debug( "%s[%3d]: (%d)", __FUNCTION__, idx, i->texture);
        glDeleteTextures(1, &i->texture), i->texture = GL_NULL;
        return true;
    }
    return false;
}

void drop_some_icon0()
{
    int count = 0;
    for (int i = 0; i < icon_panel->item_c; i++)
    {   // skip some 
        if( i < icon_panel->curr_item + icon_panel->f_size + 80
        &&  i > icon_panel->curr_item - icon_panel->f_size - 80) continue;

        if( reset_tex_slot(i) ) count++;
    }

    if(count > GL_NULL)
        log_info( "%s discarded %d textures", __FUNCTION__, count);
}

bool check_n_load_texture(const int idx, texture_load_status_t status)
{
    // address the item by its index
    item_t *i = &icon_panel->item_d[ idx ];

    char *path = i->token_d[ PICPATH ].off,
    *url  = i->token_d[ IMAGE ].off;

    if (status == TEXTURE_LOAD_DEFAULT) {
        if (i->texture == GL_NULL) {
            loadmsg(getLangSTR(DL_CACHE));
            i->texture = fallback_t;
            i->failed_dl = false;
            i->png_is_loaded = false;
            i->update_status = UPDATE_NOT_CHECKED;
        }
        return true;
    }
    else if( status == TEXTURE_DOWNLOAD 
    && (i->texture == fallback_t && !i->failed_dl)) // try to load icon0
    {
#if 0 // on pc
        path = strstr(i->token_d[ PICPATH ].off, "storedata");
#endif
        if( ! if_exists(path) ) // download
        {
            retry:
            if (strstr(path, "settings.ini") == NULL) {
                int ret = dl_from_url(url, path, NULL, false);
                if (ret != 0) {
                    log_error("dl_from_url() failed: %d", ret);
                    i->texture = fallback_t;
                    i->failed_dl = true;
                }
                else if(is_png_vaild(path)){
                   log_info("Downloaded icon %s is vaildated, Loading...", path);
                   i->png_is_loaded = true;
                   return true;
                }
            }
            else
                msgok(FATAL, "This CDN is not to be trusted, Consider deleting it Now");
        }
        else if(is_png_vaild(path)){
            log_info("icon %s is vaild, Loading...", path);
            i->png_is_loaded = true;
            return true;
        }
        else if (!is_png_vaild(path) && !i->failed_dl){ //try to redownload if it didnt fail to dl before
            unlink(path);
            goto retry;
        }
    }
    else if (status == TEXTURE_LOAD_PNG && i->texture == fallback_t && !i->failed_dl && i->png_is_loaded) {
        i->texture = load_png_asset_into_texture(path);
        log_info("%s | %i", path, i->texture);
        if(i->texture == GL_NULL){
           i->failed_dl = true;
        }
    }

    // set the fallback icon0
    if (i->texture == GL_NULL)  i->texture = fallback_t;

    return false;
}

typedef struct
{
    uint64_t numb;
} search_thread_t;

typedef struct
{
    int g_idx,    // item_count
        f_size;    // field size
    uint64_t len;
    bool is_search_q;
    search_thread_t *llp;
} dl_thread_t;

atomic_bool is_icons_finished = ATOMIC_VAR_INIT(true);
atomic_bool icons_thread_started = ATOMIC_VAR_INIT(false);

void *download_icons_thread(void* args) {
    dl_thread_t* dl_arg = (dl_thread_t *)args;

    if (!dl_arg->is_search_q) {
        //page icons
        for (int i = 0; i < dl_arg->f_size; i++){
            int loop_idx = dl_arg->g_idx + i;
            check_n_load_texture(loop_idx, TEXTURE_DOWNLOAD);
           // log_info("%s[%3d]", __FUNCTION__, loop_idx);
        }
    }
    else {
        //auxiliary search result icons
        for (int z = 0; z <= dl_arg->len; z++) 
            check_n_load_texture(dl_arg->llp[z].numb, TEXTURE_DOWNLOAD);

        free(dl_arg->llp);
    }

    free(dl_arg);
    is_icons_finished = true;
    icons_thread_started = false;
    log_info("icons_thread_started %i %i", is_icons_finished, icons_thread_started);
    pthread_exit(NULL);

    return NULL;

}

/*
    last review:
    - correct positioning, in px
    - cache normalized rectangles
    - uses cached texture from atlas + UVs
    - uses *active_panel* selector
    - renders different selector, per layout_t
    - this is actually drawing 4 layouts:
      left, icon, cf_game_option, download panels
*/
int old_idx = -1;
void* aux_addr = NULL;
int old_num_of_items = 0;

void GLES2_render_layout_v2(layout_t *l, int unused)
{

    pthread_t icons_thread = 0;
    search_thread_t* llp = NULL;
    dl_thread_t* dl_struct = NULL;
    int g_idx, loop_idx = -1;
    if( !l ) return;

//  log_info("%s %p %d", __FUNCTION__, l->vbo, l->vbo_s);

    vec2 p, s,          // item position and size, in px
         tp,            // used for text position, in px
         b1 = {30, 30}, // border between rectangles, in px
         b2 = {10, 10}; // border between text and rectangle, in px
    vec4 r,             // normalized (float) rectangle
         selection_box,
         tv = 0.;       // temporary vector //tv.xy = b1 * (l->fieldsize - 1.);

    // normalized custom color, like Settings
    vec4 c = (vec4) { 41., 41., 41., 256. } / 256.;

    /*  override, uses different borders */
    if(l ==     icon_panel )  b1 = (vec2)( 4.),      b2 = (vec2)( 5.);
    if(l ==   option_panel )  b1 = (vec2){32., 4.},  b2 = (vec2){ 16, 24 };
    if(l ==     left_panel2
    || l == download_panel )  b1 = (0.),  b2 = (vec2){ 16, 24 };

    /* destroy VBO */
    if( l->vbo_s == ASK_REFRESH ) { if(l->vbo) vertex_buffer_delete(l->vbo), l->vbo = NULL; }

    if( ! l->vbo ) // we cleaned vbo ?
    {
        l->vbo   = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
        l->vbo_s = EMPTY;
    }

    int curr_page = l->page_sel.x;

    if(l == left_panel2)
        curr_page = l->curr_item / (l->fieldsize.x * l->fieldsize.y);

    // get global item index: first one for current page
    g_idx = curr_page * (l->fieldsize.x * l->fieldsize.y);
    // but don't consider pages for left_panel!

    /* compute normalized rectangle we will draw, once! */
    if(l->vbo_s < CLOSED)
    {
        if( ! l->f_rect )
        {
            l->f_rect = calloc(l->fieldsize.x * l->fieldsize.y, sizeof(vec4));
        }
        // draw the field, according to passed layout_t
        for(int i = 0; i < l->f_size; i++)
        {
            int loop_idx = g_idx + i;

            if(loop_idx > l->item_c -1) break;

            if(l == icon_panel
            && aux && aux->len) // we have an auxiliary serch result item list to show
                loop_idx = aux[ g_idx + i +1 /*skip first*/].len; // read the stored index

            // temporary vector: use to load borders and get UVs
            tv.xy = b1;
            // compute position and size in px, then fill normalized UVs
            r     = get_rect_from_index(i, l, &tv);        
            p     = r.xy,  s = r.zw; // swap r w/ p, s and reuse r below
            p    += l->bound_box.xy; // add origin: it's bound box position
            p.y  -= l->bound_box.w;  // move back down by *minus* size.h(!)
            tp    = p + b2;          // update pen for text positioning
            tp.y += s.y;             // relate b2 to BotLef of current rectangle
            s    += p;               // turn size into a second point
            // convert to normalized coordinates
            r.xy  = px_pos_to_normalized(&p),
            r.zw  = px_pos_to_normalized(&s);
            r.yw  = r.wy;            // flip on Y axis
            l->f_rect[i] = r;        // save the normalized rectangle
            // to refresh main header/title screen
            bool save_text = false;
            // if item is the selected one save frect for optional work
            if(i == l->f_sele)
            {
                selection_box = r,
                save_text     = true;
            }
            // texts: check for its vbo
            if(l->vbo
            && l->vbo_s < CLOSED)
            {   /* draw over item: text pos, in px */
                if((l != icon_panel)
                || (l == icon_panel && i == l->f_sele && l == active_p))
                {
                   /// if(l->page_sel.x == 3)
                     //  check_n_load_texture(loop_idx, false);

                    layout_compose_text(l, loop_idx, &tp, save_text);
                }
            }
        } /// EOFor each item

        // close text vbo
        if(l->vbo_s < CLOSED) { l->vbo_s = CLOSED; refresh_atlas(); }
    }
    /* use cached normalized f_rects */
    if(l->f_rect)
    {   // read from cached normalized rectangle for more work
        selection_box = l->f_rect[ l->f_sele ];

        if (l == left_panel2)
        {   // draw the selected rectangle
            ORBIS_RenderFillRects(USE_COLOR, &grey, &l->f_rect[l->f_sele], 1);
        }
        if(l == icon_panel)
        {   // draw squared icons 

            ivec2 pi  = (ivec2) { active_p->curr_item, active_p->item_c };
                      pi /= (ivec2) ( active_p->fieldsize.x * active_p->fieldsize.y );
                      pi += 1;
            int vaild_icon_c = 0;
            for(int i = 0; i < l->f_size; i++)
            {
                loop_idx = g_idx + i;
                
                if (aux && aux->len) { // we have an auxiliary result item list to show
                    loop_idx = aux[g_idx + i + 1 /*skip first*/].len; // read the stored indx

                   if(aux != (void*)aux_addr || pi.x != old_num_of_items)
                     check_n_load_texture(loop_idx, TEXTURE_LOAD_DEFAULT);
                }
                else {
                    if(old_idx != g_idx)
                       check_n_load_texture(loop_idx, TEXTURE_LOAD_DEFAULT);
                }


                if (is_sort_panel && !icons_thread_started && is_icons_finished) {
                    item_t* i = &icon_panel->item_d[loop_idx];
                    if (i->texture == GL_NULL || (i->texture == fallback_t && !i->failed_dl)) {
                        check_n_load_texture(loop_idx, TEXTURE_DOWNLOAD);
                        check_n_load_texture(loop_idx, TEXTURE_LOAD_PNG);
                    }

                }

               if(!is_sort_panel){
                  if(check_n_load_texture(loop_idx, TEXTURE_LOAD_PNG)) vaild_icon_c++;
               }

               //log_info("Count %i, total %i", vaild_icon_c, l->f_size);

               on_GLES2_Render_icon(USE_COLOR, l->item_d[ loop_idx ].texture, 2, &l->f_rect[ i ], NULL);

            }
#ifdef __ORBIS__
            sceMsgDialogTerminate();
#endif
            if (aux && aux->len && !icons_thread_started && is_icons_finished &&
             (aux != (void*)aux_addr || pi.x != old_num_of_items)) {

                is_icons_finished = false;
                icons_thread_started = true;
                log_info("old_num_of_items %i, pi.x %i", old_num_of_items, pi.x);
                old_num_of_items = pi.x;
                dl_struct = (dl_thread_t*)malloc(sizeof(dl_thread_t));
                if (dl_struct == NULL) return;
                llp = (search_thread_t*)calloc(aux->len + 1, sizeof(search_thread_t));
                if (llp == NULL) return;

                for (int d = 1; d <= aux->len; d++)
                    llp[d].numb = aux[d /*skip first*/].len; // read the stored index

                log_debug("search_thread_t aux_addr %p, aux: %p", aux_addr, aux); 
                aux_addr = aux;
                dl_struct->len = aux->len;
                dl_struct->is_search_q = true;
                dl_struct->llp = llp;
                log_info("[StoreCore][Search] Started Icons thread for %i Search results", aux->len);
                //next turn off num of dls info for tests
                pthread_create(&icons_thread, NULL, download_icons_thread, (void*)dl_struct);
                //  sleep(5);
            }
            if (!icons_thread_started && old_idx != g_idx && is_icons_finished && l->f_size > vaild_icon_c) {
                is_icons_finished = false;
                icons_thread_started = true;

                dl_struct = (dl_thread_t*)malloc(sizeof(dl_thread_t));
                if (dl_struct == NULL) return;

                log_info("[StoreCore] Starting Icons thread for %i %i %i", g_idx, !icons_thread_started, !is_icons_finished);
                old_idx = g_idx;
                dl_struct->f_size = l->f_size;
                dl_struct->g_idx = g_idx;
                //Started Icons thread for 0 1 0
                dl_struct->is_search_q = false;
                //next turn off num of dls info for tests
                pthread_create(&icons_thread, NULL, download_icons_thread, (void*)dl_struct);
            }
            if(l == active_p)
            {   // add a border to the selection
                r = frect_add_border(&selection_box, 9);
                // apply glowing shader
                ORBIS_RenderFillRects(USE_UTIME, &sele, &r, 1);
                // which is the selected item texture?
                int sele_idx = g_idx + l->f_sele;

                if (aux && aux->len) {// we have an auxiliary serch result item list to show
                    sele_idx = aux[g_idx + l->f_sele + 1 /* skip 1st */].len; // read the stored index
                }

                // redraw the selected icon, use stored item_index
                on_GLES2_Render_icon(USE_COLOR, l->item_d[ sele_idx ].texture, 2, &selection_box, NULL);
            }
        }

        // basic
        if(l == option_panel)
        {   // draw all the rectangle array by count, at once
            ORBIS_RenderFillRects(USE_COLOR, &c, &l->f_rect[0], l->f_size);
        }
        
        if(l == download_panel)
        {
            // uses cached frects, texture from atlas and UVs
            for(int i = 0; i < l->f_size; i++)
            {   // gles render the outline box
                ORBIS_RenderDrawBox(USE_COLOR, &grey, &l->f_rect[ i ]);
            }
        }
    }

    /* elaborate more on the selected item: */
    r = selection_box;

    // panel is active, show selector
    if(l == active_p)
    {   /* also, different panel have different selector */
        GLES2_render_selector(l, &r); }

//  log_info("%s %p %d, before", __FUNCTION__, l->vbo, l->vbo_s);

    if(icon_panel != l
    || icon_panel == active_p)
    {   // texts from layout vbo
         ftgl_render_vbo(l->vbo, NULL);
    }

//  log_info("%s %p", __FUNCTION__, l);
}



void layout_refresh_VBOs(void)
{
    layout_t *l = NULL;
    for (int i = 0; i < 5; i++)
    {
        switch(i)
        {
            case 0: l =     left_panel2; break;
            case 1: l =     icon_panel;  break;
            case 2: l = download_panel;  break;
            case 3: l =   option_panel;  break;
            case 4: l =    queue_panel;  break;
        }
        GLES2_UpdateVboForLayout(l);
    }
}

