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
#include <utils.h>

#include "defines.h"
#include "shaders.h"
#include "GLES2_common.h"
#include "utils.h"

extern char* group_label[8];
extern vec2 resolution;
extern bool unsafe_source;

static const char* group_labels[] =
{   // 0 is reserved index for: (label, total count)
    "HB Game",
    "Emulator",
    "Emulator Add-On",
    "Media",
    "Mira Plugins",
    "Utility",
    "Other"
};

const char* group_labels_non_pkg_zone[] =
{   // 0 is reserved index for: (label, total count)
    "Game",
    "Patch",
    "DLC",
    "Theme",
    "App",
    "Unknown",
    "Other"
};

extern atomic_bool is_icons_finished, icons_thread_started;

// check for pattern in whitelist patterns, if there take count, if not fall in Other

/* pattern from icon tokens, item for Group arrays */
static int check_for_token(char *pattern, item_t *item, int count)
{
    int   i, plen = strlen(pattern);
    item_idx_t *t = NULL;
    /*  loop over, skip very first one (0) reserved and
        (6) for Other, used to keep "unmatched labels" */
    for(i = 1; i < count -1; i++)
    {
        t = &item[i].token_d[0];
        /* check for exact match */
        if( strstr(t->off, pattern)  // hit found +
        && (strlen(t->off) == plen)) // same length
            break;

        /* check for common 'Game' label */
        if(i == 1
        && strstr(pattern, "Game"))  // hit found
            break;
    }
    // store: if not found before falls into Other, take count
    item[i].token_c += 1;
/*  log_info("%d: %s -> %s (%d)", i, pattern, item[i].token_d[0].off,
                                              item[i].token_c); */
    return i;
}

// we return an array of indexes and count number, per Group
item_t* analyze_item_t_v2(item_t* items, int item_count)
{
    // same number of group_labels, +1 for reserved main index
    int  i, count = sizeof(group_labels_non_pkg_zone) / sizeof(group_labels_non_pkg_zone[0]) + 1;
    // dynalloc
    item_t* ret = calloc(count, sizeof(item_t));
    item_idx_t* t = NULL;

    // init: fill the Groups labels
    for (i = 1; i < count; i++)
    {   // we expect no more than this number, per Group label
        ret[i].token_c = item_count + 1;
        // dynalloc for item_idx_t
        if (!ret[i].token_d)
            ret[i].token_d = calloc(ret[i].token_c, sizeof(item_idx_t));
        //        log_info("%s %p", __FUNCTION__, layout->item_d[idx].token_d);
                // clean count, address token
        t = &ret[i].token_d[0];
        ret[i].token_c = 0;
        // address the label
        t->off =  unsafe_source ? (char*)group_labels_non_pkg_zone[i - 1] : (char*)group_labels[i - 1];
        t->len = 0;
    }
    // reserved index (0)!
    ret[0].token_d = NULL;
    ret[0].token_c = count - 1;

    /*  count for label, iterate all passed items
        index for each Group in token_data[ ].len */
    for (i = 0; i < item_count; i++)
    {
        t = &items[i].token_d[APPTYPE];
        // in which Group fall this item?
        int res = check_for_token(t->off, &ret[0], count);
        /*      log_info("%d %d: %s (%d) %s", i, res, ret[ res ].token_d[0].off,
                                                      ret[ res ].token_c, t->off); */
        int idx = ret[res].token_c;
        // store the index for item related to icon_panel list!
        ret[res].token_d[idx].len = i;
    }

#if 1
    // done building the item_t array, now we check items sum
    int check = 0;
    for (i = 1; i < count; i++)
    {   // report the main reserved index
        t = &ret[i].token_d[0];
        t->off = group_label[i - 1];
        log_info("%d %s: %d", i, ret[i].token_d[0].off,
            ret[i].token_c);
        // shrink buffers, remember +1 !!!
        ret[i].token_d = realloc(ret[i].token_d, (ret[i].token_c + 1) * sizeof(item_idx_t));

        check += ret[i].token_c;
    }
    log_info("Sorted %d items across %d Groups", check, ret[0].token_c + 1);
#endif

    return ret;
}


