/*
    GLES2 scene

    my implementation of an interactive menu, from scratch.
    (shaders
     beware)

    2020, masterzorag & LM

    deals with dynamically queued items to move, following related posix thread
    note four threads are max used, we access each thread arguments data,
    no use of mutexes yet

    each time, 'l' stand for the current layout we are drawing, on each frame
*/

#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "GLES2_common.h"
#include "shaders.h"
#include "utils.h"
#include "net.h"
#include <thread>
#include <memory>
#include "fmt/printf.h"


//std::atomic_ulong g_progress = 0;

extern int http_ret;
extern std::vector<std::string> download_panel_text;
#define ENTRIES  (4)
std::vector<dl_arg_t*> pt_info;
/*
   this function draws the variable length list, used in:
   - Ready_to_Install
   - Queue
*/
void GLES2_render_queue(std::shared_ptr<layout_t>  &l, int used)
{
    if (!l
        || !l->is_shown) return;

    // update queue panel fieldsize
    int req_status = 0;
    switch (used)
    {   /* Ready_to_install */
    case 9:  req_status = COMPLETED; break;
    case 1337: req_status = INSTALLING_APP; break;
        /* Queue */
    case 0:  req_status = RUNNING;   break;
    default:                         break;
    }


    l->item_c = thread_count_by_status( req_status );
    if(used == 0)
       l->item_c += thread_count_by_status( PAUSED );

    if (l->item_c < 1) // no entries to show...
    {
        active_p = left_panel2; // back to the left one
        return;

    }

    // update all, list could have been reduced!
    // unconditionally trigger VBO refresh
    layout_update_sele(l, 0);

    GLES2_UpdateVboForLayout(l);
    std::string tmp;

    vec2 p, s,          // position, size
        pen;           // text position, in px
    vec4 r,             // normalized rectangle
        selection_box;
    std::vector<vec4> vsele;
    int  i, j = 0;
    // iterate items we are about to draw
    for (i = 0; i < l->f_size; i++)
    {
        s = (vec2){ l->bound_box.z / l->fieldsize.x,
                     l->bound_box.w / ENTRIES };

        p = (vec2){ (float)(l->bound_box.x
                     + (j % (int)l->fieldsize.x)
                     * (s.x + 3 /* px border */))
                     ,
                     (float)(l->bound_box.y
                      - (j / l->fieldsize.x)
                      * (s.y + 3 /* px border */)) };
        p.y -= s.y;
        s += p; // .xy point addition!
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);
        // this panel is_active: check for its vbo
        if (!pt_info.empty() // elaborate on thread details
            && i < ENTRIES)
        {
            // which thread is downloading the 'i'th
            int idx = thread_find_by_status(i, req_status);
            if(used == 0 && idx == -1)
               idx = thread_find_by_status(i, PAUSED);

            // no one
            if (idx < 0) continue;

            // address the thread info
            const dl_arg_t* ta = pt_info[idx];
            // something queued to list ?
            if ((!ta->token_d.empty() && used == 0 && ta->status > CANCELED && ta->status < COMPLETED) ||
                (used == 9  && ta->status == COMPLETED) || (used == 1337 && ta->status == INSTALLING_APP))
            {
                // draw bounding box for each button selector
               // vsele.push_back(r);
                ORBIS_RenderDrawBox(USE_COLOR, grey, r);
                //vsele.clear();

                // is item the selected one ?
                if (i == l->f_sele)
                {   // save frect for optional glowing selection box or...
                    selection_box = r;
                }

                // shrink RtoL, force a square related to row heigth, in px
                vec4 t = r;
                // size in px, force 1:1 aspect on y axis
                s = (vec2)(l->bound_box.w / ENTRIES);
                // update position: displace to right by size in px
                p.x += s.x;
                // convert to normalized coordinates
                t.zw = px_pos_to_normalized(&s);
                // math addition on negative x axis translate to subtraction
                t.z = t.x - (-1. - t.z);
                t.w = r.w;
                // flip resulting f_rect Vertically
                // t.yw = t.wy;
                // use load_n_draw_texture
                if (ta->g_idx != -1 && ta->g_idx != GL_NULL){
                    check_n_load_texture(ta->g_idx, TEXTURE_DOWNLOAD);
                    check_n_load_texture(ta->g_idx, TEXTURE_LOAD_PNG);
                }
                // gles render square texture
                on_GLES2_Render_icon(USE_COLOR, icon_panel->item_d[ta->g_idx].texture, 2, t);
                // save pen origin for text, in pixel coordinates!
                pen = p;

                /* draw filling color bar */
                p += 10.;
                s = (vec2){ l->bound_box.z - 20. - s.x,   6. };
                s += p; // .xy point addition!
                t.xy = px_pos_to_normalized(&p),
                    t.zw = px_pos_to_normalized(&s);
                // gles render the frect
                vsele.push_back(t);
                ORBIS_RenderFillRects(USE_COLOR, grey, vsele, 1);
                // gles render filling progress bar
                GLES2_DrawFillingRect(vsele, sele, ta->progress.load());
                vsele.clear();

                /* item text from pthread_infos */

                // we cleaned vbo ?
                if (!l->vbo) l->vbo = VertexBuffer("vertex:3f,tex_coord:2f,color:4f");

                if (l->vbo
                    && l->vbo_s < CLOSED)
                {
                    // save origin position, in px
                    const vec2  origin = pen;
                    // if below, override
#if 0  
                    if (ta->url) format = "%d %s, %s, %.2f%% %d";
                    snprintf(&tmp[0], 63, format, ta->idx, // thread index
                        ta->token_d[ID].off,
                        ta->token_d[SIZE].off,
                        ta->progress, ta->status);
#else
                    //if (!ta->url.empty()) format = "%s, %s, %.2f%%";
                    //snprintf(&tmp[0], 255, format, ta->token_d[ID].off.c_str(), ta->token_d[SIZE].off.c_str(), ta->progress);
                    tmp = fmt::format("{}, {}, {:.2f}%", ta->token_d[ID].off.c_str(), ta->token_d[SIZE].off.c_str(), ta->progress.load());
#endif
                    pen += (vec2) { 10, 32 };
                    vec4 c = col * .75f;
                    // fill the vbo
                    //add_text(l->vbo, main_font, tmp.c_str(), &c, &pen);
                    l->vbo.add_text(main_font, tmp, c, pen);

                    // item name, upperleft title
                    pen = origin;
                    tmp = ta->token_d[NAME].off;
                    pen += (vec2) { 10, (l->bound_box.w / ENTRIES) - 36 };
                    // fill the vbo
                    //add_text(l->vbo, sub_font, &tmp[0], &col, &pen);
                    l->vbo.add_text(sub_font, tmp, col, pen);

                    // right aligned thread status on the right side
                    pen = origin;
                    pen.y += 32;
                    switch(ta->status.load())
                    {
                        case CANCELED:  
                        icon_panel->item_d[ta->g_idx].interruptible = false;
                        tmp = ", " + getLangSTR(DL_CANCELLED);
                        break;
                        case RUNNING:   
                        icon_panel->item_d[ta->g_idx].interruptible = true;
                        tmp = fmt::format(", {} {} ({:.2f}%)", getLangSTR(DOWNLOADING), calculateSize(ta->contentLength.load()), ta->progress.load());
                        break;
                        case INSTALLING_APP:
                        tmp = fmt::format(", {} ({:.2f}%)", getLangSTR(INSTALLING), ta->progress.load());
                        break;
                        case PAUSED: 
                        tmp = fmt::format(", {} ({:.2f}%)", getLangSTR(PAUSE_2), ta->progress.load());
                        break;
                        case READY:
                        case COMPLETED: 
                        icon_panel->item_d[ta->g_idx].interruptible = false;
                        if(set.auto_install.load())
                           tmp = ", " + getLangSTR(DL_COMPLETE);
                        else
                           tmp = ", " + getLangSTR(INSTALL_COMPLETE);
                        break;
                        
                        default:   
                        //only failed installs will have an error that starts with 0x80000000
                        //so if it fails download or installing they will know which operationn is failed on
                       // log_info("status %d", ta->status.load());
                        tmp = fmt::sprintf(", %s %#08x", (ta->status & 0x80000000) ? getLangSTR(INSTALL_FAILED) :  getLangSTR(DL_ERROR2), ta->status.load());
                        break;
                    }

                    if (http_ret != 200 && http_ret != 0)
                         log_debug("download failed with %i",http_ret);

                    // we need to know Text_Length_in_px in advance, so we call this:
                    texture_font_load_glyphs(main_font, tmp.c_str());
                    // we know 'tl' now, right align
                    pen.x = l->bound_box.x + l->bound_box.z - tl - 10;
                    // fill the vbo
                   // add_text(l->vbo, main_font, tmp.c_str(), &c, &pen);
                    l->vbo.add_text(main_font, tmp, c, pen);
                }
                // advance to draw next item
                j++;
            }
        }
    } // EOF iterate available items

    /* common texts for items */

    if (l->vbo
        && l->vbo_s < CLOSED)
    {   /* title
        pen = (vec2) { 670, resolution.y - 100 };
        // fill the vbo
        add_text( l->vbo, titl_font, &tmp[0], &col, &pen); */
    }

    /* we eventually added glyphs! */
    if (!l->vbo_s) { refresh_atlas(); /*l->vbo_s = IN_ATLAS_APPEND;*/ }

    if (l->vbo_s < CLOSED) l->vbo_s = CLOSED;

    /* elaborate more on the selected item: */
    r = selection_box;

    //    ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);

        // the glowing white box selector
    ORBIS_RenderDrawBox(USE_UTIME, sele, selection_box);
    // paint text out ouf vbo
    //ftgl_render_vbo(l->vbo, NULL);
    l->vbo.render_vbo(NULL);
}

