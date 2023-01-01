/* Freetype GL - A C OpenGL Freetype engine
 *
 * Distributed under the OSI-approved BSD 2-Clause License.  See accompanying
 * file `LICENSE` for more details.
 */
#if 0
#  if !defined(_WIN32) && !defined(_WIN64)
#    include <fontconfig/fontconfig.h>
#  endif
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font-manager.h"

#include <stdio.h>
#define  debugNetPrintf  fprintf
#define  ERROR           stderr
#define  DEBUG           stdout
#define  INFO            stdout

unsigned char *orbisFileGetFileContent(const char *filename);
// each orbisFileGetFileContent() call will update filesize!
size_t _orbisFile_lastopenFile_size;




// ------------------------------------------------------- font_manager_new ---
font_manager_t *
font_manager_new( size_t width, size_t height, size_t depth )
{
    font_manager_t *self;
    texture_atlas_t *atlas = texture_atlas_new( width, height, depth );
    self = (font_manager_t *) malloc( sizeof(font_manager_t) );
    if( !self )
    {
        fprintf(DEBUG,
                 "line %d: No more memory for allocating data\n", __LINE__ );
        exit( EXIT_FAILURE );
    }
    self->atlas = atlas;
    self->fonts = vector_new( sizeof(texture_font_t *) );
    self->cache = strdup( " " );
    return self;
}


// ---------------------------------------------------- font_manager_delete ---
void
font_manager_delete( font_manager_t * self )
{
    size_t i;
    texture_font_t *font;
    assert( self );

    for( i=0; i<vector_size( self->fonts ); ++i)
    {
        font = *(texture_font_t **) vector_get( self->fonts, i );
        if (font->location == 1 && font->memory.base)
        {
            /* release all the allocated .ttf buffers */
            free((void *)font->memory.base), font->memory.base = NULL;
        }
        texture_font_delete( font );
    }
    vector_delete( self->fonts );
    texture_atlas_delete( self->atlas );
    if( self->cache )
    {
        free( self->cache );
    }
    free( self );
}



// ----------------------------------------------- font_manager_delete_font ---
void
font_manager_delete_font( font_manager_t * self,
                          texture_font_t * font)
{
    size_t i;
    texture_font_t *other;

    assert( self );
    assert( font );

    for( i=0; i<self->fonts->size;++i )
    {
        other = (texture_font_t *) vector_get( self->fonts, i );
        if ( (strcmp(font->filename, other->filename) == 0)
               && ( font->size == other->size) )
        {
            vector_erase( self->fonts, i);
            break;
        }
    }
    texture_font_delete( font );
}



// ----------------------------------------- font_manager_get_from_filename ---
texture_font_t *
font_manager_get_from_filename( font_manager_t *self,
                                const char * filename,
                                const float size )
{
    size_t i;
    texture_font_t *font;

    assert( self );
    for( i=0; i<vector_size(self->fonts); ++i )
    {
        font = * (texture_font_t **) vector_get( self->fonts, i );
        if( (strcmp(font->filename, filename) == 0) && ( font->size == size) )
        {
            return font;
        }
    }

    size_t filesize = 0;
    /* load .ttf in memory, we release at font_manager_delete() */
    #ifdef __orbisdev__
      void *ttf =  orbisNfsGetFileContent(filename);
      filesize  = _orbisNfs_lastopenFile_size;
    #else // from fileIO.c
    _orbisFile_lastopenFile_size = -1;

    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Unable to open file \"%s\".", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long sizes = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *buffer = (unsigned char *)malloc((sizes + 1) * sizeof(char));
    fread(buffer, sizeof(char), sizes, file);
    buffer[sizes] = 0;
    _orbisFile_lastopenFile_size = sizes;
    fclose(file);
    void* ttf = (void*)buffer;
    #endif
    fprintf(INFO, "ttf at %p %ld\n", ttf, filesize);

    font = texture_font_new_from_memory( self->atlas, size, ttf, filesize);
//  font = texture_font_new_from_file  ( self->atlas, size, filename );
    if( font )
    {
        vector_push_back( self->fonts, &font );
        texture_font_load_glyphs( font, self->cache );

        /* release memory for next .ttf */
        //free(ttf), ttf = NULL;
        
        return font;
    }
    fprintf(DEBUG,"Unable to load \"%s\" (size=%.1f)\n", filename, size );
    return 0;
}


