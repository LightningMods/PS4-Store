#pragma once

/*
  here are the parts
*/

// nfs or local stdio
//#define USE_NFS  (1)
#if defined (USE_NFS)
#include <orbisNfs.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <utils.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO
#include <orbisGl.h>
#include <orbislink.h>
#include <libkernel.h>  //sceKernelIccSetBuzzer

#else // on linux

#define  debugNetPrintf  fprintf
#define  ERROR           stderr
#define  DEBUG           stdout
#define  INFO            stdout
#include <GLES2/gl2.h>
//#include <orbisAudio.h>

#endif // defined (__ORBIS__)

#include "zrnic_rg.h"
#include "font_ttf.h"


/// from fileIO.c
unsigned char *orbisFileGetFileContent( const char *filename );
extern size_t _orbisFile_lastopenFile_size;


/// from shader-common.c
GLuint create_vbo  (const GLsizeiptr size, const GLvoid *data, const GLenum usage);
//GLuint BuildShader (const char *source, GLenum shaderType);
/// build (and dump) from shader source code
GLuint BuildProgram(const char *vShader, const char *fShader);
//GLuint CreateProgramFromBinary(const char *vShader, const char *fShader);


// from cmd_build.c
char **build_cmd(char *cmd_line);


/// from demo-font.c
#include <freetype-gl.h>
int  es2init_text (int width, int height);
void add_text( vertex_buffer_t *buffer,
                texture_font_t *font,
                    const char *text,
                          vec4 *color,
                          vec2 *pen );
void render_text  (void);
void es2sample_end(void);

#if defined TEXT_ANI
  /// from text_ani.c
  #include "text_ani.h"
  void es2init_text_ani(int width, int height);
  void es2rndr_text_ext(fx_entry_t *_ani);
  void es2updt_text_ani(double elapsedTime);
  void es2fini_text_ani(void);
#endif


/// from png.c
GLuint load_png_asset_into_texture(const char *relative_path);
extern vec2 tex_size; // last loaded png size as (w, h)


/// from sprite.c
void on_GLES2_Init_sprite  (int view_w, int view_h);
void on_GLES2_Size_sprite  (int view_w, int view_h);
void on_GLES2_Render_sprite(int num);
void on_GLES2_Update_sprite(int frame);
void on_GLES2_Final_sprite (void);


/// from pl_mpeg.c
int  es2init_pl_mpeg  (int window_width, int window_height);
void es2render_pl_mpeg(float dt);
void es2end_pl_mpeg   (void);


/// from timing.c
unsigned int get_time_ms(void);


#if defined HAVE_NFS
/// from user_nfs.c
#include <nfsc/libnfs.h>
int    user_init (void);
size_t user_stat (void);
struct
nfsfh *user_open (const char *filename);
void   user_seek (long offset, int whence);
size_t user_read (unsigned char *dst, size_t size);
void   user_close(void);
void   user_end  (void);
#endif

// rects tests tetris primlib
void filledRect(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b);

/// for icons.c, sprite.c
#define NUM_OF_TEXTURES  (8)
#define NUM_OF_SPRITES   (6)

/// from ls_dir()
int ls_dir(char *dirpath);
int if_exists(char *path);

int get_item_count(void);
typedef struct
{
    char  *name;
    //size_t size;
} entry_t;
entry_t *get_item_entries(char *dirpath);


// from my_rects.c
vec2 px_pos_to_normalized(vec2 *pos);


// from lines_and_rects
void es2rndr_lines_and_rect( void );
void es2init_lines_and_rect( int width, int height );
void es2fini_lines_and_rect( void );


/* from icons.c
void on_GLES2_Init_icons(int view_w, int view_h);
void on_GLES2_Final (void);
void on_GLES2_Update(double frame);
void on_GLES2_Render(int index);
*/

/// GLES2_rects.c
void ORBIS_RenderFillRects_init(int width, int height);
void ORBIS_RenderFillRects_rndr(void);
void ORBIS_RenderFillRects_fini(void);

/// from GLES2_rects.c
enum SH_type
{
    USE_COLOR,
    USE_UTIME,
    NUM_OF_PROGRAMS
};
void ORBIS_RenderDrawLines(const vec2 *points, int count);
void ORBIS_RenderFillRects(enum SH_type SL_program, const vec4 *rgba, const vec4 *rects,  int count);
void ORBIS_RenderDrawBox(enum SH_type SL_program, const vec4 *rgba, const vec4 *rect);

// reuse this type to index texts around!
typedef struct
{
    char *off;
    int   len;
} item_idx_t;


