#include "jsmn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

#include "json.h"


extern int    selected_icon;
extern GLuint shader;
extern mat4   model, view, projection;

extern texture_atlas_t *atlas;

/*
 * A small example of jsmn parsing when JSON structure is known and number of
 * tokens is predictable.
 */

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}


unsigned char *http_fetch_from(const char *url);

// index (write) all used_tokens
static int json_index_used_tokens(page_info_t *page)
{
    int r, i, c = 0, idx = 0;
    jsmn_parser p;
    jsmntok_t t[512]; /* We expect no more than this tokens */

    char *json_data = page->json_data;

    jsmn_init(&p);
    r = jsmn_parse(&p, json_data, strlen(json_data), t,
                             sizeof(t) / sizeof(t[0]));
    if (r < 0) { fprintf(INFO, "Failed to parse JSON: %d\n", r); return -1; }

    item_idx_t *token = NULL;
    for(i = 1; i < r; i++)
    {
        token = &page->item[idx].token[0];
        for(int j = 0; j < NUM_OF_USER_TOKENS; j++)
        {
         //   printf("! %d [%s]: %p, %d\n", j, used_token[j], page->tokens[j].off, page->tokens[j].len);
            if (jsoneq(json_data, &t[i], used_token[j]) == 0)
            {
                /* We may use strndup() to fetch string value */
#if 0            
                printf("- %d) %d %d [%s]: %.*s\n", c, i, j,
                    used_token[j],
                              t[i + 1].end - t[i + 1].start,
                                 json_data + t[i + 1].start);
#endif
                token[j].off =    json_data + t[i + 1].start;
                token[j].len = t[i + 1].end - t[i + 1].start;
                i++; c++;
                if(j==16) idx++; // ugly but increase
            }
        }
      //printf("%d - %d [%s]: %s, %d\n", i, j, used_token[j], page->tokens[j].off, page->tokens[j].len);
    }
//    if(c == NUM_OF_USER_TOKENS * 8) printf("ok");
    /* this is the number of all counted items * NUM_OF_USER_TOKENS ! */
    return c;
}


// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;

//static page_info_t *page = NULL;

void destroy_page(page_info_t *page)
{
    if(!page) return;

    for(int i = 0; i < NUM_OF_TEXTURES; i++)
    {
        if(page->item[i].texture) glDeleteTextures(1, &page->item[i].texture);
    }
    // each page will hold its json_data
    free(page->json_data);
    // and its ft-gl vertex buffer
    vertex_buffer_delete(page->vbo), page->vbo = NULL;

    if(page) free(page), page = NULL;
}