// 
void queue_panel_init(void)
{
    if (queue_panel->item_d.empty())
    {   // dynalloc and init this panel
        queue_panel = GLES2_layout_init(ENTRIES);

        if (queue_panel)
        {
            // should match the icon panel size
            queue_panel->bound_box = icon_panel->bound_box;
            queue_panel->fieldsize = (ivec2){ 1, ENTRIES };
            queue_panel->item_c = ENTRIES;
        }
    }
    // dynalloc for threads arguments
    // log_info("queue_panel_init %p", pt_info[0]);
    if (pt_info.empty())
    {
        log_info("allocating thread resources  ...");
        for (int i = 0; i < ENTRIES; i++) {
            pt_info.push_back(new dl_arg_t);
        }
        
    }
}

void *start_routine2( void *arg )
{
     // std::unique_ptr<dl_arg_t> i = std::static_pointer_cast<dl_arg_t>(*(static_cast<std::unique_ptr<void>*>(argument)));
    // std::unique_ptr<dl_arg_t> i = argument;
    dl_arg_t *i = (dl_arg_t*)arg;

    i->status = RUNNING;
    i->progress = 0.f;

    log_info("i->url %s", i->url.c_str());
    log_info("i->dst %s", i->dst.c_str());
    log_info("i->token_d[ ID   ].off: %s", i->token_d[ID].off.c_str());
    //log_info("i->token_d[ SIZE ].off: %s", i->token_d[SIZE].off);


    int ret = -1;
    if((ret = dl_from_url_v3_threaded(i->url, i->dst, i)) == 0)
    {
      i->last_off = 0;
      if (set.auto_install.load()) {
#ifdef __ORBIS__
        return (void*)pkginstall(i->dst.c_str(), i, set.auto_install.load());
#else
        log_error("[StoreCore][HTTP] Installed PKG %s", i->dst.c_str());
        unlink(i->dst.c_str());
        i->progress = 0.f;
        i->status = INSTALLING_APP;
        for(double z = 0.0; z < 100.0; z++){
            i->progress = z;
            usleep(10000);
        }
        sleep(1);
        icon_panel->mtx.lock();
        download_panel->mtx.lock();

        if(!games.empty()){ // reset gaame update status to latest status
       games[i->g_idx].interruptible = false;

       if(games[i->g_idx].update_status.load() == UPDATE_FOUND ||
        games[i->g_idx].update_status.load() == APP_NOT_INSTALLED){

         if(updates_counter.load() > 0){
            updates_counter--;
         }

        games[i->g_idx].update_status = NO_UPDATE;
       }
       download_panel->item_d[0].token_d[0].off =  download_panel_text[0] = getLangSTR(REINSTALL_APP);
    }
        download_panel->mtx.unlock();
        icon_panel->mtx.unlock();
        i->g_idx = -1;
        i->status = READY;
        
#endif
      }
      else
          i->status = COMPLETED;

    }
    else
        i->status = ret;

    // trigger refresh of Queue active count
    left_panel2->vbo_s = ASK_REFRESH;
    return (void*)ret;
}
int thread_find_by_item(int req_idx)
{
    //if (!pt_info[0]) return 0;
   if (pt_info.empty()) return -1;

    dl_arg_t* ta = NULL; // thread_args

    for (int i = 0; i < ENTRIES; i++)
    {
        if(i > pt_info.size()){
            log_error("thread_find_by_item: %d > %d", i, pt_info.size());
            break;
        }
        ta = pt_info[i];

        if (ta
            && !ta->token_d.empty())
        {
            if (ta->g_idx == req_idx)
            {
//                log_debug( "thread_args[%d] %p, %d", i, &ta->token_d, ta->g_idx);
                return i;
            }
        }
    }
    return -1;
}

