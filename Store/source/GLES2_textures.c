/*
    GLES2 texture from png using shaders and VBOs

    basic direct correlation: buffer[num] positions texture[num]

    2019, masterzorag
*/

#include <stdio.h>
#include <math.h>

#include "defines.h"

#include "json.h"

#include "icons_shaders.h"
/*
    we setup two SL programs:
    0. use default color from texture
    1. use glowing effect on passed time

enum GLSL_programs
{
    TEXTURED,
    GLOWING,
    NUM_OF_PROGRAMS
};
*/

/* shared programs */
static GLuint glsl_Program[NUM_OF_PROGRAMS];
static GLuint curr_Program;  // the current one

//static GLuint texture[NUM_OF_TEXTURES];
//static GLuint buffer [NUM_OF_TEXTURES];
#define BUFFER_OFFSET(i) ((void*)(i))


// my_Frect position(.xy) and size (.zw)
static vec4 rects[NUM_OF_TEXTURES];

// the png we apply glowing effect by switching shader
extern int    selected_icon;
extern ivec4  menu_pos;

// shaders locations
static GLint a_position_location;
static GLint a_texture_coordinates_location;
static GLint u_texture_unit_location,
             u_time_location;

       vec2 resolution;  // (shared and constant!)

// fill all our "rects" info with normalized coordinates
static void setup_texture_position(int i, vec2 pos, const float scale_f)
{
    vec2 p, s;
    /* fill in some pos, and size */
    //for (int i = 0; i < NUM_OF_TEXTURES; ++i)
    {   // position in px
        p = (vec2) { 100. + i * (100. + 2. /*border*/),  
                     100. + 4 *  20. },
        /* convert to normalized coordinates */
        rects[i].xy = px_pos_to_normalized(&p);
        // size in px
        s  = (vec2) { 100.,  100. };
        // turn size into a second point
        s += p;
        /* convert to normalized coordinates */
        rects[i].zw = px_pos_to_normalized(&s);
    }
}

//https://github.com/learnopengles/airhockey/commit/228ce050da304258feca8d82690341cb50c27532
//OpenGLES2 handlers : init , final , update , render , touch-input
void on_GLES2_Init_icons(int view_w, int view_h)
{
    resolution = (vec2){ view_w, view_h }; // setup resolution for next setup_texture_position()

    for(int i = 0; i < NUM_OF_TEXTURES; i++)
    {
//        texture[i] = load_png_asset_into_texture(pngs[i]);
//        if(!texture[i])
//            debugNetPrintf(DEBUG, "load_png_asset_into_texture '%s' ret: %d\n", pngs[i], texture[i]);

        // fill Frects with positions and sizes
        setup_texture_position( i, (vec2){ i *100, 480 /*ATTR_ORBISGL_HEIGHT*/ /2 }, 0.25 /* scale_f */);
    }

    for(int i = 0; i < NUM_OF_PROGRAMS; i++)
    {
        glsl_Program[i] =
        #if 1 // defined HAVE_SHACC
            BuildProgram(simpleVertexShader[0], simpleFragmentShader[i]);
        #else
            CreateProgramFromBinary(i);
        #endif
        debugNetPrintf(DEBUG, "glsl_Program[%d]: %u\n", i, glsl_Program[i]);
        // make program the current one
        curr_Program = glsl_Program[i];

        glUseProgram(curr_Program);
        a_position_location            = glGetAttribLocation (curr_Program, "a_Position");
        a_texture_coordinates_location = glGetAttribLocation (curr_Program, "a_TextureCoordinates");
        u_texture_unit_location        = glGetUniformLocation(curr_Program, "u_TextureUnit");
        u_time_location                = glGetUniformLocation(curr_Program, "u_time");
    }
    // reset to default one
    curr_Program = glsl_Program[USE_COLOR];
    // reshape
    glViewport(0, 0, view_w, view_h);
}


void on_GLES2_Final(void)
{
    for(int i = 0; i < NUM_OF_PROGRAMS; ++i)
        { if(glsl_Program[i]) glDeleteProgram(glsl_Program[i]), glsl_Program[i] = 0; }
}


