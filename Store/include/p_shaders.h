/*
  shaders used in pixeshader.c

  2020, masterzorag
*/

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
