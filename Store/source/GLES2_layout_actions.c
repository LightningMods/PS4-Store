/*
    GLES2_layout_actions.c

    part of the GLES2 playground: draw a basic UI
    holding a selector to trigger some action, etc

    _from now on 'l', stands for 'layout'_

    2020, 2021, masterzorag && LM
*/

#include "defines.h"
#include "utils.h"
#include "GLES2_common.h"


int DL_CO = 0;

// (Filter_by)
/* qsort struct comparison function (C-string field) */
static int struct_cmp_by_count(const void *a, const void *b)
{
    item_idx_t *ia = (item_idx_t *)a;
    item_idx_t *ib = (item_idx_t *)b;
    return (ib->len - ia->len);
/* strcmp functions works exactly as expected from comparison function */
}

static enum token_name label;

static void layout_dispatch_X(layout_t *l)
{
    bool TID_VAILD = false;
    char tmp[256],
    pattern [ 70];
    // default indexing
    int idx = l->curr_item;

    if(l == queue_panel) // go to download_panel
    {
        int req_status;
        switch(menu_pos.z)
        {
            case ON_INSTALL:  req_status = COMPLETED;  break;
            case ON_QUEUE:    req_status = RUNNING;    break;
        }
        // get index from selected thread info
        idx = thread_find_by_status(l->f_sele, req_status);
        // no results, :shrug:
        if(idx < 0) log_error( "this should not happen", idx);
        // set to current selected
        icon_panel->curr_item = pt_info[ idx ].g_idx;
        // refresh indexes
        layout_update_sele(icon_panel, 0);

        goto switch_to_download;
    }

    // override
    if(l == icon_panel
    && aux
    && aux->len) idx = get_item_index(l);

    if(l->item_d[ idx ].token_d[0].len > 0)
    {   // try to get the first indexed token value
        item_t *li = &l->item_d[ idx ];

        if(li->token_d) {
            snprintf(&tmp[0], 255, "%s", li->token_d[0].off);
            TID_VAILD = true;
        } else { // safe fallback
            snprintf(&tmp[0], 255, "%d, %p", idx, li->token_d);
            TID_VAILD = false;
        }
        log_info( "execute %d -> '%s'", idx, tmp);
    }
    else
    {
        log_info( "execute %d -> '%p'", l->curr_item,
                                   l->item_d[ l->curr_item ].token_d);
    }

    /* follows actions for panels */

    if(l == left_panel2)
    {
        switch( l->page_sel.x )
        {
            case 0: // first page
            {
                if(l->curr_item == 0) { menu_pos.z = ON_MAIN_SCREEN;
                    // set left_panel 2nd page
                    l->page_sel.x = 1,
                    l->item_c     = 3, 
                    l->curr_item  = 0;
                    layout_fill_item_from_list(l, &new_panel_text[ l->page_sel.x ][0]);
                    layout_update_sele(l, 0);
                    active_p = icon_panel;
                    break;
                }

                if(l->curr_item == 1) { menu_pos.z = ON_ITEMzFLOW;
                    active_p = NULL;  return;
                }

                // 2 Groups
                if(l->curr_item == 2) {
                    // set left_panel 3nd page
                    l->page_sel.x = 2,
                    l->item_c     = groups->token_c;
                    l->curr_item  = 0;
                    layout_update_sele(l, 0);
                    active_p = left_panel2;
                    break;
                }

                // variable length lists
                if(l->curr_item == 3
                || l->curr_item == 4)
                { 
                    if(l->f_sele == 3) menu_pos.z = ON_INSTALL;
                    else
                    if(l->f_sele == 4) menu_pos.z = ON_QUEUE;
                    // activate and set focus
                    queue_panel_init();
                    l = queue_panel; // switch control
                    //l->page_sel.x = 0;
                    l->is_active  = 
                    l->is_shown = 1;
                    // reset selection to first entry
                    //l->item_sel   = (ivec2) (0);
                    layout_update_fsize(l);
                    active_p = queue_panel;  //active_p->is_shown = 1;
                    break;
                }

                // 5 Updates (todo)

                if(l->curr_item == 6) { menu_pos.z = ON_SETTINGS;
                    active_p = option_panel;  //active_p->is_shown = 1;
                }
                // set status
                active_p->is_shown = 1;
            } break;

            case 1: // on Games page
            {
                if(l->curr_item == 0)  /* Search for */
                {
                    memset(&pattern[0], 0, sizeof(pattern));
                    log_info("execute openDialogForSearch()");
                    // get char *pattern
                    snprintf(&pattern[0], 69, "%s", StoreKeyboard(NULL, "Search..."));
                    log_info( "@@@@@@@@@  Search for: %s", pattern);
                    loadmsg("Searching....");
                    label = NAME;

search_by_label:

                    destroy_item_t(&q);
                    // build an array of search results
                    q = search_item_t(icon_panel->item_d,
                                      icon_panel->item_c,
                                      label, &pattern[0]);
                    aux = q;

wen_found_hit:

                    if(aux && aux->len) // we got results, address aux!
                    {
                        log_info("Showing %d items for '%s' @ %p", aux->len,
                                                                   aux->off, aux);
                        //  for(int i = 0; i < aux->len; i++) { log_info("%d: %s %d", i, aux[i].off, aux[i].len); }
                        // switch focus
                        menu_pos.z   = ON_MAIN_SCREEN;
                        l = active_p = icon_panel;
                        // reset selection to first entry
                        l->curr_item = 0;
                        l->item_c    = aux->len;
                        layout_update_sele(l, 0);
                        // but now we have aux array!
                    }
                    else
                    {
                        char tmp[256];
                        snprintf(&tmp[0], 255, "No search results for %s!", aux->off);

                        ani_notify(&tmp[0]);

                        sceKernelIccSetBuzzer(3); // 1 longer
                    }
                    sceMsgDialogTerminate();
                }
                
                if(l->curr_item == 1)  /* switch to Sort labels */
                {   // set left_panel 4th page
                    l->page_sel.x =  3,
                    l->item_c     = 11;
                    active_p      = left_panel2;

                    goto switch_page;
                }
                // filter
                if(l->curr_item == 2)  /* switch to Filter_by */
                {   // set left_panel 5th page
                    l->page_sel.x = 4,
                    l->item_c     = 2; 
                    active_p      = left_panel2;
switch_page:
                    l->curr_item  = 0;
                    layout_fill_item_from_list(l, &new_panel_text[ l->page_sel.x ][0]);
                    layout_update_sele(l, 0);

                    goto refresh_active_panel;
                }

            } break;

            case 2: // on Groups page
            {
                log_info( "%s", __FUNCTION__);
                // map aux AOS to selected group
                aux      = groups[ l->curr_item +1 ].token_d;
                aux->len = groups[ l->curr_item +1 ].token_c;

                goto wen_found_hit;
            } break;

            case 3: // on Sort_by page
            {
                set_cmp_token(l->curr_item);
                /* resort using custom comparision function */
                qsort(icon_panel->item_d, icon_panel->item_c, sizeof(item_t), struct_cmp_by_token);
                // refresh Groups item_idx_t* array
                recreate_item_t(&groups);
                // switch focus, set back Games view
                menu_pos.z = ON_MAIN_SCREEN;   // switch view
                // reset selection to first entry
                icon_panel->curr_item = 0;
                layout_update_sele(icon_panel, 0);
                active_p = icon_panel;
            } break;

            case 4: // on Filter_by page
            {
                // update current label context
                switch(l->curr_item)
                {
                    case 0:  label = PV;      break;
                    case 1:  label = AUTHOR;  break;
                }
                // build_list of patterns found
                item_idx_t *ret = build_item_list(icon_panel->item_d,
                                                  icon_panel->item_c, label);
                /* resort using custom comparision function */
                qsort(&ret[1], ret->len, sizeof(item_idx_t), struct_cmp_by_count);

                l->page_sel.x = 5,
                l->item_c     = ret->len; 
                l->curr_item  = 0;
                // build the char array for next call below
                char **tmp = calloc(ret->len, sizeof(char **));
                build_char_from_items(tmp, ret);
                // point to this item_idx_t* AOS
                l->item_d[0].token_d = ret;
                layout_fill_item_from_list(l, &tmp[0]);
                layout_update_sele(l, 0);
                active_p = left_panel2;

                // fixme: not finished, we are leaking tmp and ret !!!
            } break;

            case 5: // execute Filter_by selection
            {
                // search for label and token_data
                snprintf(&pattern[0], 69, "%s", l->item_d[ l->curr_item ].token_d[0].off);

                goto search_by_label;
            }

        } // End switch

        l->vbo_s = ASK_REFRESH; // ask to refresh VBO

refresh_active_panel:

        if(active_p
        && active_p->vbo_s < ASK_REFRESH) active_p->vbo_s = ASK_REFRESH;

        return;
    }

    if(l == icon_panel) // go to download_panel
    {
        // set to current selected
        l->curr_item = idx;
        // but don't refresh indexes

switch_to_download:

        //Check if Legacy is Enabled then check DL Counter for APP DL Page
        //if(li->token_d) TID_VAILD
        if (TID_VAILD)
        {
            if (!get->Legacy)
                DL_CO = check_download_counter(get, tmp);
        }

        menu_pos.z = ON_ITEM_INFO;
        active_p   = download_panel;  active_p->is_shown = 1;
        GLES2_refresh_sysinfo();
        
        goto refresh_active_panel;
    }

    if(l == download_panel  // trigger item download
    || l ==   option_panel) // execute action
    {
    

        X_action_dispatch(l->f_sele, l);
        return;
    }
}

