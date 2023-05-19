#pragma once


#include <time.h>
/// from demo-font.c
#include <freetype-gl.h>
#ifdef __ORBIS__
#include <store_api.h>
#endif
#include <string>
#include <vector>
#include "fmt/format.h"
#include <atomic>
#include <memory>
#include "classes.hpp"
// freetype-gl pass last composed Text_Length in pixel, we use to align text!
// from json_simple.c

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

// reuse this type to index texts around!
/// for icons.c, sprite.c
#define NUM_OF_TEXTURES  (8)
#define NUM_OF_SPRITES   (6)
/// from GLES2_rects.c
enum SH_type
{
    USE_COLOR,
    USE_UTIME,
    CUSTOM_BADGE,
    NUM_OF_PROGRAMS
};

struct item_idx_t
{
	std::string off;
	bool is_active = false;
	int len = 0;
};

#include "net.h"


struct page_item_t
{
    item_idx_t token[NUM_OF_USER_TOKENS]; // indexed tokens from json_data
    ivec2      token_i[NUM_OF_USER_TOKENS]; // enough for indexing all tokens, in ft-gl VBO
    int        num_of_texts;                // num of indexed tokens we print to screen
    GLuint     texture;                     // the textured icon from PICPATH
    vec4       frect;                       // the normalized position and size of texture icon
    vec2       origin;  // normalized origin point
    // 5x3
    vec2       pen;
    char       in_atlas;
};


// the single page holds its infos
struct page_info_t
{
    // max NUM_OF_TEXTURES per page, max 8 items
    int  num_of_items;
    // each page will hold its json_data
    char* json_data;
    // and its ft-gl vertex buffer
    //vertex_buffer_t* vbo;
    VertexBuffer vbo;
    // all indexed tokens, per item... (zerocopy, eh)
    page_item_t  item[NUM_OF_TEXTURES];
    // from 1, refrect json num in filename
    int page_num;
};


struct entry_tt
{
    char* name;
};

enum pt_status
{
    CANCELED = -1,
    READY,
    PAUSED,
    RUNNING,
    COMPLETED,
    INSTALLING_APP
};
enum vbo_status
{
    EMPTY,
    APPEND,
    CLOSED,
    ASK_REFRESH,
    IN_ATLAS,
    IN_ATLAS_APPEND,
    IN_ATLAS_CLOSED
};

enum badge_t
{
    AVAILABLE = 1,
    SOON,
    SELECTED,
    NUM_OF_BADGES
};

#define ORBIS_LIBC_MALLOC_MANAGED_SIZE_VERSION (0x0001U)
/*	unsigned short sz;
	unsigned short ver;
	unsigned int reserv;
	size_t maxSysSz;
	size_t curSysSz;
	size_t maxUseSz;
	size_t curUseSz;*/
#ifndef SCE_LIBC_INIT_MALLOC_MANAGED_SIZE
#define SCE_LIBC_INIT_MALLOC_MANAGED_SIZE(mmsize) do { \
	mmsize.sz = sizeof(mmsize); \
	mmsize.ver = ORBIS_LIBC_MALLOC_MANAGED_SIZE_VERSION; \
	mmsize.reserv = 0; \
	mmsize.maxSysSz = 0; \
	mmsize.curSysSz = 0; \
	mmsize.maxUseSz = 0; \
	mmsize.curUseSz = 0; \
} while (0)
#endif

#define GL_NULL 0


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
#define UPDATE_NOT_CHECKED -1
#ifndef __ORBIS__
enum update_ret{
    APP_NOT_INSTALLED,
    UPDATE_FOUND,
    UPDATE_ERROR,
    NO_UPDATE,
};
#else
#endif

//extern active_panel active_p;
#include <mutex>
#include <vector>
#include <algorithm>
struct item_t
{
    std::atomic<update_ret>  update_status;
    std::vector<item_idx_t> token_d;
    int         token_c;
    bool        failed_dl;
    bool        png_is_loaded;
    std::atomic<GLuint>      texture;
    vec4        uv;
    badge_t     badge;
    bool        interruptible;
    bool        is_active;

    item_t() : update_status(static_cast<update_ret>(UPDATE_NOT_CHECKED)),
               token_c(0), failed_dl(false), png_is_loaded(false), texture(GL_NULL),
               interruptible(false), is_active(false) 
    {
        uv = {0, 0, 0, 0};
    }

