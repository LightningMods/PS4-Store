/*
    effects on GLSL

    2020, masterzorag
*/

#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "shaders.h"

// ------------------------------------------------------- typedef & struct ---
typedef struct {
    float x, y, z;    // position (3f)
    float r, g, b, a; // color    (4f)
} vertex_t;


// from demo-font.c
extern texture_atlas_t *atlas;
extern texture_font_t  *main_font, // small
                        *sub_font; // left panel
extern int selected_icon; // from main.c
// ------------------------------------------------------- global variables ---
// shader and locations
static GLuint program    = 0; // default program
static GLuint shader_fx  = 0; // text_ani.[vf]
static float  g_Time     = 0.f;
static GLuint meta_Slot  = 0;
static mat4   model, view, projection;
// ---------------------------------------------------------------- reshape ---
static void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    mat4_set_orthographic( &projection, 0, width, 0, height, -1, 1);
}

// ---------------------------------------------------------- animation ---
static fx_entry_t *ani;          // the fx info

static vertex_buffer_t *line_buffer,
                       *text_buffer,
                       *vbo = NULL;
// bool flag
static int ani_is_running = 0;
// callback when done looping all effect status
static void ani_post_cb(void)
{
    log_info("%s()", __FUNCTION__);
    O_action_dispatch();
}

// ---------------------------------------------------------------- display ---
static void render_ani( int text_num, int type_num )
{
    // we already clean in main render loop!

    fx_entry_t *ani = &fx_entry[0];
    program         = shader_fx;

    if(ani->t_now >= ani->t_life) // looping animation status
    {
        switch(ani->status) // setup for next fx
        {
            case ANI_CLOSED :  ani->status = ANI_IN,      ani->t_life  =   .3f;  break;
            case ANI_IN     :  ani->status = ANI_DEFAULT, ani->t_life  =  1.3f;  break;
            case ANI_DEFAULT:  ani->status = ANI_OUT,     ani->t_life  =   .3f;  break;
            case ANI_OUT    :  ani->status = ANI_CLOSED,  ani->t_life  =  2.0f,
            /* CLOSED reached: looped once all status */  ani_is_running =  0 ;
            ani_post_cb();  break;
        }
        ani->fcount = 0;   // reset framecount (useless)
        ani->t_now  = 0.f; // yes, it's using time
    }
/*
    log_info("program: %d [%d] fx state: %.1f, frame: %3d/%3.f %.3f\r", 
            program, t_n,
                     ani->status /10.,
                     ani->fcount,
                     ani->life,
                     fmod(ani->status /10. + type_num /100., .02));
*/
    glUseProgram   ( program );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture  ( GL_TEXTURE_2D, atlas->id ); // rebind glyph atlas
    glDisable      ( GL_CULL_FACE );
    glEnable       ( GL_BLEND );
    glBlendFunc    ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    {
        glUniform1i       ( glGetUniformLocation( program, "texture" ),    0 );
        glUniformMatrix4fv( glGetUniformLocation( program, "model" ),      1, 0, model.data);
        glUniformMatrix4fv( glGetUniformLocation( program, "view" ),       1, 0, view.data);
        glUniformMatrix4fv( glGetUniformLocation( program, "projection" ), 1, 0, projection.data);
        glUniform4f       ( glGetUniformLocation( program, "meta"), 
                ani->t_now,
                ani->status /10., // we use float on SL, switching fx state
                ani->t_life,
                type_num    /10.);
        if(1) /* draw whole VBO (storing all added texts) */
        {
            vertex_buffer_render( line_buffer, GL_LINES );
            vertex_buffer_render( vbo,         GL_TRIANGLES );
        }
    }
    glDisable( GL_BLEND ); 

    ani->fcount += 1; // increase frame counter (useless)

    // we already swap frame in main render loop!
}

/* wrapper from main */
void GLES2_ani_test( fx_entry_t *_ani )
{
    render_ani( 1, selected_icon %MAX_ANI_TYPE );
    //for (int i = 0; i < itemcount; ++i)
    {
    /*  render_text_extended( item_entry, fx_entry );
        read as: draw this VBO using this effect! */
//        render_ani( 0, TYPE_0 );
//        render_ani( 1, TYPE_1 );
//        render_ani( 2, TYPE_2 );
//        render_ani( 3, TYPE_3 );
    }
}

// --------------------------------------------------------- custom shaders ---
static GLuint CreateProgram( void )
{
    GLuint programID = BuildProgram(ani_vs, ani_fs, ani_vs_length, ani_fs_length); // shader_common.c

    if (!programID)
    {
        log_error( "failed!");
    }
    // feedback
    log_error( "ani program_id=%d (0x%08x)", programID, programID);
    return programID;
}


// libfreetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;
static texture_font_t *font = NULL;
// ------------------------------------------------------------------- main ---
void GLES2_ani_init(int width, int height)
{
    vec4 color  = { 1., 1., 1., 1. };
    line_buffer = vertex_buffer_new( "vertex:3f,color:4f" );

#if 0 // rects
    vec2 pen = { .5, .5 };
for (int i = 0; i < 10; ++i)
{
    int  x0 = (int)( pen.x + i * 2 );
    int  y0 = (int)( pen.y + 20 );
    int  x1 = (int)( x0 + 64 );
    int  y1 = (int)( y0 - 64 );

    GLuint indices[6] = {0,1,2,  0,2,3}; // (two triangles)

    /* VBO is setup as: "vertex:3f, vec4 color */ 
    log_info("%d %d %d %d", x0, y0, x1, y1);
    vertex_t vertices[4] = { { x0,y0,0,  color },
                             { x0,y1,0,  color },
                             { x1,y1,0,  color },
                             { x1,y0,0,  color } };
    vertex_buffer_push_back( line_buffer, vertices, 4, indices, 6 );
    pen.x   += 72.;
    pen.y   -= 32.;
    color.g -= i * 0.1; // to show some difference: less green
}
#else 
 vec2 pen, origin;

    origin.x = width /2 + 100;
    origin.y = height/2 - 200;
//    add_text( text_buffer, big, "g", &black, &origin );
//vec4 black = { 0,0,0,1 };
    // title
    pen.x = 50;
    pen.y = height - 50;
//    add_text( text_buffer, title, "Glyph metrics", &black, &pen );

    float r = color.r, g = color.g, b = color.b, a = color.a;

    // lines
    vertex_t vertices[] =
        {   // Baseline
            { 10,  10, 0,  r,g,b,a},
            {100, 100, 0,  r,g,b,a},

            // Top line
            { origin.x,       origin.y + 100, 0,  r,g,b,a},
            { origin.x + 100, origin.y + 100, 0,  r,g,b,a},
            // Bottom line
            { origin.x,       origin.y      , 0,  r,g,b,a},
            { origin.x + 100, origin.y      , 0,  r,g,b,a},
            // Left line at origin
            { origin.x,       origin.y + 100, 0,  r,g,b,a},
            { origin.x,       origin.y      , 0,  r,g,b,a},
            // Right line at origin
            { origin.x + 100, origin.y + 100, 0,  r,g,b,a},
            { origin.x + 100, origin.y      , 0,  r,g,b,a},

            // Left line
            {width/2 - 20/2, .3*height, 0,  r,g,b,a},
            {width/2 - 20/2, .9*height, 0,  r,g,b,a},

            // Right line
            {width/2 + 20/2, .3*height, 0,  r,g,b,a},
            {width/2 + 20/2, .9*height, 0,  r,g,b,a},

            // Width
            {width/2 - 20/2, 0.8*height, 0,  r,g,b,a},
            {width/2 + 20/2, 0.8*height, 0,  r,g,b,a},

            // Advance_x
            {width/2-20/2-16, 0.2*height, 0,  r,g,b,a},
            {width/2-20/2-16+16, 0.2*height, 0,  r,g,b,a},

            // Offset_x
            {width/2-20/2-16, 0.85*height, 0,  r,g,b,a},
            {width/2-20/2, 0.85*height, 0,  r,g,b,a},

            // Height
            {0.3*width/2, origin.y + 18 - 24, 0,  r,g,b,a},
            {0.3*width/2, origin.y + 18, 0,  r,g,b,a},

            // Offset y
            {0.8*width, origin.y + 18, 0,  r,g,b,a},
            {0.8*width, origin.y , 0,  r,g,b,a},

        };
    GLuint indices [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
                         13,14,15,16,17,18,19,20,21,22,23,24,25 };
    vertex_buffer_push_back( line_buffer, vertices, 26, indices, 26 );
#endif

#if 1 // text_ani
    text_buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

    font = texture_font_new_from_memory(atlas, 64, font_ttf,font_len);

    // print something
    char *s = "test ani";   // set text

    texture_font_load_glyphs( font, s );        // set textures
    pen.x = (width - tl) /2;                    // use Text_Length to align pen.x
    pen.y = 100;
    // use outline
    font->rendermode        = RENDER_OUTLINE_EDGE;
    font->outline_thickness = 1.f;

    add_text( text_buffer, font, s, &color, &pen );  // set vertexes

    texture_font_delete( font );

    refresh_atlas();
#endif

    /* shader program is custom, so
    compile, link and use shader */
    shader_fx = CreateProgram();

    if(!shader_fx) { log_error("program creation failed"); }

    /* init ani effect */
    ani         = &fx_entry[0];
    ani->status = ANI_CLOSED,
    ani->fcount = 0;    // framecount
    ani->t_now  = 0.f;  // set actual time
    ani->t_life = 2.f;  // duration in seconds

    mat4_set_identity( &projection );
    mat4_set_identity( &model );
    mat4_set_identity( &view );
    // attach our "custom infos"
    meta_Slot = glGetUniformLocation(shader_fx, "meta");

    reshape(width, height);
}

void GLES2_ani_update(double now)
{
    if(ani_is_running == 0) return;

    ani->t_now += now - g_Time;
    g_Time      = now;
}

void GLES2_ani_fini( void )
{
    //texture_atlas_delete(atlas),  atlas  = NULL;
    //vertex_buffer_delete(buffer), buffer = NULL;
    if(shader_fx) glDeleteProgram(shader_fx), shader_fx = 0;
}

int ani_get_status(void)
{
    return ani_is_running;
}

// trigger start if not already flagged running
void ani_notify(const char *message)
{
    if(ani_is_running == 0)
    {
        if(vbo) vertex_buffer_delete(vbo), vbo = NULL;
        ani_is_running = 1;
    }
    else return; // don't race any running any_notify()

    if(!vbo) // build text message
    {
        vbo = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
        texture_font_load_glyphs( sub_font, message );        // set textures
        vec4 col = (vec4) { .8164, .8164, .8125, 1. };
        vec2 pen = (vec2) { (1920 - tl) /2,  600 }; // use Text_Length to align pen.x
        // override position
        //pen = (vec2) { 670,  1080 - 300 };
        add_text( vbo, sub_font, message, &col, &pen );  // set vertexes

        refresh_atlas();
    }
}

// draw one effect loop
void GLES2_ani_draw(void)
{
    if(vbo && ani_is_running) render_ani(1, 0);
   
}
