/*
    GLES2 scene

    my implementation of an interactive menu, from scratch.
    (shaders
     beware)

    2020, masterzorag
*/

#include <stdio.h>
#include <string.h>

#include <utils.h>

#include "defines.h"
#include "shaders.h"
#include "GLES2_common.h"
#include "utils.h"
#include <array>


extern std::vector<std::string> group_label;
extern vec2 resolution;
extern bool unsafe_source;


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

extern std::atomic_bool is_icons_finished, icons_thread_started;

// check for pattern in whitelist patterns, if there take count, if not fall in Other

static int check_for_token(const std::string &pattern, std::vector<item_t> &item, int count)
{
    int plen = pattern.size();

    // Loop over, skip very first one (0) reserved and
    // (6) for Other, used to keep "unmatched labels"
    for (int i = 1; i < count - 1; i++)
    {
           if (item[i].token_d[0].off.empty()) {
            log_info("check_for_token: item[%d].token_d[0].off is null", i);
        continue; // Skip this iteration if the pointer is null
    }
        const std::string t = item[i].token_d[0].off;

        // Check for exact match
        if (t.find(pattern) != std::string::npos && // hit found
            (t.size() == plen)) // same length
        {
            item[i].token_c += 1;
            return i;
        }

        // Check for common 'Game' label
        if (i == 1 && pattern.find("Game") != std::string::npos) // hit found
        {
            item[i].token_c += 1;
            return i;
        }
    }

    // Store: if not found before falls into Other, take count
    item[count - 1].token_c += 1;
    return count - 1;
}


std::vector<item_t> analyze_item_t_v2(std::vector<item_t>& items, int numb) {
    if (items.empty()) {
        return std::vector<item_t>();
    }

    int count = sizeof(group_labels_non_pkg_zone) / sizeof(group_labels_non_pkg_zone[0]) + 1;
    std::vector<item_t> ret(count);

    for (int i = 1; i < count; i++) {
        ret[i].token_c = numb + 1;
        ret[i].token_d.resize(ret[i].token_c);

        if (!ret[i].token_d.empty()) {
            item_idx_t &t = ret[i].token_d[ID];
            ret[i].token_c = 0;
            t.off = group_label[i - 1];
            t.len = 0;
        }
    }

    ret[0].token_c = count - 1;
    ret[0].token_d.resize(numb + 1);
    ret[0].token_d[ID].off = "Total";

    for (int i = 0; i < numb; i++) {
        if (i < items.size() && !items[i].token_d.empty() && APPTYPE < items[i].token_d.size()) {
            item_idx_t t = items[i].token_d[APPTYPE];
            int res = check_for_token(t.off, ret, count);
            if (res < ret.size()) {
                int idx = ret[res].token_c;
                if (idx < ret[res].token_d.size()) {
                    ret[res].token_d[idx].len = i;
                }
            }
        }
    }

    int check = 0;
    for (int i = 1; i < count; i++) {
        if (!ret[i].token_d.empty()) {
            item_idx_t &t = ret[i].token_d[0];
            t.off = group_label[i - 1];
            log_info("%d %s", i, t.off.c_str());
            ret[i].token_d.resize(ret[i].token_c + 1);
            check += ret[i].token_c;
        }
    }

    log_info("Sorted %d items across %d Groups", check, ret[0].token_c + 1);

    return ret;
}

bool is_in_list(const std::string& pattern, const std::vector<item_idx_t>& list) {
    return std::any_of(list.begin(), list.end(),
                      [&pattern](const item_idx_t& item) { return item.off.find(pattern) != std::string::npos; });
   return false;
}

void build_char_from_items(std::vector<std::string>& data, const std::vector<item_idx_t>& filter) {
    data.clear();
    data.reserve(filter.size());
    for (const auto& item : filter) {
        data.push_back(item.off);
    }
}


