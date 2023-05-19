/*
    samples below init and draw 3 different panels
*/

#include "defines.h"
#include "GLES2_common.h"
#ifdef __ORBIS__
#include "shaders.h"
#else
#include "pc_shaders.h"
#endif
#include "utils.h"
#include "fmt/printf.h"
// the Settings
extern StoreOptions set,
* get;
extern bool unsafe_source;
// store-clone / v2
extern std::shared_ptr<layout_t>  left_panel2; // for migration
extern vec2 resolution;
extern std::vector<std::string> download_panel_text;

typedef struct {
    float x, y, z;    // position (3f)
    float r, g, b, a; // color    (4f)
} pixel_vertex_t;

std::shared_ptr<layout_t> GLES2_layout_init(int req_item_count) {
    // Use make_shared instead of new to create the layout object
    std::shared_ptr<layout_t> l = std::make_shared<layout_t>();

    // No need to check for nullptr, make_shared will throw std::bad_alloc if memory allocation fails
    l->page_sel.x = 0;
    l->item_c = req_item_count;

    // Resize the vector to store the requested number of items
    l->item_d.resize(l->item_c);

    // Check if the vector was successfully resized
    if (l->item_d.size() == static_cast<size_t>(req_item_count)) {
        return l;
    }

    // If resizing failed, return nullptr
    return nullptr;
}
std::atomic<double> updates_prog(-1.);

// left_panel clone/v2
void GLES2_render_paged_list(int unused)
{
    std::shared_ptr<layout_t>  &l = left_panel2;
    std::vector<vec4> rr;
    vec4 r = (vec4) { -.985, -.100,   -.505, -.105 };

    if(!l.get() || l->item_d.empty() )
    {   // count is known in advance!
        int count = 64;//200;
        // dynalloc and init this panel
        l = GLES2_layout_init( count );
        l->bound_box =  (vec4){ 0, 900,   500, 700 };
        l->fieldsize = (ivec2){ 1, 10 };
        // create the first screen we will show
        l->item_c    = l->fieldsize.x * l->fieldsize.y;
        int res = layout_fill_item_from_list(l, new_panel_text[0]);
//      log_info("%s: %d", __FUNCTION__, res);
        // reset count to current list
        l->item_c = res;
        // reduce field_size in case
        if( l->item_c < l->f_size ) l->f_size = l->item_c;
        // save panel and set active
        active_p = left_panel2;
        l->is_shown = 1;
       log_info("%s: %p %p %p", __FUNCTION__, left_panel2.get(), active_p.get(), l.get());
    }
    if(active_p == left_panel2 &&  l->page_sel.x == ON_MAIN_SCREEN){
    rr.push_back(r);
    ORBIS_RenderFillRects(USE_COLOR, grey, rr, 1);
    GLES2_DrawFillingRect(rr, white, updates_prog.load());
    rr.clear();
    }

   if(1)
      GLES2_render_layout_v2(l, 0);

//  log_info("%s returns", __FUNCTION__);
}

