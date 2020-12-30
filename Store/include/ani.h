/*
  implement some way to setup different kind of
  animations to use in print text with FreeType

  scope: reusing and sharing resources from the
  already available freetype-gl library

  2020, masterzorag
*/
//#include <freetype-gl.h>


// -------------------------------------------------------------- effects ---


/* each fx have those states */
enum ani_states
{
    ANI_CLOSED,
    ANI_IN,
    ANI_DEFAULT,
    ANI_OUT
};

enum ani_type_num
{
    TYPE_0,
    TYPE_1,
    TYPE_2,
    TYPE_3,
    MAX_ANI_TYPE
};

/* hold the current state values */
typedef struct
{
// GLuint program;
    int   status, // current ani_status
          fcount; // current framecount (depr.)

    float t_now,   // current time
          t_life;  // total duration
} fx_entry_t;


static fx_entry_t fx_entry[MAX_ANI_TYPE];

// vertex shaders
static GLchar *vs_s[2] = 
{   // v3f-c4f vertex  shader
    "precision mediump float;"
    "uniform   mat4  model;"
    "uniform   mat4  view;"
    "uniform   mat4  projection;"
    ""
    "attribute vec3  vertex;"
    "attribute vec4  color;"
    "varying   vec4  fragColor;"
    ""
    "void main(void){"
    "    fragColor   = color;"
    "    gl_Position = projection*(view*(model*vec4(vertex,1.0)));"
    "}"
    ,
    // placeholder
    ""
} ;

// fragment shaders
static GLchar *fs_s[2] = 
{   // lightpoint.frag (download_panel bg)
    "precision mediump float;"
    ""
    "uniform float time;"
    "uniform vec2  mouse;"
    "uniform vec2  resolution;"
    ""
    "void main( void )"
    "{"
    "   vec2 vu = ( 2.* gl_FragCoord.xy - resolution.xy ) / resolution.y;"
    "   float c = 0.0;"
    "   for(float i = 0.; i < 1.; i++){"
    "     c += abs( 1. / sin( vu.y + 0.132 + sin( i * .0103 + vu.x * 3.3 + time * .30003 -i * .134 ) * cos( vu.x + time * .10001 ) * sin( vu.x + time * .5000001 + i * .12) * cos( i * .041 )  ) );"
    "   }"
    "   vec3 kek = vec3(0.5,0.5,0.5) / dot(vu, vu) / 6.0;"
    "   float x = kek.r / (kek.r + 1.);"
    "   gl_FragColor = vec4(0.);"
    "   gl_FragColor.a = 1. - x;"
    "}"
    ,
    // placeholder
    "precision mediump float;\
    void main( void )\
    {\
        \
    }"
};