// dummmy, to catch my GL bug
void refresh_atlas(void)
{
   // fprintf(INFO, "* atlas to gpu *\n");
  //glUseProgram   ( shader );
    // setup state
    //glActiveTexture( GL_TEXTURE0 );

    if(1)//(page_num > num_of_pages)
    {
        //printf("to gpu!\n");
        /* discard old texture, we eventually added glyphs! */
        if(atlas->id) glDeleteTextures(1, &atlas->id), atlas->id = 0;

        /* re-create texture and upload atlas into gpu memory */
        glGenTextures  ( 1, &atlas->id );
        glBindTexture  ( GL_TEXTURE_2D, atlas->id );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexImage2D   ( GL_TEXTURE_2D, 0, GL_ALPHA, atlas->width, atlas->height,
                                        0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas->data );
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

texture_font_t *stock_font = NULL;
texture_font_t *title_font = NULL;
texture_font_t *curr_font  = NULL;
/*

*/
extern int num_of_pages;

int count_availables_json(void)
{
    int   count = 0;
    char  json_file[128];
    void *p;

    while(1)
    {   // json page_num starts from 1 !!!
        sprintf(&json_file[0], "/user/app/NPXS39041/homebrew-page%d.json", count +1);
        // read the file
#if defined (USE_NFS)
        #warning "we use nfs!"
//        p = &json_file[20];
//        fprintf(INFO, "open:%s\n", p);
        // move pointer to address nfs share
        p = (void*)orbisNfsGetFileContent(&json_file[20]);
#else
        p = (void*)orbisFileGetFileContent(&json_file[0]);
#endif
        // check
        if(p) free(p), p = NULL;  else break;
        // passed, increase num of available pages
        count++;   
    }
    return count;
}

// this draws the page layout:
// texts from indexed json token name, value
page_info_t *compose_page(int page_num)
{
    // do it just once for now, one page
    //if(page) return page;
    page_info_t *page = calloc(1, sizeof(page_info_t));

    char json_file[256];
    sprintf(&json_file[0], "/user/app/NPXS39041/homebrew-page%d.json", page_num);

    page->page_num  = page_num;
    page->json_data = (void*)orbisFileGetFileContent(&json_file[0]);

    if(!page->json_data)
    {
        sceKernelIccSetBuzzer(4);
        free(page), page = NULL;
        return NULL;
    }

    page->vbo   = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

    int valid_t = json_index_used_tokens(page);
    // json describe max 8 items per page
    int ret     = valid_t /NUM_OF_USER_TOKENS;

    // if < 8 realloc page
    //printf("%d\n", ret);
    if(!stock_font)
        stock_font = texture_font_new_from_memory(atlas, 17, _hostapp_fonts_zrnic_rg_ttf, 
                                                             _hostapp_fonts_zrnic_rg_ttf_len);
    if(!title_font)
        title_font = texture_font_new_from_memory(atlas, 30, _hostapp_fonts_zrnic_rg_ttf, 
                                                             _hostapp_fonts_zrnic_rg_ttf_len);
    // now prepare all the texts textures, use indexed json_data
    int count;
    for(int i = 0; i < ret; i++)
    {
        // compute item origin position, in px
        page->item[i].origin = (vec2)
        {
            100. +         i * (200. + 10. /* border between icons */),
            226. + (200+226) * (page->page_num %2 /* rows in a page */)
        };
        // fill item frect with origin position and icon size, in px
        page->item[i].frect.xy = page->item[i].origin;
        page->item[i].frect.zw = (vec2) ( 200. );

        // address tokens and VBO index (Write)
        item_idx_t *token = &page->item[i].token  [0];
        ivec2      *t_idx = &page->item[i].token_i[0];

        vec4 white = (vec4) ( 1.f ),
             color = (vec4) { .7, .7, .8, 1. };
        // lines counter
        count = 0;
        for(int j = 0; j < NUM_OF_USER_TOKENS; j++)
        {
            static char tmp[256];
            // get the indexed token value
            snprintf(&tmp[0], token[j].len + 1, "%s", token[j].off);
            switch(j) // print just the following:
            {
                case NAME:        curr_font = title_font;
                    break;
                case DESC: 
                case VERSION: 
                case REVIEWSTARS:
                case SIZE:
                case AUTHOR:
                case APPTYPE:
                case PV:
                case RELEASEDATE: curr_font = stock_font;
                    break;
                case PICPATH: {
                    printf("downloading %s from %s", token[j].off, token[ IMAGE ].off);
                        if (sceKernelOpen(token[j].off, 0x0000, 0) < 0)
                            dl_from_url(token[ IMAGE ].off, token[j].off, false);
                        else
                            page->item[i].texture = load_png_asset_into_texture(&tmp[0]);

                        while (1) {}
                    continue; }
                default: continue; // skip unknown names
            }
//            printf("%d, %d: [%p]: %d\n", i, j, token[j].off, token[j].len);
            //printf("%s\n", tmp);
            texture_font_load_glyphs( curr_font, &tmp[0] );        // set textures

            /*  start indexing */
            t_idx[count].x = vector_size( page->vbo->items );

            // same position as our rects:
            vec2 pen = { 0., count * /* down */ -18. };
            // add item origin
                pen += page->item[i].origin;
            // save as new origin: pen will move!
            vec2 no  = pen;
            // add a little offset from new origin
              pen.x += 8.;

            add_text( page->vbo, curr_font, &tmp[0], &white, &pen);

            // switch back font
            curr_font = stock_font;
            // print token name
            sprintf(&tmp[0], "%s", used_token[j]);
            // set textures
            texture_font_load_glyphs( curr_font, &tmp[0] );
            // seek pen back to new origin
            pen    = no;
            // right align, related to new origin
            pen.x -= tl;

            add_text( page->vbo, curr_font, &tmp[0], &color, &pen );

            t_idx[count].y = vector_size( page->vbo->items )
                           - t_idx[count].x;
            // advance
            count++;

        }
        page->item[i].num_of_texts = count;
    }
    //texture_font_delete( stock_font );
    //texture_font_delete( title_font );

    // todo: skip if we already cached textx!
  if(0)//(page_num > num_of_pages)
  {
    //printf("to gpu!\n");
    /* discard old texture, we eventually added glyphs! */
    if(atlas->id) glDeleteTextures(1, &atlas->id), atlas->id = 0;

    /* re-create texture and upload atlas into gpu memory */
    glGenTextures  ( 1, &atlas->id );
    glBindTexture  ( GL_TEXTURE_2D, atlas->id );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D   ( GL_TEXTURE_2D, 0, GL_ALPHA, atlas->width, atlas->height,
                                    0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas->data );
  }
  else
  { //printf("refresh_atlas\n");
    //refresh_atlas();
  }
    // don't leak buffer!
    //if(page->json_data) free (page->json_data);
    return page;
}

// review this
char *get_json_token_value(item_idx_t *item, int name)
{
    //page_info_t *p = page;
    static char tmp[256];
    // get the indexed token value
    item_idx_t *token = &item[name];

    snprintf(&tmp[0], token->len + 1, "%s", token->off);
    //printf("%s\n", tmp);
    return (char *)&tmp[0];
}

// wrapper from outside
void get_json_token_test(item_idx_t *item)
{
    printf("%s\n", get_json_token_value(item, IMAGE));

    printf("package: %s\n", get_json_token_value(item, PACKAGE));
}

// ---------------------------------------------------------------- display ---
void GLES2_render_page( page_info_t *page )
{
    // do nothing if a page not exists yet...
    if(!page) return;

    // we already clean in main renderloop()!

    // we have indexed texts: address the index pointer
    // address tokens and VBO index (Read)
    ivec2 *t_idx = &page->item[selected_icon].token_i[0];

    glUseProgram   ( shader );
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
            vertex_buffer_render( page->vbo, GL_TRIANGLES ); // all vbo
        }
        else /* draw a range/selection of indexes (= glyph) */
        {
            vertex_buffer_render_setup( page->vbo, GL_TRIANGLES ); // start draw
            //int num_of_texts = NUM_OF_USER_TOKENS;
            for(int j=0; j < page->item[selected_icon].num_of_texts; j++)  // draw all texts
            {   //printf("%d, %d %d (%d %d)\n", page_num, selected_icon, num_of_texts, t_idx[j].x, t_idx[j].y);
                // draw just text[j]
                // iterate each char in text[j]
                for(int i = t_idx[j].x;
                        i < t_idx[j].x + t_idx[j].y;
                        i++ )
                { // glyph by one (2 triangles: 6 indices, 4 vertices)
                    vertex_buffer_render_item ( page->vbo, i );
                }
            }
            vertex_buffer_render_finish( page->vbo ); // end draw
        }
    }
    glDisable( GL_BLEND );  // Reset state back
    glBindTexture(GL_TEXTURE_2D, 0);

    // we already swapframe in main renderloop()!
}