    // Custom copy constructor
    item_t(const item_t& other) :  update_status(other.update_status.load()),
                                   token_d(other.token_d),
                                   token_c(other.token_c),
                                   failed_dl(other.failed_dl),
                                   png_is_loaded(other.png_is_loaded),
                                   texture(other.texture.load()),
                                   uv(other.uv),
                                   badge(other.badge),
                                   interruptible(other.interruptible),
                                   is_active(other.is_active)
    {
    }

    // Custom copy assignment operator
    item_t& operator=(const item_t& other) {
        if (this == &other) {
            return *this;
        }

        update_status.store(other.update_status.load());
        token_d = other.token_d;
        token_c = other.token_c;
        failed_dl = other.failed_dl;
        png_is_loaded = other.png_is_loaded;
        texture = other.texture.load();
        uv = other.uv;
        badge = other.badge;
        interruptible = other.interruptible;
        is_active = other.is_active;

        return *this;
    }
};


struct layout_t
{
    vec4        bound_box;
    ivec2       fieldsize,
                item_sel,
                page_sel;
    int         retries;
   std::vector<item_t> item_d;
   //item_t* item_d = NULL;
    int         item_c,
                curr_item,
                f_size,
                f_sele;
    std::vector<vec4> f_rect;

    //vec4 f_rect;    // field rectangle array
    // each layout will hold its ft-gl vertex buffer object
    //vertex_buffer_t* vbo = NULL;
    VertexBuffer vbo;
    vbo_status   vbo_s;
    bool is_shown, is_active;
    std::recursive_mutex mtx;

    layout_t() : retries(0), item_c(0), curr_item(0), f_size(0), f_sele(0),
             vbo_s(EMPTY), is_shown(false), is_active(false)
    {
        bound_box = {0, 0, 0, 0};
        fieldsize = {0, 0};
        item_sel = {0, 0};
        page_sel = {0, 0};
    }
};



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

#if defined(__ORBIS__)

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
#define L2   (222)
#else

#define UP   (98)
#define DOW  (104)
#define LEF  (100)
#define RIG  (102)
#define CRO  ( 53)
#define CIR  ( 54)
#define TRI  ( 28)
#define SQU  ( 39)
#define OPT  ( 32)
#define L1   ( 10)
#define R1   ( 11)
#define L2   ( 12)
#endif

extern std::vector<item_idx_t> aux,  q; // aux and Search Query

extern GLuint fallback_t;
/*
    array for Special cases
*/
extern std::vector<item_t> games,
 groups;


// for Settings (options_panel)
extern bool use_reflection;

// texts
extern std::vector<std::vector<std::string>> new_panel_text;
// posix threads args, for downloading
extern std::vector<dl_arg_t*> pt_info;

void refresh_atlas(void);

// from GLES2_rects.c
void on_GLES2_Update1(double time);

// from GLES2_textures.c
void on_GLES2_Init_icons(int view_w, int view_h);
void on_GLES2_Update(double time);
//void on_GLES2_Render_icons(int num);
void on_GLES2_Render_icon(enum SH_type SL_program, GLuint texture, int num, vec4 &frect, vec4 *uv = nullptr);
void on_GLES2_Final(void);
void ORBIS_RenderSubMenu(int num);

// GLES2_font.c
void GLES2_fonts_from_ttf(const char* path);
void GLES2_init_submenu(void);
void GLES2_render_submenu_text(int num);
void ftgl_render_vbo(vertex_buffer_t* vbo, vec3* offset);

int initGL_for_the_store(bool reload_apps, int ref_pages);

std::vector<item_t> analyze_item_t_v2(std::vector<item_t>& items, int numb);
std::vector<item_idx_t> search_item_t(std::vector<item_t>& items, token_name TN, const std::string &pattern);
item_t* index_items_from_dir(const char* dirpath, const char* dirpath2);

void build_char_from_items(std::vector<std::string>& data, const std::vector<item_idx_t>& filter);
std::vector<item_idx_t> build_item_list(std::vector<item_t>& items, token_name TN);


int index_token_from_sfo(item_t* item, char* path, int lang);

void destroy_item_t(std::vector<item_idx_t> &p);

int df(std::string mountPoint, std::string &out);
void get_stat_from_file(char* out, const char* filepath);

int get_item_index(std::shared_ptr<layout_t>  &l);
// from GLES2_scene_v2.c
std::shared_ptr<layout_t>  GLES2_layout_init(int req_item_count);