int thread_find_by_status(int req_idx, int req_status)
{
    //if (!pt_info[0]) return 0;
   if (pt_info.empty()) return -1;
    int    count = 0;
    dl_arg_t* ta = NULL; // thread_args

    for (int i = 0; i < ENTRIES; i++)
    {
        if(i > pt_info.size()){
            log_error("thread_find_by_status: %d > %d", i, pt_info.size());
            break;
        }
        ta = pt_info[i];

        if (ta
            && !ta->token_d.empty())
        {
            if (ta->status == req_status)
            {
                count++;
//                log_debug( "%s thread_args[%d]  %d", __FUNCTION__, i, ta->status);
                if (req_idx == count - 1)
                    return i;
            }
        }
    }
    return -1;
}

int thread_count_by_status(int req_status)
{
   if (pt_info.empty()) return -1;

    int    count = 0;
    dl_arg_t* ta = NULL; // thread_args

    for (int i = 0; i < ENTRIES; i++)
    {
        if(i > pt_info.size()){
            log_error("thread_count_by_status: %d > %d", i, pt_info.size());
            break;
        }
        ta = pt_info[i];
        //      log_debug( "thread_args[%d] %p, %d %d", i, ta->token_d, ta->g_idx, count);

        if (ta
            && !ta->token_d.empty())
        {
            if (ta->status == req_status) count++;
        }
    }
    //  log_debug( "%s returns: %d", __FUNCTION__, count);
    return count;
}