// reuse those
extern  texture_font_t *sub_font;
extern vertex_buffer_t *text_buffer[4];
#if 0
// ---------------------------------------------------------------- display ---
void GLES2_render_page_v2( page_info_t *page )
{
    // we already clean in main renderloop()!

    // do nothing if a page not exists yet...
    if(!page) return;

    // get the item index from available ones
    int idx = // we started from page 1!!!
             (page->page_num -1) * 8 /* ITEM_IN_A_JSON */
            + selected_icon;
 #if 0
    // compute the item origin
    vec2 origin = (vec2) //
    {
        100. + selected_icon * (100. + 2. /*border*/),
        200. +          200. * (page->page_num %2 )
    };
    //printf("%d %d: origin (%.f, %.f)\n", idx, page->page_num %2, origin.x, origin.y);
#endif

    // XXX this is the "refresh trigger"
    if (text_buffer[3] != NULL)
        vertex_buffer_delete(text_buffer[3]), text_buffer[3] = NULL;

    if (text_buffer[3] == NULL)
        text_buffer[3] = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

    // now prepare all the texts textures, use indexed json_data
    int count;
    
      item_idx_t *token = &page->item[selected_icon].token  [0];
      ivec2      *t_idx = &page->item[selected_icon].token_i[0];

      vec4 white = (vec4) ( 1.f ),
           color = (vec4) { .7, .7, .8, 1. };

      count = 0;
      for(int j = 0; j < NUM_OF_USER_TOKENS; j++)
      {
          static char tmp[256];
          // get the indexed token value
          snprintf(&tmp[0], token[j].len + 1, "%s", token[j].off);
          switch(j) // print just the following:
          {
              case NAME:        curr_font = title_font;
                  break;
              case DESC: 
              case VERSION: 
              case REVIEWSTARS:
              case SIZE:
              case AUTHOR:
              case APPTYPE:
              case PV:
              case RELEASEDATE: curr_font = stock_font;
                  break;
              case PICPATH: {
                  if(page->item[selected_icon].texture)
                  continue; }
              default: continue; // skip unknown names
          }
//            printf("%d, %d: [%p]: %d\n", i, j, token[j].off, token[j].len);
          //printf("%s\n", tmp);
          texture_font_load_glyphs( curr_font, &tmp[0] );        // set textures
          
          // same position as our rects:
          vec2 pen = { 0., count * /* down */ -18. };
          // add item origin
              pen += page->item[selected_icon].origin;
          // save as new origin: pen will move!
          vec2 no  = pen;
          // add a little offset from new origin
            pen.x += 8.;

          add_text( text_buffer[3], curr_font, &tmp[0], &white, &pen);

          // switch back font
          curr_font = stock_font;
          // print token name
          sprintf(&tmp[0], "%s", used_token[j]);
          // set textures
          texture_font_load_glyphs( curr_font, &tmp[0] );
          // seek pen back to new origin
          pen    = no;
          // right align, related to new origin
          pen.x -= tl;

          add_text( text_buffer[3], curr_font, &tmp[0], &color, &pen );
          count++;
        }

    // we have indexed texts: address the index pointer
    //ivec2 *t_idx = &page->item[selected_icon].token_i[0];

    glUseProgram   ( shader );
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
        /* draw whole VBO (storing all added texts) */
        vertex_buffer_render( text_buffer[3], GL_TRIANGLES ); // all vbo
        
    }
    glDisable( GL_BLEND );  // Reset state back
    glBindTexture(GL_TEXTURE_2D, 0);

    // we already swapframe in main renderloop()!
}
#endif


