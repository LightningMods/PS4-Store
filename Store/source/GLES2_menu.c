/*
    samples below init and draw 3 different panels
*/

#include "defines.h"
#include "GLES2_common.h"
#include "shaders.h"

// the Settings
extern StoreOptions set,
* get;

// store-clone / v2
extern layout_t *left_panel2; // for migration
extern vec2 resolution;

// common init a layout panel
layout_t *GLES2_layout_init(int req_item_count)
{
    layout_t *l = calloc(1, sizeof(layout_t));

    if( !l ) return NULL;

    l->page_sel.x = 0;
    // dynalloc for max requested number of items
    l->item_c = req_item_count;
    l->item_d = calloc(l->item_c, sizeof(item_t));

    if(l->item_d) return l;

    return NULL;
}

// cf game_options
void GLES2_render_list(int unused)
{
    layout_t *l = ls_p;

    if( !l )
    {   // dynalloc and init this panel
        l = GLES2_layout_init( 5 );
        l->bound_box =  (vec4){ 120, resolution.y -230,  650, 512 };
        l->fieldsize = (ivec2){ 2, 4 };

        layout_fill_item_from_list(l, &gm_p_text[0]);
//      log_info("%s: %d", __FUNCTION__, res);
        // save panel and set it as the active one
        ls_p = active_p = l;
        l->is_shown = 1;
    }

    if(l) {
        GLES2_render_layout_v2(l, 0);
    }
    // we can also free data once vbo is done
    //destroy_item_t(&l->item_d);
}

// left_panel clone/v2
void GLES2_render_paged_list(int unused)
{
    layout_t *l = left_panel2;

    if( !l )
    {   // count is known in advance!
        int count = 64;
        // dynalloc and init this panel
        l = GLES2_layout_init( count );
        l->bound_box =  (vec4){ 0, 900,   500, 700 };
        l->fieldsize = (ivec2){ 1, 10 };
        // create the first screen we will show
        l->item_c    = l->fieldsize.x * l->fieldsize.y;
        int res = layout_fill_item_from_list(l, &new_panel_text[0][0]);
//      log_info("%s: %d", __FUNCTION__, res);
        // reset count to current list
        l->item_c = res;
        // reduce field_size in case
        if( l->item_c < l->f_size ) l->f_size = l->item_c;
        // save panel and set active
        left_panel2 = active_p = l;
        l->is_shown = 1;
//      log_info("%s: %p %p %p", __FUNCTION__, left_panel2, active_p, l);
    }

    if(l) GLES2_render_layout_v2(l, 0);

//  log_info("%s returns", __FUNCTION__);
}

// icon_panel clone/v2
void GLES2_render_icon_list(int unused)
{
    layout_t *l = icon_panel;
    // guard to fallback
    if( !l )
    {   // count is known in advance!
        int count = 18;
        // dynalloc and init this panel
        l = GLES2_layout_init( count );
        l->bound_box =  (vec4){ 680, 900,   1096, 664 };
        l->fieldsize = (ivec2){ 5, 3 };
        //ls_p->item_d = // create the first screen we will show
        layout_update_fsize(l);
        // save panel
        //icon_panel2 = l;
        l->is_shown = 1;
    }

    if(l) {
        l->is_shown = 1;
        GLES2_render_layout_v2(l, 0);
    }
//  log_info("%s returns", __FUNCTION__);
}

extern int DL_CO;