int thread_dispatch_index(void)
{
    int i, first_free = -1;
    dl_arg_t* ta = NULL; // thread_args
    //if (!pt_info[0]) return 0;
   if (pt_info.empty()) return -1;

    for (i = 0; i < ENTRIES; i++)
    {
        if(i > pt_info.size()){
            log_error("thread_dispatch_index: %d > %d", i, pt_info.size());
            break;
        }
        ta = pt_info[i];

        if (ta->status == READY
            || ta->status == CANCELED)
        {
            first_free = i; 
            break;
        }

        log_debug("s: %d e: %i", ta->status.load(), first_free);
    }
    // if no free slot found
    if (i == ENTRIES) first_free = -1;

    log_debug("%s returns: %d", __FUNCTION__, first_free);
    return first_free;
}

int dl_from_url_v2(std::string url_, std::string dst_, std::vector<item_idx_t>& t)
{
    queue_panel_init();
    // thread Args
    int idx = thread_dispatch_index();

    log_debug("[StoreCore][NET_OPS] %s idx: %d", __FUNCTION__, idx);

    if (idx < 0)
    {
        msgok(WARNING,"Full Queue, please wait!");
        return 0;
    }
    // address pointer to thread args
    dl_arg_t* ta = pt_info[idx];
    // init thread args:
    ta->url = url_;
    ta->dst = dst_;
    ta->req = -1;
    ta->idx = idx;
    ta->is_threaded = false;
    ta->token_d = t;
    // which item is selected in icon_panel ?
    ta->g_idx = get_item_index(icon_panel);
    // get pre-download headers and content length
    int ret = ini_dl_req(ta);
    if (ret == 200)
    {
        pthread_t thread = 0;
        pthread_create(&thread, NULL, start_routine2, ta);
        log_debug("[StoreCore][NET_OPS] %s: pthread_create[%d] for %s, ret:%d", __FUNCTION__, idx, url_.c_str(), ret);

    }
    else{
        msgok(WARNING, fmt::format("[StoreCore][NET_OPS] {} {:#x}/{}, URL: {}, DEST: {}", getLangSTR(DL_FAILED_W), ret, url_, dst_));
        ta->status = CANCELED;
    }

    log_debug("icon_panel->item_d[%d]", ta->g_idx.load());

    // 0 means OK!
    return ret;
}
