/* Freetype GL - A C OpenGL Freetype engine
 *
 * Distributed under the OSI-approved BSD 2-Clause License.  See accompanying
 * file `LICENSE` for more details.
 */

/*
    a simple shader player using freetype-gl VBO helpers
*/

#include "defines.h"

/* glslsandbox uniforms
uniform float time;
uniform vec2  resolution;
uniform vec2  mouse;
*/


// ------------------------------------------------------- typedef & struct ---
typedef struct {
    float x, y, z;    // position (3f)
    float r, g, b, a; // color    (4f)
} vertex_t;

// ------------------------------------------------------- global variables ---
static vertex_buffer_t *rects_buffer;
static GLuint shader,
           mz_shader,
        curr_Program;  // the current one
static mat4 model, view, projection;
//static GLuint g_TimeSlot = 0;
static vec2 resolution;

// from main.c
extern double u_t;
extern vec2 p1_pos;  // unused yet

// ---------------------------------------------------------------- display ---
void pixelshader_render( GLuint program_i )
{
    curr_Program = mz_shader;

    if(program_i) { printf("unimplemented yet\n"); return; }

    glUseProgram( curr_Program );
    {
        // enable alpha channel
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // notify shader about screen size, mouse position and actual cumulative time
        glUniform2f       ( glGetUniformLocation( curr_Program, "resolution" ), resolution.x, resolution.y);
        glUniform2f       ( glGetUniformLocation( curr_Program, "mouse" ),      p1_pos.x, p1_pos.y);
        glUniform1f       ( glGetUniformLocation( curr_Program, "time" ),       u_t); // notify shader about elapsed time
        // ft-gl style: MVP
        glUniformMatrix4fv( glGetUniformLocation( curr_Program, "model" ),      1, 0, model.data);
        glUniformMatrix4fv( glGetUniformLocation( curr_Program, "view" ),       1, 0, view.data);
        glUniformMatrix4fv( glGetUniformLocation( curr_Program, "projection" ), 1, 0, projection.data);
        // draw whole VBO items array: fullscreen rectangle
        vertex_buffer_render( rects_buffer, GL_TRIANGLES );

    glDisable(GL_BLEND);
    }
    glUseProgram( 0 );
}

// ---------------------------------------------------------------- reshape ---
static void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    mat4_set_orthographic( &projection, 0, width, 0, height, -1, 1);
}

// ------------------------------------------------------ from source shaders ---
#include "p_shaders.h"

static GLuint CreateProgramFE(int program_num)
{
    GLchar  *vShader = vs_s[0];
    GLchar  *fShader = fs_s[ program_num ];
    // use shader_common.c
    GLuint programID = BuildProgram(vShader, fShader);

    if (!programID) { fprintf(ERROR, "program creation failed!\n"); sleep(2); }
    
    return programID;
}

// ------------------------------------------------------------------- init ---
void pixelshader_init( int width, int height )
{
    resolution   = (vec2) { width, height };
    rects_buffer = vertex_buffer_new( "vertex:3f,color:4f" );
    vec4   color = { 1, 0, 0, 1 };
    float r = color.r, g = color.g, b = color.b, a = color.a;

#define TEST_1  (0)

#if TEST_1
    #warning "splitted rects pixelshader"
    vec2 pen = { 100, 100 };
    for (int i = 0; i < 10; ++i)
    {
        int  x0 = (int)( pen.x + i * 2 );
        int  y0 = (int)( pen.y + 20 );
        int  x1 = (int)( x0 + 64 );
        int  y1 = (int)( y0 - 64 );
#else
    #warning "one fullstreen pixelshader"
    int  x0 = (int)( 0 );
    int  y0 = (int)( 0 );
    int  x1 = (int)( width );
    int  y1 = (int)( height );

#endif
    /* VBO is setup as: "vertex:3f, color:4f */ 
    vertex_t vtx[4] = { { x0,y0,0,  r,g,b,a },
                        { x0,y1,0,  r,g,b,a },
                        { x1,y1,0,  r,g,b,a },
                        { x1,y0,0,  r,g,b,a } };
    // two triangles: 2 * 3 vertex
    GLuint   idx[6] = { 0, 1, 2,    0, 2, 3 };
    vertex_buffer_push_back( rects_buffer, vtx, 4, idx, 6 );

#if TEST_1
        pen     += (vec2){ 72., -32. };
        color.g -= i * 0.1; // to show some difference: less green
    }
#endif
    /* compile, link and use shader */
    /* load from file */
    mz_shader = CreateProgramFE(0);
    // feedback
    fprintf(INFO, "[%s] program_id=%d (0x%08x)\n", __FUNCTION__, mz_shader, mz_shader);

    mat4_set_identity( &projection );
    mat4_set_identity( &model );
    mat4_set_identity( &view );

    reshape(width, height);
}

void pixelshader_fini( void )
{
    vertex_buffer_delete(rects_buffer);    rects_buffer = NULL;

    if(shader)    glDeleteProgram(shader),    shader    = 0;
    if(mz_shader) glDeleteProgram(mz_shader), mz_shader = 0;
}