void layout_dispatch_O(layout_t *l)
{
    if(l == left_panel2)
    {
        l->page_sel.x = 0, // back initial page
        l->vbo_s      = ASK_REFRESH;
        l->item_c     = 7, // num of texts
        l->curr_item  = 0;
        layout_fill_item_from_list(l, &new_panel_text[ l->page_sel.x ][0]);
        layout_update_sele(l, 0);
    }
    else
    if(l == icon_panel)
    {
        menu_pos.z   = ON_LEFT_PANEL;
        active_p     = left_panel2;    // back to Left panel
        l->curr_item = 0;

drop_aux:

        if(aux) // drop aux
        {
            l->item_c =  games->token_c;
            l->item_d = &games[1];
                  aux = NULL;
        }
        layout_update_sele(l, 0);
    }
    else
    if(l ==  queue_panel
    || l == option_panel)
    {
        menu_pos.z           = ON_LEFT_PANEL;
        active_p             = left_panel2; // back to Left panel
        active_p->page_sel.x = 0;
    }
    else
    if(l == download_panel)
    {
        menu_pos.z   = ON_MAIN_SCREEN;
        l = active_p = icon_panel; // back to Icon panel
        active_p   ->vbo_s = ASK_REFRESH;
        left_panel2->vbo_s = ASK_REFRESH;

        goto drop_aux;
    }
}

/* deal with menu position / actions */
void GLES2_scene_on_pressed_button(int button)
{
    if(menu_pos.z == ON_ITEMzFLOW) { fw_action_to_cf(button); return; }

    if( ! active_p ) active_p = left_panel2;

    GLES2_refresh_sysinfo();

    layout_t *l = active_p;

//  log_debug( "%s, l:%p", __FUNCTION__, l);

    switch(button)
    {
        case LEF:  layout_update_sele( l, -1 ); break; // l_or_r
        case RIG:  layout_update_sele( l, +1 ); break;
        case UP :  layout_update_sele( l, -l->fieldsize.x ); break;
        case DOW:  layout_update_sele( l, +l->fieldsize.x ); break;

        case TRI: { /* go cf */ 
            //menu_pos.z = ON_ITEMzFLOW;  active_p = NULL;
            //drop_all_icons();
        } break;

        case CIR: { // back
            layout_dispatch_O(l);
        } break;

        case CRO: { // execute action
            layout_dispatch_X(l);
        } break;

        // we don't catch button for migration, fallbak old way
        default: return;
    }

    log_debug( "%s, %d/%d", __FUNCTION__, l->curr_item +1, l->item_c);

    GLES2_refresh_common();
}