void on_GLES2_Update(double time)
{
    float t = (float)time;
    for(int i = 0; i < NUM_OF_PROGRAMS; ++i)
    {
        glUseProgram(glsl_Program[i]);
        // write the value to the shaders
//printf("%s pro:%d glUniform1f before.\n", __FUNCTION__, i);
        glUniform1f(glGetUniformLocation(glsl_Program[i], "u_time"), t);
//printf("%s pro:%d glUniform1f after.\n", __FUNCTION__, i);
    }
//printf("time:%.6f\n", time);
}


void on_GLES2_Render_icon(GLuint texture, int num, vec4 *frect) // which texture to draw
{
    if(texture == 0 || !frect) return;

    // we already clean

    // set default shader
    curr_Program = glsl_Program[USE_COLOR];
    // change shader if item selected
//    if(num == selected_icon) curr_Program = glsl_Program[GLOWING];

    glUseProgram(curr_Program);

    glDisable(GL_CULL_FACE);
    // enable alpha for png textures
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // select requested texture
    glActiveTexture(GL_TEXTURE0 + num);
    glBindTexture  (GL_TEXTURE_2D, texture);
    glUniform1i    (u_texture_unit_location, num);  // tell to shader

    // setup positions
    //const vec4 *rect = &rects[num];
    const vec4 *rect = frect;
/*
    vec4 r;// = (vec4) { .2, .2, .3, .3 };
    if(num == selected_icon)
    {
        r    = *rect;
        //r   *= 1.15;
        rect = &r;
    }
*/
    GLfloat  xMin = rect->x,  xMax = rect->z,
             yMin = rect->y,  yMax = rect->w;
    //printf("%d: %.f %.f %.f %.f\n", num, xMin, xMax, yMin, yMax);
    /* (x, y) for 4 points: 8 vertices */
    const float v_pos[] = { xMin, yMin,   // TPLF
                            xMin, yMax,   // BTLF
                            xMax, yMin,   // BTRG
                            xMax, yMax }; // TPRG
    const float v_tex[] = { 0.f,  1.f,    // TPLF
                            0.f,  0.f,    // BTLF
                            1.f,  1.f,    // BTRG
                            1.f,  0.f  }; // TPRG
    // setup attr
    glVertexAttribPointer(a_position_location,
        2, GL_FLOAT, GL_FALSE, 0, v_pos);
    glVertexAttribPointer(a_texture_coordinates_location,
        2, GL_FLOAT, GL_FALSE, 0, v_tex);
    // pin variables
    glEnableVertexAttribArray(a_position_location);
    glEnableVertexAttribArray(a_texture_coordinates_location);
    // draw from arrays buffer
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // revert state back
    glDisable(GL_BLEND);
    // release VBO, texture and program
    //glActiveTexture(0); // error on piglet !!
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    // we already flip/swap
}

// UI: pngs
static vec4 selection_box;
static int  to_ani = 0;
void on_GLES2_Render_icons(page_info_t *row)
{
    if(!row) return;

    vec4 r;
    vec2 p, s;
    for(int i = 0; i < NUM_OF_TEXTURES; i++)
    {
#if 1
       // position, size in px
        p = row->item[i].origin;
        s = row->item[i].frect.zw;
        // turn size into the second point
        s += p;
        // compute the normalized frect
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);
        // save the currently selected frect!
        if(i == selected_icon
        &&(((row->page_num -1) %2) == menu_pos.y %2)
        ) selection_box = r;
        // render texture using normalized frect
        on_GLES2_Render_icon(row->item[i].texture, i, &r);
#else
        // take px coordinates
        p = row->item[i].origin;
        s = p
          + (vec2) ( 128. );
        // do something
        s *= sin(to_ani++);
        // compute the normalized frect
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);
        // update the item frect and draw
        row->item[i].frect = r;
        on_GLES2_Render_icon(row->item[i].texture, i, &row->item[i].frect);
#endif
    }
}

void on_GLES2_Render_box(vec4 *frect)
{
    vec4 white = (vec4)( 1. );
    ORBIS_RenderDrawBox(USE_UTIME, &white, &selection_box);
}
