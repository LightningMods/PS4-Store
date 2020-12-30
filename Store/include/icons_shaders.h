/*
    icons_shaders.h

    shaders used in icons.c

    - one shared vertex shader:
      locations for added: offset (unused), u_time

    - two fragments shaders:
      1. use default texture color (no change)
      2. use time to apply glow effect to all color components (fade in/out effect)
*/


/// 1. Vertex shaders
static const char *simpleVertexShader[2] =
{
 /// 1. shared between 2 SL programs
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
//  "  gl_Position.xy      *= vec2(abs(sin(u_time)));" /* zoom in/out effect */
    "} "
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
    "  gl_Position.w -= ( abs(sin(u_time)) /50. );"
  //"  gl_Position.xy      *= 1.1;"//vec2(abs(sin(u_time)));" /* zoom in/out effect */
    "} "
} ;

/// 2. Fragment shaders
static const char *simpleFragmentShader[2] =
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
    "  gl_FragColor.a *= abs(sin(u_time));" /* fade in/out effect */
    "}"
} ;