enum views
{
    ON_TEST_ANI = -2,
    ON_LEFT_PANEL,
    ON_MAIN_SCREEN, // 0
    ON_SUBMENU,     // 1 - Queue
    ON_SUBMENU3,    // Ready
    ON_SUBMENU_2,   // 2 - Groups
    ON_ITEM_PAGE,   // 3 - aux/temp
    ON_EXTRA_PAGE   // 4 - pixelshader
};



// from GLES2_ani.c
void GLES2_ani_init  (int width, int height);
void GLES2_ani_update(double time);
void GLES2_ani_fini  ( void );
void ani_notify(const char *message);

// from json_simple.c
#include "json.h"
int count_availables_json(void);
void GLES2_RenderPageForItem(page_item_t *item);
char *get_json_token_value(item_idx_t *item, int name);
void refresh_atlas(void);

// from GLES2_rects.c
void on_GLES2_Update1(double time);

// from GLES2_textures.c
void on_GLES2_Init_icons(int view_w, int view_h);
void on_GLES2_Update(double time);
//void on_GLES2_Render_icons(int num);
void on_GLES2_Render_icon(GLuint texture, int num, vec4 *frect);
void on_GLES2_Render_icons(page_info_t *row);
void on_GLES2_Render_box(vec4 *frect);
void on_GLES2_Final(void);


// from GLES2_scene.c
void GLES2_scene_init( int w, int h );
void GLES2_scene_render(void);
void GLES2_scene_on_pressed_button(int button);


// from pixelshader.c
void pixelshader_render( GLuint SL_program );
void pixelshader_init( int width, int height );
void pixelshader_fini( void );

void ORBIS_RenderSubMenu(int num);

void GLES2_init_submenu( void );
void GLES2_render_submenu_text( int num );

// todo: resrt out those proto
uint32_t sceKernelGetCpuTemperature(uint64_t *res);
int notify(char *message);
void escalate_priv(void);
int initGL_for_the_store(void);
int pingtest(int libnetMemId, int libhttpCtxId, const char* src);

/*
    GLES2 layout rework, v2
*/

/*
    todo: we need to differentiate and made use of vbo_status 
    to avoid useless pushes to gpu
*/
typedef enum vbo_status
{
    EMPTY,
    APPEND,
    CLOSED,
    IN_ATLAS,
    IN_ATLAS_APPEND,
    IN_ATLAS_CLOSED
} vbo_status;

typedef struct
{
    ivec2 pos;
    item_idx_t *token_d;  // data
    int         token_c;  // count
    char  in_atlas; // flag
} item_t;

typedef struct
{
    ivec4   bound_box; // total field size, in px
    ivec2   fieldsize, // total field size, in items number
            item_sel,  // current selected item into field
            page_sel;  // page_selection: current, max page
    // an array of textures
    int retries;
    GLuint *texture;   // max: fieldsize in items number
    // an array of items
    item_t *item_d;    // item_data array
    int     item_c,    // item_count
            curr_item; // curent selected item:
    // each layout will hold its ft-gl vertex buffer
    vertex_buffer_t *vbo;
    enum vbo_status  vbo_s; // vbo_status
    // layout status
    char   is_shown,
           is_active, // to control selected_item
           refresh;   // to trigger texture reload/refresh
} layout_t;


item_idx_t *analyze_item_t   (item_t *items, int item_count);
item_t     *analyze_item_t_v2(item_t *items, int item_count);
item_idx_t *search_item_t    (item_t *items, int item_count, enum token_name TN, char *pattern);

void destroy_item_t(item_idx_t **p);

int df(char *out, const char *mountPoint);

int get_item_index(layout_t *l);

/// pthreads

typedef enum pt_status
{
    CANCELED = -1,
    READY,
    PAUSED,
    RUNNING,
    COMPLETED
} pt_status;

typedef struct
{
    const char *url,
               *dst;
    int req,
        idx,    // thread index!
        connid,
        tmpid,
        status, // thread status
        g_idx;  // global item index in icon panel
    double      progress;
    uint64_t    contentLength; 
    item_idx_t *token_d;  // token_data item index type pointer
    bool is_threaded;
} dl_arg_t;


int dl_from_url   (const char *url_, const char *dst_, bool is_threaded);
int dl_from_url_v2(const char *url_, const char *dst_, item_idx_t *t);

void queue_panel_init(void);
int  thread_find_by_item   (int req_idx);
int  thread_find_by_status (int req_idx, int req_status);
int  thread_count_by_status(int req_status);
int  thread_dispatch_index(void);

uint8_t pkginstall(const char* path);
