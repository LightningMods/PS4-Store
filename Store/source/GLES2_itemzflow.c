/*
    GLES2 ItemzFlow

    my implementation of coverlow, from scratch.
    (shaders
     beware)

    2021, masterzorag && LM
*/

#include "defines.h"

#include "GLES2_common.h"

#include "sig_handler.h"
#include "utils.h"

// ------------------------------------------------------- global variables ---
// the Settings
extern StoreOptions set,
                   *get;
extern double u_t;

static GLuint sh_prog1,
              sh_prog2,
              sh_prog3;

/* VBOs for shapes */
static vertex_buffer_t *cover_o, // object, with depth: 6 faces
                       *cover_t, // full coverbox, template
                       *cover_i, // just icon coverbox, overlayed
                       *title_vbo = NULL;

static mat4  model,
             view,
             projection;

typedef struct { float s,t;     } st;
typedef struct { float x,y,z;   } xyz;
typedef struct { float r,g,b,a; } rgba;

typedef struct retry_t
{
    GLuint tex;      // the real texture
    bool exists;
} retry_t;

enum Ops_for_cf
{
    Launch_Game_opt, Dump_Game_opt, Uninstall_Update_opt, Uninstall_Game_opt, Trainers_opt
};

struct retry_t   *cf_tex = NULL;

// ------------------------------------------------------ freetype-gl shaders ---
typedef struct { xyz position,
                       normal;
                 rgba   color;  } v3f3f4f_t;
static GLuint CreateProgram1(void)
{
    const GLchar s_vertex_shader_code[] =
       "precision mediump float; \
        uniform mat4 model; \
        uniform mat4 view; \
        uniform mat4 projection; \
        \
        uniform   vec4 Color;  \
        attribute vec3 vertex; \
        attribute vec3 normal; \
        attribute vec4 color;  \
        \
        varying   vec4 fragColor; \
        varying   vec3 norm; \
        \
        void main() \
        { \
            fragColor   = color*Color; \
            norm        = normal; \
            gl_Position = projection*(view*(model*vec4(vertex,1.0))); \
        }";

    const GLchar s_fragment_shader_code[] =
       "precision mediump float;  \
        varying   vec4 fragColor; \
        varying   vec3 norm; \
        \
        void main() \
        { \
                gl_FragColor = fragColor; \
        }";

    GLuint programID = BuildProgram(s_vertex_shader_code, s_fragment_shader_code);
    // feedback
    log_info( "TEST: program_id=%d (0x%08x)", programID, programID);
    return programID;
}

typedef struct { xyz  position; 
                 st     tex_uv;
                 rgba    color;  } v3f2f4f_t;
static GLuint CreateProgram2(void)
{
    const GLchar s_vertex_shader_code[] =
       "precision mediump float; \
        uniform mat4 model; \
        uniform mat4 view; \
        uniform mat4 projection; \
        \
        uniform   vec4 Color;  \
        attribute vec3 vertex; \
        attribute vec2 tex_coord; \
        attribute vec4 color; \
        \
        varying vec2 vTexCoord; \
        varying vec4 fragColor; \
        \
        void main() \
        { \
            vTexCoord.xy = tex_coord.xy; \
            fragColor    = Color; \
            gl_Position  = projection*(view*(model*vec4(vertex,1.0))); \
        }";

    const GLchar s_fragment_shader_code[] =
       "precision mediump float; \
        uniform sampler2D texture; \
        \
        varying vec2 vTexCoord; \
        varying vec4 fragColor; \
        \
        void main() \
        { \
            gl_FragColor    = texture2D(texture, vTexCoord); \
            gl_FragColor.a *= fragColor.a; \
        }";

    GLuint programID = BuildProgram(s_vertex_shader_code, s_fragment_shader_code);
    // feedback
    log_info( "TEST: program_id=%d (0x%08x)", programID, programID);
    return programID;
}

// draws the reflection
static GLuint CreateProgram3(void)
{
    const GLchar s_vertex_shader_code[] =
       "precision mediump float; \
        uniform mat4 model; \
        uniform mat4 view; \
        uniform mat4 projection; \
        \
        uniform   vec4 Color;  \
        attribute vec3 vertex; \
        attribute vec2 tex_coord; \
        attribute vec4 color; \
        \
        varying vec2 vTexCoord; \
        varying vec4 fragColor; \
        \
        void main() \
        { \
            vTexCoord.xy = tex_coord.xy; \
            fragColor    = Color; \
            gl_Position  = projection*(view*(model*vec4(vertex,1.0))); \
        }";

    const GLchar s_fragment_shader_code[] =
       "precision mediump float; \
        uniform sampler2D texture; \
        \
        varying vec2 vTexCoord; \
        varying vec4 fragColor; \
        \
        void main() \
        { \
            gl_FragColor   = texture2D(texture, vTexCoord); \
            gl_FragColor.a = sin(vTexCoord.y) * fragColor.a; \
        }";

    GLuint programID = BuildProgram(s_vertex_shader_code, s_fragment_shader_code);
    // feedback
    log_info( "TEST: program_id=%d (0x%08x)", programID, programID);
    return programID;
}

vec4 c[8];

