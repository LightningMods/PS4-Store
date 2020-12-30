/*
    GLES2 scene

    my implementation of an interactive menu, from scratch.
    (shaders
     beware)

    2020, masterzorag
*/

#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#if defined(__ORBIS__)
    #include <orbisPad.h>
    extern OrbisPadConfig *confPad;
    #include <KeyboardDialog.h>
    #include <installpkg.h>

#endif

#include <freetype-gl.h>
// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;

#include "defines.h"

#include "jsmn.h"
#include "json.h"
int jsoneq(const char *json, jsmntok_t *tok, const char *s);

int install_ret = -1, http_ret = -1, last_item_dl = 999;

#include "utils.h"

bool install_done = false;
extern int group_c; // panel.c

extern StoreOptions *get;

// related indexes from json.h enum
// for sorting entries
unsigned char cmp_token = 0,
      sort_patterns[10] = { 0, 1, 4, 5, 9, 10, 11, 12, 13, 16 };

/* set comparison token */
static void set_cmp_token(const int index)
{    //cmp_token = index;
    cmp_token = sort_patterns[index];
}

/* qsort struct comparison function (C-string field) */
static int struct_cmp_by_token(const void *a, const void *b)
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

extern double u_t;
extern int    selected_icon;
       ivec4  menu_pos = (0),
              rela_pos = (0);

extern texture_font_t  *main_font, // small
                       *sub_font, // left panel
                       *titl_font;
extern vertex_buffer_t *text_buffer[4];

extern dl_arg_t        *pt_info;
// available num of jsons
int json_c = 0;

layout_t *icon_panel     = NULL,
         *left_panel     = NULL,
         *option_panel   = NULL,
         *download_panel = NULL,
         *queue_panel    = NULL;

item_t *groups = NULL;

/*
    auxiliary array of item_index_t:
    - shared for Search/Groups
    - the query to save search results;
*/
item_idx_t *aux = NULL,
           *q   = NULL;  // Query

// common title in different panels
char title [128];


void GLES2_render_submenu_text_v2( vertex_buffer_t *vbo, vec3 *offset );

// index (write) all used_tokens
int json_index_used_tokens_v2(layout_t *layout, char *json_data)
{
    int r, i, c = 0;
    jsmn_parser p;
    jsmntok_t t[512]; /* We expect no more than this tokens */
    //char *json_data = page->json_data;
    jsmn_init(&p);
    r = jsmn_parse(&p, json_data, strlen(json_data), t,
                             sizeof(t) / sizeof(t[0]));
//  klog("%d\n", r);
    if (r < 0) { klog("Failed to parse JSON: %d\n", r); return -1; }

    // grab last added index from current item count
    int idx = layout->item_c;

    item_idx_t *token = NULL;

    for(i = 1; i < r; i++)
    {   // entry
        layout->item_d[idx].token_c = NUM_OF_USER_TOKENS;
        // dynalloc for json tokens
        if(!layout->item_d[idx].token_d)
            layout->item_d[idx].token_d = calloc(layout->item_d[idx].token_c, sizeof(item_idx_t)); /// XXX
//        klog("%s %p\n", __FUNCTION__, layout->item_d[idx].token_d);
        token = &layout->item_d[idx].token_d[0];

        int j;
        for(j = 0; j < NUM_OF_USER_TOKENS; j++)
        {
            //klog("! %d [%s]: %p, %d\n", j, used_token[j], layout->tokens[j].off, page->tokens[j].len);
            if (jsoneq(json_data, &t[i], used_token[j]) == 0)
            {
                /* We may use strndup() to fetch string value */
#if 0
            klog("- %d) %d %d [%s]: %.*s\n", c, i, j,
                    used_token[j],
                              t[i + 1].end - t[i + 1].start,
                                 json_data + t[i + 1].start);
#endif
                token[j].off = strndup(json_data + t[i + 1].start,
                                    t[i + 1].end - t[i + 1].start);
                token[j].len =      t[i + 1].end - t[i + 1].start;
//              klog("token_d[].off:%s\n", layout->item_d[idx].token[0].off);
                i++; c++;
                if(j==16) idx++; // ugly but increases :facepalm:
            }
        }
        // if less, shrink buffer: realloc
        if(j < NUM_OF_USER_TOKENS)
        {   // shrink
            layout->item_d[idx].token_c = j;
            layout->item_d[idx].token_d = realloc(layout->item_d[idx].token_d,
                                                  layout->item_d[idx].token_c * sizeof(item_idx_t));
        }
        // save current index from current item count
        layout->item_c = idx;
    }

    /* this is the number of all counted items * NUM_OF_USER_TOKENS ! */
    return c;
}


void clean_textures(layout_t *layout)
{
    //glBindTexture( GL_TEXTURE_2D, 0 );

    if(layout->texture)
    {
        printf("%s: layout: %p, %p\n", __FUNCTION__, layout, layout->texture);
        for(int i = 0; i < layout->fieldsize.x * layout->fieldsize.y; i++)
        {
            if(layout->texture[i] != 0)
            {
                printf("glDeleteTextures texture[%d]: %8x\n", i, layout->texture[i]);
                glDeleteTextures(1, &layout->texture[i] ), layout->texture[i] = 0;
            }
        }
        free(layout->texture), layout->texture = NULL;
    }
}


void GLES2_destroy_layout(layout_t *layout)
{
    if(!layout) return;

    clean_textures(layout);

    item_idx_t *token_d = NULL;
    for(int i = 0; i < layout->item_c; i++)
    {
        token_d = &layout->item_d[i].token_d[0];
        for(int j = 0; j < NUM_OF_USER_TOKENS; j++)
                        // < token_c
        {
            if(token_d[j].off) free(token_d[j].off);
        }
        if(token_d) free(token_d);
    }
    if(layout->item_d) free(layout->item_d);

    if(layout->vbo) vertex_buffer_delete(layout->vbo), layout->vbo = NULL;

    if(layout) free(layout), layout = NULL;
}


int GLES2_create_layout_from_json(layout_t *layout)
{
    char  json_file[128];
    void *p;
    int   count = 0;
    while(1)
    {   // json page_num starts from 1 !!!
        sprintf(&json_file[0], "/user/app/NPXS39041/homebrew-page%d.json", count +1);
        // read the file
#if defined (USE_NFS)
        // move pointer to address nfs share
        p = (void*)orbisNfsGetFileContent(&json_file[20]);
#else
        p = (void*)orbisFileGetFileContent(&json_file[0]);
#endif
        // check
        if(!p) break;

        int valid_tokens = json_index_used_tokens_v2(layout, p);

        fprintf(INFO, "%d valid_tokens!\n", valid_tokens);
        // passed, increase num of available pages
        count++;

        if(p) free(p), p = NULL;
    }
    return count;
}


const char *download_panel_text[] =
{
    "Download",
    "Install",
    "What more?"
};

const char *option_panel_text[] =
{
    "eat",
    "drink",
    "smoke",
    "fuck",
    "code",
    "repeat",
    "Games",
    "Apps",
    "Groups",
    "Ready to install",
    "Updates",
    "Queue",
    "Game Pass",
    "Gold","Games",
    "Apps",
    "Groups",
    "Ready to install",
    "Updates",
    "Queue",
    "Game Pass",
    "Gold",
    "eat",
    "drink",
    "smoke",
    "fuck",
    "code",
    "repeat",
    "eat",
    "drink",
    "smoke",
    "fuck",
    "code",
    "repeat"
};

