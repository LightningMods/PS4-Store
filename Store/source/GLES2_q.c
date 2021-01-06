/*
    GLES2 scene

    my implementation of an interactive menu, from scratch.
    (shaders
     beware)

    2020, masterzorag & LM

    deals with dynamically queued items to move, following related posix thread
    note four threads are max used, we access each thread arguments data,
    no use of mutexes yet
*/

#include <stdio.h>
#include <string.h>

#if defined(__ORBIS__)
    #include <debugnet.h>
#endif

#include "defines.h"

#include <pthread.h>
#include <stdatomic.h>
//atomic_ulong g_progress = 0;

#include <freetype-gl.h>
// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;

extern texture_font_t  *main_font, // small
                       *sub_font,  // left panel
                       *titl_font;
extern vec2 resolution;

extern layout_t *left_panel,
                *icon_panel,
                *queue_panel;
extern int http_ret;

#define ENTRIES  (4)
dl_arg_t      *pt_info = NULL;
//item_idx_t *unused_yet = NULL;


void GLES2_render_queue(layout_t *layout, int used)
{
    if( ! layout
    ||  ! layout->is_shown ) return;

    GLES2_UpdateVboForLayout(layout);

    // update queue panel fieldsize
    int req_status;
    switch(left_panel->item_sel.y)
    {   
        case 3: /* Ready to install */ 
                     req_status = COMPLETED;
            layout->fieldsize.y = thread_count_by_status( req_status ); break;
        case 4: /* Queue */
                     req_status = RUNNING;
            layout->fieldsize.y = thread_count_by_status( req_status ); break;
    }
    // the field size
    int field_s     = layout->fieldsize.x * layout->fieldsize.y,
        f_selection = layout->item_sel.y  * layout->fieldsize.x + layout->item_sel.x;

    if(field_s > 0) // just if we actually have items!
    {
        if(layout->refresh)
        {
            clean_textures(layout), layout->refresh = 0;
        }
    }
    else return;

    // if we discarded layout textures recreate them!
    if(!layout->texture)
        layout->texture = calloc(ENTRIES, sizeof(GLuint)); // ENTRIES

    char tmp[256];

    vec2 p, s,          // position, size
         pen;           // text position, in px
    vec4 r,             // normalized rectangle
         selection_box,
    /* normalize rgba colors */
    sele  = (vec4) {  29, 134, 240, 256 } / 256, // blue
    grey  = (vec4) {  66,  66,  66, 256 } / 256, // fg
    white = (vec4) ( 1. ),
    col   = (vec4) { .8164, .8164, .8125, 1. };

    if(field_s > ENTRIES) field_s = ENTRIES;

    int   i, j = 0;
    // iterate items we are about to draw
    for(i = 0; i < field_s; i++)
    {
        s = (vec2) { layout->bound_box.z / layout->fieldsize.x,
                     layout->bound_box.w / ENTRIES };

        p = (vec2) { (float)(layout->bound_box.x
                     + (j % (int)layout->fieldsize.x)
                     * (s.x +3 /* px border */))
                     ,
                     (float)(layout->bound_box.y
                     - (j / layout->fieldsize.x)
                     * (s.y +3 /* px border */)) };
        s   += p; // .xy point addition!
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);

        // this panel is_active: check for its vbo
        if(pt_info // elaborate on thread details
        && i < ENTRIES)
        {
            // which thread is the 'i'th
            int idx = thread_find_by_status(i, req_status);

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
                if(i == f_selection)
                {   // save frect for optional glowing selection box or...
                    selection_box = r; }

                // shrink RtoL, force a square related to row heigth, in px
                vec4 t = r;
                // size in px, force 1:1 aspect on y axis
                     s = (vec2) (layout->bound_box.w / ENTRIES);
                // update position: displace to right by size in px
                  p.x += s.x;
                // convert to normalized coordinates
                  t.zw = px_pos_to_normalized(&s);
                // math addition on negative x axis translate to subtraction
                  t.z  = t.x - ( -1. - t.z );
                  t.w  = r.w;
                // load texture
                if(layout->texture[ idx ] == 0)
                {
                    sprintf(&tmp[0], "%s", ta->token_d[ PICPATH ].off);
                    /* on pc we use different path
                    char *f = strstr(&tmp[0], "storedata"); */
                    layout->texture[ idx ] = load_png_asset_into_texture(&tmp[0]);
                }
                // gles render square texture
                if(layout->texture[ idx ]) on_GLES2_Render_icon(layout->texture[ idx ], 2, &t);
                // save pen origin for text, in pixel coordinates!
                pen = p;

                /* draw filled color bar */
                p   += 10.;
                s    = (vec2) { layout->bound_box.z - 20. - s.x,   6. };
                s   += p; // .xy point addition!
                t.xy = px_pos_to_normalized(&p),
                t.zw = px_pos_to_normalized(&s);
                // gles render the frect
                ORBIS_RenderFillRects(USE_COLOR, &grey, &t, 1);
                // shrink frect RtoL by custom_percentage
                t.z = t.x + (t.z - t.x) * (ta->progress / 100.f);
        //      fprintf(INFO, "%.2f %.2f, %.2f, %.2f %f\n", r.x, r.y, r.z, r.w, dfp);
                ORBIS_RenderFillRects(USE_COLOR, &sele, &t, 1);

                /* item text from pthread_infos */

                // we cleaned vbo ?
                if( ! layout->vbo ) layout->vbo = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

                if(layout->vbo
                && layout->vbo_s < CLOSED)
                {
                    // save origin position, in px
                    const vec2  origin = pen;
                    const char *format = "%d %p, %p, %.2f%% %d";
                    // if below, override
#if 0  
                    if(ta->url) format = "%d %s, %s, %.2f%% %d";
                    sprintf(&tmp[0], format, ta->idx, // thread index
                                             ta->token_d[ ID   ].off,
                                             ta->token_d[ SIZE ].off,
                                             ta->progress, ta->status);
#else
                    if(ta->url) format = "%s, %s, %.2f%%";
                    sprintf(&tmp[0], format, ta->token_d[ ID   ].off,
                                             ta->token_d[ SIZE ].off,
                                             ta->progress);
#endif
                    pen   += (vec2) { 10, 32 };
                    vec4 c = col * .75f;
                    // fill the vbo
                    add_text( layout->vbo, main_font, &tmp[0], &c, &pen);

                    // item name, upperleft title
                    pen  = origin;
                    sprintf(&tmp[0], "%s", ta->token_d[ NAME ].off);
                    pen +=  (vec2) { 10, (layout->bound_box.w / ENTRIES) - 36 };
                    // fill the vbo
                    add_text( layout->vbo, sub_font, &tmp[0], &col, &pen);

                    // right aligned thread status on the right side
                    pen    = origin;
                    pen.y += 32;
                    switch(ta->status)
                    {
                        case CANCELED:  sprintf(&tmp[0], "Download Canceled");
                        break;
                        case RUNNING:   sprintf(&tmp[0], "Downloading");
                        break;
                        case READY:
                        case COMPLETED: 
                        sprintf(&tmp[0], " Download Completed");
                        break;
                        default:  
                        sprintf(&tmp[0], " Download error: %i", ta->status);
                        break;
                    }

                    if (http_ret != 200 && http_ret != 0)
                    {
                        sprintf(&tmp[0], ", Download Failed with %i", http_ret);
                    }

                    // we need to know Text_Length_in_px in advance, so we call this:
                    texture_font_load_glyphs( main_font, &tmp[0] );
                    // we know 'tl' now, right align
                    pen.x = layout->bound_box.x + layout->bound_box.z - tl - 10;
                    // fill the vbo
                    add_text( layout->vbo, main_font, &tmp[0], &c, &pen);
                }
                // advance to draw next item
                j++;
            }
        }
    } // EOF iterate available items

    /* common texts for items */

    if(layout->vbo
    && layout->vbo_s < CLOSED)
    {   /* title
        pen = (vec2) { 670, resolution.y - 100 };
        // fill the vbo
        add_text( layout->vbo, titl_font, &tmp[0], &col, &pen); */
    }

    /* we eventually added glyphs! */
    if( ! layout->vbo_s ) { refresh_atlas(); /*layout->vbo_s = IN_ATLAS_APPEND;*/ }

    if(layout->vbo_s < CLOSED) layout->vbo_s = CLOSED;

    /* elaborate more on the selected item: */
    r = selection_box;