std::vector<item_idx_t> build_item_list(std::vector<item_t>& items, token_name TN) {
    std::vector<item_idx_t> ret;
    int curr_count = 1;

    ret.resize(items.size() + 1);

    for (int i = 0; i < items.size(); i++) {
        item_idx_t t = items[i].token_d[TN];

        if (t.off.find(", ") != std::string::npos ||
            t.off.find("& ") != std::string::npos) {
            continue;
        }

        if (!is_in_list(t.off, ret)) {
            ret[curr_count].off  = t.off;
            ret[curr_count].len += 1;
            ret[0].len += 1;
            curr_count += 1;
        } 
        else // add count
            ret[curr_count].len += 1;
    }
    
    ret.resize(curr_count);
    ret[0].len = curr_count -1;

    for (int j = 1; j < ret.size() + 1; j++) {
        int count = 0;
        for (int i = 0; i < items.size(); i++) {
            if (items[i].token_d[TN].off.find(ret[j].off) != std::string::npos) {
           // if(strstr(items[i].token_d[TN].off, ret[j].off) != NULL) {
                count += 1;
            }
        }
        // save count
        ret[j].len = count;
        log_info("%2d: %24s, %d, %d", j, ret[j].off.c_str(), ret.size(), count);
    }
    log_info("%s ret", __FUNCTION__);

    return ret;
}

// Implement strcasestr if not available on your system
// Case-insensitive string search function
std::string::const_iterator case_insensitive_search(const std::string& haystack, const std::string& needle) {

    if (haystack.empty() || needle.empty()) {
        return haystack.end();
    }


    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                       [](char a, char b) {
                           return std::tolower(a) == std::tolower(b);
                       });
}

bool case_insensitive_compare(const std::string &s1, const std::string &s2)
{
    return (s1.size() == s2.size()) && std::equal(s1.begin(), s1.end(), s2.begin(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        });
}


std::vector<item_idx_t> search_item_t(std::vector<item_t>& items, token_name TN, const std::string &pattern) {
    std::vector<item_idx_t> ret;
    int curr_count = 1;
    int len = pattern.size();
    std::string::const_iterator p;

    ret.resize(items.size() + 1);

    if(items[0].token_c > items.size()){
        log_info("items[0].token_c > items.size()");
        return std::vector<item_idx_t>();
    }

    for (int i = 0; i < items[0].token_c; i++) {
        if (TN == NAME || TN == PV || TN == AUTHOR || TN == ID) {
           p = case_insensitive_search(items[i].token_d[TN].off, pattern);
           if (p != items[i].token_d[TN].off.end())  {
                log_info("hit[%d]:%d %s %s", curr_count, i, items[i].token_d[TN].off.c_str(), p);
                ret[curr_count].off = items[i].token_d[TN].off,
                ret[curr_count].len = i; // store index for icon_panel!
                curr_count++;
            }
        }
        else if (TN == APPTYPE) {
            bool res = case_insensitive_compare(items[i].token_d[TN].off, pattern);
            if (!res && len == items[i].token_d[TN].off.size()){//items[i].token_d[TN].off.size()) {
                log_info("hit[%d]:%d %s %s", curr_count, i, items[i].token_d[TN].off.c_str(), p);
                ret[curr_count].off = items[i].token_d[TN].off,
                ret[curr_count].len = i; // store index for icon_panel!
                curr_count++;
            }
        }
    }

    log_info("ret.len %d", ret.size());
    ret.resize(curr_count);
    ret[0].off = pattern;
    ret[0].len = curr_count -1;

    return ret;
}

// use pointer to pointer to clean the struct
void destroy_item_t(std::vector<item_idx_t> &p)
{
    if(!p.empty() )
    {   // clean auxiliary array
        for(int i=0; i < TOTAL_NUM_OF_TOKENS; i++)
        {
            log_info("item_idx_t[%d] %p", i, p[i].off.c_str());
            p[i].len = 0;
            
        }
        p.clear();
    }
}

/* common text */

static VertexBuffer t_vbo, // for sysinfo
                       c_vbo; // common texts, per view

vec3 clk = (vec3){ 0., 0, 15 }; // timer(now, prev, limit in seconds)

// disk free percentages
double dfp_hdd  = -1.,
       dfp_ext  = -1.,
       dfp_fmem = -1.;

static bool refresh_clk = true;
extern std::atomic_bool refresh_icons;
/* upper right sytem_info, updates
   internally each 'clk.z' seconds */
