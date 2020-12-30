/*
    shader_common.c

    GLES2 shaders facilities about loading and building shaders

    2019, 2020, masterzorag
*/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <unistd.h> // sleep

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <orbisGl.h>
#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO

#else // on linux

#include <stdio.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#define  debugNetPrintf  fprintf
#define  ERROR           stderr
#define  DEBUG           stdout
#define  INFO            stdout

//#include "defines.h"

#endif

GLuint create_vbo(const GLsizeiptr size, const GLvoid* data, const GLenum usage)
{
    assert(data != NULL);
    GLuint vbo_object;
    glGenBuffers(1, &vbo_object);
    assert(vbo_object != 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_object);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vbo_object;
}

static GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vertexShader);
    glAttachShader(programHandle, fragmentShader);
    glLinkProgram (programHandle);

    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE)
        { fprintf(ERROR, "GL_LINK_STATUS error!\n"); sleep(2); }

    /* once we have the SL porgram, don't leak any shader! */
    if(vertexShader)   { glDeleteShader(vertexShader),   vertexShader   = 0; }
    if(fragmentShader) { glDeleteShader(fragmentShader), fragmentShader = 0; }

    return programHandle;
}

static GLuint BuildShader(const char *source, GLenum shaderType)
{
    GLuint shaderHandle = glCreateShader(shaderType);

    glShaderSource (shaderHandle, 1, &source, 0);
    glCompileShader(shaderHandle);
    GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);

    if (compileSuccess == GL_FALSE)
    {
        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
        fprintf(ERROR, "compile glsl error : %s\n", messages);
    }
    return shaderHandle;
}

GLuint BuildProgram(const char *vShader, const char *fShader)
{
    GLuint vertexShader   = BuildShader(vShader, GL_VERTEX_SHADER);
    GLuint fragmentShader = BuildShader(fShader, GL_FRAGMENT_SHADER);
    /* dump those now, in .sb form
    DumpShader(vertexShader,   "vShader");
    DumpShader(fragmentShader, "fShader");
    */
    return LinkProgram(vertexShader, fragmentShader);
}