void GLES2_render_download_panel(void)
{
    layout_t *l = download_panel;

    if(l)
    {
        l->is_shown = 1;

        /* 1. bg image: fullscreen frect (normalized coordinates) */
        vec4    r  = (vec4) { -1., -1.,   1., 1. };
        // address layout item to current icon_panel selection
        int    idx =  icon_panel->curr_item;
        item_t *li = &icon_panel->item_d[ idx ];
        // don't use cached data, texture from atlas + UVs
        on_GLES2_Render_icon(USE_COLOR, li->texture, 2, &r, NULL);
        // apply a shader on top of background image (shading, eh...)
        pixelshader_render(0, NULL, NULL);

        /* 2. the item icon: custom rectangle, in px */
        vec2 p = (vec2) { 100., 350. };
        vec2 s = (vec2) { 300., 300. };
        //if(offset.y != 0) p.y += offset.y;
        s   += p; // .xy point addition!
        r.xw = px_pos_to_normalized(&p);
        r.zy = px_pos_to_normalized(&s);
        // flip Vertically
        r.yw = r.wy;
        // don't use cached data, texture from atlas + UVs
        on_GLES2_Render_icon(USE_COLOR, li->texture, 2, &r, NULL);

        /* 3. a thead is operate on this one ? */
        dl_arg_t *ta = NULL; // thread arguments
        // there is some thread working on item ?
        int x = thread_find_by_item( idx );
        if( x > -1 )
        {   
            ta   = &pt_info[ x ];
            /* draw filling color bar */
            p    = (vec2) { l->bound_box.x, 650. /* + offset.y*/ - 20. };
            s    = (vec2) { l->bound_box.z,   5. };
            s   += p; // .xy point addition!
            r.xy = px_pos_to_normalized(&p);
            r.zw = px_pos_to_normalized(&s);
            // gles render the frect
            ORBIS_RenderFillRects(USE_COLOR, &grey, &r, 1);
            /* draw filling color bar, by percentage */
            GLES2_DrawFillingRect(&r, &sele, &ta->progress);
            // auto_trigger VBO refresh if operating on item
            if( l->vbo_s < ASK_REFRESH ) l->vbo_s = ASK_REFRESH;
        }

        /* destroy VBO */
        if( l->vbo_s == ASK_REFRESH ) { if(l->vbo) vertex_buffer_delete(l->vbo), l->vbo = NULL; }

        if( ! l->vbo ) // we cleaned vbo ?
        {
            l->vbo   = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
            l->vbo_s = EMPTY;
            // add more text
            if(l->vbo_s < CLOSED)
            {
                char tmp[256];
                snprintf(&tmp[0], 255, "%s", li->token_d[ NAME ].off);
                // title
                vec2 pen = (vec2) { 500, 650 + /*offset.y +*/ + 10. };
                // fill the vbo
                add_text( l->vbo, titl_font, &tmp[0], &col, &pen);

                if(ta
                && ta->url)
                {
                    switch(ta->status)
                    {
                        case CANCELED:  snprintf(&tmp[0], 255, ", %s", getLangSTR(DL2));
                        break;
                        case RUNNING:   snprintf(&tmp[0], 255, ", %s %s, %.2f%%", getLangSTR(DOWNLOADING),ta->token_d[ SIZE ].off, ta->progress);
                        break;
                        case READY:
                        case COMPLETED: snprintf(&tmp[0], 255, ", %s", getLangSTR(DL_COMPLETE));
                        break;
                        default:        snprintf(&tmp[0], 255, ", %s: %i", getLangSTR(DL_ERROR2), ta->status);
                        break;
                    }
                    // fill the vbo
                    add_text(l->vbo, main_font, &tmp[0], &col, &pen);
                }
                pen.y -=  32;

                /* tokens for info from sfo */

                item_idx_t *t = &li->token_d[0]; // address the selected item
                unsigned char sort_patterns[10] = { 0, 1, 4, 5, 9, 10, 11, 12, 13, 16 };
                for(int i = 0; i <= 10; i++)
                {
                    if(i == 1
                    || i == 3) continue; // skip those
                    // update position
                    pen.x  = 500,
                    pen.y -=  32;

                    // token name
                    snprintf(&tmp[0], 255, "%s: ", new_panel_text[3][i]);

                    // fill the vbo
                    add_text(l->vbo, main_font, &tmp[0], &col, &pen);
                    // get the indexed token value

                    //10 is the Number of Downloads Text
                    if(i != 10)
                        snprintf(&tmp[0], 255, "%s", t[  sort_patterns[i]  ].off);
                    else
                    {
                        //Check for the Legacy INI Setting, its a Bool
                        if(!get->Legacy)
                            snprintf(&tmp[0], 255, "%i", /* Check with Stores API for # of DLs*/ DL_CO);
                        else
                            snprintf(&tmp[0], 255, /* Legacy Enabled no API for it*/ "Legacy Enabled");
                    }
                    // fill the vbo
                    add_text(l->vbo, main_font, &tmp[0], &col, &pen);
                }
                // displace to, or by, in px
                pen.x  = 500,
                pen.y -= 200;
                for(int i = 0; i < 3; i++)
                {   
                    enum token_name req; 
                    switch(i) {
                        case 0: req = DESC  ; break;
                        case 1: req = DESC_1; break;
                        case 2: req = DESC_2; break;
                    }
                    snprintf(&tmp[0], 255, "%s", t[ req ].off);
                    add_text(l->vbo, main_font, &tmp[0], &col, &pen);
                    pen.x  = 500,
                    pen.y -=  32;
                }
            }
        }
        // note we didn't close the VBO, we will add more texts

        // draw the common layout
        GLES2_render_layout_v2(l, 0);
    }
}