extern "C" int sceKernelGetCpuTemperature(uint32_t *temp);
void GLES2_Draw_sysinfo(void)
{   
    unsigned int usb_numb = usbpath();
    std::string dfp_ext_str;
    std::vector<vec4> rr;
    std::string dfp_hdd_str;
    std::string usb = fmt::format("/mnt/usb{}/", usb_numb);

    // update time
    clk.x += u_t - clk.y;
    clk.y  = u_t;
    // time limit passed!
    if(clk.x / clk.z > 1. || refresh_clk) // by time or requested
    {   // destroy VBO
        t_vbo.clear();
        // reset clock
        clk.x = 0., refresh_clk = false;
    }

    if( ! t_vbo ) // refresh
    {
        std::string tmp;
        vec2 pen,
             origin   = resolution,
             border   = (vec2)(50.);
             t_vbo    = VertexBuffer( "vertex:3f,tex_coord:2f,color:4f" );
        // add border, in px
              origin -= border;
              pen     = origin;
        /* get systime */
        time_t     t  = time(NULL);
        struct tm *tm = localtime(&t);
        std::array<char, 65> time_str;
        strftime(time_str.data(), time_str.size(), "%A, %B %e %Y, %H:%M", tm); // custom date string
        tmp = time_str.data();
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( sub_font, tmp.c_str() );
        // we know 'tl' now, right align
        pen.x -= tl;
        // fill the vbo
        //add_text( t_vbo, sub_font, tmp.c_str(), &col, &pen);
        t_vbo.add_text( sub_font, tmp, col, pen);

        uint32_t numb = 70;
        size_t fmem = 0;
#if defined(__ORBIS__)
        sceKernelGetCpuTemperature(&numb);
        sceKernelAvailableFlexibleMemorySize(&fmem);
#endif
        tmp = fmt::format("{}: {:x}, {}°C, {}", getLangSTR(SYS_VER), SysctlByName_get_sdk_version(), numb, fmem);
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( main_font, tmp.c_str() ); 
        // we know 'tl' now, right align
        pen.x  = origin.x - tl,
        pen.y -= 32;
        vec4 c = col * .75f;
        // fill the vbo
        //add_text( t_vbo, main_font, tmp.c_str(), &c, &pen);
        t_vbo.add_text( main_font, tmp, c, pen);

        tmp = fmt::format("{}: {}", getLangSTR(STORE_VER), completeVersion);
        // we need to know Text_Length_in_px in advance, so we call this:
        texture_font_load_glyphs( main_font, tmp.c_str() ); 
        // we know 'tl' now, right align
        pen.x  = origin.x - tl,
        pen.y -= 32;
        // fill the vbo
        //add_text( t_vbo, main_font, tmp.c_str(), &c, &pen);
        t_vbo.add_text( main_font, tmp, c, pen);

        // eventually, skip dfp on some view...
        if(menu_pos.z < ON_ITEMzFLOW)
        {   /* text for disk_free stats */
               pen = (vec2) { 26, 100 };
            vec4 c = col * .75f;
            //
            if(usb_numb != -1)
            {
                              // get mountpoint stat info
                dfp_ext = df(usb, dfp_ext_str);
            }
            else
                dfp_ext = 0;
            // start upper
            if(dfp_ext > 0) pen.y += 32;
            // fill the vbo
            //add_text( t_vbo, sub_font, getLangSTR(STORAGE).c_str(), &col, &pen);
            t_vbo.add_text( sub_font, getLangSTR(STORAGE), col, pen);
            // new line
            pen.x   = 26,
            pen.y  -= 32;

            if(dfp_ext > 0)
            {
                //add_text( t_vbo, main_font, dfp_ext_str.c_str(), &c, &pen);
                t_vbo.add_text( main_font, dfp_ext_str, c, pen);
                // a new line
                pen.x   = 26,
                pen.y  -= 32;
            }

            //if(dfp_hdd < 0)
            {  // get mountpoint stat info
                dfp_hdd = df("/user", dfp_hdd_str);
            } 
            //add_text( t_vbo, main_font, dfp_hdd_str.c_str(), &c, &pen);
            t_vbo.add_text( main_font, dfp_hdd_str, c, pen);

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
        rr.push_back(r);
        
        // gles render the frect
        ORBIS_RenderFillRects(USE_COLOR, grey, rr, 1);
        /* draw filling color bar, by percentage */
        GLES2_DrawFillingRect(rr, sele, dfp_hdd);
        rr.clear();

        /* the vertical grey frect (4px line) */
             r = (vec4) { 500, 0,   4, resolution.y };
        vec2 p = r.xy,  s = r.xy + r.zw;
        /* convert to normalized coordinates */
        r.xy = px_pos_to_normalized(&p),
        r.zw = px_pos_to_normalized(&s);
        rr.push_back(r);
        // gles render the frect
        ORBIS_RenderFillRects(USE_COLOR, grey, rr, 1);
        rr.clear();
    }

    //ORBIS_RenderArrowAtCoords(USE_COLOR, 500, 700);
    /* FMEM: draw filling color bar, by percentage */
    vec4 r = (vec4) { -.975, -.950,   -.505, -.955 };
    rr.push_back(r);
    //ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
    ORBIS_RenderDrawBox(USE_COLOR, grey, r);
    GLES2_DrawFillingRect(rr, white, dfp_fmem);
    rr.clear();
   /* if(menu_pos.z == ON_MAIN_SCREEN || menu_pos.z == ON_LEFT_PANEL){
    r = (vec4) { -.985, -.100,   -.505, -.105 };
    rr.push_back(r);
     ORBIS_RenderFillRects(USE_COLOR, grey, rr, 1);
    GLES2_DrawFillingRect(rr, white, updates_prog.load());
    rr.clear();
    }
*/
    // texts out of VBO
    //if(t_vbo) ftgl_render_vbo(t_vbo, NULL);
    t_vbo.render_vbo(NULL);
}

void GLES2_refresh_sysinfo(void)
{   // will trigger t_vbo refresh (all happen in renderloop)
    refresh_clk = true;
}

extern std::string selected_text;

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

    // ORBIS_DrawControls(800, 25);

    if( ! c_vbo ) // refresh
    {
        std::string tmp;
        vec2 pen     = (vec2){ 650, 950 },
             origin  = resolution,
             border  = (vec2)(50.);
             c_vbo   = VertexBuffer( "vertex:3f,tex_coord:2f,color:4f" );
        // add border, in px
             origin -= border;

        // text from pointed layout_item token label
       // add_text( c_vbo, titl_font, selected_text.c_str(), &col, &pen);
        c_vbo.add_text( titl_font, selected_text, col, pen);

        /* lower-right item/page infos */
        if( menu_pos.z == ON_MAIN_SCREEN
        || (menu_pos.z == ON_LEFT_PANEL && active_p->item_c > active_p->f_size) )
        {
            tmp = fmt::format("{}/{}", active_p->curr_item +1, active_p->item_c);

            // can be more than one page?
            if(active_p->item_c > active_p->f_size)
            {
                // compute page_infos
                ivec2 pi  = (ivec2) { active_p->curr_item, active_p->item_c };
                      pi /= (ivec2) ( active_p->fieldsize.x * active_p->fieldsize.y );
                      pi += 1;

                tmp = fmt::format(", {} {}/{}", getLangSTR(PAGE2), std::to_string(pi.x), std::to_string(pi.y));
            }
            // we need to know Text_Length_in_px in advance, so we call this:
            texture_font_load_glyphs( sub_font, tmp.c_str() ); 
            // we know 'tl' now, right align
            pen.x = origin.x - tl,
            pen.y = border.y;
            // fill the vbo
            //add_text( c_vbo, sub_font, tmp.c_str(), &col, &pen);
            c_vbo.add_text( sub_font, tmp, col, pen);
        }
        // we could have added new glyps
        refresh_atlas();
    }
    else // texts out of VB // ftgl_render_vbo(c_vbo, NULL);
        c_vbo.render_vbo(NULL);
}

void GLES2_refresh_common(void)
{
    //if(c_vbo) vertex_buffer_delete(c_vbo), c_vbo = NULL;
    c_vbo.clear();
}

