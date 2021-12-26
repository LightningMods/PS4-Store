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

#if defined(__ORBIS__)
    #include <debugnet.h>
#endif

#include "defines.h"

#include "GLES2_common.h"

#include <pthread.h>
#include <stdatomic.h>
//atomic_ulong g_progress = 0;

extern int http_ret;

#define ENTRIES  (4)
dl_arg_t *pt_info = NULL;

/*
   this function draws the variable length list, used in:
   - Ready_to_Install
   - Queue
*/
void GLES2_render_queue(layout_t *l, int used)
{
    if( ! l
    ||  ! l->is_shown ) return;

    // update queue panel fieldsize
    int req_status = 0;
    switch(used)
    {   /* Ready_to_install */
        case 9:  req_status = COMPLETED; break;
        /* Queue */
        case 0:  req_status = RUNNING;   break;
        default:                         break;
    }
    l->item_c = thread_count_by_status( req_status );

    if( l->item_c < 1) // no entries to show...
    {
        active_p = left_panel2; // back to the left one
        return;
    }
    // update all, list could have been reduced!
    // unconditionally trigger VBO refresh
    layout_update_sele(l, 0);

    GLES2_UpdateVboForLayout(l);

    char tmp[256];

    vec2 p, s,          // position, size
         pen;           // text position, in px
    vec4 r,             // normalized rectangle
         selection_box;
    int  i, j = 0;
    // iterate items we are about to draw
    for(i = 0; i < l->f_size; i++)
    {
        s = (vec2) { l->bound_box.z / l->fieldsize.x,
                     l->bound_box.w / ENTRIES };

        p = (vec2) { (float)(l->bound_box.x
                     + (j % (int)l->fieldsize.x)
                     * (s.x +3 /* px border */))
                     ,
                     (float)(l->bound_box.y
                      - (j / l->fieldsize.x)
                      * (s.y +3 /* px border */)) };
        p.y -= s.y;
        s   += p; // .xy point addition!
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);

        // this panel is_active: check for its vbo
        if(pt_info // elaborate on thread details
        && i < ENTRIES)
        {
            // which thread is downloading the 'i'th
            int idx = thread_find_by_status(i, req_status);
            // no one
            if(idx < 0) continue;

            // address the thread info
            const dl_arg_t *ta = &pt_info[ idx ];
            // something queued to list ?
            if(( ta->token_d ) &&
            // Queued
             ( used == 0
            && ta->status > CANCELED
            && ta->status < COMPLETED )
            || // Ready to install
             ( used == 9
            && ta->status == COMPLETED ))
            {
                // draw bounding box for each button selector
                ORBIS_RenderDrawBox(USE_COLOR, &grey, &r);

                // is item the selected one ?
                if(i == l->f_sele)
                {   // save frect for optional glowing selection box or...
                    selection_box = r; }

                // shrink RtoL, force a square related to row heigth, in px
                vec4 t = r;
                // size in px, force 1:1 aspect on y axis
                     s = (vec2) (l->bound_box.w / ENTRIES);
                // update position: displace to right by size in px
                  p.x += s.x;
                // convert to normalized coordinates
                  t.zw = px_pos_to_normalized(&s);
                // math addition on negative x axis translate to subtraction
                  t.z  = t.x - ( -1. - t.z );
                  t.w  = r.w;
                // flip resulting f_rect Vertically
                // t.yw = t.wy;
                // use load_n_draw_texture
                check_n_load_texture( ta->g_idx );
                // gles render square texture
                on_GLES2_Render_icon(USE_COLOR, icon_panel->item_d[ ta->g_idx ].texture, 2, &t, NULL);
                // save pen origin for text, in pixel coordinates!
                pen = p;

                /* draw filling color bar */
                p   += 10.;
                s    = (vec2) { l->bound_box.z - 20. - s.x,   6. };
                s   += p; // .xy point addition!
                t.xy = px_pos_to_normalized(&p),
                t.zw = px_pos_to_normalized(&s);
                // gles render the frect
                ORBIS_RenderFillRects(USE_COLOR, &grey, &t, 1);
                // gles render filling progress bar
                GLES2_DrawFillingRect(&t, &sele, &ta->progress);

                /* item text from pthread_infos */

                // we cleaned vbo ?
                if( ! l->vbo ) l->vbo = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

                if(l->vbo
                && l->vbo_s < CLOSED)
                {
                    // save origin position, in px
                    const vec2  origin = pen;
                    const char *format = "%d %p, %p, %.2f%% %d";
                    // if below, override
#if 0  
                    if(ta->url) format = "%d %s, %s, %.2f%% %d";
                    snprintf(&tmp[0], 63, format, ta->idx, // thread index
                                             ta->token_d[ ID   ].off,
                                             ta->token_d[ SIZE ].off,
                                             ta->progress, ta->status);
#else
                    if(ta->url) format = "%s, %s, %.2f%%";
                    snprintf(&tmp[0], 255, format, ta->token_d[ ID   ].off,
                                             ta->token_d[ SIZE ].off,
                                             ta->progress);
#endif
                    pen   += (vec2) { 10, 32 };
                    vec4 c = col * .75f;
                    // fill the vbo
                    add_text( l->vbo, main_font, &tmp[0], &c, &pen);

                    // item name, upperleft title
                    pen  = origin;
                    snprintf(&tmp[0], 255, "%s", ta->token_d[ NAME ].off);
                    pen +=  (vec2) { 10, (l->bound_box.w / ENTRIES) - 36 };
                    // fill the vbo
                    add_text( l->vbo, sub_font, &tmp[0], &col, &pen);

                    // right aligned thread status on the right side
                    pen    = origin;
                    pen.y += 32;
                    switch(ta->status)
                    {
                        case CANCELED:  snprintf(&tmp[0], 255, "Download Canceled");
                        break;
                        case RUNNING:   snprintf(&tmp[0], 255, "Downloading");
                        break;
                        case READY:
                        case COMPLETED: 
                        snprintf(&tmp[0], 255, " Download Completed");
                        break;
                        default:  
                        snprintf(&tmp[0], 255, " Download error: %i", ta->status);
                        break;
                    }

                    if (http_ret != 200 && http_ret != 0)
                    {
                        log_debug( "? Download Failed with %i", http_ret);
                    }

                    // we need to know Text_Length_in_px in advance, so we call this:
                    texture_font_load_glyphs( main_font, &tmp[0] );
                    // we know 'tl' now, right align
                    pen.x = l->bound_box.x + l->bound_box.z - tl - 10;
                    // fill the vbo
                    add_text( l->vbo, main_font, &tmp[0], &c, &pen);
                }
                // advance to draw next item
                j++;
            }
        }
    } // EOF iterate available items

    /* common texts for items */

    if(l->vbo
    && l->vbo_s < CLOSED)
    {   /* title
        pen = (vec2) { 670, resolution.y - 100 };
        // fill the vbo
        add_text( l->vbo, titl_font, &tmp[0], &col, &pen); */
    }

    /* we eventually added glyphs! */
    if( ! l->vbo_s ) { refresh_atlas(); /*l->vbo_s = IN_ATLAS_APPEND;*/ }

    if(l->vbo_s < CLOSED) l->vbo_s = CLOSED;

    /* elaborate more on the selected item: */
    r = selection_box;