bool is_in_list(char *pattern, item_idx_t *t)
{
    for(int i = 0; i < t->len; i++)
    {   /* check for match */
        if( strstr(t[ i +1 ].off, pattern) )
            return true; // hit found
    }
    return false;
}

// update all double pointers
void build_char_from_items(char **data, item_idx_t *filter)
{
    for(int i = 0; i < filter->len; i++)
    {
        data[ i ] = filter[ i +1 ].off;
       
    }
}

// build unique list of Token_Label (PV, AUTHOR)
item_idx_t *build_item_list(item_t *items, int item_count, enum token_name TN)
{
    
    // at most same number of items, we realloc then
    item_idx_t *ret = calloc(item_count +1, sizeof(item_idx_t));
    int  curr_count = 1;

    for(int i = 0; i < item_count; i++)
    {
        item_idx_t *t = &items[i].token_d[ TN ];
        // skip "not single token" for now
        if( strstr(t->off, ", ")
        ||  strstr(t->off, "& ") ) continue;

        if( ! is_in_list(t->off, ret) )
        {   // add entry
            ret[curr_count].off  = t->off;
            ret[curr_count].len += 1;
            ret[0].len += 1;
            curr_count += 1;
        }
        else // add count
            ret[curr_count].len += 1;
    }
    ret = realloc(ret, curr_count * sizeof(item_idx_t));
    // save total count
    ret[0].len = curr_count -1;

    // report unique entries and count
    for(int j = 1; j < ret->len +1; j++)
    {
        volatile int count = 0;
        for(int i = 0; i < item_count; i++)
        {
            if( strstr(items[i].token_d[ TN ].off,
                         ret[j].off) ) count += 1;
        }
        // save count
        ret[j].len = count;
        log_info( "%2d: %24s, %d, %d", j, ret[j].off, ret[j].len, count);
    }
    log_info( "%s ret %p", __FUNCTION__, ret);
    return ret;
}

// search for pattern in items tokens
item_idx_t *search_item_t(item_t *items, int item_count, enum token_name TN, char *pattern)
{
    /* store in the very first item_idx_t:
       pattern searched and array length, so +1 !!! */

    // at most same number of items, we realloc then
    item_idx_t *ret = calloc(item_count +1, sizeof(item_idx_t));
    int  curr_count = 1,
                len = strlen(pattern);
    char         *p = NULL;

    // build an array of discovered unique entries
    for(int i = 0; i < item_count; i++)
    {
        item_idx_t *t = &items[i].token_d[ TN ];

        if(TN == NAME
        || TN == PV
        || TN == AUTHOR)
        {
            p = strstr(t->off, pattern);
            if(p) // hit found ?
            {
                log_info("hit[%d]:%3d %s", curr_count, i, t->off);
                ret[curr_count].off = t->off,
                ret[curr_count].len = i; // store index for icon_panel!
                curr_count++;
            }
        }
        else
        if(TN == APPTYPE)
        {
            int res = memcmp(t->off, pattern, len);
            if(!res // hit found?
            && len == strlen(t->off)) // same length?
            {
                log_info("hit[%2d]:%3d %s %d %lu", curr_count, i, t->off, res, strlen(pattern));
                ret[curr_count].off = t->off,
                ret[curr_count].len = i; // store index for icon_panel!
                curr_count++;
            }
        }
    }
    // shrink buffer, consider +1 !!!
    ret = realloc(ret, curr_count * sizeof(item_idx_t));
    // save count as very first item_idx_t.len
    ret[0].len = curr_count -1;

    log_info("pattern '%s' counts %d hits", pattern, ret[0].len);

    ret[0].off = strdup(pattern); // save pattern too

    return ret;
}

// use pointer to pointer to clean the struct
void destroy_item_t(item_idx_t **p)
{
    if(*p)
    {   // clean auxiliary array
        for(int i=0; i<1 /*NUM_OF_USER_TOKENS*/ ; i++)
        {
            if(p[i]->off) { free(p[i]->off), p[i]->off = NULL; p[i]->len = 0; }
        }
        free(*p), *p = NULL;
    }
}

