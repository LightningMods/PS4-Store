/*
    shader_common.c

    GLES2 shaders facilities about loading and building shaders

    2019-21, masterzorag
*/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <unistd.h> // sleep

#include "defines.h"

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
        { log_error( "GL_LINK_STATUS error!"); programHandle = 0; }

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
        log_error( "compile glsl error : '%s', '%s'", messages, source);
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