// menu entry strings
#define  LPANEL_Y  (5)
const char *new_panel_text[LPANEL_Y][10] = {
{   // Main page: 6
    "Games",
    "Apps",
    "Groups",
    "Ready to install",
    "Queue",
    "Updates",
    "Settings"
},
{   // Games page
    "Search for",
    "Sort by",
    "Filter by"
},
{   // 
    "1: connect socket",
    "2: screenshot",
    "3: Alpha",
    "4: Red",
    "5: Green",
    "6: Blue",
    "7: test"
},
{   // 
    "CDNURL",
    "http...",
    "Background Path",
    "5:write config"
},
{   // "Sort by" page, 10
    "Id",
    "Name",
    "Package",
    "Version",
    "Review Stars",
    "Size",
    "Author",
    "Type",
    "Playable Version",
    "Release date"
}
    // LPANEL_Y index used for 'Groups'
    // LPANEL_Y + 1 for 'Queue'
};

/*
    indexes item_t item_data
*/
int GLES2_create_layout_from_list_v2(layout_t *layout, const char **i_list)
{
    int i, j;
    item_idx_t *token = NULL;

    for(i = 0; i < layout->item_c; i++)
    {   // dynallocs for items
        if(!layout->item_d[i].token_d)
        {
            layout->item_d[i].token_c = 10; // minimum
            layout->item_d[i].token_d = calloc(layout->item_d[i].token_c, sizeof(item_idx_t));
        }
        // iterate token_data
        for(j = 0; j < layout->item_d[i].token_c; j++)
        {// klog("\n%d, %d %s\n", i, j, new_panel_text[i][j]);
                                           //i_list[i][j]);
            token      = &layout->item_d[i].token_d[j];
            token->off = new_panel_text[i][j];
            if(!token->off)
            { token->len = -1; break; } // stop here
            else
                token->len = strlen( token->off );
            //
            klog("%s %d\n", layout->item_d[i].token_d[j].off,
                            layout->item_d[i].token_d[j].len);
        }
        if(j < layout->item_d[i].token_c)
        {   // shrink
            layout->item_d[i].token_c = j;
            layout->item_d[i].token_d = realloc(layout->item_d[i].token_d,
                                                layout->item_d[i].token_c * sizeof(item_idx_t)); }
    }
    return i;
}

// return index to selected item_data token_data
int get_item_index(layout_t *l)
{
    int
    // compute some indexes, first one of layout:
    i_paged = l->fieldsize.y * l->fieldsize.x
            * l->page_sel.x,
    // which icon is selected in field X*Y ?
    f_sele  = l->item_sel.y * l->fieldsize.x
            + l->item_sel.x;
    // which item is selected over all ?
    int idx = f_sele + i_paged;

    // we have an auxiliary serch result item list to show...
    if(aux)
        idx = aux[ l->fieldsize.x * l->fieldsize.y * l->page_sel.x // i_paged
                 + l->curr_item
                 +1     // skip first reserved
                 ].len; // read stored indexes
    return idx;
}

/*
    indexes item_idx_t* token_data in a single item_t item_data
*/
int GLES2_create_layout_from_list(layout_t *layout, const char **i_list)
{
    int i;
    item_idx_t *token = NULL;

    for(i = 0; i < layout->item_c; i++)
    {
        // dynallocs
        if(!layout->item_d[i].token_d)
        {
            layout->item_d[i].token_c = 1; // minimum
            layout->item_d[i].token_d = calloc(layout->item_d[i].token_c, sizeof(item_t));
        }

        token      = &layout->item_d[i].token_d[0];
        token->off = i_list[i];
        token->len = strlen( i_list[i] );
    }
    return i;
}

