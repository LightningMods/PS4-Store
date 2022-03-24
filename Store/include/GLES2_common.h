#pragma once
#include "defines.h"
#include <time.h>
/// from demo-font.c
#include <freetype-gl.h>
// freetype-gl pass last composed Text_Length in pixel, we use to align text!
// from json_simple.c

#include "net.h"

/*
    GLES2 layout rework, v2
*/

/*
    todo: we need to differentiate and made use of vbo_status
    to avoid useless pushes to gpu
*/

/*
    auxiliary array of item_index_t:
    - shared for Search/Groups
    - the query to save search results;
*/


typedef struct
{
    char* name;
} entry_t;

typedef enum pt_status
{
    CANCELED = -1,
    READY,
    PAUSED,
    RUNNING,
    COMPLETED
} pt_status;
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

#define ORBIS_LIBC_MALLOC_MANAGED_SIZE_VERSION (0x0001U)

#ifndef SCE_LIBC_INIT_MALLOC_MANAGED_SIZE
#define SCE_LIBC_INIT_MALLOC_MANAGED_SIZE(mmsize) do { \
	mmsize.size = sizeof(mmsize); \
	mmsize.version = ORBIS_LIBC_MALLOC_MANAGED_SIZE_VERSION; \
	mmsize.reserved1 = 0; \
	mmsize.maxSystemSize = 0; \
	mmsize.currentSystemSize = 0; \
	mmsize.maxInuseSize = 0; \
	mmsize.currentInuseSize = 0; \
} while (0)
#endif

typedef struct SceLibcMallocManagedSize {
    uint16_t size;
    uint16_t version;
    uint32_t reserved1;
    size_t maxSystemSize;
    size_t currentSystemSize;
    size_t maxInuseSize;
    size_t currentInuseSize;
} SceLibcMallocManagedSize;


#define ARRAYSIZE(a) ((sizeof(a) / sizeof(*(a))) / ((size_t)(!(sizeof(a) % sizeof(*(a))))))

struct _ent {
    char fname[1048];
    uint32_t sz;
};

struct CVec
{
    uint32_t sz, cur;		// In BYTES
    void* ptr;

};

typedef struct
{
    item_idx_t* token_d;  // data
    int         token_c; // count
    bool        is_ext_hdd;
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
    item_t* item_d;    // item_data array
    int     item_c,    // item_count
        curr_item, // current selected item:
        f_size,    // field size
        f_sele;    // selected in field
    vec4* f_rect;    // field rectangle array
    // each layout will hold its ft-gl vertex buffer object
    vertex_buffer_t* vbo;
    enum vbo_status  vbo_s; // vbo_status
    // layout status
    char   is_shown,
        is_active; // to control selected_item (deprecated by active_p)
} layout_t;

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

// menu entry strings
#define  LPANEL_Y  (5)
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

extern item_idx_t* aux, * q; // aux and Search Query

extern GLuint fallback_t;
/*
    array for Special cases
*/
extern item_t* games,
* groups,
* all_apps; // Installed_Apps


// for Settings (options_panel)
extern bool use_reflection,
use_pixelshader;

// texts
extern char* gm_p_text[5]; // 5
extern char* new_panel_text[5][11];
// posix threads args, for downloading
extern dl_arg_t* pt_info;

char* get_json_token_value(item_idx_t* item, int name);
void refresh_atlas(void);

// from GLES2_rects.c
void on_GLES2_Update1(double time);

// from GLES2_textures.c
void on_GLES2_Init_icons(int view_w, int view_h);
void on_GLES2_Update(double time);
//void on_GLES2_Render_icons(int num);
void on_GLES2_Render_icon(enum SH_type SL_program, GLuint texture, int num, vec4* frect, vec4* opt_uv);

void on_GLES2_Render_box(vec4* frect);
void on_GLES2_Final(void);

// from pixelshader.c
void pixelshader_render(GLuint SL_program, vertex_buffer_t* vbo, vec2* resolution);
void pixelshader_init(int width, int height);
void pixelshader_fini(void);

void ORBIS_RenderSubMenu(int num);

// GLES2_font.c
void GLES2_fonts_from_ttf(const char* path);
void GLES2_init_submenu(void);
void GLES2_render_submenu_text(int num);
void ftgl_render_vbo(vertex_buffer_t* vbo, vec3* offset);

// todo: sort out those proto
uint32_t sceKernelGetCpuTemperature(uint32_t* res);
int sceKernelAvailableFlexibleMemorySize(size_t* free_mem);

int notify(char* message);
void escalate_priv(void);
int initGL_for_the_store(bool reload_apps, int ref_pages);
int pingtest(int libnetMemId, int libhttpCtxId, const char* src);


item_idx_t* analyze_item_t(item_t* items, int item_count);
item_t* analyze_item_t_v2(item_t* items, int item_count);
item_idx_t* search_item_t(item_t* items, int item_count, enum token_name TN, char* pattern);
item_t* index_items_from_dir(const char* dirpath, const char* dirpath2);

void build_char_from_items(char** data, item_idx_t* filter);
item_idx_t* build_item_list(item_t* items, int item_count, enum token_name TN);


int index_token_from_sfo(item_t* item, char* path, int lang);

void destroy_item_t(item_idx_t** p);

