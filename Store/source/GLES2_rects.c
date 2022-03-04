/*
    mimic an SDL_Rect

    standard GLES2 code to DrawLines and FillRects

    2020, masterzorag
*/

#include "defines.h"
#include "shaders.h"

/* glsl programs */
static GLuint glsl_Program[NUM_OF_PROGRAMS];
static GLuint curr_Program;  // the current one

extern vec2   resolution;
static vec4   color = { 1., 0., .5, 1. }; // current RGBA color
// shaders locations
static GLint  a_position_location;
static GLint  u_color_location;
static GLint  u_time_location;
 // from main.c
extern double u_t;

/* translate px vector to normalized coord */
vec2 px_pos_to_normalized(vec2 *pos)
{
    return 2. / resolution * (*pos) - 1.;
}

void on_GLES2_Update1(double time)
{
    for(int i = 0; i < NUM_OF_PROGRAMS; ++i)
    {
        glUseProgram(glsl_Program[i]);
        // write the value to the shaders
        glUniform1f(glGetUniformLocation(glsl_Program[i], "u_time"), time);
    }

}

/*================= RECT SHADERS  =================================*/


void ORBIS_RenderFillRects_init(int width, int height)
{
    resolution = (vec2){ width, height };

    curr_Program = glsl_Program[0] = BuildProgram(rects_vs0, rects_fs0, rects_vs0_length, rects_fs0_length);
    curr_Program = glsl_Program[1] = BuildProgram(rects_vs1, rects_fs1, rects_vs1_length, rects_fs1_length);
    curr_Program = glsl_Program[2] = BuildProgram(rects_vs2, rects_fs2, rects_vs2_length, rects_fs2_length);

    for (int i = 0; i < NUM_OF_PROGRAMS; i++)
    {
        curr_Program = glsl_Program[i];

        glUseProgram(curr_Program);
        // gles2 attach shader locations
        a_position_location = glGetAttribLocation(curr_Program, "a_Position");
        u_color_location = glGetUniformLocation(curr_Program, "u_color");
        u_time_location = glGetUniformLocation(curr_Program, "u_time");
    }
    
    curr_Program = glsl_Program[0];
    // select for setup
    glUseProgram(curr_Program);
    // reshape
    glViewport(0, 0, width, height);
}
/* ================================= =========================*/


void ORBIS_RenderFillRects_fini(void)
{
    for(int i = 0; i < NUM_OF_PROGRAMS; ++i)
    {
        if (glsl_Program[i]) glDeleteProgram(glsl_Program[i]), glsl_Program[i] = 0;
    }
}