//    ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);

    // the glowing white box selector
    ORBIS_RenderDrawBox(USE_UTIME, &sele, &selection_box);
    // paint text out ouf vbo
    ftgl_render_vbo(l->vbo, NULL);
}

// 
void queue_panel_init(void)
{
    if(!queue_panel)
    {   // dynalloc and init this panel
        queue_panel = GLES2_layout_init( ENTRIES );

        if(queue_panel)
        {
            // should match the icon panel size
            queue_panel->bound_box = icon_panel->bound_box;
            queue_panel->fieldsize = (ivec2) { 1, ENTRIES };
            queue_panel->item_c    = ENTRIES;
        }
    }
    // dynalloc for threads arguments
    if(!pt_info)
    {
        pt_info = calloc(ENTRIES, sizeof(dl_arg_t));

        for(int i=0; i < ENTRIES; i++) pt_info[i].g_idx = -1;
    }
}

void *start_routine2(void *argument)
{
    dl_arg_t *i = (dl_arg_t*) argument;
    i->status   = RUNNING;
    i->progress = 0.f;

    int ret = CANCELED;
    uint64_t total_read = 0;

    log_info("i->url %s",   i->url);
    log_info("i->dst %s",   i->dst);
    log_info("i->req %x",   i->req);
    log_info("i->cLn %lub", i->contentLength);
    log_info("i->token_d[ ID   ].off: %s", i->token_d[ ID   ].off);
    log_info("i->token_d[ SIZE ].off: %s", i->token_d[ SIZE ].off);

    unsigned char buf[8000] = { 0 };

    unlink(i->dst);

    int fd = sceKernelOpen(i->dst, O_WRONLY | O_CREAT, 0777);
    // fchmod(fd, 777);
    if (fd < 0) { i->status = CANCELED;  return fd; }

    while (1)
    {
        int read = sceHttpReadData(i->req, buf, sizeof(buf));
        if (read <  0) { i->status = read; goto cleanup; }
        if (read == 0) { i->status = CANCELED;   break; }

        ret = sceKernelWrite(fd, buf, read);
        if (ret < 0 || ret != read)
        {
            i->status = CANCELED;
            goto cleanup;
        }
        total_read += read;

        if(total_read >= i->contentLength)
            total_read = i->contentLength;

        i->progress = (double)(((float)total_read / i->contentLength) * 100.f);

        if (i->progress >= 100.)
        {
            i->status = COMPLETED;
            break;
        }

        if(total_read %(4096*128) == 0)
            log_debug( "%s, thread[%d] reading data, %lub / %lub (%.2f%%)", __FUNCTION__, i->idx, total_read, i->contentLength, i->progress);
    }

    // don't wait before returning
    // clean thread / reset
cleanup:
    if (i->req > 0) {
        ret = sceHttpDeleteRequest(i->req);
        if (ret < 0) {
            log_error("sceHttpDeleteRequest(%i) error: 0x%08X\n", i->req, ret);
        }
    }
    if (i->connid > 0) {
        ret = sceHttpDeleteConnection(i->connid);
        if (ret < 0) {
            log_error("sceHttpDeleteConnection(%i) error: 0x%08X\n", i->connid, ret);
        }
    }
    if (i->tmpid > 0) {
        ret = sceHttpDeleteTemplate(i->tmpid);
        if (ret < 0) {
            log_error("sceHttpDeleteTemplate(%i) error: 0x%08X\n", i->tmpid, ret);
        }
    }
    if (fd > 0) {
        ret = sceKernelClose(fd);
        if (ret < 0) {
            log_error("sceKernelClose(%i) error: 0x%08X\n", fd, ret);
        }
    }

    // trigger refresh of Queue active count
    left_panel2->vbo_s = ASK_REFRESH;

    return ret;
}