void GLES2_RenderPageForItem(page_item_t *item)
{
    if (text_buffer[3] != NULL)
        vertex_buffer_delete(text_buffer[3]), text_buffer[3] = NULL;
    // we have called GLES2_InitPageForItem() in advance,
    // font and vbo already should be already there
    if (text_buffer[3] == NULL)
        text_buffer[3] = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

/*  item_idx_t token  [NUM_OF_USER_TOKENS]; // indexed tokens from json_data
    ivec2      token_i[NUM_OF_USER_TOKENS]; // enough for indexing all tokens, in ft-gl VBO
    int        num_of_texts;                // num of indexed tokens we print to screen
    GLuint     texture;                     // the textured icon from PICPATH
} page_item_t;
*/
    vec2   pen = (vec2) { 0, 500 };
    vec4 white = (vec4) ( 1.f ),
         color = (vec4) {  .7, .7, .8, 1. };

#define TEXT_VSTEP  (50)
    static char tmp[256],
                lbl[256];
    for(int i = 0; i < NUM_OF_USER_TOKENS; i++)
    {
        // get the indexed token value
        snprintf(&tmp[0], item->token[i].len + 1, "%s", item->token[i].off);

        switch(i) // print just the following:
        {
            case NAME:        sprintf(&lbl[0], "");
            // align center title at px pos
                 texture_font_load_glyphs( sub_font, &tmp[0] );
                 pen = (vec2) { 600 + (700 - tl) /2,
                                500 - TEXT_VSTEP *0 };
                 break;
            case DESC:        sprintf(&lbl[0], "Description: ");
                 pen = (vec2) { 480,  500 - TEXT_VSTEP *2 };
                 break;
//          case VERSION:     sprintf(&lbl[0], "Name: "); break;
            case REVIEWSTARS: sprintf(&lbl[0], "Rank: "); 
                 pen = (vec2) { 480,  500 - TEXT_VSTEP *6 };
                 break;
//          case SIZE:        sprintf(&lbl[0], "Name: "); break;
            case AUTHOR:      sprintf(&lbl[0], "Author: ");
            // align right at px pos
                 texture_font_load_glyphs( sub_font, &lbl[0] );
                 pen = (vec2) { 600 + (700 - tl),
                                500 - TEXT_VSTEP *1 };
                 break;
            case APPTYPE:     sprintf(&lbl[0], "App type: ");
                 pen = (vec2) { 480,  500 - TEXT_VSTEP *3 };
                 break;
//          case PV:          sprintf(&lbl[0], "Name: "); break;
            case RELEASEDATE: sprintf(&lbl[0], "Released: ");
                 pen = (vec2) { 480,  500 - TEXT_VSTEP *4 };
                break;
            default: continue; // skip unknown names
        }
        /* append to VBO */

        //printf("%d: %s\n", i, &tmp[0]);
      //pen.x  = 480.;
        add_text( text_buffer[3], sub_font, &lbl[0], &color, &pen);
        //pen.y -=  28.;

//      texture_font_load_glyphs( font, &tmp[0] );        // set textures
//      pen.y = 400 - font->height;  // reset pen, 1 line down

        //pen.x  = 480.;
        add_text( text_buffer[3], sub_font, &tmp[0], &white, &pen);
      //pen.y -=  28.;
    }

    /* texts under the app icon */
    pen = (vec2) { 210, 260 };
    add_text( text_buffer[3], sub_font, "Version: ", &white, &pen);
    snprintf(&tmp[0], item->token[VERSION].len + 1, "%s", item->token[VERSION].off);
    add_text( text_buffer[3], sub_font, &tmp[0], &white, &pen);
    pen.x  = 210.;
    pen.y -=  50.;
    add_text( text_buffer[3], sub_font, "Size: ", &white, &pen);
    snprintf(&tmp[0], item->token[SIZE].len + 1, "%s", item->token[SIZE].off);
    add_text( text_buffer[3], sub_font, &tmp[0], &white, &pen);
    pen.x  = 270.;
    pen.y -=  50.;
    add_text( text_buffer[3], sub_font, "Download", &white, &pen);
    pen.x  = 210.;
    pen.y -=  50.;
    add_text( text_buffer[3], sub_font, "Playable on: ", &white, &pen);
    snprintf(&tmp[0], item->token[PV].len + 1, "%s", item->token[PV].off);
    add_text( text_buffer[3], sub_font, &tmp[0], &white, &pen);
    pen.x  = 210.;
    pen.y -=  50.;

    // refresh once per VBO
    if(!item->in_atlas)
        refresh_atlas(), item->in_atlas = 1;
}