VertexBuffer vbo_from_rect(vec4 &rect);

void GLES2_UpdateVboForLayout(std::shared_ptr<layout_t>  &l);
int layout_fill_item_from_list(std::shared_ptr<layout_t>  &l, std::vector<std::string> &i_list);
// from GLES2_scene.c
void GLES2_scene_init(int w, int h);
void GLES2_scene_render(const char* query);
void GLES2_scene_on_pressed_button(int button);
void X_action_dispatch(int action, std::shared_ptr<layout_t> &l);
void layout_refresh_VBOs(void);
void GLES2_Draw_common_texts(void);
void GLES2_render_download_panel(void);
void O_action_dispatch(void);
void pixelshader_init(int width, int height);
void pixelshader_render();

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

extern std::string completeVersion;

void GLES2_Draw_sysinfo(void);
void GLES2_refresh_sysinfo(void);

// disk free percentages
extern double dfp_hdd,
dfp_ext,
dfp_now;

extern std::shared_ptr<layout_t>  active_p,     // the active panel moves selector around
 icon_panel,
left_panel,
left_panel2,
option_panel,
download_panel,
queue_panel;

// single item infos, for the page_info_t below
// GLES2_layout
void layout_update_fsize(std::shared_ptr<layout_t>  &l);
void layout_update_sele(std::shared_ptr<layout_t>  &l, int movement);
void GLES2_Refresh_for_settings();
void layout_set_active(std::shared_ptr<layout_t>  &l);
void GLES2_render_layout_v2(std::shared_ptr<layout_t>  &l, int unused);

void GLES2_render_list(int unused);
void GLES2_render_paged_list(int unused);
void GLES2_render_icon_list(int unused);


// scene_v2.c
void set_cmp_token(const int index);
int struct_cmp_by_token(const void* a, const void* b);

// used for Groups
void recreate_item_t(std::vector<item_t> &i);


/// from shader-common.c
GLuint BuildProgram(const char* vShader, const char* fShader, int vs_size, int fs_size);
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

typedef enum {
    TEXTURE_LOAD_MAIN_MENU = -1,
    TEXTURE_LOAD_DEFAULT,
    TEXTURE_DOWNLOAD,
    TEXTURE_LOAD_PNG,
} texture_load_status_t;

RawImageData get_raw_image_data_from_png(const void* png_data, const int png_data_size, const char* fp);
void         release_raw_image_data(const RawImageData* data);
bool is_png_vaild(const char *relative_path);
GLuint load_texture(const GLsizei width, const GLsizei height,
    const GLenum  type, const GLvoid* pixels);
// higher level helper
GLuint load_png_data_into_texture(const unsigned char* data, int size);
bool check_n_load_texture(const int idx, texture_load_status_t status);
extern vec2 tex_size; // last loaded png size as (w, h)
int writeImage(char* filename, int width, int height, int* buffer, const char* title);
GLuint load_png_asset_into_texture(const char* relative_path);
void Install_View(std::shared_ptr<layout_t>  &l, const char* query_string, enum token_name nm);
void Update_View(std::shared_ptr<layout_t>  &l);

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

/// from ls_dir()
bool if_exists(const char* path);
int check_stat(const char* filepath);

// from my_rects.c
vec2 px_pos_to_normalized(vec2* pos);

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
void ORBIS_RenderFillRects(enum SH_type SL_program, const vec4 &rgba, std::vector<vec4> &rects, int count);
void ORBIS_RenderDrawBox(enum SH_type SL_program, const vec4 &rgba, vec4 &r);
void GLES2_DrawFillingRect(std::vector<vec4> &r, vec4 &c, const double percentage);
void ORBIS_RenderArrowAtCoords(enum SH_type SL_program, float x, float y);
// from GLES2_ani.c
// UI panels new way, v3
vec4 get_rect_from_index(const int idx, const std::shared_ptr<layout_t>  &l, vec4* io);
void buffer_to_off(item_t* ret, int index_t, char* str);


/// from GLES2_badges
int  scan_for_badges(std::shared_ptr<layout_t>  &l, item_t* apps);
void GLES2_Init_badge(void);
void GLES2_Render_badge(int idx, vec4* rect);

void GLES2_render_queue(std::shared_ptr<layout_t>  &l, int used);
void glDeleteTextures1(int unused, GLuint* text);
void GLES2_refresh_common(void);