/*
    GLES2 texture atlas

    my implementation of texture-atlas-mapping, from scratch.
    (shaders
     beware)

    2021, masterzorag
*/

#include "defines.h"
#include <semaphore.h> 
#ifdef __ORBIS__
#include "shaders.h"
#else
#include "pc_shaders.h"
#endif
#include "GLES2_common.h"

extern vec4 c[8];       // palette
extern vec2 resolution;
extern int *dump_buffer;
// import Installed_Apps AOS
extern layout_t *icon_panel;
extern item_t   *all_apps;
extern double    u_t;
extern GLuint    fallback_t;  // fallback texture

// io loads borders, then save UVs
vec4 get_rect_from_index(const int idx, const layout_t *l, vec4 *io)
{   // output vector, in px
    vec4 r  = (0);
    // item position and size, fieldsize, borders, in px
    vec2 p  = (0),//l->bound_box.xy,
         s  = l->bound_box.zw,
         fs = (vec2) { l->fieldsize.x, l->fieldsize.y },
    borders = (0);
    // override, load vector
    if(io) borders = io->xy;
    // 1st is up, then decrease and draw going down
    p.y += s.y;
    // size minus total borders
    s   -= borders * (fs - 1.);
    s   /= fs;
    // compute position
    p.x += (idx % (int)fs.x) * (s.x + borders.x),
    p.y -= floor(idx / fs.x) * (s.y + borders.y);
    // fill output vector: pos, size
    r.xy = p,  r.zw = s;
    // we draw upperL-bottomR, flip size.h sign
    r.w *= -1.;

    if(io)
    {   // convert to normalized coordinates
        io->xy =         r.xy  / resolution,
        io->zw = (r.zw + r.xy) / resolution;
    }
   
#if 0
    log_info("%2d: %3.1f,%3.1f \t%3.1fx%3.1f", idx, r.x, r.y, r.z, r.w);
    if(io) log_info("%2d: %.3f,%.3f \t%.3fx%.3f", idx, io->x, io->y, io->z, io->w);
#endif

    return r; // in px size!
}

typedef struct tex_idx_t
{
    GLuint ta; // which texture atlas
    vec4   uv; // normalized pos, size
} tex_idx_t;

// texture cache
typedef struct tcache_t
{
    int     tex_c;   // textures count, as pages
    GLuint *tex_a;   // textures array
} tcache_t;

// holds array of textures
tcache_t cache;

// RENDER TO TEXTURE VARIABLES
GLuint fb, depthRb;             // the framebuffer, the renderbuffer and the texture to render

layout_t *atlas_l = NULL; // temp, reuse


void create_atlas_from_tokens(layout_t *l)
{
    // load all textures from icon0.png
    for(int i = 0; i < l->item_c; i++)
    {
        
#if defined (USE_NFS)
        char *p = strstr( l->item_d[i].token_d[PICPATH].off, "storedata" );
        
        //l->texture[0] = load_png_asset_into_texture(p);
        l->item_d[i].texture = load_png_asset_into_texture(p);
#else
        // local path is directly token value
//XXX   l->item_d[i].texture = load_png_asset_into_texture(l->item_d[i].token_d[PICPATH].off);
#endif
        // don't put the default one now
    }

    // define our texture atlas: split 1080px height into 7cols/4rows squares
    atlas_l = calloc(1, sizeof(layout_t));
    atlas_l->bound_box =  (vec4) { 0, 0, resolution.y /4. *7., resolution.y },
    atlas_l->fieldsize = (ivec2) { 7, 4 };

    // done prepared
}


void create_tcache(layout_t *l)
{   
  
    // malloc and zerofill
    memset(&cache, 0, sizeof(tcache_t));
    // compute how many pages we need
    cache.tex_c = 1 + ( icon_panel->item_c / (atlas_l->fieldsize.x * atlas_l->fieldsize.y) );
    cache.tex_a = calloc(cache.tex_c, sizeof(GLuint));
   
    if(cache.tex_a[0] == 0)
    {
        // dynalloc for minimum requested atlas
#if 1
        int *texBuffer = calloc(resolution.x * resolution.y, sizeof(int));
#else
        if( ! dump_buffer )
        {
#if defined __ORBIS__
            dump_buffer = malloc(resolution.x * resolution.y * sizeof(int));
#else
            dump_buffer = calloc(resolution.x * resolution.y, sizeof(int));
#endif
        }
        int *texBuffer = dump_buffer;
#endif

        /* create the ints for the framebuffer, depth render buffer and texture */
        glGenFramebuffers (1, &fb);
        glGenRenderbuffers(1, &depthRb); // the depth buffer
        int j = 0;

create_atlas_page:

        glGenTextures(1, &cache.tex_a[ j ]);
        // generate texture
        glBindTexture  (GL_TEXTURE_2D, cache.tex_a[ j ]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        /* create an empty intbuffer first */

        // generate the textures
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, texBuffer);
        // create render buffer and bind 16-bit depth buffer
        glBindRenderbuffer   (GL_RENDERBUFFER, depthRb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, resolution.x, resolution.y);

        /* now render (offscreen) */

        // Bind the framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, fb);
        // viewport should match texture size
        glViewport(0, 0, resolution.x, resolution.y);
        // specify texture as color attachment
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cache.tex_a[ j ], 0);
        // attach render buffer as depth buffer
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRb);
        // check status
        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            log_error( "%d Framebuffer status: %x", j, (int)status); //return false;
        // Clear the texture (buffer) and then render as usual...