// ------------------------------------------------------- typedef & struct ---
typedef struct {
    float x, y, z;    // position (3f)
    float r, g, b, a; // color    (4f)
} vertex_t;

// ------------------------------------------------------- global variables ---
static vertex_buffer_t* rects_buffer;
static GLuint shader,
mz_shader,
curr_Program;  // the current one
static mat4 model, view, projection;
//static GLuint g_TimeSlot = 0;
static vec2 resolution2;

// from main.c
extern double u_t;
extern vec2 p1_pos;  // unused yet

// ---------------------------------------------------------------- display ---
void pixelshader_render(GLuint program_i, vertex_buffer_t* vbo, vec2* req_size)
{
    curr_Program = mz_shader;

    if (program_i) curr_Program = shader; // just one for now

    glUseProgram(curr_Program);
    {
        // enable alpha channel
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // notify shader about screen size
        if (req_size)
            glUniform2f(glGetUniformLocation(curr_Program, "resolution"), req_size->x, req_size->y);
        else
            glUniform2f(glGetUniformLocation(curr_Program, "resolution"), resolution2.x, resolution2.y);
        // mouse position and actual cumulative time
        glUniform2f(glGetUniformLocation(curr_Program, "mouse"), p1_pos.x, p1_pos.y);
        glUniform1f(glGetUniformLocation(curr_Program, "time"), u_t); // notify shader about elapsed time
        // ft-gl style: MVP
        glUniformMatrix4fv(glGetUniformLocation(curr_Program, "model"), 1, 0, model.data);
        glUniformMatrix4fv(glGetUniformLocation(curr_Program, "view"), 1, 0, view.data);
        glUniformMatrix4fv(glGetUniformLocation(curr_Program, "projection"), 1, 0, projection.data);
        // draw whole VBO items array: fullscreen rectangle

        if (vbo)
            vertex_buffer_render(vbo, GL_TRIANGLES);
        else
            vertex_buffer_render(rects_buffer, GL_TRIANGLES);

        glDisable(GL_BLEND);
    }
    glUseProgram(0);
}

// ---------------------------------------------------------------- reshape ---
static void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    mat4_set_orthographic(&projection, 0, width, 0, height, -1, 1);
}

