#pragma once

/*
    ES2_goodness, a GLES2 playground on EGL

    2019-2021 masterzorag & LM

    here follows the parts:
*/

// nfs or local stdio
//#define USE_NFS  (1)
#if defined (USE_NFS)
#include <orbisNfs.h>
#endif

// common
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
#include <math.h>
#include <time.h>
#include "log.h"

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


#define SCE_LNC_ERROR_APP_NOT_FOUND 0x80940031 // Usually happens if you to launch an app not in app.db
#define SCE_LNC_UTIL_ERROR_ALREADY_INITIALIZED 0x80940018
#define SCE_LNC_UTIL_ERROR_ALREADY_RUNNING 0x8094000c
#define SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_KILL_NEEDED 0x80940010
#define SCE_LNC_UTIL_ERROR_ALREADY_RUNNING_SUSPEND_NEEDED 0x80940011
#define SCE_LNC_UTIL_ERROR_APP_ALREADY_RESUMED 0x8094001e
#define SCE_LNC_UTIL_ERROR_APP_ALREADY_SUSPENDED 0x8094001d
#define SCE_LNC_UTIL_ERROR_APP_NOT_IN_BACKGROUND 0x80940015
#define SCE_LNC_UTIL_ERROR_APPHOME_EBOOTBIN_NOT_FOUND 0x80940008
#define SCE_LNC_UTIL_ERROR_APPHOME_PARAMSFO_NOT_FOUND 0x80940009
#define SCE_LNC_UTIL_ERROR_CANNOT_RESUME_INITIAL_USER_NEEDED 0x80940012
#define SCE_LNC_UTIL_ERROR_DEVKIT_EXPIRED 0x8094000b
#define SCE_LNC_UTIL_ERROR_IN_LOGOUT_PROCESSING 0x8094001a
#define SCE_LNC_UTIL_ERROR_IN_SPECIAL_RESUME 0x8094001b
#define SCE_LNC_UTIL_ERROR_INVALID_PARAM 0x80940005
#define SCE_LNC_UTIL_ERROR_INVALID_STATE 0x80940019
#define SCE_LNC_UTIL_ERROR_INVALID_TITLE_ID 0x8094001c
#define SCE_LNC_UTIL_ERROR_LAUNCH_DISABLED_BY_MEMORY_MODE 0x8094000d
#define SCE_LNC_UTIL_ERROR_NO_APP_INFO 0x80940004
#define SCE_LNC_UTIL_ERROR_NO_LOGIN_USER 0x8094000a
#define SCE_LNC_UTIL_ERROR_NO_SESSION_MEMORY 0x80940002
#define SCE_LNC_UTIL_ERROR_NO_SFOKEY_IN_APP_INFO 0x80940014
#define SCE_LNC_UTIL_ERROR_NO_SHELL_UI 0x8094000e
#define SCE_LNC_UTIL_ERROR_NOT_ALLOWED 0x8094000f
#define SCE_LNC_UTIL_ERROR_NOT_INITIALIZED 0x80940001
#define SCE_LNC_UTIL_ERROR_OPTICAL_DISC_DRIVE 0x80940013
#define SCE_LNC_UTIL_ERROR_SETUP_FS_SANDBOX 0x80940006
#define SCE_LNC_UTIL_ERROR_SUSPEND_BLOCK_TIMEOUT 0x80940017
#define SCE_LNC_UTIL_ERROR_VIDEOOUT_NOT_SUPPORTED 0x80940016
#define SCE_LNC_UTIL_ERROR_WAITING_READY_FOR_SUSPEND_TIMEOUT 0x80940021
#define SCE_SYSCORE_ERROR_LNC_INVALID_STATE 0x80aa000a
#define SCE_LNC_UTIL_ERROR_NOT_INITIALIZED 0x80940001

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
// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;

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
typedef struct {
    /* for image data */
    const int width;
    const int height;
    const int size;
    const GLenum gl_color_format;
    const void* data;
} RawImageData;

RawImageData get_raw_image_data_from_png(const void* png_data, const int png_data_size);
void         release_raw_image_data     (const RawImageData* data);
GLuint load_texture(const GLsizei width, const GLsizei height,
                    const GLenum  type,  const GLvoid *pixels);
// higher level helper
GLuint load_png_asset_into_texture(const char *relative_path);
extern vec2 tex_size; // last loaded png size as (w, h)
int writeImage(char* filename, int width, int height, int *buffer, char* title);


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
bool if_exists(const char *path);
int check_stat(const char *filepath);

int get_item_count(void); // to remove
typedef struct
{
    char  *name;
    //size_t size;
} entry_t;
entry_t *get_item_entries(const char *dirpath, int *count);
void    free_item_entries(entry_t *e);

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
    CUSTOM_BADGE,
    NUM_OF_PROGRAMS
};

void ORBIS_RenderDrawLines(const vec2 *points, int count);
void ORBIS_RenderFillRects(enum SH_type SL_program, const vec4 *rgba, const vec4 *rects,  int count);
void ORBIS_RenderDrawBox  (enum SH_type SL_program, const vec4 *rgba, const vec4 *rect);
void GLES2_DrawFillingRect(vec4 *frect, vec4 *color, double *percentage);

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
    ON_QUEUE,       // 1 - Queue
    ON_INSTALL,     // 2 - Ready to install
    ON_EXTRA_PAGE,  // 5 - pixelshader
    ON_SETTINGS,    // 6
    ON_FILEMANAGER, // 7
    ON_ITEMzFLOW,    // 8
    // draws Storage infos
    ON_ITEM_INFO   // download_panel
};