static void append_to_vbo(vertex_buffer_t *vbo, vec3 *v)
{
    float s0 = 0., t0 = 0.,
          s1 = 1., t1 = 1.;
    GLuint     idx[6] = { 0, 1, 2,   0, 2, 3 };
    v3f2f4f_t  vtx[4] = {
        { {(v+0)->x, (v+0)->y, (v+0)->z},  {s1, t0},  {c[0].r, c[0].g, c[0].b, c[0].a} },
        { {(v+1)->x, (v+1)->y, (v+1)->z},  {s0, t0},  {c[1].r, c[1].g, c[1].b, c[1].a} },
        { {(v+2)->x, (v+2)->y, (v+2)->z},  {s0, t1},  {c[2].r, c[2].g, c[2].b, c[2].a} },
        { {(v+3)->x, (v+3)->y, (v+3)->z},  {s1, t1},  {c[3].r, c[3].g, c[3].b, c[3].a} }
    };
    vertex_buffer_push_back( vbo, vtx, 4, idx, 6 );
}

// better if odd to get one in the center, eh
#define NUM_OF_PIECES  (7)

// one in the center: draw +/- item count: it's the `plusminus_range`
const int pm_r = (NUM_OF_PIECES -1) /2;

GLuint cb_tex = 0, // default coverbox textured image (empty: w/ no icon)
       bg_tex = 0; // background textured image
int    g_idx  = pm_r,
       pr_dbg = 1;    // just to debug once

typedef struct { xyz  position; 
                 rgba    color; } v3fc4f_t;

// add to vbo from rectangle, in px coordinates
void vbo_add_from_rect(vertex_buffer_t *vbo, vec4 *rect)
{
    vec4   color = { 1, 0, 0, 1 };
    float r = color.r, g = color.g, b = color.b, a = color.a;
    /* VBO is setup as: "vertex:3f, color:4f */ 
    v3fc4f_t vtx[4] = { { {rect->x, rect->y, 0},  {r,g,b,a} },
                        { {rect->x, rect->w, 0},  {r,g,b,a} },
                        { {rect->z, rect->w, 0},  {r,g,b,a} },
                        { {rect->z, rect->y, 0},  {r,g,b,a} } };
    // two triangles: 2 * 3 vertex
    GLuint   idx[6] = { 0, 1, 2,    0, 2, 3 };
    vertex_buffer_push_back( vbo, vtx, 4, idx, 6 );
}


vec3 off = (vec3){ 1.05, 0,  0 }; // item position offset

/* for animations */
typedef struct ani_t
{
    double now,    // current time
           life;   // total duration
    vec3   v3_t,   // single translate vector
           v3_r;   // single rotation vector
    void  *handle; // what we are animating?
} ani_t;

// we have two animation pointers, and the pointer to current one
ani_t *g_ani[2],
        *ani = NULL;

/* view (matrix) vectors */
vec3 v0 = (vec3) (0),
     v1,
     v2;