//    ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);

    // the glowing white box selector
    ORBIS_RenderDrawBox(USE_UTIME, &white, &selection_box);
    // paint text out ouf vbo
    GLES2_render_submenu_text_v2(layout->vbo, NULL);
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

    printf("i->url %s\n",   i->url);
    printf("i->dst %s\n",   i->dst);
    printf("i->req %x\n",   i->req);
    printf("i->cLn %lub\n", i->contentLength);
    printf("i->token_d[ ID   ].off: %s\n", i->token_d[ ID   ].off);
    printf("i->token_d[ SIZE ].off: %s\n", i->token_d[ SIZE ].off);

    unsigned char buf[8000] = { 0 };

    unlink(i->dst);

    int fd = sceKernelOpen(i->dst, O_WRONLY | O_CREAT, 0777);
    // fchmod(fd, 777);
    if (fd < 0) { i->status = CANCELED;  return fd; }

    while (1)
    {
        int read = sceHttpReadData(i->req, buf, sizeof(buf));
        if (read < 0) { i->status = read;  return read; }
        if (read == 0) { i->status = CANCELED; break; }

        ret = sceKernelWrite(fd, buf, read);
        if (ret < 0 || ret != read)
        {
            if (ret < 0) { i->status = CANCELED; return ret; };
            i->status = CANCELED;
            return -1;
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
            fprintf(DEBUG, "thread[%d] reading data, %lub / %lub (%.2f%%)\n", i->idx, total_read, i->contentLength, i->progress);
    }
    ret = sceKernelClose(fd);

    // don't wait before returning
    //sleep(2);

    

    // clean thread / reset
    if(i->req) sceHttpDeleteRequest(i->req);


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
                //fprintf(DEBUG, "thread_args[%d] %p, %d\n", i, ta->token_d, ta->g_idx);
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
//              fprintf(DEBUG, "%s thread_args[%d] %p, %d\n", __FUNCTION__, i, ta->token_d, ta->status);
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
//      fprintf(DEBUG, "thread_args[%d] %p, %d %d\n", i, ta->token_d, ta->g_idx, count);

        if(ta
        && ta->token_d)
        {
            if(ta->status == req_status) count++;
        }
    }
//  fprintf(DEBUG, "%s returns: %d\n", __FUNCTION__, count);
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

    fprintf(DEBUG, "%s returns: %d\n", __FUNCTION__, first_free);
    return first_free;
}

int dl_from_url_v2(const char *url_, const char *dst_, item_idx_t *t)
{
    queue_panel_init();
    // thread Args
    int idx = thread_dispatch_index();
    
    fprintf(DEBUG, "%s idx: %d\n", __FUNCTION__, idx);

    if(idx < 0)
    {
        ani_notify("Full Queue, please wait!");
//      fprintf(DEBUG, "no free slot to queue, please wait!\n");
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

        fprintf(DEBUG, "%s: pthread_create[%d] for %s, ret:%d\n", __FUNCTION__, idx, url_, ret);

        ani_notify("Download thread start");
    }
    else
        msgok(NORMAL, "Download Failed with code\n StatusCode: %i\n\n URL: %s\n\n DEST: %s", ret, url_, dst_);

    fprintf(DEBUG, "icon_panel->item_d[%d]\n\n", ta->g_idx);



    return ret;
}