/* common text */

static vertex_buffer_t *t_vbo = NULL, // for sysinfo
                       *c_vbo = NULL; // common texts, per view

vec3 clk = (vec3){ 0., 0, 15 }; // timer(now, prev, limit in seconds)

// disk free percentages
double dfp_hdd  = -1.,
       dfp_ext  = -1.,
       dfp_fmem = -1.;

static bool refresh_clk = true;
extern atomic_bool refresh_icons;
/* upper right sytem_info, updates
   internally each 'clk.z' seconds */
int sceKernelGetCpuTemperature(uint32_t *temp);
void GLES2_Draw_sysinfo(void)
{   
    
    char usb[255];
    unsigned int usb_numb = usbpath();
    sprintf(&usb[0], "/mnt/usb%d/", usb_numb);
    // update time
    clk.x += u_t - clk.y;
    clk.y  = u_t;
    // time limit passed!
    if(clk.x / clk.z > 1. || refresh_clk) // by time or requested
    {   // destroy VBO
        if(t_vbo) vertex_buffer_delete(t_vbo), t_vbo = NULL;
        // reset clock
        clk.x = 0., refresh_clk = false;
    }

    if( ! t_vbo ) // refresh
    {
        char tmp[128];
        vec2 pen,
             origin   = resolution,
             border   = (vec2)(50.);
             t_vbo    = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
        // add border, in px
              origin -= border;
              pen     = origin;
        /* get systime */
        time_t     t  = time(NULL);
        struct tm *tm = localtime(&t);
        char s[65];
        strftime(s, sizeof(s), "%A, %B %e %Y, %H:%M", tm); // custom date string
//      log_info("%s", s);
        snprintf(&tmp[0], 127, "%s", s);
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( sub_font, &tmp[0] );
        // we know 'tl' now, right align
        pen.x -= tl;
        // fill the vbo
        add_text( t_vbo, sub_font, &tmp[0], &col, &pen);

        uint32_t numb = 70;
        size_t fmem = 0;
#if defined(__ORBIS__)
        sceKernelGetCpuTemperature(&numb);
        sceKernelAvailableFlexibleMemorySize(&fmem);
#endif
        snprintf(&tmp[0], 127, "%s: %x, %d°C, %zub", getLangSTR(SYS_VER),SysctlByName_get_sdk_version(), numb, fmem);
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( main_font, &tmp[0] ); 
        // we know 'tl' now, right align
        pen.x  = origin.x - tl,
        pen.y -= 32;
        vec4 c = col * .75f;
        // fill the vbo
        add_text( t_vbo, main_font, &tmp[0], &c, &pen);

        snprintf(&tmp[0], 127, "%s: %s", getLangSTR(STORE_VER),&completeVersion[0]);
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( main_font, &tmp[0] ); 
        // we know 'tl' now, right align
        pen.x  = origin.x - tl,
        pen.y -= 32;
        // fill the vbo
        add_text( t_vbo, main_font, &tmp[0], &c, &pen);

        // eventually, skip dfp on some view...
        if(menu_pos.z < ON_ITEMzFLOW)
        {   /* text for disk_free stats */
               pen = (vec2) { 26, 100 };
            vec4 c = col * .75f;
            //
            if(usb_numb != -1)
            {   // get mountpoint stat info
                dfp_ext = df(&tmp[0], usb);
                
            } else
                dfp_ext = 0;
            // start upper
            if(dfp_ext > 0) pen.y += 32;
            // fill the vbo
            add_text( t_vbo, sub_font, getLangSTR(STORAGE), &col, &pen);
            // new line
            pen.x   = 26,
            pen.y  -= 32;

            if(dfp_ext > 0)
            {
                add_text( t_vbo, main_font, &tmp[0], &c, &pen);
                // a new line
                pen.x   = 26,
                pen.y  -= 32;
            }

            //if(dfp_hdd < 0)
            {  // get mountpoint stat info
                dfp_hdd = df(&tmp[0], "/user");
            } 
            add_text( t_vbo, main_font, &tmp[0], &c, &pen);

            // when not on cf
            // dont race the icons thread
            if (is_icons_finished && !icons_thread_started
             && icon_panel->item_c > 150) {
                log_info("dropping ...");
                drop_some_icon0();
            }
        }

        // update FMEM
        dfp_fmem = (1. - (double)fmem / (double)0x8000000) * 100.;

        // we eventually added glyphs... (todo: glyph cache)
        refresh_atlas();
    }

    // skip on some view...
    if(menu_pos.z < ON_ITEMzFLOW)
    {   /* draw filled color bar */
        vec4 r = (vec4) { -.975, -.900,   -.505, -.905 };
        // gles render the frect
        ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
        /* draw filling color bar, by percentage */
        GLES2_DrawFillingRect(&r, &sele, &dfp_hdd);

        /* the vertical grey frect (4px line) */
             r = (vec4) { 500, 0,   4, resolution.y };
        vec2 p = r.xy,  s = r.xy + r.zw;
        /* convert to normalized coordinates */
        r.xy = px_pos_to_normalized(&p),
        r.zw = px_pos_to_normalized(&s);
        // gles render the frect
        ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
    }

    /* FMEM: draw filling color bar, by percentage */
    vec4 r = (vec4) { -.975, -.950,   -.505, -.955 };
    //ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
    ORBIS_RenderDrawBox(USE_COLOR, &grey, &r);
    GLES2_DrawFillingRect(&r, &white, &dfp_fmem);

    // texts out of VBO
    if(t_vbo) ftgl_render_vbo(t_vbo, NULL);
}