// disk free percentage
float dfp = 0.;
// pthread 
extern atomic_ulong g_progress;
/*
    layout tracks curr_page, so we pass +/-1 page from caller!
    - this trigger texture fetching/sampling
    - this adapts fieldsize to available items
*/
void GLES2_render_layout(layout_t *layout, int unused)
{
    if( ! layout->is_shown ) return;

    if(layout->refresh) { clean_textures(layout), layout->refresh = 0; }

    // the field size
    int field_s = layout->fieldsize.x * layout->fieldsize.y;

    // if we discarded layout textures recreate them!
    if ( layout != left_panel
    && ! layout->texture )
         layout->texture = calloc(field_s, sizeof(GLuint));

    // default for item page (or icons page)
    int max_page = layout->item_c / field_s;
    // if we have an auxiliary serch result item list to show...
    if(aux)
    {   // field_s is valid, update max_page
        max_page = (aux->len / field_s) +1;
    }

    // complete this crap, force N menus
    if(layout == left_panel) max_page = LPANEL_Y;

    // compute some indexes, first one of layout:
    int i_paged     = field_s * layout->page_sel.x;
    // which icon is selected in field X*Y ?
    int f_selection = layout->item_sel.y * layout->fieldsize.x + layout->item_sel.x;
    // which item is selected over all ?
    //layout->curr_item = f_selection + i_paged;
/*
    klog("p:%d/%d, s:%d/%d (%d, %d):%d\n",
                     layout->curr_page, max_page,
                     layout->curr_item, layout->item_c,
                     menu_pos.x, menu_pos.y, f_selection);
*/
    // we can't select more than max item
    if(layout->curr_item > layout->item_c)
    {   // select the last item
        layout->curr_item = layout->item_c;
    }

    /* if last page can't fill the field... */
    int rem = layout->item_c - i_paged;
    // if we have an auxiliary serch result item list to show...
    if(aux)
        rem = aux->len - i_paged;
    // decrease the field size!
    if(rem < field_s)
        field_s = rem; // printf("field_s:%d\n", field_s);


    if(layout == left_panel)
    {
        field_s = layout->item_d[layout->page_sel.x].token_c;
        i_paged = 0;
    }

    char    tmp[256];
    item_t *li = NULL;  // layout item iterator

    if(layout == download_panel)
    {   //f_selection = icon_panel->item_sel.y * icon_panel->fieldsize.x + icon_panel->item_sel.x;
        field_s = layout->fieldsize.x * layout->fieldsize.y;
        // fullscreen frect (normalized coordinates)
        vec4 t = (vec4) { -1., -1.,   1., 1. };

        int i = get_item_index(icon_panel);
           li = &icon_panel->item_d[i];

        if(layout->texture[0] == 0)
        {   // we have an entry for picture path?
            if(li->token_d[PICPATH].off)
            {   // read icon path from token value
                sprintf(&tmp[0], "%s", li->token_d[PICPATH].off);
    #if defined (USE_NFS)
                char *p = strstr(&tmp[0], "storedata");
                printf("%s\n", p);
                layout->texture[0] = load_png_asset_into_texture(p);
    #else
                // local path is directly token value
                layout->texture[0] = load_png_asset_into_texture(&tmp[0]);
    #endif
            }
        }
        else
            on_GLES2_Render_icon(layout->texture[0], 2, &t);

        sprintf(&title[0], "%s", icon_panel->item_d[i].token_d[NAME].off);

        // apply a shader on top of background
        pixelshader_render(0);
    }
    // we cleaned vbo ?
    if( ! layout->vbo ) layout->vbo = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

    vec2 p, s,          // position, size
         pen;           // text position, in px
    vec4 r,             // normalized rectangle
         selection_box,
    /* normalize rgba colors */
    sele  = (vec4) {  29, 134, 240, 256 } / 256, // blue
    grey  = (vec4) {  66,  66,  66, 256 } / 256, // fg
    white = (vec4) ( 1. ),
    col   = (vec4) { .8164, .8164, .8125, 1. },
    t_sel = (vec4) { 256,   0,   0, 256 } / 256; // red

    int i;
    // iterate items we are about to draw
    for(i = 0; i < field_s; i++)
    {
        if(layout == left_panel) // new_panel uses different indexing!
            li = &layout->item_d[ layout->page_sel.x ];
        else
            li = &layout->item_d[ i_paged + i ];

        if(layout == icon_panel)
        {
            if(aux) // we have an auxiliary serch result item list to show...l
                li = &icon_panel->item_d[ aux[ i_paged + i +1 /*skip first*/].len ];

        }
        /* compute origin position for this item */
        s = (vec2) { layout->bound_box.z / layout->fieldsize.x,
                     layout->bound_box.w / layout->fieldsize.y };

        p = (vec2) { (float)(layout->bound_box.x
                     + (i % (int)layout->fieldsize.x)
                     * (s.x +3 /* px border */))
                     ,
                     (float)(layout->bound_box.y
                     - (i / layout->fieldsize.x)
                     * (s.y +3 /* px border */)) };

        // here we scroll page UP/DOWN
        if(layout == download_panel && offset.y != 0) p.y += offset.y;

        s   += p; // .xy point addition!
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);

#if 1
    // this should never happen!
    if(r.x < -1. || r.x > 1.
    || r.y < -1. || r.y > 1.
    || r.z < -1. || r.z > 1.
    || r.w < -1. || r.w > 1.)
    {
        // this can help us debugging problems!
        continue;

        printf("VIEW %d is clipping %d  out, check your math!\n", i, menu_pos.z);
        klog("pos  :%.3f %.3f, sz:%.3f %.3f\n", p.x, p.y, s.x, s.y);
        klog("r (p1:%.3f,%.3f, p2:%.3f,%.3f)\n", r.x, r.y, r.z, r.w);
        klog("b_box:%d,%d,  %d,%d\n", layout->bound_box.x, layout->bound_box.y, layout->bound_box.z, layout->bound_box.w);
        klog("f_sz :%i,%i\n", layout->fieldsize.x, layout->fieldsize.y);
        msgok(FATAL, "VIEW %d is clipping something out\n pos  :%.3f %.3f, sz:%.3f %.3f\n\n Please Cotact Support", menu_pos.z, r.x, r.y, r.z, r.w);
        //sleep(1);
    }
#endif

        /* draw texture, if there */
        if(layout == icon_panel)
        {
            if( layout->texture[i] == 0 )
            {   // we have an entry for picture path?
                if(li->token_d[PICPATH].off)
                {   // read icon path from token value
                    sprintf(&tmp[0], "%s", li->token_d[PICPATH].off);
#if defined (USE_NFS)
                    char *p = strstr(&tmp[0], "storedata");
                    layout->texture[i] = load_png_asset_into_texture(p);
#else
                    // local path is directly token value
                    if (sceKernelOpen(li->token_d[PICPATH].off, 0x0000, 0) < 0)
                    {
                        //give up after 2 tries of downloading
                        if (last_item_dl != i)
                        {
                            loadmsg("Downloading Icons....\n");

                            klog("downloading %s from %s", li->token_d[PICPATH].off, li->token_d[IMAGE].off);
                            dl_from_url(li->token_d[IMAGE].off, li->token_d[PICPATH].off, false);
                            printf("last_item_dl %i\n", last_item_dl);
                            fprintf(INFO, "texture[%2d] = %3d\n", i, layout->texture[i]);
                            last_item_dl = i;
                        }            
                        
                    }
                    else
                        layout->texture[i] = load_png_asset_into_texture(&tmp[0]);

                  

                    if(i == field_s -1)
                        sceMsgDialogTerminate();
#endif
                }
            }
            else
                on_GLES2_Render_icon(layout->texture[i], 2, &r);
        }
        // save pen origin for text, in pixel coordinates!
        pen = p;

        if(layout == download_panel)
        {  // draw bounding box for each button selector
            ORBIS_RenderDrawBox(USE_COLOR, &grey, &r);
        }

        // is item the selected one?
        if(i == f_selection)
        {
            if(layout == left_panel && layout->is_active)
            {   // set title screen
                if(li->token_d)
                    sprintf(&title[0], "%s", li->token_d[i].off);
                else // guard
                {   // address current layout item_t to Groups array
                    sprintf(&title[0], "%s", groups[i +1].token_d[0].off);
                }
            }
            // this panel is_active: check for its vbo
            if(layout == icon_panel && layout->is_active
            && layout->vbo
            && layout->vbo_s < CLOSED)
            {   /* draw over item */
                // print ID of selected item *over* selection rectangle
                // get the indexed token value
                snprintf(&tmp[0], li->token_d[ID].len + 1, "%s", li->token_d[ID].off);

                fprintf(INFO, "%.3f %.3f '%s'\n", pen.x, pen.y, tmp);
                // fill the vbo
                add_text( layout->vbo, main_font, &tmp[0], &white, &pen);
                // update pen
                pen    =  p;
                pen.y += 24;
                // print NAME of selected item *over* selection rectangle
                // set title: get the indexed token value
                snprintf(&title[0], li->token_d[NAME].len + 1, "%s", li->token_d[NAME].off);
                // cut NAME token to n chars
                if(li->token_d[NAME].len > 18)
                    snprintf(&tmp[0], 22, "%.18s...", title);
                else
                    sprintf(&tmp[0], "%s", title);
                // fill the vbo
                add_text( layout->vbo, main_font, &tmp[0], &white, &pen);
            }
            // save frect for optional glowing selection box or...
            selection_box = r;
        }
        // LEFT PANEL: texts
        // setup vbo texts from token data, for item
//      if(layout->is_active)
        {
            if(layout ==     left_panel
            || layout ==   option_panel
            || layout == download_panel
            ){
                if(layout->vbo_s < CLOSED)
                {   // get the indexed token value
                    if(layout == left_panel)
                    {   // the text/item/label
                        if(li->token_d)
                            sprintf(&tmp[0], "%s", li->token_d[i].off);
                        else
                        {   // address current layout item_t to Groups array
                            sprintf(&tmp[0], "%s", groups[i +1].token_d[0].off);
                        }
                    } else { // get the first indexed token value
                        snprintf(&tmp[0], li->token_d[0].len + 1,
                                    "%s", li->token_d[0].off); }
//                  fprintf(INFO, "%.3f %.3f '%s'\n", pen.x, pen.y, tmp);
                    // append the vbo
                    pen += 26;
                    add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);

                    // draw Ready to install, Queue, Groups numbers
                    if(layout == left_panel)
                    {
                        if(layout->page_sel.x == 0 && i == 2) // on main page
                        {
                            sprintf(&tmp[0], "%d", groups->token_c);
                            texture_font_load_glyphs( sub_font, &tmp[0] );
                            pen.x = 460 - tl;
                            add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);
                        }
                        else
                        if(layout->page_sel.x == LPANEL_Y) // on Group list page
                        {   // read the stored item count
                            li = &groups[i +1];
                            sprintf(&tmp[0], "%d", li->token_c);
                            texture_font_load_glyphs( sub_font, &tmp[0] );
                            pen.x = 460 - tl;
                            add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);
                        }
                        else
                        if(layout->page_sel.x == 0 && i == 3) // on main page, Ready to install
                        {
                            int ret = thread_count_by_status( COMPLETED );
                            if(ret)
                            {
                                sprintf(&tmp[0], "%d", ret);
                                texture_font_load_glyphs( sub_font, &tmp[0] );
                                pen.x = 460 - tl;
                                add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);
                            }
                        }
                        else
                        if(layout->page_sel.x == 0 && i == 4) // on main page, Queue
                        {
                            int ret = thread_count_by_status( RUNNING );
                            if(ret)
                            {
                                sprintf(&tmp[0], "%d", ret);
                                texture_font_load_glyphs( sub_font, &tmp[0] );
                                pen.x = 460 - tl;
                                add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);
                            }
                        }
                        else
                        if(layout->page_sel.x == LPANEL_Y + 1) // on group item page
                        {
                            printf("QUEUE\n");
                        }
                    }
                }
            }
        }

    } // EOF iterate available items

    // which item is selected by icon_panel ?
    const int idx = get_item_index(icon_panel);

    /* common texts for items, drawn by left_panel! */

    if(layout == left_panel
    //&& layout->is_active
    //&& layout->vbo
    && layout->vbo_s < CLOSED)
    {
        /* text for this page */

        time_t     t  = time(NULL);
        struct tm *tm = localtime(&t);
        char s[64];
        strftime(s, sizeof(s), "%A, %B %e %Y, %H:%M", tm); // custom date string
//      printf("%s\n", s);
        sprintf(&tmp[0], "%s", s);
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( sub_font, &tmp[0] );
        // we know 'tl' now, right align
        pen = (vec2) { resolution.x - tl - 50, resolution.y - 50 };
        // fill the vbo
        add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);

        uint64_t numb;
        sceKernelGetCpuTemperature(&numb);
        sprintf(&tmp[0], "System Version: %x, %d°C", SysctlByName_get_sdk_version(), numb);
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( main_font, &tmp[0] ); 
        // we know 'tl' now, right align
        pen.x  = resolution.x - tl - 50,
        pen.y -= 32;
        vec4 c = col * .75f;
        // fill the vbo
        add_text( layout->vbo, main_font, &tmp[0], &c, &pen);


        //sprintf(&tmp[0], "Available Apps to download (Store)");
    //  snprintf(&title[0], li->token_d[f_selection].len + 1,
    //                "%s", li->token_d[f_selection].off);
        pen = (vec2) { 670, resolution.y - 100 };
        // fill the vbo
        add_text( layout->vbo, sub_font, &title[0], &col, &pen);

        // item and page index infos:
        ivec2 item_index = (ivec2)(0),
              page_index = (ivec2)(0);

        if(aux) { // if we have an auxiliary serch result item list to show...
            item_index.x =  icon_panel->item_sel.y  * icon_panel->fieldsize.x + icon_panel->item_sel.x
                         + (icon_panel->fieldsize.y * icon_panel->fieldsize.x * icon_panel->page_sel.x) +1;
            item_index.y = aux->len;
            page_index.x = icon_panel->page_sel.x +1;
            page_index.y = (aux->len / (icon_panel->fieldsize.x
                                     *  icon_panel->fieldsize.y)) +1;
        }
        else { // default
            item_index.x = icon_panel->curr_item +1;
            item_index.y = icon_panel->item_c;
            page_index.x = icon_panel->page_sel.x +1;
            page_index.y = icon_panel->page_sel.y +1;
        }
        // we have at least an entry ?
        if(item_index.y > 0)
        {
            sprintf(&tmp[0], "%d/%d, Page:%d/%d", item_index.x, item_index.y,
                                                  page_index.x, page_index.y);
            // we need to know Text_Length_in_px, so we call this
            texture_font_load_glyphs( sub_font, &tmp[0] ); 
            // we know 'tl' now, right align
            pen = (vec2) { resolution.x - tl - 50, 50 };
            // fill the vbo
            add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);

            sprintf(&tmp[0], "%s", li->token_d[ID].off);
            // fill the vbo
            pen = (vec2) { 670, 150 };
            add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);
        }
        // eventually, skip dfp on some view...
        //if(layout->page_sel.x < LPANEL_Y)
        {   /* text for disk_free stats */
            pen = (vec2) { 26, 100 };
            dfp = df(&tmp[0], "/user");
            // fill the vbo
            add_text( layout->vbo, sub_font, "Storage", &col, &pen);
            pen.x  = 26,
            pen.y -= 32;
            vec4 c = col * .75f;
            add_text( layout->vbo, main_font, &tmp[0], &c, &pen);
        }
    }

    /* some more texts for 'download page' */

    if(layout == download_panel)
    //&& layout->vbo_s < CLOSED)
    {
        dl_arg_t *ta = NULL; // thread arguments
               t_sel = sele;

        int f_selection = icon_panel->item_sel.y * icon_panel->fieldsize.x + icon_panel->item_sel.x;
        //if(layout->is_active)

       // info from selected item in icon field!
        // custom rectangle, in px
        p = (vec2) { 100., 256. };
        s = (vec2) { 300., 300. };

        if(offset.y != 0) p.y += offset.y;

        s   += p; // .xy point addition!
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);
        //vec4 t = (vec4) { -.85, -.5,   -.55, .0 };
        if( layout->texture[0] )
            on_GLES2_Render_icon(layout->texture[0], 2, &r);

        // no thread is downloading item
        int x = thread_find_by_item( idx );
        if(x > -1)
        {   //printf("thread[%d] is downloading %d\n", x, idx);
            ta = &pt_info[ x ];
            /* draw filling color bar */
            p    = (vec2) { layout->bound_box.x, 520. + offset.y - 20. };
            s    = (vec2) { layout->bound_box.z,   5. };
            s   += p; // .xy point addition!
            r.xy = px_pos_to_normalized(&p);
            r.zw = px_pos_to_normalized(&s);
            // gles render the frect
            ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
            // shrink frect RtoL by custom_percentage
            r.z = r.x + (r.z - r.x) * (ta->progress / 100.f);
//          fprintf(INFO, "%.2f %.2f, %.2f, %.2f %f\n", r.x, r.y, r.z, r.w, dfp);
            ORBIS_RenderFillRects(USE_COLOR, &sele, &r, 1);

            switch(f_selection)
            {   // change color when disabled
                case 0: if(ta->status == RUNNING )  t_sel = grey; break;
                case 1: if(ta->status != COMPLETED) t_sel = grey; break;
            }
        }

        if(layout->vbo_s < CLOSED)
        {   // 
            pen = (vec2) { 500, 520 + offset.y + 10 };
            // address the selected item
            item_idx_t *t = &icon_panel->item_d[ idx ].token_d[0];

            // set title screen, we know length
            snprintf(&tmp[0], t[ NAME ].len + 1,
                        "%s", t[ NAME ].off);
            // fill the vbo
            add_text( layout->vbo, titl_font, &tmp[0], &col, &pen);

            if(ta
            && ta->url)
            {
                switch(ta->status)
                {
                    case CANCELED:  sprintf(&tmp[0], ", Download Canceled");
                    break;
                    case RUNNING:   sprintf(&tmp[0], ", Downloading %s, %.2f%%", ta->token_d[ SIZE ].off, ta->progress);
                    break;
                    case READY:
                    case COMPLETED: sprintf(&tmp[0], ", Download Completed");
                    break;
                }
                // fill the vbo
                add_text( layout->vbo, main_font, &tmp[0], &col, &pen);
            }

            pen.y -=  32;

            //static t_wanted[10] = { 0, 1, 4, 5, 9, 10, 11, 12, 13, 16 };
            for(int i = 0; i < 10; i++)
            {
                if(i == 1
                || i == 3) continue; // skip those
                // update position
                pen.x  = 500,
                pen.y -=  32;
                // token name
                sprintf(&tmp[0], "%s: ", new_panel_text[4][i]);
                // fill the vbo
                add_text( layout->vbo, main_font, &tmp[0], &col, &pen);
                // get the indexed token value
                sprintf(&tmp[0], "%s", t[  sort_patterns[i]  ].off);
                // fill the vbo
                add_text( layout->vbo, main_font, &tmp[0], &col, &pen);
            }
            pen.x  = 500,
            pen.y -= 400;
            for(int i = 0; i < 10; i++)
            {
                sprintf(&tmp[0], "%s", "some more texts");
                add_text( layout->vbo, main_font, &tmp[0], &col, &pen);
                pen.x  = 500,
                pen.y -=  32;
            }
        }
    }

    /* we eventually added glyphs! */
    if(layout->vbo_s < CLOSED) { layout->vbo_s = CLOSED; refresh_atlas(); }

    /* elaborate more on the selected item: */
    r = selection_box;

    if(layout == icon_panel && layout->is_active)
    {   /* increase the size of rectangle by appling
           9 pixel offset to each of two rect points */
        vec2 off = (vec2) ( 9 )
                 / resolution *2;
           r.xy -= off,
           r.zw += off;
        // apply glowing shader
        ORBIS_RenderFillRects(USE_UTIME, &sele, &r, 1);
        // redraw icon, use stored item_index
        on_GLES2_Render_icon(layout->texture[f_selection], 2, &selection_box);
        // now draw the foreground rect, for text
             r    = selection_box;
             r.w -= ( 150. + 24. ) / resolution.y *2;
        // use same color
        ORBIS_RenderFillRects(USE_COLOR, &sele, &r, 1);
        // texts out of layout vbo
        GLES2_render_submenu_text_v2(layout->vbo, NULL);
    }


    if(layout == left_panel
    || layout == option_panel
    || layout == download_panel)
    {
        r = selection_box;

        // highlight selected item, bagkground color
        if(layout == download_panel)
        {   // no thread is downloading item
            ORBIS_RenderFillRects(USE_COLOR, &t_sel, &r, 1);
        }
        else
        {
            ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
        }
        // rightmost blue rect, just on selection!
        if(layout->is_active)
        {   // shrink frect LtoR
            r.x = r.z - .0075f;
            ORBIS_RenderFillRects(USE_COLOR, &sele, &r, 1);
        }
        // texts out of layout vbo
        GLES2_render_submenu_text_v2(layout->vbo, NULL);
    }



    if(layout != download_panel)
    {   /* a 4 px rect, not a vertical line! */
           r = (vec4) { 500, 0,   4, resolution.y };
           p = r.xy,
           s = r.xy + r.zw;
        /* convert to normalized coordinates */
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);
        // gles render the frect
        ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);

        // skip on some view...
        if(layout == left_panel)
        {   /* draw filled color bar */
            r = (vec4) { -.975, -.900,   -.505, -.905 };
            // gles render the frect
            ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
            // shrink frect RtoL by disk_free_percentage
            r.z = r.x + (r.z - r.x) * (dfp / 100.f);
//          fprintf(INFO, "%.2f %.2f, %.2f, %.2f %f\n", r.x, r.y, r.z, r.w, dfp);
            ORBIS_RenderFillRects(USE_COLOR, &sele, &r, 1);
        }
    }
}