int thread_find_by_item(int req_idx)
{
    if( ! pt_info ) return -1;

    dl_arg_t *ta = NULL; // thread_args

    for(int i=0; i < ENTRIES; i++)
    {
        ta = &pt_info[i];

        if(ta
        && ta->token_d)
        {
            if(ta->g_idx == req_idx)
            {
                //log_debug( "thread_args[%d] %p, %d", i, ta->token_d, ta->g_idx);
                return i;
            }
        }
    }
    return -1;
}

int thread_find_by_status(int req_idx, int req_status)
{
    if( ! pt_info ) return -1;

    int    count = 0;
    dl_arg_t *ta = NULL; // thread_args

    for(int i=0; i < ENTRIES; i++)
    {
        ta = &pt_info[i];

        if(ta
        && ta->token_d)
        {
            if(ta->status == req_status)
            {
                count++;
//              log_debug( "%s thread_args[%d] %p, %d", __FUNCTION__, i, ta->token_d, ta->status);
                if(req_idx == count -1)
                    return i;
            }
        }
    }
    return -1;
}

int thread_count_by_status(int req_status)
{
    if( ! pt_info ) return 0;

    int    count = 0;
    dl_arg_t *ta = NULL; // thread_args

    for(int i=0; i < ENTRIES; i++)
    {
        ta = &pt_info[i];
//      log_debug( "thread_args[%d] %p, %d %d", i, ta->token_d, ta->g_idx, count);

        if(ta
        && ta->token_d)
        {
            if(ta->status == req_status) count++;
        }
    }
//  log_debug( "%s returns: %d", __FUNCTION__, count);
    return count;
}

int thread_dispatch_index(void)
{
    int i, first_free = -1;
    dl_arg_t *ta  = NULL; // thread_args

    for(i=0; i < ENTRIES; i++)
    {
        ta = &pt_info[i];

        if(ta->status == READY
        || ta->status == CANCELED )
            { first_free = i; break; }
    }
    // if no free slot found
    if(i == ENTRIES) first_free = -1;

    log_debug( "%s returns: %d", __FUNCTION__, first_free);
    return first_free;
}

int dl_from_url_v2(const char *url_, const char *dst_, item_idx_t *t)
{
    queue_panel_init();
    // thread Args
    int idx = thread_dispatch_index();
    
    log_debug( "%s idx: %d", __FUNCTION__, idx);

    if(idx < 0)
    {
        ani_notify("Full Queue, please wait!");
//      log_debug( "no free slot to queue, please wait!");
        return 0;
    }
    // address pointer to thread args
    dl_arg_t *ta = &pt_info[ idx ];
    // init thread args:
    ta->url     = url_;
//  (!) note passed dst_ is destroyed by caller so it's lost in
//  memory, so we shall strdup() for it and then free() it, eh!
    if(!ta->dst)
        ta->dst = strdup(dst_);
    ta->req     = -1;
    ta->idx     = idx;
    ta->token_d = t;
    // which item is selected in icon_panel ?
    ta->g_idx   = get_item_index(icon_panel);

    int ret = ini_dl_req(ta);
    if (ret == 200)
    {
        pthread_t thread = 0;

        ret = pthread_create(&thread, NULL, start_routine2, ta);

        log_debug( "%s: pthread_create[%d] for %s, ret:%d", __FUNCTION__, idx, url_, ret);

        ani_notify("Download thread start");
    }
    else
        msgok(NORMAL, "Download Failed with StatusCode: %i, URL: %s, DEST: %s", ret, url_, dst_);

    log_debug( "icon_panel->item_d[%d]", ta->g_idx);

    // 0 means OK!
    return ret;
}

