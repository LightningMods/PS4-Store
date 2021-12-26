/*
    we dump the framebuffer by glReadPixels(),
    then resample a new texture from same data
*/


#include "defines.h"

extern vec2 resolution;

/*
    dump frame into a texture: we have to
    do it AFTER drawn AND flipped frame!!

    we "trig it" to do following this flow
 */

#include <memory.h>

GLuint renderedTexture = 0;     // The texture we're going to render to
int *dump_buffer       = NULL;  // never free, but reuse it
bool ask_to_dump_frame = false;

// this is the trigger to use
void trigger_dump_frame()
{
    ask_to_dump_frame = true;
}

// compose filename and trig "save as png"
void save_dumped_frame(void)
{
    //int  ret = -1;
    char tmp[256],
           s[ 64];
    time_t     t  = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(s, sizeof(s), "%y%m%d-%H%M%S", tm); // custom date string
    //log_debug( "%s", s);

    if( usbpath() )
    {
        snprintf(&tmp[0], 255, "%s/Store-%s.png", usbpath(), s);
#if 0
        // plain save binary blob to file, tested
        FILE   *fp = fopen( tmp, "wb" );
        res = fwrite(dump_buffer, sizeof(int), resolution.x * resolution.y, fp);
        log_info( "written %zub to: %s", res, tmp);
        fclose(fp);
#else
        writeImage(&tmp[0], resolution.x, resolution.y, dump_buffer, "HB Store");
#endif
        // feedback!
        ani_notify("Frame dumped!");
    }
}

// don't call this directly, use trigger_*
void dump_frame(void)
{
    glBindTexture(GL_TEXTURE_2D, 0);

    if( ! ask_to_dump_frame ) return;
    
    glDeleteTextures(1, &renderedTexture), renderedTexture = 0;
    ask_to_dump_frame = false;

    if( ! renderedTexture )
    {
        if( ! dump_buffer )
        {
#if defined __ORBIS__
            dump_buffer = calloc(resolution.x * resolution.y * sizeof(int), sizeof(resolution.x * resolution.y * sizeof(int)));
#else
            dump_buffer = calloc(resolution.x * resolution.y, sizeof(int));
#endif
        }
        // dump entire screen
        glReadPixels( 0, 0, resolution.x, resolution.y, GL_RGBA, GL_UNSIGNED_BYTE, dump_buffer );
        // resample same texture
        renderedTexture = load_texture(resolution.x, resolution.y, GL_RGBA, dump_buffer);

        //free(buffer), buffer = NULL;
        log_info("%s: %d", __FUNCTION__, renderedTexture);

        save_dumped_frame();
    }
}

void show_dumped_frame(void)
{
    if( ! renderedTexture )
        return;
    else
    {
        vec4 r = (vec4) { -.5,.5,  .0,.0 };
        on_GLES2_Render_icon(USE_COLOR, renderedTexture, 2, &r, NULL);
    }
}