#define UP   (111)
#define DOW  (116)
#define LEF  (113)
#define RIG  (114)
#define CRO  ( 53)
#define CIR  ( 54)
#define TRI  ( 28)
#define SQU  ( 39)
#define OPT  ( 32)
#define L1   ( 10)
#define R1   ( 11)


// from GLES2_ani.c
void GLES2_ani_init  (int width, int height);
void GLES2_ani_update(double time);
void GLES2_ani_fini  (void);
void GLES2_ani_draw  (void);
void ani_notify(const char *message);

// from json_simple.c
#include "json.h"
int count_availables_json(void);

char *get_json_token_value(item_idx_t *item, int name);
void refresh_atlas(void);

// from GLES2_rects.c
void on_GLES2_Update1(double time);

// from GLES2_textures.c
void on_GLES2_Init_icons(int view_w, int view_h);
void on_GLES2_Update(double time);
//void on_GLES2_Render_icons(int num);
void on_GLES2_Render_icon(enum SH_type SL_program, GLuint texture, int num, vec4 *frect, vec4 *opt_uv);

void on_GLES2_Render_box(vec4 *frect);
void on_GLES2_Final(void);

// from pixelshader.c
void pixelshader_render( GLuint SL_program, vertex_buffer_t *vbo, vec2 *resolution );
void pixelshader_init  ( int width, int height );
void pixelshader_fini  ( void );

void ORBIS_RenderSubMenu(int num);

// GLES2_font.c
void GLES2_fonts_from_ttf(const char *path);
void GLES2_init_submenu( void );
void GLES2_render_submenu_text( int num );
void ftgl_render_vbo( vertex_buffer_t *vbo, vec3 *offset );

// todo: sort out those proto
uint32_t sceKernelGetCpuTemperature(uint32_t *res);
int sceKernelAvailableFlexibleMemorySize(size_t *free_mem);

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
    ASK_REFRESH,
    IN_ATLAS,
    IN_ATLAS_APPEND,
    IN_ATLAS_CLOSED
} vbo_status;

typedef enum badge_t
{
    AVAILABLE = 1,
    SOON,
    SELECTED,
    NUM_OF_BADGES
} badge_t;

typedef struct
{
    item_idx_t *token_d;  // data
    int         token_c;  // count
    GLuint      texture;  // the item icon
    // if the item icon is cached in atlas, use UV
    vec4             uv;  // normalized p1, p2
    enum badge_t  badge;  // 

    // need to add index for texture atlas
} item_t;

typedef struct
{
     vec4   bound_box; // total field size, in px
    ivec2   fieldsize, // total field size, in items number
            item_sel,  // current selected item into field
            page_sel;  // page_selection: current, max page
    int retries;       // check for what's used for and remove
    // an array of items
    item_t *item_d;    // item_data array
    int     item_c,    // item_count
            curr_item, // current selected item:
            f_size,    // field size
            f_sele;    // selected in field
    vec4   *f_rect;    // field rectangle array
    // each layout will hold its ft-gl vertex buffer object
    vertex_buffer_t *vbo;
    enum vbo_status  vbo_s; // vbo_status
    // layout status
    char   is_shown,
           is_active; // to control selected_item (deprecated by active_p)
} layout_t;


item_idx_t *analyze_item_t      (item_t *items, int item_count);
item_t     *analyze_item_t_v2   (item_t *items, int item_count);
item_idx_t *search_item_t       (item_t *items, int item_count, enum token_name TN, char *pattern);
item_t     *index_items_from_dir(const char *dirpath);

void build_char_from_items(char **data, item_idx_t *filter);
item_idx_t *build_item_list(item_t *items, int item_count, enum token_name TN);


int index_token_from_sfo(item_t *item, char *path);

void destroy_item_t(item_idx_t **p);

int  df(char *out, const char *mountPoint);
void get_stat_from_file(char *out, const char *filepath);

int get_item_index(layout_t *l);
// from GLES2_scene_v2.c
layout_t *GLES2_layout_init(int req_item_count);

vertex_buffer_t *vbo_from_rect(vec4 *rect);

void GLES2_UpdateVboForLayout(layout_t *l);

int layout_fill_item_from_list(layout_t *l, const char **i_list);

void fw_action_to_cf(int button);

void GLES2_render_menu(int unused);

// from GLES2_scene.c
void GLES2_scene_init( int w, int h );
void GLES2_scene_render(void);
void GLES2_scene_on_pressed_button(int button);
//void X_action_dispatch(int action, layout_t *l);

/// pthreads used in GLES2_q.c

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


/// from GLES2_badges
int  scan_for_badges(layout_t *l, item_t *apps);
void GLES2_Init_badge(void);
void GLES2_Render_badge(int idx, vec4 *rect);

/// GLES2_filemamager
void fw_action_to_fm(int button);
void GLES2_render_filemanager(int unused);

void GLES2_render_queue(layout_t *l, int used);

//void X_action_dispatch(int action, layout_t *l);

// test fts.c and cp()
int main_cp(int argc, char *argv[]);


void GLES2_render_menu(int unused);

// UI panels new way, v3
vec4 get_rect_from_index(const int idx, const layout_t *l, vec4 *io);


