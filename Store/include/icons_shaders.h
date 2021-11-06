/*
    icons_shaders.h

    shaders used in icons.c

    - one shared vertex shader:
      locations for added: offset (unused), u_time

    - two fragments shaders:
      1. use default texture color (no change)
      2. use time to apply glow effect to all color components (fade in/out effect)
*/
#define MAX_SL_PROGRAMS  (3)

/// 1. Vertex shaders
static GLchar *simpleVertexShader[ MAX_SL_PROGRAMS ] =
{
 /// 1. default textured vertex
    "precision mediump float;"
    "attribute vec4  a_Position;"
    "attribute vec2  a_TextureCoordinates;"
    "varying   vec2  v_TextureCoordinates;"
    "uniform   vec2  u_offset;"
    "uniform   float u_time;"
    "void main(void)"
    "{"
    "  v_TextureCoordinates = a_TextureCoordinates;"
    "  gl_Position          = a_Position;"
    "}"
    ,
    /// 2.
    "precision mediump float;"
    "attribute vec4  a_Position;"
    "attribute vec2  a_TextureCoordinates;"
    "varying   vec2  v_TextureCoordinates;"
    "uniform   vec2  u_offset;"
    "uniform   float u_time;"
    "void main(void)"
    "{"
    "  v_TextureCoordinates = a_TextureCoordinates;"
    "  gl_Position          = a_Position;"
    "}"
    ,
    /// 3. CUSTOM_BADGE
    "precision mediump float;"
    "attribute vec4  a_Position;"
    "attribute vec2  a_TextureCoordinates;"
    "varying   vec2  v_TextureCoordinates;"
    "uniform   vec2  u_offset;"
    "uniform   float u_time;"
    "void main(void)"
    "{"
    "  v_TextureCoordinates = a_TextureCoordinates;"
    "  gl_Position          = a_Position;"
    "}"
} ;

/// 2. Fragment shaders
static GLchar *simpleFragmentShader[ MAX_SL_PROGRAMS ] =
{
 /// 1. default textured fragment
    "precision mediump float;"
    "uniform   sampler2D u_TextureUnit;"
    "varying   vec2      v_TextureCoordinates;"
    "uniform   float     u_time;"
    "void main(void)"
    "{"
    "  gl_FragColor = texture2D(u_TextureUnit, v_TextureCoordinates);"
    "}"
    ,
 /// 2. fade in/out effect by time, like glowing
    "precision mediump float;"
    "uniform   sampler2D u_TextureUnit;"
    "varying   vec2      v_TextureCoordinates;"
    "uniform   float     u_time;"
    "void main(void)"
    "{"
    "  gl_FragColor    = texture2D(u_TextureUnit, v_TextureCoordinates);"
    "  gl_FragColor.a *= abs(sin(u_time));"
    "}"
    ,
 /// 3. CUSTOM_BADGE
    "precision mediump float;"
    "uniform   sampler2D u_TextureUnit;"
    "varying   vec2      v_TextureCoordinates;"
    "uniform   float     u_time;"
    "void main(void)"
    "{"
    "  gl_FragColor    = texture2D(u_TextureUnit, v_TextureCoordinates);"
    "  gl_FragColor.r += abs( cos(u_time *1.) * sin(u_time *.5) /4. );"
    "}"
} ;