// icon_panel clone/v2
void GLES2_render_icon_list(int unused)
{
    std::shared_ptr<layout_t>  &l = icon_panel;
    // guard to fallback
    if(!l || l->item_d.empty() )
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
int old_status = 0;
void GLES2_render_download_panel(void)
{
    std::shared_ptr<layout_t> &l = download_panel;
    std::vector<vec4>rr;
    if(l &&  !l->item_d.empty())
    {
        l->is_shown = true;

        /* 1. bg image: fullscreen frect (normalized coordinates) */
        vec4    r  = (vec4) { -1., -1.,   1., 1. };
        // address layout item to current icon_panel selection
        int    idx =  icon_panel->curr_item;
        item_t li = icon_panel->item_d[ idx ];
        // don't use cached data, texture from atlas + UVs
        on_GLES2_Render_icon(USE_COLOR, li.texture, 2, r);
        // apply a shader on top of background image (shading, eh...)
        pixelshader_render();  
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
        on_GLES2_Render_icon(USE_COLOR, li.texture, 2, r);  
        /* 3. a thead is operate on this one ? */
        dl_arg_t *ta = nullptr; // thread arguments
        // there is some thread working on item ?
        int x = thread_find_by_item( idx );
        if( x > -1 )
        {   
            ta   = pt_info[ x ];
            /* draw filling color bar */
            p    = (vec2) { l->bound_box.x, 650. /* + offset.y*/ - 20. };
            s    = (vec2) { l->bound_box.z,   5. };
            s   += p; // .xy point addition!
            r.xy = px_pos_to_normalized(&p);
            r.zw = px_pos_to_normalized(&s);
            rr.push_back(r);
            // gles render the frect
            ORBIS_RenderFillRects(USE_COLOR, grey, rr, 1);
            /* draw filling color bar, by percentage */
            GLES2_DrawFillingRect(rr, sele, ta->progress);
            rr.clear();
            /* draw text */
            // auto_trigger VBO refresh if operating on item
            if((set.auto_install.load() && download_panel->item_c == 1) || 
            (!set.auto_install.load() && download_panel->item_c == 2)){
               download_panel->item_c++;
               if(!set.auto_install.load())
                   download_panel_text[2] = getLangSTR(CANCEL);
            }

            if(ta->status > READY && ta->status < COMPLETED){
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = (ta->status == PAUSED) ? getLangSTR(RESUME_2) : getLangSTR(PAUSE_2);
                li.interruptible = true;
            }
            else if (ta->status == COMPLETED){
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = getLangSTR(INSTALL2);
            }
            else{
                 li.interruptible = false;
            }

             // ONLY REDRAW IF STATUS CHANGED
             if(old_status != ta->status) {
                // SET OLD STATUS TO CURRENT STATUS
                log_info("status changed to %d from %d", ta->status.load(), old_status);
                // set old_status = ta->status; later in the func so we can still use the old value
                download_panel->vbo.clear();
                layout_update_sele(download_panel, 0);
            }

            if( l->vbo_s < ASK_REFRESH ) l->vbo_s = ASK_REFRESH;
        }
        else{
            if((set.auto_install.load() && download_panel->item_c == 2) || 
            (!set.auto_install.load() && download_panel->item_c == 3)){ //IF WE NEED LESS SETTINGS
                download_panel->item_c--;

                // BACK PRESSED SO STATUS IS CHANGED TO REDRAW
               old_status = -1;
              // if(games[ idx ].update_status == (update_ret)UPDATE_NOT_CHECKED)
                //  CheckUpdate(li.token_d[ID].off.c_str(), li);
               // RESET MENU SELECTIOM
               if(games[ idx ].update_status == UPDATE_FOUND)
                  download_panel_text[0] = getLangSTR(UPDATE_NOW);
               else if(games[ idx ].update_status == NO_UPDATE)
                  download_panel_text[0] = getLangSTR(REINSTALL_APP);
               else
                  download_panel_text[0] = set.auto_install.load() ? getLangSTR(DL_AND_IN) : getLangSTR(DL2);

               layout_fill_item_from_list(download_panel, download_panel_text);
               layout_update_sele(download_panel, -1);
            }
        }

        /* destroy VBO */
        if( l->vbo_s == ASK_REFRESH ) { l->vbo.clear(); }

        if( ! l->vbo ) // we cleaned vbo ?
        {
            l->vbo   = VertexBuffer( "vertex:3f,tex_coord:2f,color:4f" );
            l->vbo_s = EMPTY;
            // add more text
            if(l->vbo_s < CLOSED)
            {
                std::string tmp;
                tmp = li.token_d[ NAME ].off;
                // title
                vec2 pen = (vec2) { 500, 650 + /*offset.y +*/ + 10. };
                // fill the vbo
               // add_text( l->vbo, titl_font, tmp.c_str(), &col, &pen);
                l->vbo.add_text(titl_font, tmp, col, pen);


                if(ta
                && !ta->url.empty())
                {
                    switch(ta->status.load())
                    {
                        case CANCELED:  tmp = ", " + getLangSTR(DL_CANCELLED);
                        break;
                        case RUNNING:   
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
                    // set the old status to the new status
                    old_status = ta->status.load();
                    // fill the vbo
                    l->vbo.add_text(main_font, tmp, col, pen);
                }
                pen.y -=  32;

                /* tokens for nfo from sfo */
                unsigned char sort_patterns[10] = { 0, 1, 4, 5, 9, 10, 11, 12, 13, 16 };
                for(int i = 0; i <= 10; i++)
                {
                    if(i == 1 || i == 2) continue; // skip those
                    // update position
                    pen.x  = 500,
                    pen.y -=  32;

                    // token name
                    tmp = new_panel_text[3][i] + ": ";

                    // fill the vbo
                    //add_text(l->vbo, main_font, tmp.c_str(), &col, &pen);
                    l->vbo.add_text(main_font, tmp, col, pen);


                    // get the indexed token value
                    //10 is the Number of Downloads Text
                    if(i != 10)
                        tmp = li.token_d[  sort_patterns[i]  ].off;
                    else {
                        if(DL_CO == -999)
                          tmp = "Pending";
                        else
                          tmp = std::to_string(/* Check with Stores API for # of DLs*/ DL_CO);
              
                       // if(!ta)
                         //  CheckUpdate(li.token_d[ ID ].off.c_str(), li);
                    }
                    // fill the vbo
                    //add_text(l->vbo, main_font, tmp.c_str(), &col, &pen);
                    l->vbo.add_text(main_font, tmp, col, pen);
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
                    tmp = li.token_d[ req ].off;
                    //add_text(l->vbo, main_font, tmp.c_str(), &col, &pen);
                    l->vbo.add_text(main_font, tmp, col, pen);

                    pen.x  = 500,
                    pen.y -=  32;
                }
            }
        }
        // note we didn't close the VBO, we will add more texts

        // draw the common layout
        GLES2_render_layout_v2(l, 0);
    }
    layout_update_sele(l, 0);
}

/*-------------------- global variables ---*/
//vertex_buffer_t* rects_buffer = nullptr;
VertexBuffer rects_buffer;
GLuint shader_s = 0,
mz_shader = 0,
curr_Program = 0;  // the current one
static mat4 model, view, projection;
//static GLuint g_TimeSlot = 0;
vec2 resolution2;

// from main.c
extern double u_t;
extern vec2 p1_pos;  // unused yet

// ---------------------------------------------------------------- display ---
void pixelshader_render()
{
    curr_Program = mz_shader;
    glUseProgram(curr_Program);
    {
        // enable alpha channel
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUniform2f(glGetUniformLocation(curr_Program, "resolution"), resolution2.x, resolution2.y);
        // mouse position and actual cumulative time
        glUniform2f(glGetUniformLocation(curr_Program, "mouse"), p1_pos.x, p1_pos.y);
        glUniform1f(glGetUniformLocation(curr_Program, "time"), u_t); // notify shader about elapsed time
        // ft-gl style: MVP
        glUniformMatrix4fv(glGetUniformLocation(curr_Program, "model"), 1, 0, model.data);
        glUniformMatrix4fv(glGetUniformLocation(curr_Program, "view"), 1, 0, view.data);
        glUniformMatrix4fv(glGetUniformLocation(curr_Program, "projection"), 1, 0, projection.data);

        //vertex_buffer_render2(rects_buffer, GL_TRIANGLES);
        rects_buffer.render(GL_TRIANGLES);

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
// ------------------------------------------------------------------- init ---
void pixelshader_init(int width, int height)
{
    resolution2 = (vec2){ width, height };
    rects_buffer = VertexBuffer("vertex:3f,color:4f");
    vec4   color = { 1, 0, 0, 1 };
    float r = color.r, g = color.g, b = color.b, a = color.a;

        int  x0 = (int)(0);
    int  y0 = (int)(0);
    int  x1 = (int)(width);
    int  y1 = (int)(height);

    /* VBO is setup as: "vertex:3f, color:4f */
    pixel_vertex_t vtx[4] = { { x0,y0,0,  r,g,b,a },
                        { x0,y1,0,  r,g,b,a },
                        { x1,y1,0,  r,g,b,a },
                        { x1,y0,0,  r,g,b,a } };
    // two triangles: 2 * 3 vertex
    GLuint   idx[6] = { 0, 1, 2,    0, 2, 3 };
    //vertex_buffer_push_back(rects_buffer, vtx, 4, idx, 6);
    rects_buffer.push_back(vtx, 4, idx, 6);

mz_shader = CreateProgramFE(0, NULL);
shader_s = CreateProgramFE(4, NULL); // test emb2
 // feedback
log_info("[%s] program_id=%d (0x%08x)", __FUNCTION__, mz_shader, mz_shader);
log_info("[%s] program_id=%d (0x%08x)", __FUNCTION__, shader_s, shader_s);

mat4_set_identity(&projection);
mat4_set_identity(&model);
mat4_set_identity(&view);

reshape(width, height);
}

void pixelshader_fini(void)
{
    //vertex_buffer_delete(rects_buffer);    rects_buffer = NULL;
    rects_buffer.clear();
    if (shader_s)    glDeleteProgram(shader_s), shader_s = 0;
    if (mz_shader) glDeleteProgram(mz_shader), mz_shader = 0;
}