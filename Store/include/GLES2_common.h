
#include "defines.h"
#include "utils.h"

/* GLES2_panel.h */
extern vec2           resolution;
extern texture_font_t *main_font, // small
                       *sub_font, // left panel
                      *titl_font; // bigger
/* normalized rgba colors */
extern vec4 sele, // default for selection (blue)
            grey, // rect fg
           white,
             col; // text fg

extern ivec4  menu_pos;
extern double u_t;

extern const unsigned char completeVersion[];

#include <time.h>

void GLES2_Draw_sysinfo(void);
void GLES2_refresh_sysinfo(void);

// disk free percentages
extern double dfp_hdd,
              dfp_ext,
              dfp_now;

extern layout_t *active_p,     // the active panel moves selector around
                    *ls_p,     // example list panel
                  *icon_panel,
                  *left_panel,
                  *left_panel2,
                *option_panel,
              *download_panel,
                 *queue_panel;
/*
    array for Special cases
*/
extern item_t *games,
             *groups,
             *i_apps; // Installed_Apps
/*
    auxiliary array of item_index_t:
    - shared for Search/Groups
    - the query to save search results;
*/
extern item_idx_t *aux, *q; // aux and Search Query

extern GLuint fallback_t;

// the Settings
extern StoreOptions set,
                   *get;

// for Settings (options_panel)
extern bool use_reflection,
            use_pixelshader;

// texts
extern const char *gm_p_text[], // 5
                  *new_panel_text[5][11];
// posix threads args, for downloading
extern dl_arg_t        *pt_info;

// GLES2_layout
void layout_update_fsize(layout_t *l);
void layout_update_sele (layout_t *l, int movement);
void layout_set_active  (layout_t *l);
void swap_panels(void);
void GLES2_render_layout_v2(layout_t *l, int unused);

void GLES2_render_list(int unused);
void GLES2_render_paged_list(int unused);
void GLES2_render_icon_list(int unused);


// scene_v2.c
void set_cmp_token(const int index);
int struct_cmp_by_token(const void *a, const void *b);

// used for Groups
void recreate_item_t(item_t **items);