//      glClearColor(.0f, 1.0f, .0f, 1.0f);
        glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    
    /// draw entire page
    if(1)
    {
        vec2 p, s,      // item position and size, in px
             b1 = (0.); // border between rectangles, in px
        vec4 r, uv;
        // draw them offscreen, according to passed layout
        for(int i = 0; i < l->fieldsize.x
                         * l->fieldsize.y; i++)
        {   // check if we have the entry
            int g_idx = i + (j * l->fieldsize.x
                               * l->fieldsize.y);
            if(g_idx > icon_panel->item_c -1) break;

            // store borders to pass then, see below
            uv.xy = b1;
            // compute position and size in px, fill UVs
            r = get_rect_from_index(i, l, &uv);
            // swap r with p, s to reuse r below
            p = r.xy, s = r.zw;
#if 0
            log_info("%.2d: @%4.2f,%4.2f,\t%.2fx%.2f\tuv:%.3f,%.3f, %.3f,%.3f",
                                         i,  p.x,  p.y,  s.x,  s.y,
                                            uv.x, uv.y, uv.z, uv.w);
#endif
            // turn size into a second point
            s += p;
            // convert to normalized coordinates
            r.xy = px_pos_to_normalized(&p),
            r.zw = px_pos_to_normalized(&s);

            /* now replace the texture using atlas data */

            // load
            icon_panel->item_d[ g_idx ].texture = load_png_asset_into_texture(icon_panel->item_d[ g_idx ].token_d[ PICPATH ].off);

            // 1. draw squared icon into texture atlas
            if( icon_panel->item_d[ g_idx ].texture == GL_NULL) {
                on_GLES2_Render_icon(USE_COLOR, fallback_t, 2, &r, NULL);
            } else {
                on_GLES2_Render_icon(USE_COLOR, icon_panel->item_d[ g_idx ].texture, 2, &r, NULL);
            }

            // 2. now we can destroy the single icon texture
            if( icon_panel->item_d[ g_idx ].texture != GL_NULL )
            {
                log_debug( "glDeleteTextures(%d)", icon_panel->item_d[ g_idx ].texture);
                glDeleteTextures(1, &icon_panel->item_d[ g_idx ].texture);
            }

            // 3. now replace it with the cached one
            icon_panel->item_d[ g_idx ].texture = cache.tex_a[ j ];
            // 4. finally, save UV for this item
            icon_panel->item_d[ g_idx ].uv = uv;
        }


        j++; // advance for next atlas page

        glBindTexture(GL_TEXTURE_2D, 0);

        if(j < cache.tex_c) goto create_atlas_page;
    }
    /// draw ends

        // unbind frame buffer (texture)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // restore back app dafault!
        glClearColor( 0.1211 , 0.1211 , 0.1211 , 1.); // background color
        glViewport(0, 0, resolution.x, resolution.y);

        //notify("create_tcache passed!");
        if(texBuffer) free(texBuffer);
    }

 
}

void InitTextureAtlas(void) // one from cache
{
    if( ! cache.tex_a ) create_tcache(atlas_l);
}

/// draw one from atlas pages
void GLES2_draw_new_test4(int g_uv)
{
    if( ! cache.tex_a ) return;

    vec4 ur  = (0), // user frect
         uv  = icon_panel->item_d[ g_uv ].uv;
    vec2 ps  = resolution /2.,       // center the screen
         sq  = (vec2) { 270, -270 }; // square, in px
         sq += ps;                   // turn size in p2
      ur.xy  = px_pos_to_normalized(&ps),
      ur.zw  = px_pos_to_normalized(&sq);

    on_GLES2_Render_icon(USE_COLOR, icon_panel->item_d[ g_uv ].texture, 2, &ur, &uv);
}