// common init a layout panel
layout_t * GLES2_layout_init(int req_item_count)
{
    layout_t *l = calloc(1, sizeof(layout_t));

    if( !l ) return NULL;

    l->page_sel.x = 0;
    // dynalloc for max requested num of items
    l->item_c    = req_item_count;
    l->item_d    = calloc(l->item_c, sizeof(item_t));

    if(l->item_d) return l;

    return NULL;
}

void recreate_item_t(item_t **i)
{
    if( *i )
    {
        printf("%s destroy %p, %p, %p\n", __FUNCTION__, i, *i, **i);
        for(int j = 0; j < i[0]->token_c +1; j++)
        {
            //item_idx_t **t = &i[ j ]->token_d;
            //destroy_item_t(*t);
            //printf("destroy %p %p\n", *i[ j ], &groups[j].token_d);
            printf("destroy %i %p %d\n", j, &groups[j].token_d, groups[j].token_c);
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
    // try to compose any page, count available ones
    json_c = count_availables_json();
    fprintf(INFO, "%d available json!\n", json_c);

    /* icon panel */

    if( ! icon_panel ) icon_panel = calloc(1, sizeof(layout_t));
    // pos.xy, size.wh
    icon_panel->bound_box = (ivec4) { 680, 700,   1096, 664 };
    // five columns, three rows
    icon_panel->fieldsize = (ivec2) { 5, 3 };
    icon_panel->page_sel  = (ivec2) (0);
    // malloc for max items
    icon_panel->item_c    = json_c * NUM_OF_TEXTURES;
    icon_panel->item_d    = calloc(icon_panel->item_c, sizeof(item_t));
    // reset count, we refresh from ground
    icon_panel->item_c    = 0;
    // create the first screen we will show
    GLES2_create_layout_from_json(icon_panel);

    fprintf(INFO, "layout->item_c: %d\n", icon_panel->item_c);
    // shrink
    icon_panel->item_d    = realloc(icon_panel->item_d, icon_panel->item_c * sizeof(item_t));
    // flag as to show it
    icon_panel->is_shown  = 1;
    // set max_pages
    icon_panel->page_sel.y = icon_panel->item_c / (icon_panel->fieldsize.x * icon_panel->fieldsize.y);

    /* resort using custom comparision function */
    //qsort(icon_panel->item_d, icon_panel->item_c, sizeof(item_t), struct_cmp_by_token);


    /* main left panel */

    if( ! left_panel ) left_panel = calloc(1, sizeof(layout_t));

    left_panel->bound_box = (ivec4) { 0, 900,   500, 700 };
    left_panel->fieldsize = (ivec2) { 1, 9 };
    // by calloc curr page is 0!
    /* malloc for max pages, plus additional list  */
    left_panel->item_c    = LPANEL_Y;   //sizeof(new_panel_text) / sizeof(new_panel_text[0][0]);
    left_panel->item_d    = calloc(left_panel->item_c + 1 /* Groups */, sizeof(item_t));
    // create the first page we will show
    GLES2_create_layout_from_list_v2(left_panel, &new_panel_text[0][0]);
    // new_p indexes differently!
    left_panel->item_c = 10;
    fprintf(INFO, "layout->item_c: %d\n", left_panel->item_c);

    // build an array of indexes for each one Groups
    if( ! groups ) groups = analyze_item_t_v2(icon_panel->item_d, icon_panel->item_c);
    // append additional list to item_data: take care, we
    // address this array: count is number of whitelist labels

    left_panel->item_d[LPANEL_Y].token_c = groups->token_c;
    fprintf(INFO, "groups->token_c: %d\n", groups->token_c);

    // flag as active
    left_panel->is_shown  =
    left_panel->is_active = 1;
    // set max_pages
    left_panel->page_sel.y = LPANEL_Y;

  {
    if( ! option_panel ) option_panel = calloc(1, sizeof(layout_t));

    option_panel->bound_box = (ivec4) { resolution.x -100 -1280, resolution.y -300,
                                                           1280,               720 };
    option_panel->fieldsize = (ivec2) { 1, 16 };
    option_panel->page_sel.x = 0;
    // malloc for max items
    option_panel->item_c    = option_panel->fieldsize.x * option_panel->fieldsize.y;
    option_panel->item_d    = calloc(option_panel->item_c, sizeof(item_t));
    // create the first screen we will show
    GLES2_create_layout_from_list(option_panel, &option_panel_text[0]);

    fprintf(INFO, "layout->item_c: %d\n", option_panel->item_c);
    // flag as active
    option_panel->is_shown  =
    option_panel->is_active = 0;
    // set max_pages
    option_panel->page_sel.y = LPANEL_Y;
  }

  {
    if( ! download_panel ) download_panel = calloc(1, sizeof(layout_t));

    download_panel->bound_box = (ivec4) { 500, (resolution.y -64) /8,
                                         1024,                    64 };
    download_panel->fieldsize = (ivec2) { 3, 1 };
    download_panel->page_sel.x = 0;
    // malloc for max items
    download_panel->item_c    = download_panel->fieldsize.x * download_panel->fieldsize.y;
    download_panel->item_d    = calloc(download_panel->item_c, sizeof(item_t));
    // create the first screen we will show
    GLES2_create_layout_from_list(download_panel, &download_panel_text[0]);

    fprintf(INFO, "layout->item_c: %d\n", download_panel->item_c);
    // flag as active
    download_panel->is_shown  =
    download_panel->is_active = 0;
    // set max_pages
    download_panel->page_sel.y = download_panel->item_c / (download_panel->fieldsize.x * download_panel->fieldsize.y);
  }
    // force initial status
    menu_pos.z = ON_LEFT_PANEL;
    GLES2_scene_on_pressed_button(0);

    /* UI: menu */

    // init shaders for textures
    on_GLES2_Init_icons(w, h);

    // init shaders for lines and rects
    ORBIS_RenderFillRects_init(w, h);
    // init shaders for texts
    GLES2_init_submenu();

    // optional additions
    GLES2_ani_init(w, h);
    //
    pixelshader_init(w, h);
}


void GLES2_UpdateVboForLayout(layout_t *layout)
{
    if(layout->vbo) vertex_buffer_delete(layout->vbo), layout->vbo = NULL;
    //for(int i= 0; i < layout->item_c; i++)
    //  { layout->item_d[i].in_atlas = 0; }
    if(layout->vbo_s) layout->vbo_s = EMPTY;
}



#if _DEBUG && 0
void _prt() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    unsigned long long millisecondsSinceEpoch =
        (unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000;

    printf("timestamp %llu\n", millisecondsSinceEpoch);
}
#endif




void GLES2_scene_render(void)
{
    // update the time
    on_GLES2_Update (u_t);
    on_GLES2_Update1(u_t);
    GLES2_ani_update(u_t);

    switch(menu_pos.z) // as view
    {
        case ON_TEST_ANI:
        {
            render_text();   // freetype demo-font.c, renders text just from init_

         /* GLES2_ani_test(NULL);
            GLES2_ani_update(u_t); */
        }   break;

        // the two sided panels view
        case ON_LEFT_PANEL :
        case ON_MAIN_SCREEN:
            GLES2_render_layout(icon_panel, 0);
            GLES2_render_layout(left_panel, 0);
            break; 
        // download item view
        case ON_SUBMENU_2:
            GLES2_render_layout(download_panel, 0);
            break;
        // Queue view
        case ON_SUBMENU:
            GLES2_render_queue (queue_panel, 0);
            GLES2_render_layout(left_panel, 0);
            break;
        // Ready to install view
        case ON_SUBMENU3:
            GLES2_render_queue (queue_panel, 9);
            GLES2_render_layout(left_panel, 0);
            break;
        default: 
            break;
    }

    GLES2_ani_draw(NULL);

    // ...

    if(left_panel->is_shown)
        GLES2_UpdateVboForLayout(left_panel);
    if(download_panel->is_shown)
        GLES2_UpdateVboForLayout(download_panel);
}



// just used in download panel for now
void X_action_dispatch(int action, layout_t *l)
{
    // selected item from active panel
    fprintf(INFO, "execute %d -> '%s'\n", action, l->item_d[ action ].token_d[0].off);

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
    fprintf(INFO, "item:%d, %s '%s'\n", idx,
                                        trigger,
                                     // icon_panel->item_d[ idx ].token_d[ label ].off,
                                        t[ label ].off);
    char tmp[256];
    // default
    //sprintf(&tmp[0], "/user/app/temp.pkg");
    // avoid same donwload path
    sprintf(&tmp[0], "%s/%s.pkg", get->temppath, t[ ID ].off);

    dl_arg_t *ta = NULL;
    int x = thread_find_by_item( idx );
    if(x > -1) // a thread is operating on item
    {
        ta = &pt_info[ x ];
        fprintf(INFO, "thread[%d] is handling %d, status:%d\n", x, idx, ta->status);
        //action = -1; // disable, skip action
    }

    switch(action)
    {
        case 0: {
            // a thread is downloading item, skip
            if(x > -1) return;

            http_ret = 0;
            if( dl_from_url_v2( t[ label ].off, &tmp[0], t) )
                ls_dir("/user/app");
        } break;
        case 1: {
            if(ta
            && ta->status == COMPLETED)
            {
                printf("pkginstall(%s)\n", tmp);
                // install
                pkginstall(&tmp[0]);

                // clean thread args for next job
                if(ta->dst) free(ta->dst), ta->dst = NULL;
                memset(ta, 0, sizeof(dl_arg_t));
                ta->g_idx  = -1;
                ta->status = READY;
            }
        } break;
        default:
            printf("%s\n", "more");
          break;
    }
}

void O_action_dispatch(void)
{   // reset the auxiliary list
    if(menu_pos.z != ON_SUBMENU_2)
        aux = NULL;
    // trigger textures refresh
    if(menu_pos.z != ON_SUBMENU_2)
        icon_panel->refresh = 1;
    // refresh vbo for common texts
    GLES2_UpdateVboForLayout(left_panel);

    refresh_atlas();
}

#define UP   (111)
#define DOW  (116)
#define LEF  (113)
#define RIG  (114)
#define CRO  ( 53)
#define CIR  ( 54)
#define TRI  ( 28)
#define SQU  ( 39)
#define OPT  ( 32)
#define L1   ( 10)
#define R1   ( 11)

// return vector to index for layout_t fieldsize
static ivec2 idx_to_pos_xy(const int index, const ivec2 bounds)
{
    int x = index % bounds.x,
        y = index / bounds.x;
/*  klog("%d = %d x %d\n", index, x, y);
    klog("%d = %d x %d + %d, (%d, %d)\n", bounds.x *y +x,
                                          bounds.x, y, x,
                                                    x, y); */
    return (ivec2) { x, y };
}

// return index for layout_t fieldsize from position vector
static int pos_xy_to_idx(const ivec2 pos, const ivec2 bounds)
{
    return pos.y * bounds.x + pos.x;
}

/* deal with menu position / actions */
void GLES2_scene_on_pressed_button(int button)
{
    ivec2 posi   = (ivec2) ( 0 ),
          bounds,
          result; // resulting position in selected menu:

    char  we_moved = 0;
    int   i_paged,
    // field size and selected index
          f_size,
          f_selection;
    // pointer to current panel
    layout_t *p = NULL;
    // on which VIEW are we?
    switch(menu_pos.z) // as view
    {
        case ON_LEFT_PANEL :  p = left_panel;      break;
        case ON_SUBMENU    :
        case ON_SUBMENU3   :  p = queue_panel;     break;
        case ON_SUBMENU_2  :  p = download_panel;  break;
        case ON_MAIN_SCREEN:  http_ret = 0;
        default:              p = icon_panel;      break;
    }

step1:
    result = p->item_sel;
    f_size = p->fieldsize.x * p->fieldsize.y;

    if(p == left_panel)
        bounds = (ivec2) { p->fieldsize.x -1, p->item_d[p->page_sel.x].token_c -1 };
    else
    {
        // compute some indexes, first one of layout:
        i_paged      = f_size
                     * p->page_sel.x;
        // which icon is selected in field X*Y ?
        f_selection  = p->item_sel.y * p->fieldsize.x
                     + p->item_sel.x;
        // which item is selected over all ?
        p->curr_item = f_selection + i_paged;
        // set default bounds
        result = p->item_sel;
        // bounds indexes from 0!
        bounds = p->fieldsize -1;

        /* if last page can't fill the field, how many are / REMains? */
        int rem = p->item_c - i_paged;
        // if we have an auxiliary item list to show...
        if(p == icon_panel
        && aux)
            rem = aux->len - i_paged;
        // decrease the field size!
        if(rem < f_size)
        {
            f_size = rem;
            //printf("f_size = %d\n", f_size); //3
            //int ret = p->item_c - p->curr_item;
            //fprintf(INFO, "we can't fill the page, just %d items\n", ret);
            fprintf(INFO, "last page, fieldsize: %d items\n", f_size);
            // max possible bounds
            ivec2 ret = idx_to_pos_xy(f_size, p->fieldsize);
            // 7: 2, 1
            if(ret.y > 0) ret.x = p->fieldsize.x;
            // remember we indexes bounds from 0!
            ret.x -= 1;
            bounds = ret;
            fprintf(INFO, "bounds -> (%d, %d)\n", ret.x, ret.y);

        }

        /* if we have no result */

    }

    if(we_moved) goto step2;

    switch(button)
    {
        // take in account the movement
        case UP :  {
            if(menu_pos.z == ON_SUBMENU_2) { offset.y += -100.; }
                   posi.y--;  we_moved = 1;
            } break;
        case DOW:  {
            if(menu_pos.z == ON_SUBMENU_2) { offset.y += 100.; }
                   posi.y++;  we_moved = 1;
            } break;
        case LEF:  posi.x--;  we_moved = 1;  break;
        case RIG:  posi.x++;  we_moved = 1;  break;

        case L1 : {
            if(menu_pos.z == ON_SUBMENU_2)
            {
                p = icon_panel;  posi.x--;  we_moved = 1;
                goto step1;
            }
            } break;
        case R1 : {
            if(menu_pos.z == ON_SUBMENU_2)
            {
                p = icon_panel;  posi.x++;  we_moved = 1;
                goto step1;
            }
            } break;
        // or action triggers
        case CRO: {
            we_moved = 1; // trigger vbo refresh

            fprintf(INFO, "execute: (%d/%d), %d, %d\n", p->page_sel.x, p->page_sel.y, 
                                                        p->item_sel.x, p->item_sel.y);
            // token value
            //klog("%s\n", p->item_d[p->curr_page].token_d[p->curr_item].off);

            if(menu_pos.z == ON_LEFT_PANEL)
            {
                /* "Main" page (0) */

                if(p->page_sel.x == 0  // first page
                && p->item_sel.y  < 1) // first entry
                {   // switch menu entry, lose focus
                    p->page_sel.x = 1; p->is_active = 0,
                    // activate and set focus
                    icon_panel->is_active = 1;  menu_pos.z = ON_MAIN_SCREEN;
                    break;
                }

                if(p->page_sel.x == 0 &&  // first page,
                 ( p->item_sel.y == 1     // first entry
                || p->item_sel.y == 5     // or sixth
                || p->item_sel.y == 6 ))  // or seventh
                {
                    //ani_notify("Cooming soon!");
                    msgok(NORMAL, "Coming Soon!");
                    break;
                }

                // LP: X on Groups
                if(p->page_sel.x == 0   // first page
                && p->item_sel.y == 2)  // second entry
                {
                   p->page_sel.x  = 5;  // switch page
                   result = (ivec2)(0); // reset first entry
                   break;
                }

                if(p->page_sel.x == 0   // first page
                &&(p->item_sel.y == 3   // fourth entry, or
                || p->item_sel.y == 4)) // fifth entry
                {   // switch menu entry, lose focus
                    p->page_sel.x = 0; p->is_active = 0;   
                    left_panel->is_active  =
                    icon_panel->is_active  = 0;
                    if(p->item_sel.y == 3) menu_pos.z = ON_SUBMENU3;
                    else
                    if(p->item_sel.y == 4) menu_pos.z = ON_SUBMENU;
                    // activate and set focus
                    queue_panel_init();
                    p             = queue_panel; // switch control
                    p->page_sel.x = 0;
                    p->is_active  = p->is_shown = 1;
                    // reset selection to first entry
                    p->item_sel   =
                           result = (ivec2) (0);
                    break;
                }

                // LP: X on Sort token: trigger action
                if(p->page_sel.x == 4) // trigger sort selection by token
                {
                    set_cmp_token(p->item_sel.y);
                    /* resort using custom comparision function */
                    qsort(icon_panel->item_d, icon_panel->item_c, sizeof(item_t), struct_cmp_by_token);
                    // refresh Groups item_idx_t* array
                    recreate_item_t(&groups);
//same              // refresh icons from first page
                    icon_panel->refresh = 1;
                    // switch focus, set back Games view
                    menu_pos.z = ON_MAIN_SCREEN;   // switch view
                             p->is_active = 0;  p->page_sel.x = 1;
                    icon_panel->is_active = 1;  p->item_sel.y = 1;
                    // first page, reset selection to first item
                    icon_panel->page_sel.x = 0;
                    // reset selection to first entry
                    icon_panel->item_sel  = 
                                   result = (ivec2) (0);
                    break;
                }

                /* "Games" page (1) */

                // LP: X on Search
                if(p->page_sel.x == 1    // second page
                && p->item_sel.y == 0)   // first entry
                {
                    char pattern[32];
                    klog("execute openDialogForSearch()\n");
                    // get char *pattern
                    sprintf(&pattern[0], "%s", StoreKeyboard());
                    klog("@@@@@@@@@  Search for: %s\n", pattern);
                    loadmsg("Searching....");

                    result = (ivec2)(0);

                    destroy_item_t(&q);
                    // build an array of search results
                    q = search_item_t(icon_panel->item_d,
                                      icon_panel->item_c,
                                      NAME, &pattern[0]);
                    aux = q;

wen_found_hit:

                    if(aux
                    && aux->len) // we got results, address aux!
                    {
                        printf("Showing %d items for '%s' @ %p\n", aux->len,
                                                                   aux->off, aux);
                        //  for(int i = 0; i < aux->len; i++) { printf("%d: %s %d\n", i, aux[i].off, aux[i].len); }
                        // refresh icons from first page
                        icon_panel->refresh = 1;
                        // switch focus
                        menu_pos.z = ON_MAIN_SCREEN;   // switch view
                                 p->is_active  = 0;  //p->page_sel.x  = 1;
                        icon_panel->is_active  = 1;  //p->item_sel.y = 1;
                        // first page, reset selection to first item
                        icon_panel->page_sel.x = 0;
                        // set max_pages by search results count!
                        icon_panel->page_sel.y =  aux->len
                                               / (icon_panel->fieldsize.x * icon_panel->fieldsize.y);
                        // reset selection to first entry
                        icon_panel->item_sel   = (ivec2) (0);
                        // but now we have aux array!
                    }
                    else
                    {
                        char tmp[32];
                        sprintf(&tmp[0], "No search results for %s!", aux->off);

                        ani_notify(&tmp[0]);

                        sceKernelIccSetBuzzer(3); // 1 longer
                    }
                    sceMsgDialogTerminate();
                    break;
                }

                // LP: X on Sort: switch LP page
                if(p->page_sel.x == 1    // second page
                && p->item_sel.y == 1)   // second entry
                {
                   p->page_sel.x  = 4;   // switch entry
                   result = (ivec2)(0);  // reset first entry
                }

                /* "Group" page (5 (LPANEL_Y)) */

                // LP: X on Group item
                if(p->page_sel.x == LPANEL_Y) // Group page
                {
                    printf("%d: %s %d\n", p->item_sel.y,
                                   groups[p->item_sel.y +1].token_d[0].off,
                                   groups[p->item_sel.y +1].token_c);

                    aux      = &groups[p->item_sel.y +1].token_d[0];
                    aux->len =  groups[p->item_sel.y +1].token_c;
                    // leave selection saved
                    result = p->item_sel;
                    // jump there to complete the job
                    goto wen_found_hit;
                }
            }
            else
            if(menu_pos.z == ON_MAIN_SCREEN)
            {

switch_to_download:

                menu_pos.z = ON_SUBMENU_2;   // switch view
                         p = download_panel; // switch control
                         p->page_sel.x = 0;
                         p->is_active  = p->is_shown = 1;
                left_panel->is_active  =
                icon_panel->is_active  = 0;
                // reset selection to first item
                p->item_sel = 
                     result = (ivec2) (0);
                // reset vertical displace
                     offset =  (vec3) (0);
                break;
            }
            else
            if(menu_pos.z == ON_ITEM_PAGE)
            {

                    //klog("package: %s\n", get_json_token_value(&selected_row->item[selected_icon].token[0], PACKAGE));
            }
            else
            if(menu_pos.z == ON_SUBMENU
            || menu_pos.z == ON_SUBMENU3)
            {
                int req_status;
                switch(left_panel->item_sel.y)
                {   
                    case 3: // Ready to install
                        req_status = COMPLETED; break;
                        queue_panel->fieldsize.y = thread_count_by_status( req_status ); break;
                    case 4: // Queue
                        req_status = RUNNING;   break;
                        queue_panel->fieldsize.y = thread_count_by_status( req_status ); break;
                }
                // which thread is the 'i'th
                int idx = thread_find_by_status(p->item_sel.y, req_status);
                // no results, skip
                if(idx < 0) return;

                dl_arg_t *ta = &pt_info[ idx ];
                printf("item_sel.y:%d -> ta[%d]->g_idx:%d\n", p->item_sel.y,
                                                              idx, ta->g_idx);

                /* install fpkg */
                if(ta
                && ta->status == COMPLETED)
                {
                    char tmp[256];
                    sprintf(&tmp[0], "Installing %s", ta->token_d[ NAME ].off);
                    ani_notify(&tmp[0]);

                    // install
                    sprintf(&tmp[0], "%s/%s.pkg", get->temppath, ta->token_d[ ID ].off);
                    printf("pkginstall(%s)\n", tmp);

                    pkginstall(&tmp[0]);

                    // clean thread args for next job
                    if(ta->dst) free(ta->dst), ta->dst = NULL;
                    memset(ta, 0, sizeof(dl_arg_t));
                    ta->g_idx   = -1;
                    ta->status  = READY;
                    p->item_sel = (ivec2)(0);
                    return;
                }

                /* go to download page for selected item */
                if(ta
                && ta->status == RUNNING)
                {
                    // get vector from selected item index
                    ivec2 x = idx_to_pos_xy( ta->g_idx, icon_panel->fieldsize );

                    printf("x (%d, %d), %d %d\n", x.x, x.y, 
                                                  x.y % icon_panel->fieldsize.y,
                                                  x.y / icon_panel->fieldsize.y);

                    ivec3 X = (ivec3) { x.x,
                                        x.y % icon_panel->fieldsize.y,
                                        x.y / icon_panel->fieldsize.y };
                    printf("(x:%d, y:%d, z:%d)\n", X.x, X.y, X.z);

                    // update current selection in icon_panel
                    icon_panel->curr_item = x.x;
                    icon_panel->item_sel  = X.xy,
                    icon_panel->page_sel  = X.z;
                    // switch to download page
                    goto switch_to_download;
                }
            }
            else
            if(menu_pos.z == ON_SUBMENU_2)
            {
                X_action_dispatch(f_selection, p);
            }

            } break;
        case CIR: {

            if(aux
            && left_panel->page_sel.x != LPANEL_Y)
            {
                aux = NULL;
                icon_panel->refresh = 1;
            }

            if(menu_pos.z == ON_LEFT_PANEL)
            {   // switch menu entry, set focus
                p         ->is_active = 1; p->page_sel.x = 0,
                icon_panel->is_active = 0;
                left_panel->item_sel  = (ivec2)(0);
                // reset to first icon page
                icon_panel->page_sel.x = 0;
            }
            else
            if(menu_pos.z == ON_MAIN_SCREEN
            || menu_pos.z == ON_SUBMENU
            || menu_pos.z == ON_SUBMENU3)
            {
                menu_pos.z = ON_LEFT_PANEL;
                option_panel->is_active = option_panel->is_shown = 0,
                icon_panel  ->is_active = 0;
                left_panel  ->is_active = 1; // back
                if(queue_panel) queue_panel->refresh = 1;
            }
            else
            if(menu_pos.z == ON_SUBMENU_2)
            {
                menu_pos.z = ON_MAIN_SCREEN;
                offset = (vec3) (0);
                download_panel->is_shown  =
                download_panel->is_active =
                left_panel    ->is_active = 0;
                icon_panel    ->is_active = 1; // back
                download_panel->refresh =
                icon_panel    ->refresh = 1;
            }
            we_moved = 1; // trigger vbo refresh
//              fprintf(INFO, "CIR %d\n", menu_pos.z);
            } break;
        case TRI: {
             menu_pos.z = ON_SUBMENU; rela_pos = (0); // reset on open
             left_panel  ->is_active =
             icon_panel  ->is_active = 0;
             option_panel->is_active = 1; option_panel->is_shown = 1;
                }  break;
        case SQU:  {
             //menu_pos.z = ON_TEST_ANI;
                }  break;
             break;
        // whatever other, do nothing
        default:  return;
    }

step2:

    // apply the movement
    result += posi;

    // handle the movement:
    fprintf(INFO, "layout:%p, bounds: %d %d\n", p, bounds.x, bounds.y);
    /* keep main bounds on available items */
    // left panel
    if(result.x < 0) 
    {
        result.x = bounds.x;//p->fieldsize.x -1; // max

        if(p == icon_panel)
        {   // switch active panel, don't move
            menu_pos.z = ON_LEFT_PANEL; p = left_panel;
            bounds     = p->fieldsize -1;
            result     = p->item_sel;
            left_panel->is_active = 1;
            icon_panel->is_active = 0;
        }
    }
    else
    if(result.x > bounds.x)
    {
        result.x = 0; // min

        if(p == left_panel)
        {   // switch active panel, don't move
            menu_pos.z = ON_MAIN_SCREEN; p = icon_panel;
            bounds     = p->fieldsize -1;
            result     = p->item_sel;
            left_panel->is_active = 0;
            icon_panel->is_active = 1;
        }
    }
    /* keep main bounds on available pages */
    if(result.y < 0)
    {
        result.y = bounds.y; // max XXX

      //if(p == left_panel) p->page_num = 0;
        if(p == icon_panel && p->page_sel.y != 0)
        {   // change page, refresh textures
            p->refresh = 1; p->page_sel.x -= 1;
        }
        we_moved = 1; //GLES2_UpdateVboForLayout(left_panel);
    }
    else
    if(result.y > bounds.y)
    {
        result.y = 0;

        if(p == icon_panel && p->page_sel.y != 0)
        {   // change page, refresh textures
            p->refresh = 1; p->page_sel.x += 1;
        }
        we_moved = 1; //GLES2_UpdateVboForLayout(left_panel);
    }

    // keep bounds on pages
    if(p->page_sel.x < 0)
    {  // go to last page!, take care
       p->page_sel.x = p->page_sel.y, result = (ivec2)(0);
    }
    else
    if(p->page_sel.x > p->page_sel.y)
       p->page_sel.x = 0;

    fprintf(INFO, "%s result( %d, %d)\n", __FUNCTION__, result.x, result.y);
    //menu_pos.xy = result;
    p->item_sel = result;

    if(p == icon_panel)
    {   // keep bounds on remaining entries
        int idx = pos_xy_to_idx(result, p->fieldsize);

        if(idx > f_size -1)
        {
            p->page_sel.x = 0;
            p->item_sel   = (ivec2)(0);
            // refresh icons from first page
            p->refresh    = 1;
        }
    }

    // refresh the common texts, page number etc.
    if(we_moved == 1)
    {
        GLES2_UpdateVboForLayout(left_panel);
        GLES2_UpdateVboForLayout(icon_panel);
        GLES2_UpdateVboForLayout(download_panel);
        GLES2_UpdateVboForLayout(option_panel);
    }
    // if we dropped auxiliary list, refresh max_page
    if(p == icon_panel && !aux)
    {
        p->page_sel.y =  p->item_c
                      / (p->fieldsize.x * p->fieldsize.y);
    }
    // compute some indexes, first one of layout:
    i_paged      = p->fieldsize.y * p->fieldsize.x
                 * p->page_sel.x;
    // which icon is selected in field X*Y ?
    f_selection  = p->item_sel.y * p->fieldsize.x
                 + p->item_sel.x;
    // which item is selected over all ?
    p->curr_item = f_selection + i_paged;

    fprintf(INFO, "p:%d/%d, s:%d/%d (%d, %d):%d\n",
                     p->page_sel.x, p->page_sel.y,
                     p->curr_item,  p->item_c,
                     p->item_sel.x, p->item_sel.y, f_selection);

    idx_to_pos_xy(p->curr_item, p->fieldsize);

    fprintf(INFO, "dpad:%d, %d\n", confPad->padDataCurrent->lx,
                                   confPad->padDataCurrent->ly);

}