void GLES2_refresh_sysinfo(void)
{   // will trigger t_vbo refresh (all happen in renderloop)
    refresh_clk = true;
}

extern char selected_text[64];

void GLES2_Draw_common_texts(void)
{
    // split dfp, item, pages?

    switch(menu_pos.z) // skip draw on those views
    {
        case ON_INSTALL:
        case ON_QUEUE:
        case ON_ITEM_INFO:
            return;
    }

    if( ! c_vbo ) // refresh
    {
        char tmp[128];
        vec2 pen     = (vec2){ 650, 950 },
             origin  = resolution,
             border  = (vec2)(50.);
             c_vbo   = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
        // add border, in px
             origin -= border;

        // text from pointed layout_item token label
        add_text( c_vbo, titl_font, &selected_text[0], &col, &pen);

        /* lower-right item/page infos */
        if( menu_pos.z == ON_MAIN_SCREEN
        || (menu_pos.z == ON_LEFT_PANEL && active_p->item_c > active_p->f_size) )
        {
            snprintf(&tmp[0], 127, "%d/%d", active_p->curr_item +1,
                                           active_p->item_c);
            // can be more than one page?
            if(active_p->item_c > active_p->f_size)
            {
                int  l   = strlen(tmp);
                // compute page_infos
                ivec2 pi  = (ivec2) { active_p->curr_item, active_p->item_c };
                      pi /= (ivec2) ( active_p->fieldsize.x * active_p->fieldsize.y );
                      pi += 1;
                      snprintf(&tmp[l], 127, ", %s %d/%d", getLangSTR(PAGE2) ,pi.x, pi.y);
            }
            // we need to know Text_Length_in_px in advance, so we call this:
            texture_font_load_glyphs( sub_font, &tmp[0] ); 
            // we know 'tl' now, right align
            pen.x = origin.x - tl,
            pen.y = border.y;
            // fill the vbo
            add_text( c_vbo, sub_font, &tmp[0], &col, &pen);
        }
        // we could have added new glyps
        refresh_atlas();
    }
    else // texts out of VBO
        ftgl_render_vbo(c_vbo, NULL);
}

void GLES2_refresh_common(void)
{
    if(c_vbo) vertex_buffer_delete(c_vbo), c_vbo = NULL;
}