void InitScene_5(int width, int height)
{
    glFrontFace(GL_CCW);

    // reset view
    v2 = v1 = v0;

    g_ani[ 0 ] = calloc(1, sizeof(ani_t));
    g_ani[ 1 ] = calloc(1, sizeof(ani_t));
    // address it to default
    ani = g_ani[ 0 ];

    // unused normals
    vec3 n[6];
    n[0] = (vec3) { 0, 0, 1 },
    n[1] = (vec3) { 1, 0, 0 },
    n[2] = (vec3) { 0, 1, 0 },
    n[3] = (vec3) {-1, 0, 1 },
    n[4] = (vec3) { 0,-1, 0 },
    n[5] = (vec3) { 0, 0,-1 };

    // setup color palette
    c[0] = (vec4) {1, 1, 1, 1}, // 1. white   TL
    c[1] = (vec4) {1, 1, 0, 1}, // 2. yellow  BL (fix bottom)
    c[2] = (vec4) {1, 0, 1, 1}, // 3. purple  BR (fix bottom)
    c[3] = (vec4) {0, 1, 1, 1}, // 4. cyan    TR
    c[4] = (vec4) {1, 0, 0, 1}, // 5: red
    c[5] = (vec4) {0, 0.1145, 0.9412, 1}, // 6. blue
    c[6] = (vec4) {0, 1, 0, 1}, // 7. green
    c[7] = (vec4) {0, 0, 0, 1}; // 8. black

    // the object
    cover_o = vertex_buffer_new( "vertex:3f,color:4f" );
    // the textuted objects
    cover_t = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" ); // owo template
    cover_i = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" ); // single diff item

    // one rect
    for(int i=0; i < 1 /* ONE_PIECE */; i++)
    {
        /* the single different item texture (overlayed) */
        vec3 cover_item_v[8];
        // keep center dividing by 2.f, CW
        cover_item_v[0] = (vec3) {  .956 /2.,   .925 /2., .0 /2. }; // up R
        cover_item_v[1] = (vec3) { -.991 /2.,   .925 /2., .0 /2. }; // up L
        cover_item_v[2] = (vec3) { -.991 /2., -1.195 /2., .0 /2. }; // bm L
        cover_item_v[3] = (vec3) {  .956 /2., -1.195 /2., .0 /2. }; // bm R
        append_to_vbo(cover_i, &cover_item_v[0]);
/*
  xyz  v[] = { { 1, 1, 1},  {-1, 1, 1},  {-1,-1, 1}, { 1,-1, 1},
               { 1,-1,-1},  { 1, 1,-1},  {-1, 1,-1}, {-1,-1,-1} };
*/
        vec3 cover_v[8];
        // keep center dividing by 2.f
        cover_v[0] = (vec3) {  1. /2.,  1.25 /2., .0 /2. };
        cover_v[1] = (vec3) { -1. /2.,  1.25 /2., .0 /2. };
        cover_v[2] = (vec3) { -1. /2., -1.25 /2., .0 /2. };
        cover_v[3] = (vec3) {  1. /2., -1.25 /2., .0 /2. };
        // for shared/template cover box
        append_to_vbo(cover_t, &cover_v[0]);

        // for the other 5 faces of the object
        cover_v[4] = (vec3) {  1. /2., -1.25 /2., -.2 /2. };
        cover_v[5] = (vec3) {  1. /2.,  1.25 /2., -.2 /2. };
        cover_v[6] = (vec3) { -1. /2.,  1.25 /2., -.2 /2. };
        cover_v[7] = (vec3) { -1. /2., -1.25 /2., -.2 /2. };

        v3fc4f_t cover_vtx[24] = {
// back
          { {cover_v[6].x, cover_v[6].y, cover_v[6].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
/* w */   { {cover_v[7].x, cover_v[7].y, cover_v[7].z},  {c[4].r, c[4].g, c[4].b, c[4].a} }, // red
          { {cover_v[4].x, cover_v[4].y, cover_v[4].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
          { {cover_v[5].x, cover_v[5].y, cover_v[5].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
//top 056, 061
          { {cover_v[0].x, cover_v[0].y, cover_v[0].z},  {c[0].r, c[0].g, c[0].b, c[0].a} }, // white
          { {cover_v[5].x, cover_v[5].y, cover_v[5].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, 
          { {cover_v[6].x, cover_v[6].y, cover_v[6].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
          { {cover_v[1].x, cover_v[1].y, cover_v[1].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
// bottom
          { {cover_v[3].x, cover_v[3].y, cover_v[3].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
          { {cover_v[4].x, cover_v[4].y, cover_v[4].z},  {c[0].r, c[0].g, c[0].b, c[0].a} }, // white
          { {cover_v[7].x, cover_v[7].y, cover_v[7].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
          { {cover_v[2].x, cover_v[2].y, cover_v[2].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
// Right
          { {cover_v[0].x, cover_v[0].y, cover_v[0].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, // white
          { {cover_v[3].x, cover_v[3].y, cover_v[3].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, 
          { {cover_v[4].x, cover_v[4].y, cover_v[4].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, 
          { {cover_v[5].x, cover_v[5].y, cover_v[5].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
// left
          { {cover_v[6].x, cover_v[6].y, cover_v[6].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, // 
          { {cover_v[7].x, cover_v[7].y, cover_v[7].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, // 
          { {cover_v[2].x, cover_v[2].y, cover_v[2].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, // 
          { {cover_v[1].x, cover_v[1].y, cover_v[1].z},  {c[5].r, c[5].g, c[5].b, c[5].a} }, // 
// front
          { {cover_v[1].x, cover_v[1].y, cover_v[1].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
          { {cover_v[2].x, cover_v[2].y, cover_v[2].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
          { {cover_v[3].x, cover_v[3].y, cover_v[3].z},  {c[5].r, c[5].g, c[5].b, c[5].a} },
          { {cover_v[0].x, cover_v[0].y, cover_v[0].z},  {c[0].r, c[0].g, c[0].b, c[0].a} }  // white
      };

        // define front to origin, ClockWise
        GLuint cover_idx[36] = {  0, 1, 2,  0, 2, 3,  // Front: 012, 023 CCW -> not drawn
                                  4, 5, 6,  4, 6, 7,  // R:     034, 045 CCW ->
                                  8, 9,10,  8,10,11,  // Top:   056, 061
                                 12,13,14, 12,14,15,  // L:     167, 172 green
                                 16,17,18, 16,18,19,  // Bot:   743, 732
                                 20,21,22, 20,22,23   // Back: 476 rwb, 465 rbb -> CW is drawn
                             };

        vertex_buffer_push_back( cover_o, cover_vtx, 24, cover_idx, 36 );

        // we have shapes/objects
    }

    if(!cb_tex) // fallback, one texture fits all
    {
        if (!if_exists("/mnt/sandbox/NPXS39041_000/app0/assets/aaa.png"))  
            cb_tex = load_png_asset_into_texture("/user/app/NPXS39041/storedata/aaa.png");
        else
            cb_tex = load_png_asset_into_texture("/mnt/sandbox/NPXS39041_000/app0/assets/aaa.png");

    }
    // the fullscreen image
 
    if (!bg_tex)
    {
        if(if_exists("/mnt/sandbox/NPXS39041_000/app0/assets/backgroundUnitOffline.png"))
           bg_tex = load_png_asset_into_texture("/mnt/sandbox/NPXS39041_000/app0/assets/backgroundUnitOffline.png");
    }
    

    // we will need at max i_apps count coverbox
    if(!cf_tex)
        cf_tex = calloc(i_apps->token_c +1 /* 1st is reseved */, sizeof(retry_t));

    // all done
}

// ------------------------------------------------------------------- init ---
void InitScene_4(int width, int height)
{
    // compile, link and use custom shaders
    sh_prog1 = CreateProgram1();
    sh_prog2 = CreateProgram2();
    sh_prog3 = CreateProgram3();

    mat4_set_identity( &projection );
    mat4_set_identity( &model );
    mat4_set_identity( &view );

    // reshape
    glViewport(0, 0, width, height);
    mat4_set_perspective( &projection, 60.0f, width/(float) height, 1.0, 10.0 );
}


extern GLuint shader;
static void render_tex( GLuint texture, int idx, vertex_buffer_t *vbo, vec4 col )
{
    // we already clean in main renderloop()!

    GLuint sh_prog = sh_prog2;

    if(idx == -1) sh_prog = sh_prog3;

    glUseProgram( sh_prog );

    int num = 2;
    glEnable       ( GL_BLEND );
    glBlendFunc    ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glActiveTexture( GL_TEXTURE0 + num );
    glBindTexture  ( GL_TEXTURE_2D, texture );
    {
        glUniform1i       ( glGetUniformLocation( sh_prog, "texture" ),    num );
        glUniformMatrix4fv( glGetUniformLocation( sh_prog, "model" ),      1, 0, model.data);
        glUniformMatrix4fv( glGetUniformLocation( sh_prog, "view" ),       1, 0, view.data);
        glUniformMatrix4fv( glGetUniformLocation( sh_prog, "projection" ), 1, 0, projection.data);

        glUniform4f       ( glGetUniformLocation( sh_prog, "Color" ), col.r, col.g, col.b, col.a );

        /* draw whole VBO (storing all added) */
        vertex_buffer_render( vbo, GL_TRIANGLES );
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram( 0 );
}

// ---------------------------------------------------------------- display ---

/* model rotation angles and camera position */
float gx =  0.,
      gy = 90.,
      gz = 90.;
/*    xC =  0.,
      yC =  0.,
      zC = -5.; // camera z (-far-, +near) */

float testoff = 3.6;

static double g_Time    = .0,
              xoff      = .0;

// for Settings (options_panel)
bool use_reflection  = true,
     use_pixelshader = true;

extern layout_t *icon_panel;
extern item_t   *i_apps;
extern ivec4     menu_pos;


static bool reset_tex_slot(int idx)
{
    bool res = false;
    if(cf_tex[ idx ].tex > 0)
    {
//      log_debug( "%s[%3d]: %3d, %3d", __FUNCTION__, idx, cf_tex[ idx ].tex, i_apps[ idx ].texture);
        if(cf_tex[ idx ].tex != cb_tex) // keep the coverbox template
        {
//          log_debug( "%s[%3d]: (%d)  %d ", __FUNCTION__, idx, cf_tex[ idx ].tex, i_apps[ idx ].texture);
            glDeleteTextures(1, &cf_tex[ idx ].tex), cf_tex[ idx ].tex = 0;
            res = true;
        }
        else
        {
            if(i_apps[ idx ].texture  > 0
            && i_apps[ idx ].texture != fallback_t) // keep icon0 template
            {   // discard icon0.png
//              log_debug( "%s[%3d]:  %d  (%d)", __FUNCTION__, idx, cf_tex[ idx ].tex, i_apps[ idx ].texture);
                glDeleteTextures(1, &i_apps[ idx ].texture), i_apps[ idx ].texture = 0;
                res = true;
            }
        }
    }

    return res;
}


void drop_some_coverbox(void)
{
    int count = 0;

    for (int i = 0; i < i_apps->token_c; i++)
    {
        if(i > g_idx - pm_r -1
        && i < g_idx + pm_r +1) continue;

        if( reset_tex_slot(i) ) count++;
    }

    if(count > 0)
        log_info( "%s discarded %d textures", __FUNCTION__, count);
}


static void download_texture(int idx)
{
    if (!cf_tex[idx].tex)
    {
        char* id = i_apps[idx].token_d[ID].off,
            cb_path[256];

        snprintf(cb_path, 255, "/user/app/NPXS39041/covers/%s.png", id);

        mkdir("/user/app/NPXS39041/covers", 0777);
        //DONT CARE IF IT ALREADY EXISTS
        log_info("Trying to Download cover for %s", id);

        loadmsg("Trying to Download cover for %s", id);

        if (!if_exists(cb_path)) // download
        {
            cf_tex[idx].exists = false;
            char cb_url[256];
#if LOCALHOST_WINDOWS

            snprintf(cb_url, 255, "%s/00/download.php?cover_tid=%s", get->opt[CDN_URL], id);
#else
            snprintf(cb_url, 255, "%s/download.php?cover_tid=%s", get->opt[CDN_URL], id);
#endif

            int ret = dl_from_url(cb_url, cb_path, false);
            if (ret != 0) {
                log_warn("dl_from_url for %s failed with %i", id, ret);
            }
            else if (ret == 0)
                cf_tex[idx].exists = true;
        }
        else
            cf_tex[idx].exists = true;

    }
}

static void check_n_load_texture(int idx)
{
    if( ! cf_tex[ idx ].tex )
    {
        char *id = i_apps[ idx ].token_d[ ID ].off,
              cb_path[256];

        snprintf(cb_path, 255, "/user/app/NPXS39041/covers/%s.png", id);

        cf_tex[idx].tex = cb_tex;

        if(cf_tex[idx].exists)
           cf_tex[ idx ].tex = load_png_asset_into_texture(cb_path);
        
    }
    // as final fallback
    if( cf_tex[ idx ].tex == 0 ) cf_tex[ idx ].tex = cb_tex;
}


static void check_n_draw_texture(int idx, int SH_type, vec4 col)
{
    if( cf_tex[ idx ].tex == cb_tex ) // craft a coverbox stretching the icon0
    {
        render_tex( cb_tex, /* template */ SH_type, cover_t, col );

        if( i_apps[ idx ].texture == 0 )
            i_apps[ idx ].texture = load_png_asset_into_texture( i_apps[ idx ].token_d[ PICPATH ].off );
        // fallback icon0
        if( i_apps[ idx ].texture == 0 )
            i_apps[ idx ].texture = fallback_t;
        // overlayed stretched icon0
        render_tex( i_apps[ idx ].texture, SH_type, cover_i, col );
    }
    else // available cover box texture
    {
        render_tex( cf_tex[ idx ].tex, SH_type, cover_t, col );
    }
}


static void send_cf_action(int plus_or_minus_one, int type)
{

    if(ani) return; // don't race any running ani

    ani = g_ani[0];
    // we will operate on this
    ani->handle = &g_idx;

    //set ani, off
    switch(type)
    {
      case 0: 
        ani->life       = .1976, // time_life in seconds
        ani->now        = .0,    // set actual time
        ani->v3_t       = (vec3){ 1.05,  0., 0. }; // item position offset
        ani->v3_r       = (vec3){ 0.,   90., 0. }; // item position rotation
        break;
      case 1:
        ani->life       = 3* .1976, // time_life in seconds
        ani->now        = .0,    // set actual time
        ani->v3_t       = (vec3){ 1.05,  0., .2 }; // item position offset
        ani->v3_r       = (vec3){ 0.,   60., .0 }; // item position rotation
        break;
    }

    // we will move +/- 1 so: keep the sign!
    xoff   = plus_or_minus_one * off.x; // set total x offset with sign!

    log_info("%d, %s, %s, %d", g_idx,
                           i_apps[ g_idx ].token_d[ NAME ].off,
                           i_apps[ g_idx ].token_d[ ID   ].off,
                           i_apps[ g_idx ].texture);
}

typedef enum view_t
{
    RANDOM = -1,
    ITEMzFLOW,
    ITEM_PAGE
} view_t;

int v_curr = ITEMzFLOW;

static vec3 set_view(enum view_t vt) // view type
{
    vec3 ret = (0);

    if(ani) return ret; // don't race any running ani
    
    // setup one
    ani = g_ani[0];
    // we will operate on this
    ani->handle = &v_curr;

    ani->life       = 2* .1976, // time_life in seconds
    ani->now        = .0,    // set actual time
    ani->v3_t       = (vec3){ 1.05,  0., 0. }; // item position offset
    ani->v3_r       = (vec3){ 0.,   90., 0. }; // item position rotation

    switch(vt)
    {
        case ITEM_PAGE:
            ret = (vec3){ .70, -.55, 1.85 };
            break;

        case RANDOM:
        {
            double r = rand() % 2 -1;
            ret = (vec3){ sin(r), cos(r), tan(r) };
        }   break;

        case ITEMzFLOW:
        default: break;
    }
    log_info("%.3f %.3f %.3f", ret.x, ret.y, ret.z);

    return ret;
}

layout_t *gm_p = NULL, // game_option_panel
         *ls_p = NULL; // game_option_panel v2



extern char* title[300];
extern char* title_id[30];

bool IS_ERROR(uint32_t a1)
{
    return a1 & 0x80000000;
}

static void X_action_dispatch(int action, layout_t *l)
{
    char tmp[256];
    int libcmi = -1;
    int error = 0;
    // selected item from active panel
    log_info( "execute %d -> '%s' for '%s'", l->curr_item, 
                                        l->item_d[ l->curr_item ].token_d[    0 ].off,
                                           i_apps[ g_idx        ].token_d[ NAME ].off);

    
    snprintf(title,   400, "%s", i_apps[ g_idx ].token_d[ NAME ].off);
    snprintf(title_id, 30, "%s", i_apps[ g_idx ].token_d[ ID   ].off);

    log_info("title_id: %s, title: %s",title_id, title);

    switch(l->curr_item)
    {
        case Dump_Game_opt:
        {
            dump = true;

            msgok(NORMAL, "AFTER the Game launches when you press OK wait few seconds then go back to the Store\n\nand the game will be begin dumping DO NOT CLOSE THE GAME OR STORE UNTIL THE DUMP IS COMPLETE");
        }
        case Launch_Game_opt:
        {
            libcmi = sceKernelLoadStartModule("/system/common/lib/libSceSystemService.sprx", 0, NULL, 0, 0, 0);
            if (libcmi > 0)
            {
                log_info("Starting action Launch_Game_opt");


                OrbisUserServiceLoginUserIdList userIdList;

                log_info("ret %x", sceUserServiceGetLoginUserIdList(&userIdList));

                for (int i = 0; i < 4; i++)
                {
                    if (userIdList.userId[i] != 0xFF)
                    {
                        log_info("[%i] User ID 0x%x", i, userIdList.userId[i]);
                    }
                }


                LncAppParam param;
                param.sz = sizeof(LncAppParam);

                if(userIdList.userId[0] != 0xFF)
                     param.user_id = userIdList.userId[0];
                else if (userIdList.userId[1] != 0xFF)
                    param.user_id = userIdList.userId[1];

                param.app_opt = 0;
                param.crash_report = 0;
                param.check_flag = SkipSystemUpdateCheck;

                log_info( "l1 %x", sceLncUtilInitialize());


                uint32_t sys_res = sceLncUtilLaunchApp(title_id, 0, &param);
                if (IS_ERROR(sys_res))
                {
                    log_info("Switch 0x%x", sys_res);
                    switch (sys_res) {
                       case SCE_LNC_ERROR_APP_NOT_FOUND: {
                        msgok(WARNING, "App is NOT Found ref: SCE_LNC_ERROR_APP_NOT_FOUND");
                        break;
                      }
                      case SCE_LNC_UTIL_ERROR_ALREADY_RUNNING: {
                        msgok(WARNING, "App is already running ref: SCE_LNC_UTIL_ERROR_ALREADY_RUNNING");
                        break;
                      }
                      case SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_KILL_NEEDED: {
                        log_debug("ALREADY RUNNING KILL NEEDED");
                        break;
                      }
                      case SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_SUSPEND_NEEDED: {
                        log_debug("ALREADY RUNNING SUSPEND NEEDED");
                        break;
                      }
                      case SCE_LNC_UTIL_ERROR_SETUP_FS_SANDBOX: {
                        msgok(WARNING, "App is NOT Launchable ref: SCE_LNC_UTIL_ERROR_SETUP_FS_SANDBOX");
                        break;
                      }
                      case SCE_LNC_UTIL_ERROR_INVALID_TITLE_ID: {
                        msgok(WARNING, "TITLE_ID IS NOT VAILED ref: SCE_LNC_UTIL_ERROR_SETUP_FS_SANDBOX");
                        break;
                      }

                      default:{
                        msgok(WARNING, "App Launch has failed with error code: 0x%x", sys_res);
                        break;
                       }
                    }
                }

                log_info("launch ret 0x%x", sys_res);

            }
            else
                msgok(WARNING, "Game Launch has failed with 0x%X", libcmi);

            break;
        }
        case Uninstall_Update_opt: {

            if (app_inst_util_uninstall_patch(title_id, &error) && error == 0)
                msgok(NORMAL, "Game Update Uninstalled successfully");
            else
                msgok(WARNING, "Game Update Uninstall failed with code: 0x%X (%i)", error, error);
            break;
        }

        case Uninstall_Game_opt: {

            if (app_inst_util_uninstall_game(title_id, &error) && error == 0)
            {
                msgok(NORMAL, "Game Uninstalled successfully, the installed App list will now reload");

                refresh_apps_for_cf();
                fw_action_to_cf(CIR);
            }
            else
                msgok(WARNING, "Game Uninstall failed with code: %x (%i)", error, error);

            break;
        }

        case Trainers_opt: {
            log_info("NOT IMPLMENTED!!!!");
            msgok(NORMAL, "Coming soon:tm:");

            break;
        }
    };  
}

void fw_action_to_cf(int button)
{   // ask to refresh vbo to update title
    GLES2_UpdateVboForLayout(gm_p);
    GLES2_UpdateVboForLayout(ls_p);
    GLES2_UpdateVboForLayout(icon_panel);

    GLES2_refresh_sysinfo();

    // start controlling option_v2
    if( ! active_p ) active_p = ls_p;

    if(ani) return; // don't race any running ani

    if(v_curr == ITEMzFLOW)
    {
        active_p = NULL;// 
        switch(button)
        {   // movement
            case LEF:  send_cf_action(-1, 0); break; // l_or_r
            case RIG:  send_cf_action(+1, 0); break;

            case CRO:  active_p = ls_p; // control cf_game_options
                       v2 = set_view(ITEM_PAGE);  break; // in_out
            case TRI: {  //REFRESH APP
                //tell the user
                msgok(NORMAL, "The Installed Apps list will now Reload");
                //refresh app list
                refresh_apps_for_cf(); 
                //go back to main menu
                goto refresh_apps;
                    break;
            }

/*          case UP:   gx -= 5.; log_info("gx:%.3f", gx); break;
            case DOW:  gx += 5.; log_info("gx:%.3f", gx); break; */

back_to_Store:
            case CIR: {
refresh_apps:
                drop_some_coverbox();
                menu_pos.z = ON_MAIN_SCREEN;
                active_p   = icon_panel;
                if(active_p->vbo_s < ASK_REFRESH) active_p->vbo_s = ASK_REFRESH;
            } return;
        }
    } else
    if(v_curr == ITEM_PAGE) // move into game_option_panel
    {
        switch(button)
        {   // movement
            case LEF:  layout_update_sele( active_p, -1 ); break; // l_or_r
            case RIG:  layout_update_sele( active_p, +1 ); break;
            case UP:   layout_update_sele( active_p, -active_p->fieldsize.x ); break;
            case DOW:  layout_update_sele( active_p, +active_p->fieldsize.x ); break;

            case CRO:  X_action_dispatch(0, active_p);  break; // in_out

            case CIR: v2 = set_view(ITEMzFLOW); break;
            

            case TRI:  break;//goto back_to_Store;
        }
    }
}

GLuint btn_X = 0;
void draw_additions(void)
{
    if(btn_X == 0)
    {
        //check if PKG has the assets
        if(!if_exists("/mnt/sandbox/NPXS39041_000/app0/assets/btn_X.png"))
            btn_X = load_png_asset_into_texture("/user/app/NPXS39041/storedata/btn_X.png");
        else
            btn_X = load_png_asset_into_texture("/mnt/sandbox/NPXS39041_000/app0/assets/btn_X.png");
    }
    else
    {
        vec4 r;
        vec2
        s    = (vec2) (68),
        p    = (resolution - s) /2.;
        p.y -= 400;
        s   += p;
        // convert to normalized coordinates
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);
        //char *p = strstr( l->item_d[i].token_d[PICPATH].off, "storedata" );
        // draw texture, use passed frect
        on_GLES2_Render_icon(USE_COLOR, btn_X, 2, &r, NULL);
    }
}

bool Download_icons = true;

void DrawScene_4(void)
{

     layout_t *l;
    /* animation alpha fading */
    vec4 colo = (1.), ani_c;
    /* animation vectors */
    vec3 ani_p, 
         ani_r;
    /* animation timing */
    double ani_ratio = 0.;

    /* animation l/r */
    const vec3 offset = (vec3) { 1.05,  0., 0. };

    /* delta view movement vector */
    vec3 view_v = v2 - v1;

    if(ani)
    {   // update time
        ani->now += u_t - g_Time;
        // check on delta ani time_ratio
        ani_ratio = ani->now / ani->life;
        // ani ended
        if(ani_ratio > 1.)
        {
            ani_ratio = 0;
            /*  switch on (ani type) but with pointers:
                l/R op_on g_idx
                CRO op_on view_v */
            if(ani->handle == &g_idx)
            {   // ani ended, move global index by sign!
                g_idx += (xoff > 0) ? 1 : -1; // L:-1, R:+1
                // bounds checking on item count
                if(g_idx < 1) g_idx = i_apps[0].token_c;
                if(g_idx > i_apps[0].token_c) g_idx = 1;
            }

            if(ani->handle == &v_curr)
            {   // ani ended: now finally switch the view
                    v1 = v2;
                v_curr = (v_curr == ITEMzFLOW) ? ITEM_PAGE : ITEMzFLOW;
            }
            // detach
            ani = NULL;

            xoff   = 0.; // ani_flag, pm_flag
            pr_dbg = 1;
            // refresh title
            if(title_vbo) vertex_buffer_delete(title_vbo), title_vbo = NULL;
        }
        // the (delta!) fraction
        view_v *= ani_ratio;

        /* consider ani vector, get a fraction from offset
           and get a fraction of degree, for a single step */
        ani_p  = ani_ratio * offset;
        ani_r  = (vec3) { 0., 90. /pm_r, 0. };
        ani_r *= ani_ratio;
        // ani color fraction
        ani_c  = ani_ratio * colo;
#if 0
        if(ani) log_info("ani_now:%f (%3.1f%%), %.3f (%.3f, %.3f, %.3f)",
                            ani->now, ani_ratio * 100., ani_ratio * xoff,
                                            view_v.x, view_v.y, view_v.z);
#endif
    }
    else xoff = 0.; // no ani, no offset

    // update time
    g_Time  = u_t;
    // add start point to view vector
    view_v += v1;

    switch(v_curr) // 
    {
        case ITEMzFLOW:  ani_c *= -1.;   break; // decrease 1-0
        case ITEM_PAGE:  colo   = (0.);  break; // increase 0-1
    }
    
    // we fade color
    if( !xoff ) colo += ani_c;

    /* background image, or pixelshader */
    if(use_pixelshader)
        pixelshader_render(2, NULL, NULL); // use PS_symbols shader
    else
    {   /* bg image: fullscreen frect (normalized coordinates) */
        vec4 r = (vec4) { -1., -1.,   1., 1. };
       // see if its even loaded
       if(bg_tex)
        on_GLES2_Render_icon(USE_COLOR, bg_tex, 2, &r, NULL);
    }

    int i, j, c = 0;
    /* loop outer to inner, care overlap */
    for(i = pm_r; i > 0; i--) // ...3, 2, 1
    {   // set the iterator
        j = i;
        
        vec3 pos, rot;

draw_item:

        // next item position
        pos    = j * offset;
        // set -DEFAULT_Z_CAMERA
        pos.z -= testoff; // adjust testoff

        rot    = (vec3) { gx, 90. / (float)pm_r, 0. };
        rot   *= -j;

      //if(pr_dbg)
        if(0)
        {
            log_info("%2d: %3d %3d (%3.2f, %3.2f, %3.2f)\t",
                                         c, i, j, 
                                                  pos.x, pos.y, pos.z);
            log_info("(%3.2f, %3.2f, %3.2f)", rot.x, rot.y, rot.z);
        }

        if( !j ) // means the selected, last one
        {
            colo = (1.);
            if( !xoff ) rot.y = ani_ratio * 360; // coverbox backflip effect
        }

        // adjust on l/r action
        if(xoff > 0)
        {
            pos -= ani_p;  rot += ani_r; // right
        }
        else
        if(xoff < 0)
        {
            pos += ani_p;  rot -= ani_r; // left
        }

        // ...

        glUseProgram( sh_prog1 );
        glEnable ( GL_BLEND );
        glDisable( GL_CULL_FACE );
        {
            // order matters!
            mat4_set_identity( &model );
            mat4_translate   ( &model, 0, 0, 0);
             // move to "floor" plane
            mat4_translate( &model, 0, 1.25 /2., 0 );
            // rotations
            mat4_rotate( &model, rot.x, 1, 0, 0 );
            mat4_rotate( &model, rot.y, 0, 1, 0 );
            mat4_rotate( &model, rot.z, 0, 0, 1 );
            // set -DEFAULT_Z_CAMERA
            mat4_translate( &model, pos.x, pos.y, pos.z);

            if(1) // ani_view
            {
                mat4_set_identity( &view );
                mat4_translate( &view, view_v.x, view_v.y, view_v.z );
            }

            /* don't USE */ //glEnable( GL_DEPTH_TEST );
            {
                glUniformMatrix4fv( glGetUniformLocation( sh_prog1, "model" ),      1, 0, model.data      );
                glUniformMatrix4fv( glGetUniformLocation( sh_prog1, "view"  ),      1, 0, view.data       );
                glUniformMatrix4fv( glGetUniformLocation( sh_prog1, "projection" ), 1, 0, projection.data );
                glUniform4f       ( glGetUniformLocation( sh_prog1, "Color" ), colo.r, colo.g, colo.b, colo.a );
            }
            vertex_buffer_render( cover_o, GL_TRIANGLES );
        }

        /* index texture from i_apps array */
        int tex_idx = g_idx + j;
        // check bounds: skip first
        if( tex_idx  < 1)
            tex_idx += i_apps[0].token_c;
        else
        if( tex_idx  > i_apps[0].token_c)
            tex_idx -= i_apps[0].token_c;


        if (i_apps[0].token_c < 4) msgok(FATAL, "This Game Launcher has a Requirement of min. 4 Apps\nTo use it you need to install: %i More apps\n", i_apps[0].token_c - 4);

        if (Download_icons)
        { 
            loadmsg("Downloading Covers from the server");

            for (int i = 1; i <= i_apps[0].token_c; i++)
                download_texture(i);

            Download_icons = false;
            sceMsgDialogTerminate();
            log_info("i_apps[0].token_c: %i", i_apps[0].token_c);
        }
        // load textures (once)
        check_n_load_texture(tex_idx);
        // draw coverbox, if not avail craft coverbox from icon
        check_n_draw_texture( tex_idx, +1, colo );


    // reflection option
    if(use_reflection)
    {   /* reflect on the Y, actual position matters! */
        mat4_scale( &model, 1, -1, 1 );

        // draw coverbox reflection, if not avail craft coverbox from icon
        check_n_draw_texture( tex_idx, -1, colo );
    }
        /* set title */
        if(! j           // last drawn one, the selected in the center
        && ! title_vbo ) // we cleaned VBO
        {
            char    tmp[128];

            //       vec2 pen = (vec2){ (float)l->bound_box.x - 20., resolution.y - 140. };

            item_t *li = &i_apps[ tex_idx ];

            title_vbo  = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
    // NAME, is size checked
            size_t len = strlen(li->token_d[NAME].off);
            if(len < 30)
                strcpy(&tmp[0], li->token_d[NAME].off);
            else
                snprintf(&tmp[0], 127, "%.26s..." , li->token_d[NAME].off);

            // we need to know Text_Length_in_px in advance, so we call this:
            texture_font_load_glyphs( titl_font, &tmp[0] );

            // Default for ITEMzFlow enum
            vec2 pen = (vec2) { (resolution.x - tl) /2., (resolution.y /10.) *2. };
            //Game Options Pen
            vec2 pen_game_options = (vec2){ 100., resolution.y - 150. };

            if (v_curr == ITEM_PAGE)
              add_text( title_vbo, titl_font, &tmp[0], &col, &pen_game_options);
            else // fill the vbo
              add_text( title_vbo, titl_font, &tmp[0], &col, &pen);
                          
            // ID
            // we need to know Text_Length_in_px in advance, so we call this:
            texture_font_load_glyphs( sub_font, li->token_d[ID].off);
            // align text centered
              
              vec4 c = col * .75f; // 75% of default fg color

              // Default for ITEMzFlow enum
              pen.x  = (resolution.x - tl) /2.,
              pen.y -= 32;
              
              if (v_curr == ITEM_PAGE){
                pen_game_options.x  += 10.;
                // fill the vbo
                 add_text( title_vbo, sub_font, li->token_d[ID].off, &c, &pen_game_options);
              }
              else // fill the vbo
                  add_text( title_vbo, sub_font, li->token_d[ID].off, &c, &pen);
                 
            
            // we eventually added glyphs... (todo: glyph cache)
            refresh_atlas();
        }

        // ...

        // count: index is outer (+R, -L) to inner!
        c += 1;

        // flip sign, get mirrored
        if(j > 0) { j *= -1; goto draw_item; }

        // last one drawn: break loop!
        if( !j ) goto all_drawn;
    }
    // last one is 0: the selected item
    if( !i ) { j = 0; goto draw_item; }

all_drawn:

    glDisable(GL_BLEND);
    glUseProgram(0);

    switch(v_curr) // draw more things, per view
    {
        case ITEMzFLOW:
            if(pr_dbg)
            {
                log_info("%d,\t%s\t%s", g_idx,
                                i_apps[ g_idx ].token_d[ NAME ].off,
                                i_apps[ g_idx ].token_d[ ID   ].off);
            }
            draw_additions();
            // texts out of VBO
            ftgl_render_vbo(title_vbo, NULL);
            break;
        // add menu
        case ITEM_PAGE:
            GLES2_render_list(0);       // v2
//          GLES2_draw_new_test4( (int)u_t % icon_panel->item_c ); // one from cache
            ftgl_render_vbo(title_vbo, NULL); break;
            break;
    }

    pr_dbg = 0;
}

const char *gm_p_text[] =
{
    "Launch game",
    "Dump game",
    "Uninstall update",
    "Uninstall game",
    "Trainers"
};

