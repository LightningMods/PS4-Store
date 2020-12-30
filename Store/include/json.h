/*
    index json tokens for each items, for a single malloc'd page 

    2020, masterzorag
*/
#pragma once

typedef enum token_name
{
    ID,
    NAME,
    DESC,
    IMAGE,
    PACKAGE,
    VERSION,
    PICPATH,
    DESC_1,
    DESC_2,
    REVIEWSTARS,
    SIZE,
    AUTHOR,
    APPTYPE,
    PV,
    MAIN_ICON_PATH,
    MAIN_MENU_PIC,
    RELEASEDATE
    // should match NUM_OF_USER_TOKENS
} token_name;

static const char *used_token[] =
{
    "id",
    "name",
    "desc",
    "image",
    "package",
    "version",
    "picpath",
    "desc_1",
    "desc_2",
    "ReviewStars",
    "Size",
    "Author",
    "apptype",
    "pv",
    "main_icon_path",
    "main_menu_pic",
    "releaseddate"
};

#define NUM_OF_USER_TOKENS  (sizeof(used_token) / sizeof(used_token[0]))

// single item infos, for the page_info_t below
typedef struct
{
    item_idx_t token  [NUM_OF_USER_TOKENS]; // indexed tokens from json_data
    ivec2      token_i[NUM_OF_USER_TOKENS]; // enough for indexing all tokens, in ft-gl VBO
    int        num_of_texts;                // num of indexed tokens we print to screen
    GLuint     texture;                     // the textured icon from PICPATH
    vec4       frect;                       // the normalized position and size of texture icon
    vec2       origin;  // normalized origin point
    // 5x3
    vec2       pen;
    char       in_atlas;
} page_item_t;


// the single page holds its infos
typedef struct
{
    // max NUM_OF_TEXTURES per page, max 8 items
    int  num_of_items;
    // each page will hold its json_data
    char *json_data;
    // and its ft-gl vertex buffer
    vertex_buffer_t *vbo;
    // all indexed tokens, per item... (zerocopy, eh)
    page_item_t  item[NUM_OF_TEXTURES];
    // from 1, refrect json num in filename
    int page_num;
} page_info_t;


page_info_t *compose_page(int page_num);
void destroy_page(page_info_t *page);
void GLES2_render_page( page_info_t *page );