// ------------------------------------------------------ from source shaders ---
/* ============================= Pixel shaders ============================= */
static GLuint CreateProgramFE(int program_num, char* path)
{
    GLuint programID;

    switch (program_num)
    {
    case 0:
        programID = BuildProgram(p_vs1, p_fs0, p_vs1_length, p_fs0_length);
        break;
    case 1:
        programID = BuildProgram(p_vs1, p_fs1, p_vs1_length, p_fs1_length);
        break;
    case 2:
        programID = BuildProgram(p_vs1, p_fs2, p_vs1_length, p_fs2_length);
        break;
    case 3:
        programID = BuildProgram(p_vs1, p_fs3, p_vs1_length, p_fs3_length);
        break;
    case 4:
        programID = BuildProgram(p_vs1, p_fs5, p_vs1_length, p_fs5_length);
        break;
    default:
        programID = BuildProgram(p_vs1, p_fs2, p_vs1_length, p_fs2_length);
        break;
    }


    if (!programID) { log_error("program creation failed!, switch: %i", program_num); sleep(2); }

    return programID;
}
/* =========================================================================== */
// get a vbo from rectangle, in px coordinates
vertex_buffer_t* vbo_from_rect(vec4* rect)
{
    vertex_buffer_t* vbo = vertex_buffer_new("vertex:3f,color:4f");
    vec4   color = { 1, 0, 0, 1 };
    float r = color.r, g = color.g, b = color.b, a = color.a;
    /* VBO is setup as: "vertex:3f, color:4f */
    vertex_t vtx[4] = { { rect->x, rect->y, 0,   r,g,b,a },
                        { rect->x, rect->w, 0,   r,g,b,a },
                        { rect->z, rect->w, 0,   r,g,b,a },
                        { rect->z, rect->y, 0,   r,g,b,a } };
    // two triangles: 2 * 3 vertex
    GLuint   idx[6] = { 0, 1, 2,    0, 2, 3 };
    vertex_buffer_push_back(vbo, vtx, 4, idx, 6);

    return vbo;
}

// ------------------------------------------------------------------- init ---
void pixelshader_init(int width, int height)
{
    resolution2 = (vec2){ width, height };
    rects_buffer = vertex_buffer_new("vertex:3f,color:4f");
    vec4   color = { 1, 0, 0, 1 };
    float r = color.r, g = color.g, b = color.b, a = color.a;

#define TEST_1  (0)

#if TEST_1
    #warning "splitted rects pixelshader"
        vec2 pen = { 100, 100 };
    for (int i = 0; i < 10; ++i)
    {
        int  x0 = (int)(pen.x + i * 2);
        int  y0 = (int)(pen.y + 20);
        int  x1 = (int)(x0 + 64);
        int  y1 = (int)(y0 - 64);
#else
    #warning "one fullscreen pixelshader"
        int  x0 = (int)(0);
    int  y0 = (int)(0);
    int  x1 = (int)(width);
    int  y1 = (int)(height);

#endif
    /* VBO is setup as: "vertex:3f, color:4f */
    vertex_t vtx[4] = { { x0,y0,0,  r,g,b,a },
                        { x0,y1,0,  r,g,b,a },
                        { x1,y1,0,  r,g,b,a },
                        { x1,y0,0,  r,g,b,a } };
    // two triangles: 2 * 3 vertex
    GLuint   idx[6] = { 0, 1, 2,    0, 2, 3 };
    vertex_buffer_push_back(rects_buffer, vtx, 4, idx, 6);

#if TEST_1
    pen += (vec2) { 72., -32. };
    color.g -= i * 0.1; // to show some difference: less green
    }
#endif
/* compile, link and use shader */
#define SNOW_DLC "/mnt/sandbox/NPXS39041_000/app0/assets/snow.frag"

mz_shader = CreateProgramFE(0, NULL);

if (if_exists(SNOW_DLC)) {
    log_info("Loading....");
    shader = CreateProgramFE(5, SNOW_DLC);
}
else
shader = CreateProgramFE(4, NULL); // test emb2
 // feedback
log_info("[%s] program_id=%d (0x%08x)", __FUNCTION__, mz_shader, mz_shader);
log_info("[%s] program_id=%d (0x%08x)", __FUNCTION__, shader, shader);

mat4_set_identity(&projection);
mat4_set_identity(&model);
mat4_set_identity(&view);

reshape(width, height);
}

void pixelshader_fini(void)
{
    vertex_buffer_delete(rects_buffer);    rects_buffer = NULL;

    if (shader)    glDeleteProgram(shader), shader = 0;
    if (mz_shader) glDeleteProgram(mz_shader), mz_shader = 0;
}