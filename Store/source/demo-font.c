/* ============================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * ----------------------------------------------------------------------------
 * Copyright 2011,2012 Nicolas P. Rougier. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLAS P. ROUGIER ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLAS P. ROUGIER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Nicolas P. Rougier.
 * ============================================================================
 *
 * Example showing regular font usage
 *
 * ============================================================================
 */
#include <stdio.h>
#include <string.h>

#include <freetype-gl.h>  // links against libfreetype-gl
#include <user_mem.h> 

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <utils.h>
#include <debugnet.h>
#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO


#else // on pc

#include <stdio.h>
#define  debugNetPrintf  fprintf
#define  ERROR           stderr
#define  DEBUG           stdout
#define  INFO            stdout
#define  msgok           printf
#define  FATAL           "FATAL"

#endif


#include "defines.h"
#include "shaders.h"

// ------------------------------------------------------- typedef & struct ---
typedef struct {
    float x, y, z;    // position (3f)
    float s, t;       // texture  (2f)
    float r, g, b, a; // color    (4f)
} vertex_t;

// then we can print them splitted
typedef struct {
    int offset;
    int count;
} textline_t;

/* basic indexing of lines/texts */
GLuint num_of_texts = 0;  // text lines, or num of lines/how we split entire texts
/* shared freetype-gl function! */
void add_text( vertex_buffer_t * buffer, texture_font_t * font,
               const char * text, vec4 * color, vec2 * pen );


#define MAX_NUM_OF_TEXTS  16
static textline_t   texts[MAX_NUM_OF_TEXTS];
static unsigned int framecount = 0; // to animate something
static unsigned int selected   = 0;
// ------------------------------------------------------- global variables ---
GLuint shader;

texture_atlas_t *atlas  = NULL;
vertex_buffer_t *buffer = NULL;
mat4             model, view, projection;

// ---------------------------------------------------------------- reshape ---
static void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    mat4_set_orthographic( &projection, 0, width, 0, height, -1, 1);
}

// ---------------------------------------------------------------- display ---
void render_text( void )
{
    // we already clean in main renderloop()!

    glUseProgram( shader );
    // setup state
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture  ( GL_TEXTURE_2D, atlas->id ); // rebind glyph atlas
    glDisable      ( GL_CULL_FACE );
    glEnable       ( GL_BLEND );
    glBlendFunc    ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    {
        glUniform1i       ( glGetUniformLocation( shader, "texture" ),    0 );
        glUniformMatrix4fv( glGetUniformLocation( shader, "model" ),      1, 0, model.data);
        glUniformMatrix4fv( glGetUniformLocation( shader, "view" ),       1, 0, view.data);
        glUniformMatrix4fv( glGetUniformLocation( shader, "projection" ), 1, 0, projection.data);

        if(0) /* draw whole VBO (storing all added texts) */
        {
            vertex_buffer_render( buffer, GL_TRIANGLES ); // all vbo
        }
        else /* draw a range/selection of indexes (= glyph) */
        {
            vertex_buffer_render_setup( buffer, GL_TRIANGLES ); // start draw
            
            int j = selected;
            if(framecount %120 == 0)  // sometime...
            {
                selected++;
                j = selected %num_of_texts;
                // selected = framecount %num_of_texts; // ... flip text
                selected = j;
            }
            //for(int j=0; j<num_of_texts; j+=1)  // draw all texts
            {
                // draw just text[j]
                // iterate each char in text[j]
                for(int i = texts[j].offset;
                        i < texts[j].offset + texts[j].count;
                        i += 1)
                { // glyph by one (2 triangles: 6 indices, 4 vertices)
                    vertex_buffer_render_item ( buffer, i );
                }
            }
            
            vertex_buffer_render_finish( buffer ); // end draw
            framecount++;
        }
    }

    glDisable( GL_BLEND );  // Reset state back
    glUseProgram( 0 );

    // we already swapframe in main renderloop()!
}

// --------------------------------------------------------------- add_text ---
void add_text( vertex_buffer_t * buffer, texture_font_t * font,
               const char * text, vec4 * color, vec2 * pen )
{
    if(!buffer) { log_error(buffer); return; }

    size_t i;
    float r = color->r, g = color->g, b = color->b, a = color->a;

    for( i = 0; i < strlen(text); ++i )
    {
        texture_glyph_t *glyph = texture_font_get_glyph( font, text + i );
        if( glyph != NULL )
        {
            float kerning = 0.0f;
            if(i > 0)
            {
                kerning = texture_glyph_get_kerning( glyph, text + i - 1 );
            }
            pen->x  += kerning;
            int   x0 = (int)( pen->x + glyph->offset_x );
            int   y0 = (int)( pen->y + glyph->offset_y );
            int   x1 = (int)( x0 + glyph->width );
            int   y1 = (int)( y0 - glyph->height );
            float s0 = glyph->s0;
            float t0 = glyph->t0;
            float s1 = glyph->s1;
            float t1 = glyph->t1;
            GLuint indices[6] = {0,1,2,   0,2,3}; // (two triangles)
            /* VBO is setup as: "vertex:3f, tex_coord:2f, color:4f" */
            vertex_t vertices[4] = { { x0,y0,0,   s0,t0,   r,g,b,a },
                                     { x0,y1,0,   s0,t1,   r,g,b,a },
                                     { x1,y1,0,   s1,t1,   r,g,b,a },
                                     { x1,y0,0,   s1,t0,   r,g,b,a } };
            vertex_buffer_push_back( buffer, vertices, 4, indices, 6 );
            pen->x += glyph->advance_x;
        }
    }
}
/* 
 my wrapper for standard ft-gl add_text(), but with
 added indexing of text to render then by selection!

 we append to default ft-gl VBOs, atlas and texture!
*/
static void my_add_text( vertex_buffer_t * buffer, texture_font_t * font,
                         char * text, vec4 * color, vec2 * pen,
                         textline_t *idx )
{   /* index text */
    idx->offset = vector_size( buffer->items );
    add_text( buffer, font, text, color, pen );  // set vertexes
    idx->count  = vector_size( buffer->items ) - idx->offset;
    num_of_texts++;
    /* report the item info */
    log_info( "item[%.2d] .off: %3d, .len: %2d, buffer glyph count: %3zu, '%s'",
            num_of_texts, idx->offset, idx->count, vector_size( buffer->items ), text );
}
// ------------------------------------------------------ freetype-gl shaders ---
static GLuint CreateProgram( void )
{
    GLuint programID = 0;
    /* we can use OrbisGl wrappers, or MiniAPI ones */
    /* else,
       include and links against MiniAPI library!
    programID = miniCompileShaders(s_vertex_shader_code, s_fragment_shader_code);
    */
    programID = BuildProgram(font_vs, font_fs, font_vs_length, font_fs_length);

    // feedback
    log_info( "program_id=%d (0x%08x)", programID, programID);

    return programID;
}


// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;
texture_font_t *font = NULL;
// ------------------------------------------------------------------- main ---
int es2init_text (int width, int height)
{
    size_t h; // text size in pt

    /* init page: compose all texts to draw */
    atlas  = texture_atlas_new( 1024, 1024, 1 );

    /* load .ttf in memory */
//  void *ttf = orbisFileGetFileContent( "/hostapp/fonts/zrnic_rg.ttf" );
    buffer    = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

    char *text = "this is a DKS Studios store homebrew";

    int tx = 20; // text position

    vec2 pen = { 100, 100 }; // init pen: 0,0 is lower left

//    vec4 black = { 0.1, 0.1, 0.4, 1.f }; // RGBA color
    vec4 white = { 1.f, 1.f, 1.f, 1.f }; // RGBA color
//    vec4 col   = { 1.f, 0.f, 0.4, 1.f }; // RGBA color


    font = texture_font_new_from_memory(atlas, 24, font_ttf, font_len);


    // 1.
    char *s = "coming before xmas :lol:";   // set text

    texture_font_load_glyphs( font, s );        // set textures
    pen.x = (width - tl);                       // use Text_Length to align pen.x
    pen.x = 0,
    pen.y = 10;
    // use outline
//  font->rendermode        = RENDER_OUTLINE_EDGE;
//  font->outline_thickness = .3f;

    my_add_text( buffer, font, s, &white, &pen, &texts[num_of_texts] );  // set vertexes

    // 2.
    s = "System Version: 0xdeadcafe";  // set text

    texture_font_load_glyphs( font, s );        // set textures
    pen.x  = (width - tl);                      // use Text_Length to align pen.x
    pen.y -= font->height;                      // 1 line down!
    pen.x = 10,
    pen.y = height - font->height;
    my_add_text( buffer, font, s, &white, &pen, &texts[num_of_texts] );  // set vertexes

    texture_font_delete( font );    // end with font, cleanup

#if 0
    {
        ttf  = orbisFileGetFileContent("/hostapp/fonts/razors.ttf");
        font = texture_font_new_from_memory(atlas, 36, ttf, _orbisFile_lastopenFile_size);

        char *s = "make#orbisdev";                // set text
        texture_font_load_glyphs( font, s );      // set textures
        pen.x = 200, pen.y = 200;                 // set position
        
        /*font->rendermode = RENDER_OUTLINE_POSITIVE;
        font->outline_thickness = 2;*/
        font->rendermode        = RENDER_OUTLINE_EDGE;
        font->outline_thickness = 1.;
        my_add_text( buffer, font, s, &black, &pen, &texts[num_of_texts] );  // set vertexes

        font->rendermode        = RENDER_OUTLINE_EDGE;
        font->outline_thickness = 1.2;
        pen.x -= 100;
        my_add_text( buffer, font, s, &col, &pen, &texts[num_of_texts] );  // set vertexes

        texture_font_delete( font );   // end with font, cleanup
    }
#endif

//    ttf   = orbisFileGetFileContent("/hostapp/fonts/zrnic_rg.ttf");
    pen.y = 400;
    for( h=16; h < 24; h += 2)
    {

        font = texture_font_new_from_memory(atlas, h, font_ttf, font_len);
        pen.x  = tx;
        pen.y -= font->height;  // reset pen, 1 line down

        texture_font_load_glyphs( font, text );
        
        pen.x = (width - tl) /2;  // use Text_Length to center pen.x

        my_add_text( buffer, font, text, &white, &pen, &texts[num_of_texts] );  // set vertexes

        texture_font_delete( font );   // end with font, cleanup
    }

    /* create texture and upload atlas into gpu memory */
    glGenTextures  ( 1, &atlas->id );
    glBindTexture  ( GL_TEXTURE_2D, atlas->id );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D   ( GL_TEXTURE_2D, 0, GL_ALPHA, atlas->width, atlas->height,
                                    0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas->data );
    // compile, link and use shader
    shader = CreateProgram();
                                  
    if(!shader) { msgok(FATAL, "%s %x", getLangSTR(FAILED_W_CODE),shader); }

    mat4_set_identity( &projection );
    mat4_set_identity( &model );
    mat4_set_identity( &view );

    reshape(width, height);

    return 0;
}

void es2sample_end( void )
{
    texture_atlas_delete(atlas),  atlas  = NULL;
    vertex_buffer_delete(buffer), buffer = NULL;

    if(shader) glDeleteProgram(shader), shader = 0;
}