// ----------------------------------------- font_manager_get_from_description ---
texture_font_t *
font_manager_get_from_description( font_manager_t *self,
                                   const char * family,
                                   const float size,
                                   const int bold,
                                   const int italic )
{
    texture_font_t *font;
    char *filename = 0;

    assert( self );

#ifndef __PS4__
    if( file_exists( family ) )
    {
        filename = strdup( family );
    }
    else
    {
#if defined(_WIN32) || defined(_WIN64)
        fprintf( stderr, "\"font_manager_get_from_description\" not implemented yet.\n" );
        return 0;
#endif
        filename = font_manager_match_description( self, family, size, bold, italic );
        if( !filename )
        {
            fprintf(DEBUG, "No \"%s (size=%.1f, bold=%d, italic=%d)\" font available.\n",
                     family, size, bold, italic );
            return 0;
        }
    }
#endif

    filename = strdup( family );
    fprintf(DEBUG,"font_manager_get_from_filename \"%s\" (size=%.1f)\n", filename, size );
    font = font_manager_get_from_filename( self, filename, size );

    free( filename );
    return font;
}

// ------------------------------------------- font_manager_get_from_markup ---
texture_font_t *
font_manager_get_from_markup( font_manager_t *self,
                              const markup_t *markup )
{
    assert( self );
    assert( markup );
    fprintf(DEBUG,"font_manager_get_from_markup: %p, %s %.1f %d %d\n",
                          self,
                          markup->family, markup->size,
                          markup->bold,   markup->italic);

    return font_manager_get_from_description( self, markup->family, markup->size,
                                              markup->bold,   markup->italic );
}

// ----------------------------------------- font_manager_match_description ---
char *
font_manager_match_description( font_manager_t * self,
                                const char * family,
                                const float size,
                                const int bold,
                                const int italic )
{
// Use of fontconfig is disabled by default.
#if 1
    return 0;
#else
#  if defined _WIN32 || defined _WIN64
      fprintf( stderr, "\"font_manager_match_description\" not implemented for windows.\n" );
      return 0;
#  endif
    char *filename = 0;
    int weight = FC_WEIGHT_REGULAR;
    int slant = FC_SLANT_ROMAN;
    if ( bold )
    {
        weight = FC_WEIGHT_BOLD;
    }
    if( italic )
    {
        slant = FC_SLANT_ITALIC;
    }
    FcInit();
    FcPattern *pattern = FcPatternCreate();
    FcPatternAddDouble( pattern, FC_SIZE, size );
    FcPatternAddInteger( pattern, FC_WEIGHT, weight );
    FcPatternAddInteger( pattern, FC_SLANT, slant );
    FcPatternAddString( pattern, FC_FAMILY, (FcChar8*) family );
    FcConfigSubstitute( 0, pattern, FcMatchPattern );
    FcDefaultSubstitute( pattern );
    FcResult result;
    FcPattern *match = FcFontMatch( 0, pattern, &result );
    FcPatternDestroy( pattern );

    if ( !match )
    {
        fprintf( stderr, "fontconfig error: could not match family '%s'", family );
        return 0;
    }
    else
    {
        FcValue value;
        FcResult result = FcPatternGet( match, FC_FILE, 0, &value );
        if ( result )
        {
            fprintf( stderr, "fontconfig error: could not match family '%s'", family );
        }
        else
        {
            filename = strdup( (char *)(value.u.s) );
        }
    }
    FcPatternDestroy( match );
    return filename;
#endif
}