// takes point count
void ORBIS_RenderDrawLines(//SDL_Renderer *renderer,
    const vec2 *points, int count)
{
    GLfloat vertices[4];

    glUseProgram(curr_Program);
    // enable alpha
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* emit a line for each point pair */
    for (int i = 0; i < count; i += 2) {
#if 0
        log_info("%d, %d, %d %.3f,%.3f,%.3f,%.3f", 
            idx, count, count/2,
            points[idx   ].x, points[idx   ].y,
            points[idx +1].x, points[idx +1].y);
#endif
        /* (x, y) for 2 points: 4 vertices */
        vertices[0] = points[i   ].x;  vertices[1] = points[i   ].y;
        vertices[2] = points[i +1].x;  vertices[3] = points[i +1].y;
        /* each (vec2)point comes from pairs of floats */
        glVertexAttribPointer    (a_position_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(a_position_location);
        /* write color to use to the shader location */
        glUniform4f(u_color_location, color.r, color.g, color.b, color.a);
        /* floats pairs for points from 0-4 */
        glDrawArrays(GL_LINES, 0, 2);
    }
    // revert state back
    glDisable(GL_BLEND);
    // release program
    glUseProgram(0);
}


/* draw a white box around selected rect */
void ORBIS_RenderDrawBox(enum SH_type SL_program, const vec4 *rgba, const vec4 *r)
{
    if(rgba) color = *rgba;
    // backup current color
    vec4 curr_color = color;
    // a box is 4 segments joining 2 points
    vec2 b[4 * 2];
    // 1 line for each 2 points
    b[0] = r->xy,  b[1] = r->xw;
    b[2] = r->xw,  b[3] = r->zw;
    b[4] = r->zw,  b[5] = r->zy;
    b[6] = r->zy,  b[7] = r->xy;
    // select shader to use
    curr_Program = glsl_Program[SL_program];
    // gles render all lines
    ORBIS_RenderDrawLines(&b[0], 8);
    // restore current color
    color = curr_color;
}

/* main function to draw FillRects */
void ORBIS_RenderFillRects(enum SH_type SL_program, const vec4 *rgba, const vec4 *rects, int count)
{
    if(rgba) color = *rgba;
    // (4 float pairs!)
    GLfloat vertices[8];
    // select shader to use
    curr_Program = glsl_Program[SL_program];

    glUseProgram(curr_Program);
    //glDisable(GL_CULL_FACE);
    // enable alpha
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* emit a triangle strip for each rectangle */
    for(int i = 0; i < count; ++i)
    {
        const vec4 *rect = &rects[i];

        GLfloat xMin = rect->x,  xMax = rect->z,
                yMin = rect->y,  yMax = rect->w;
        /* (x, y) for 4 points: 8 */
        vertices[0] = xMin;  vertices[1] = yMin;
        vertices[2] = xMax;  vertices[3] = yMin;
        vertices[4] = xMin;  vertices[5] = yMax;
        vertices[6] = xMax;  vertices[7] = yMax;
        /* each (vec2) point comes from pairs of floats */
        glVertexAttribPointer    (a_position_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(a_position_location);
        /* write color to use to the shader location */
        glUniform4f(u_color_location, color.r, color.g, color.b, color.a);
        /* floats pairs for points from 0-4 */
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    // revert state back
    glDisable(GL_BLEND);
    // release VBO, texture, program, ...
    glUseProgram(0);
}

/* draw filling color bar */
void GLES2_DrawFillingRect(vec4 *r, // float/normalized rectangle
                           vec4 *c, // normalized color
                double *percentage) // how much filled from left
{   // shrink frect RtoL by_percentage
    r->z = r->x + (r->z - r->x) * (*percentage / 100.f);
//  log_info( "%.2f %.2f, %.2f, %.2f %f", r.x, r.y, r.z, r.w, dfp);
    ORBIS_RenderFillRects(USE_COLOR, c, r, 1);
}

/* older UI version code, for reference */

#if 0
extern int    selected_icon;
extern ivec4  rela_pos;

typedef vec2  my_Point;
typedef vec4  my_FRect; // ( p1.xy, p2.xy )

#define COUNT  8
vec4 r[COUNT];


// old way, tetris uses it
vec4 px_pos_to_normalized2(vec2 *pos, vec2 *size)
{
    vec4 n; // 2 points .xy pair: (x, y),  (x + texture.w, y + texture.h)

    n.xy  = 2. / resolution * (*pos) - 1.; // (-1,-1) is BOTTOMLEFT, (1,1) is UPRIGHT
    n.zw  = 2. / resolution * (*size);
    n.yw *= -1.; // flip Y axis!
//  log_info("%f,%f,%f,%f", n.x, n.y, n.w, n.w);
    return n;
}

// used in line_and_rects sample
void ORBIS_RenderFillRects_rndr(void)
{
    vec2 p, s;
    // fill in some pos, and size
    for (int i = 0; i < COUNT; ++i)
    {
        // position in px
        p = (vec2) { 100. + i * (100. + 2. /*border*/),  
                     100. + 4 *  20. };
        /* convert to normalized coordinates */
        r[i].xy = px_pos_to_normalized(&p);
        // size in px
        s  = (vec2) { 100.,  100. };
        // turn size into destination point!
        s += p;
        /* convert to normalized coordinates */
        r[i].zw = px_pos_to_normalized(&s);
    }
    // gles render all rects
    ORBIS_RenderFillRects(USE_UTIME, NULL, r, COUNT);

#if defined TETRIS
    // test tetris primlib
    filledRect(1000, 400, 1010, 410, 255, 0, 0); // p1, p2
#endif

    /* now test a line, same color */
    my_Point q[2];
    q[0].x = -.4, q[0].y = .4;
    q[1].x = -.2, q[1].y = .6;
    ORBIS_RenderDrawLines(&q[0], 2);

    /* ... */
}


/* pos = rect.xy, size = rect.zw */
void ORBIS_RenderSubMenu(int num)
{
    // save current color and set another one
    vec4 curr_color = color;
    vec4 r;

    switch(num)
    {
        default: /* skip */ goto step2;

        case ON_LEFT_PANEL :
        case ON_MAIN_SCREEN: {
            color = (vec4) { .2578, .2578, .2578, 1. };
            // a 2 px rect, not a vertical line!
            r = (vec4) { 500, 0,  3, resolution.y }; }
        break;
        case ON_SUBMENU:
        case ON_SUBMENU_2:
        // a background alpha blended rectangle
        color = (vec4) { 0., 0., 0., .8 };
        // position and size in px
        r = (vec4){ (resolution.x - 640) /2, // pos x (center)
                    (resolution.y - 700) /2, // pos y (center)
                    640, 700 };  // size w, h
        break;
        case ON_ITEM_PAGE:
        r = (vec4){ 600, // pos x
                    (resolution.y - 640) /2, // pos y (center)
                    700, 640 };  // size w, h
        break;
    }
    // rectangle: position, size
    vec2 p = r.xy,
         s = r.xy + r.zw;
    /* convert to normalized coordinates */
      r.xy = px_pos_to_normalized(&p);
      r.zw = px_pos_to_normalized(&s);
    // gles render the frect
    ORBIS_RenderFillRects(USE_COLOR, NULL, &r, 1);

    // draw selection rectangle, use normalized coordinates!
    vec4 b = r;
    b.yw /= 10;
    switch(num)
    {
        default: /* skip the box */ break;

        case ON_LEFT_PANEL: {
        //color = (vec4) { 0., 0., 0., .8 };
        r    = (vec4) { 0, 900,  499, 100 };
        r.y -= rela_pos.y * 86;
        p    = r.xy,
        s    = r.xy + r.zw;
        /* convert to normalized coordinates */
        r.xy = px_pos_to_normalized(&p);
        r.zw = px_pos_to_normalized(&s);
        // gles render the frect
        ORBIS_RenderFillRects(USE_COLOR, NULL, &r, 1);
        // reuse same vbo from main screen
        num = ON_MAIN_SCREEN;
        } break;
        case ON_MAIN_SCREEN: {
        /*    color = (vec4) { .2578, .2578, .2578, 1. };
            // a 2 px rect, not a vertical line!
            r = (vec4) { -.500, 1.,   -.505, -1. };
            ORBIS_RenderFillRects(USE_COLOR, &r, 1); */
        } break;
        /* compute the selection box (shrink vertically to position) */
        case ON_SUBMENU:   {
        b.yw -= r.yy
              + (b.w - b.y) /2.
              + (b.w - b.y)
              * ( 2   // start from
                + (2  // even/odd items
                   * abs(rela_pos.y %4))); // 4 choices
        } break;
        case ON_SUBMENU_2: {
        b.yw -= r.yy
              + (b.w - b.y) /2.
              + (b.w - b.y)
              * ( 1   // start from
                + (1  // all items
                   * abs(rela_pos.y %3))); // 3 choices
        } break;
        case ON_ITEM_PAGE: {
        b.yw -= r.yy
              + (b.w - b.y) /2.  // half bar height
              + (b.w - b.y)      // one bar height
              * 9;               // start from

        /* the alpha blended box under the app icon */
        // position and size in px
        vec4 r = { 200, 200, 400, 250 };
        vec2 p = r.xy,
             s = r.xy + r.zw;  // turn size into the second point
        // compute the normalized frect
          r.xy = px_pos_to_normalized(&p);
          r.zw = px_pos_to_normalized(&s);
        // gles render our rect
        ORBIS_RenderFillRects(USE_COLOR, NULL, &r, 1);

        // recompute the selection box from this rect
        b = r;
        vec4 o = (vec4){ .055f, .16f, -.055f, -.21f };
        b += o;
        //b    *= .9f;
        //b.yw -= (b.w - b.y);
              //+ (b.w - b.y) *1;

        // Download button
        //GLES2_render_submenu_text(0);
        } break;
    }
    // revert back previous current color
    color = curr_color;

    // the glowing selection box
//    ORBIS_RenderDrawBox(&b);

step2:
    // texts
    GLES2_render_submenu_text(num);
}
#endif