int  df(char* out, const char* mountPoint);
void get_stat_from_file(char* out, const char* filepath);

int get_item_index(layout_t* l);
// from GLES2_scene_v2.c
layout_t* GLES2_layout_init(int req_item_count);

vertex_buffer_t* vbo_from_rect(vec4* rect);

void GLES2_UpdateVboForLayout(layout_t* l);

int layout_fill_item_from_list(layout_t* l, const char** i_list);

void fw_action_to_cf(int button);

void GLES2_render_menu(int unused);

// from GLES2_scene.c
void GLES2_scene_init(int w, int h);
void GLES2_scene_render(void);
void GLES2_scene_on_pressed_button(int button);
//void X_action_dispatch(int action, layout_t *l);

/// pthreads used in GLES2_q.c


/* GLES2_panel.h */
extern texture_font_t* main_font, // small
* sub_font, // left panel
* titl_font; // bigger
/* normalized rgba colors */
extern vec4 sele, // default for selection (blue)
grey, // rect fg
white,
col; // text fg

extern ivec4  menu_pos;
extern double u_t;

extern const unsigned char completeVersion[];

void GLES2_Draw_sysinfo(void);
void GLES2_refresh_sysinfo(void);

// disk free percentages
extern double dfp_hdd,
dfp_ext,
dfp_now;

extern layout_t* active_p,     // the active panel moves selector around
* ls_p,     // example list panel
* icon_panel,
* left_panel,
* left_panel2,
* option_panel,
* download_panel,
* queue_panel;

// GLES2_layout
void layout_update_fsize(layout_t* l);
void layout_update_sele(layout_t* l, int movement);
void GLES2_Refresh_for_settings();
void layout_set_active(layout_t* l);
void swap_panels(void);
void GLES2_render_layout_v2(layout_t* l, int unused);

void GLES2_render_list(int unused);
void GLES2_render_paged_list(int unused);
void GLES2_render_icon_list(int unused);


// scene_v2.c
void set_cmp_token(const int index);
int struct_cmp_by_token(const void* a, const void* b);

// used for Groups
void recreate_item_t(item_t** items);


/// from shader-common.c
GLuint BuildProgram(const char* vShader, const char* fShader, int vs_size, int fs_size);
static GLuint compile(GLenum type, const char* source, int size);


// from cmd_build.c
char** build_cmd(char* cmd_line);


extern float tl;

int  es2init_text(int width, int height);
void add_text(vertex_buffer_t* buffer,
    texture_font_t* font,
    const char* text,
    vec4* color,
    vec2* pen);
void render_text(void);
void es2sample_end(void);

#if defined TEXT_ANI
/// from text_ani.c
#include "text_ani.h"
void es2init_text_ani(int width, int height);
void es2rndr_text_ext(fx_entry_t* _ani);
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
void         release_raw_image_data(const RawImageData* data);
GLuint load_texture(const GLsizei width, const GLsizei height,
    const GLenum  type, const GLvoid* pixels);
// higher level helper
GLuint load_png_asset_into_texture(const char* relative_path);
GLuint load_png_data_into_texture(const char* data, int size);
extern vec2 tex_size; // last loaded png size as (w, h)
int writeImage(char* filename, int width, int height, int* buffer, char* title);


/// from sprite.c
void on_GLES2_Init_sprite(int view_w, int view_h);
void on_GLES2_Size_sprite(int view_w, int view_h);
void on_GLES2_Render_sprite(int num);
void on_GLES2_Update_sprite(int frame);
void on_GLES2_Final_sprite(void);


/// from pl_mpeg.c
int  es2init_pl_mpeg(int window_width, int window_height);
void es2render_pl_mpeg(float dt);
void es2end_pl_mpeg(void);


/// from timing.c
unsigned int get_time_ms(void);


#if defined HAVE_NFS
/// from user_nfs.c
#include <nfsc/libnfs.h>
int    user_init(void);
size_t user_stat(void);
struct
    nfsfh* user_open(const char* filename);
void   user_seek(long offset, int whence);
size_t user_read(unsigned char* dst, size_t size);
void   user_close(void);
void   user_end(void);
#endif

// rects tests tetris primlib
void filledRect(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b);

/// from ls_dir()
bool if_exists(const char* path);
int check_stat(const char* filepath);

entry_t* get_item_entries(const char* dirpath, int* count);
void free_item_entries(entry_t* e, int num);

// from my_rects.c
vec2 px_pos_to_normalized(vec2* pos);


// from lines_and_rects
void es2rndr_lines_and_rect(void);
void es2init_lines_and_rect(int width, int height);
void es2fini_lines_and_rect(void);


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
void ORBIS_RenderDrawLines(const vec2* points, int count);
void ORBIS_RenderFillRects(enum SH_type SL_program, const vec4* rgba, const vec4* rects, int count);
void ORBIS_RenderDrawBox(enum SH_type SL_program, const vec4* rgba, const vec4* rect);
void GLES2_DrawFillingRect(vec4* frect, vec4* color, double* percentage);

// from GLES2_ani.c
void GLES2_ani_init(int width, int height);
void GLES2_ani_update(double time);
void GLES2_ani_fini(void);
void GLES2_ani_draw(void);
void ani_notify(const char* message);