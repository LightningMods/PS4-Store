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
#include "shaders.h"
#include <algorithm>


extern std::vector<std::string> download_panel_text;
extern bool unsafe_source;
int DL_CO = -999;
// related indexes from json.h enum
// for sorting entries
unsigned char cmp_token = 0,
sort_patterns[11] = { 0, 1, 4, 5, 9, 10, 11, 12, 13, 16, 16 };

/* set comparison token */
void set_cmp_token(const int index)
{    //cmp_token = index;
    cmp_token = sort_patterns[index];
}


static enum token_name label;

extern std::atomic_bool is_icons_finished;

void Install_View(std::shared_ptr<layout_t>  &l, const char* query_string, enum token_name nm)
{
  if(!query_string) return;

  icon_panel->mtx.lock();

  log_info("Pulling up %s", query_string);
  loadmsg(getLangSTR(SEARCHING));
  destroy_item_t(q);
  // build an array of search results
  q = search_item_t(icon_panel->item_d,  nm, std::string(query_string));
  destroy_item_t(aux);

  aux = q;

  if ( !aux.empty() && aux[0].len) // we got results, address aux!
  {
    log_info("Showing %d items for '%s' @ %p", aux[0].len,
      aux[0].off.c_str(), &aux);
    //  for(int i = 0; i < aux[0].len; i++) { log_info("%d: %s %d", i, aux[i].off, aux[i].len); }
    // switch focus
    menu_pos.z = ON_MAIN_SCREEN;
    l = active_p = icon_panel;

    // reset selection to first entry
    l ->curr_item = 0;
    l ->item_c = aux[0].len;
    layout_update_sele(l, 0);
    layout_fill_item_from_list(download_panel, download_panel_text);
    // but now we have aux array!
  } else {

    #ifdef __ORBIS__
    sceKernelIccSetBuzzer(3);
    #endif
  }
  #ifdef __ORBIS__
  sceMsgDialogTerminate();
  #endif

  icon_panel->mtx.unlock();

}

extern std::atomic_bool update_check_finised;
std::atomic_bool show_prog(false);
extern std::atomic<double> updates_prog;

int CheckforUpdates(online_check_t check) {
    int i = 0;
    int progress = 0;
    int curr_count = 1;

   
    if(show_prog.load())
      icon_panel->mtx.lock();

    if (check == ONLINE_CHECK_FOR_VIEW && icon_panel->item_c > q.size()) {
        log_error("Checking for updates: q.size() > icon_panel->item_c");
        return 0;
    }
#ifdef __ORBIS__
    if(show_prog.load())
        progstart(getLangSTR(CHECKING_FOR_UPDATES));
#endif

    for (auto &t : games) {

        if ( t.token_d.size() < NAME || t.token_d[ID].off.empty()) {
            continue;
        }

        log_info("tokens: %d %d", games[0].token_c, i );

        progress = (i * 100) / games[0].token_c;
        if(check != DONT_CHECK_FOR_UPDATES){
#ifdef __ORBIS__
          if(show_prog.load())
            ProgSetMessagewText(progress, fmt::format("{}\n\n {}", getLangSTR(CHECKING_FOR_UPDATES), t.token_d[NAME].off));
#endif
            CheckUpdate(t.token_d[ID].off.c_str(), t);
            updates_prog.store(progress);
            games.at(i).update_status.store(t.update_status.load());
        }
        //log_info("ID %s t %i", t.token_d[ID].off.c_str(), t.update_status.load());

        if (t.update_status == UPDATE_FOUND) { // hit found ?
            log_info("UPDATE NEEDED[%d]:%3d %s", curr_count, i, t.token_d[ID].off.c_str());
            if (check != ONLINE_CHECK_FOR_STARTUP) {
                q[curr_count].off = t.token_d[ID].off;
                q[curr_count].len = i; // store index for icon_panel!
            }
            curr_count++;
        }
        i++;
    }
    show_prog.store(false);
  //  icon_panel->mtx.unlock();

    return curr_count;
}

void Update_View(std::shared_ptr<layout_t>  &l)
{

  int curr_count = 1;
  loadmsg(getLangSTR(SEARCHING));
  destroy_item_t(q);

    // build an array of search results
    q.resize(icon_panel->item_c +1);


    curr_count = CheckforUpdates(DONT_CHECK_FOR_UPDATES);

    // shrink buffer, consider +1 !!!
    q.resize(curr_count);
    // save count as very first item_idx_t.len
    q[0].len = curr_count -1;
    q[0].off = "???????"; // save pattern too
  
    destroy_item_t(aux);

  aux = q;

      //  for(int i = 0; i < aux[0].len; i++) { log_info("%d: %s %d", i, aux[i].off, aux[i].len); }
    // switch focus
   menu_pos.z = ON_MAIN_SCREEN;
   l = active_p = icon_panel;

   // reset selection to first entry
   l ->curr_item = 0;

  if (!aux.empty() && aux[0].len) // we got results, address aux!
  {
    log_info("Showing %d items for '%s' @ %p", aux[0].len,
      aux[0].off.c_str(), &aux);

    l ->item_c = aux[0].len;
    // but now we have aux array!
  } else {

    #ifdef __ORBIS__
    sceKernelIccSetBuzzer(3);
    #endif
  }

  layout_update_sele(l, 0);
  layout_fill_item_from_list(download_panel, download_panel_text);

  #ifdef __ORBIS__
  sceMsgDialogTerminate();
  #endif
}
extern std::atomic_bool show_prog;
static void layout_dispatch_X(std::shared_ptr<layout_t>  &l)
{
    bool TID_VAILD = false;
    char tmp[256],
        pattern[70];
    // default indexing
    int idx = l->curr_item;
    std::string pattern_str;

    if (l == queue_panel) // go to download_panel
    {
        int req_status;
        switch (menu_pos.z)
        {
        case ON_INSTALL: {
            if (set.auto_install.load())
                req_status = INSTALLING_APP;
            else{
                req_status = COMPLETED;
            }
            break;
        }
        case ON_QUEUE:    req_status = RUNNING;    break;
        }
        // get index from selected thread info
        idx = thread_find_by_status(l->f_sele, req_status);
        if (menu_pos.z == ON_QUEUE && idx == -1)
            idx = thread_find_by_status(l->f_sele, PAUSED);
        // no results, :shrug:
        if (idx < 0) {
            log_error("this should not happen %i", idx);
            msgok(WARNING, "WTF");
        }
        // set to current selected
        icon_panel->curr_item = pt_info[idx]->g_idx;
        // refresh indexes
        layout_update_sele(icon_panel, 0);

        goto switch_to_download;
    }

    // override
    if (l == icon_panel
        &&  !aux.empty()
        && aux[0].len > 0) idx = get_item_index(l);

    if (l->item_d[idx].token_d[0].len > 0)
    {   // try to get the first indexed token value
        item_t* li = &l->item_d[idx];

        if (!li->token_d.empty()) {
            snprintf(&tmp[0], 255, "%s", li->token_d[0].off.c_str());
            TID_VAILD = true;
        }
        else { // safe fallback
            snprintf(&tmp[0], 255, "%d, %p", idx, &li->token_d);
            TID_VAILD = false;
        }
        log_info("execute %d -> '%s'", idx, tmp);
    }
    else
    {
        log_info("execute %d -> '%p'", l->curr_item,
            &l->item_d[l->curr_item].token_d);
    }

    /* follows actions for panels */
    if (l == left_panel2)
    {
        left_panel2->mtx.lock();
        switch (l->page_sel.x)
        {
        case 0: // first page
        {
            if (l->curr_item == 0) {
                menu_pos.z = ON_MAIN_SCREEN;
                // set left_panel 2nd page
                l->page_sel.x = 1,
                    l->item_c = 3,
                    l->curr_item = 0;
                layout_fill_item_from_list(l, new_panel_text[l->page_sel.x]);
                layout_update_sele(l, 0);
                active_p = icon_panel;
                break;
            }

            if (l->curr_item == 1) {
                menu_pos.z = ON_MAIN_SCREEN;
                Install_View(l, "Itemzflow", NAME);
                break;
            }

            // 2 Groups
            if (l->curr_item == 2) {
                // set left_panel 3nd page
                l->page_sel.x = 2,
                l->item_c = groups[0].token_c;
                l->curr_item = 0;
                layout_update_sele(l, 0);
                active_p = left_panel2;
                break;
            }

            // variable length lists
            if (l->curr_item == 3
                || l->curr_item == 4)
            {
                if (l->f_sele == 3) menu_pos.z = ON_INSTALL;
                else
                    if (l->f_sele == 4) menu_pos.z = ON_QUEUE;
                // activate and set focus
                queue_panel_init();
                l = queue_panel; // switch control
               // l->page_sel.x = 0;
                l->is_active =
                    l->is_shown = 1;
                // reset selection to first entry
                //l->item_sel   = (ivec2) (0);
                layout_update_fsize(l);
                active_p = queue_panel;  //active_p->is_shown = 1;
                break;
            }

            // 5 Updates (todo)
            if (l->curr_item == 5) {
                 if(!unsafe_source){
                    if(update_check_finised){
                       Update_View(l);
                    }
                    else{
                          //msgok(WARNING, "Please wait for the update check to finish");
                          left_panel2->mtx.unlock();
                           #ifdef __ORBIS__
                          if(options_dialog(getLangSTR(UPDATES_STILL_LOADING), getLangSTR(SHOW_PROG), getLangSTR(STAY_IN_BACKGROUND)) == 1){
                            progstart(getLangSTR(CHECKING_FOR_UPDATES));
                            show_prog = true;
                            while(show_prog.load()){
                                usleep(100000);

                            }
                          }
                          #endif
                    }
                 }
                 else{
                    #ifdef __ORBIS__
                    msgok(WARNING, "The Updates feature is not available on Unsafe CDNs");
                    #endif
                    log_info("unsafe source, skip update");
                 }
                break;
            }

            if (l->curr_item == 6) {
                menu_pos.z = ON_SETTINGS;
                active_p = option_panel;  //active_p->is_shown = 1;
            }
            // set status
            active_p->is_shown = 1;
        } 
        break;

        case 1: // on Games page
        {
            if (l->curr_item == 0)  /* Search for */
            {
                memset(&pattern[0], 0, sizeof(pattern));
                log_info("execute openDialogForSearch()");
                // get char *pattern
                if(!Keyboard("Store Keyboard", "", &pattern[0], false)) goto cancel_kb_search;

                log_info("@@@@@@@@@  Search for: %s", pattern);
                loadmsg(getLangSTR(SEARCHING));
                label = NAME;

            search_by_label:
                destroy_item_t(q);
                // build an array of search results
                pattern_str = pattern;
                log_info("Searching for '%s' ...", pattern_str.c_str());
                q = search_item_t(icon_panel->item_d,
                    label, pattern_str);
                aux = q;

            wen_found_hit:

                if (!aux.empty() && aux[0].len) // we got results, address aux!
                {
                    log_info("Showing %d items for '%s' @ %p", aux[0].len, aux[0].off.c_str(), &aux);
                    //  for(int i = 0; i < aux[0].len; i++) { log_info("%d: %s %d", i, aux[i].off, aux[i].len); }
                    // switch focus
                    menu_pos.z = ON_MAIN_SCREEN;
                    l = active_p = icon_panel;
                    // reset selection to first entry
                    l->curr_item = 0;
                    l->item_c = aux[0].len;
                    layout_update_sele(l, 0);
                    // but now we have aux array!
                }
                else
                {

                    #ifdef __ORBIS__
                    sceKernelIccSetBuzzer(3);
                    #endif
                }
                cancel_kb_search:
                log_info("Cancelling search");
                #ifdef __ORBIS__
                sceMsgDialogTerminate();
                #endif
            }

            if (l->curr_item == 1)  /* switch to Sort labels */
            {   // set left_panel 4th page
                l->page_sel.x = 3,
                    l->item_c = 11;
                active_p = left_panel2;

                goto switch_page;
            }
            // filter
            if (l->curr_item == 2)  /* switch to Filter_by */
            {   // set left_panel 5th page
                l->page_sel.x = 4,
                    l->item_c = 2;
                active_p = left_panel2;
            switch_page:
                l->curr_item = 0;
                layout_fill_item_from_list(l, new_panel_text[l->page_sel.x]);

                layout_update_sele(l, 0);

                goto refresh_active_panel;
            }

        } break;

        case 2: // on Groups page
        {
            // map aux AOS to selected group
            aux = groups[l->curr_item + 1].token_d;
            aux[0].len = groups[l->curr_item + 1].token_c;

            goto wen_found_hit;
        } break;

        case 3: // on Sort_by page
        {
            icon_panel->mtx.lock();
            log_info("l->curr_item) %i", l->curr_item);
            set_cmp_token(l->curr_item);
            /* resort using custom comparision function */
            //qsort(icon_panel->item_d.data(), icon_panel->item_d.size(), sizeof(item_t), struct_cmp_by_token);
            // Sort using custom comparison function
            icon_panel->item_d.resize(icon_panel->item_c);
            std::sort(icon_panel->item_d.begin(), icon_panel->item_d.end(),
              [](const item_t& a, const item_t& b) {
                  if(a.token_d.empty() || b.token_d.empty()) {
                     log_error("a or b is empty");
                     return false;
                  }
                  // Replace this condition with your custom comparison logic
                   return a.token_d[cmp_token].off.compare(b.token_d[cmp_token].off) < 0;
              });

            // refresh Groups item_idx_t* array
            recreate_item_t(groups);
            // switch focus, set back Games view
            menu_pos.z = ON_MAIN_SCREEN;   // switch view
            // reset selection to first entry
            icon_panel->curr_item = 0;
            layout_update_sele(icon_panel, 0);
            active_p = icon_panel;
            icon_panel->mtx.unlock();
        } break;

        case 4: // on Filter_by page
        {
            // update current label context
            switch (l->curr_item)
            {
            case 0:  label = PV;      break;
            case 1:  label = AUTHOR;  break;
            }
            // build_list of patterns found
            std::vector<item_idx_t> ret = build_item_list(icon_panel->item_d, label);
            /* resort using custom comparision function */
            std::sort(ret.begin(), ret.end(), [](const item_idx_t& a, const item_idx_t& b) {
                     return b.len < a.len;
            });

            l->page_sel.x = 5,
                l->item_c = ret.size();
            l->curr_item = 0;
             // build_list of patterns found
            std::vector<item_idx_t> tmp_ret = build_item_list(icon_panel->item_d, label);
            /* resort using custom comparision function */
            std::sort(tmp_ret.begin(), tmp_ret.end(), [](const item_idx_t& a, const item_idx_t& b) {
                     return b.len < a.len;
            });
            l->page_sel.x = 5,
            l->item_c = tmp_ret.size();
            l->curr_item = 0;
            // build the char array for next call below
            std::vector<std::string> data;
            build_char_from_items(data, tmp_ret);
            // point to this item_idx_t* AOS
            l->item_d[0].token_d = tmp_ret;
            layout_fill_item_from_list(l, data);
            layout_update_sele(l, 0);
            active_p = left_panel2;
           // free(tmp);
            // fixme: not finished, we are leaking tmp and ret !!!
        } break;

        case 5: // execute Filter_by selection
        {
            // search for label and token_data
            snprintf(&pattern[0], 69, "%s", l->item_d[l->curr_item].token_d[0].off.c_str());
            goto search_by_label;
        }

        } // End switch

        l->vbo_s = ASK_REFRESH; // ask to refresh VBO

    refresh_active_panel:

        if (active_p
            && active_p->vbo_s < ASK_REFRESH) active_p->vbo_s = ASK_REFRESH;
            
        sceMsgDialogTerminate();
        left_panel2->mtx.unlock();
        return;
    }
    if (l == icon_panel) // go to download_panel
    {
        // set to current selected
        l->curr_item = idx;
        // but don't refresh indexes

    switch_to_download:
        loadmsg(getLangSTR(DL_CACHE));
        //Check if Legacy is Enabled then check DL Counter for APP DL Page
        //if(li->token_d) TID_VAILD
        //CheckUpdate(l->item_d[l->curr_item].token_d[ ID ].off.c_str(), l->item_d[l->curr_item]);
        if (TID_VAILD)
        { 
            DL_CO = PENDING_DOWNLOADS;
            if (is_icons_finished){
                DL_CO = check_download_counter(tmp);
            }
        }

        menu_pos.z = ON_ITEM_INFO;
        active_p = download_panel;  active_p->is_shown = 1;
        GLES2_refresh_sysinfo();
        icon_panel->mtx.lock();
        if (games[idx].update_status == UPDATE_FOUND){
            download_panel->item_d[0].token_d[0].off = download_panel_text[0] = getLangSTR(UPDATE_NOW);
        }
        else if (games[idx].update_status == NO_UPDATE){
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = getLangSTR(REINSTALL_APP);
        }
        else if (games[idx].update_status == APP_NOT_INSTALLED){
                download_panel->item_d[0].token_d[0].off = download_panel_text[0] = set.auto_install.load() ? getLangSTR(DL_AND_IN) : getLangSTR(DL2);
        }
        icon_panel->mtx.unlock();
        goto refresh_active_panel;
    }

    if (l == download_panel  // trigger item download
        || l == option_panel) // execute action
    {
        X_action_dispatch(l->f_sele, l);
        return;
    }
}

void layout_dispatch_O(std::shared_ptr<layout_t>  &l)
{
    if (l == left_panel2)
    {
        left_panel2->mtx.lock();
        l->page_sel.x = 0, // back initial page
            l->vbo_s = ASK_REFRESH;
        l->item_c = 7, // num of texts
            l->curr_item = 0;
            log_info("layout_dispatch_O %i", l->item_c);
        layout_fill_item_from_list(l, new_panel_text[l->page_sel.x]);
        layout_update_sele(l, 0);
        left_panel2->mtx.unlock();
    }
    else
        if (l == icon_panel)
        {
            log_info("layout_dispatch_O: icon_panel");
            menu_pos.z = ON_LEFT_PANEL;
            l->curr_item = 0;

        drop_aux:

            if (!aux.empty()) // drop aux
            {
                l->item_c = games[0].token_c;
                l->item_d.clear();
                l->item_d = games;
                l->item_d.resize(l->item_c);

                //l->item_d = games;
                log_info("dropping aux %i", l->item_c);
               // std::copy(games.begin() + 1, games.end() , l->item_d.begin());
                //std::copy(icon_panel->item_d.begin(), icon_panel->item_d.begin() + icon_panel->item_c, games.begin() + 1);
     /*           int count = 0;
                for (item_t num : icon_panel->item_d) {
        if (!num.token_d.empty() && ID >= 0 && ID < num.token_d.size()) { // Check if num.token_d is not empty and ID is within the valid range
            //std::cout << num.token_d[ID].off << " ";
            log_info("game: %s %i %i", num.token_d[ID].off.c_str(), count, icon_panel->item_d.size()) ;
        } else {
            log_info("Invalid num.token_d or ID out of range");
        }
        count++;
    }
    log_info("ddddd aux %i", l->item_c);*/
                aux.clear();
            }
            log_info("layout_dispatch_O: layout_update_sele is icon_panel %s", l == icon_panel ? "true" : "false");
            layout_update_sele(l, 0);
            active_p = left_panel2;    // back to Left panel
        }
        else
            if (l == queue_panel
                || l == option_panel)
            {
                menu_pos.z = ON_LEFT_PANEL;
                active_p = left_panel2; // back to Left panel
                active_p->page_sel.x = 0;
            }
            else
                if ( l == download_panel)
                {
                    menu_pos.z = ON_MAIN_SCREEN;
                    l = active_p = icon_panel; // back to Icon panel
                    active_p->vbo_s = ASK_REFRESH;
                    left_panel2->vbo_s = ASK_REFRESH;

                    goto drop_aux;
                }
}

/* deal with menu position / actions */
void GLES2_scene_on_pressed_button(int button)
{

    if (!active_p)
         active_p = left_panel2;

    GLES2_refresh_sysinfo();

    auto &l = active_p;

    //  log_debug( "%s, l:%p", __FUNCTION__, l);

    switch (button)
    {
    case LEF:  layout_update_sele(l, -1); break; // l_or_r
    case RIG:  layout_update_sele(l, +1); break;
    case UP:  layout_update_sele(l, -l->fieldsize.x); break;
    case DOW:  layout_update_sele(l, +l->fieldsize.x); break;

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

    log_debug("%s, %d/%d", __FUNCTION__, l->curr_item + 1, l->item_c);

    GLES2_refresh_common();
}


void GLES2_Refresh_for_settings()
{

    left_panel2->mtx.lock();
    if (!active_p) active_p = left_panel2;

    GLES2_refresh_sysinfo();

    layout_update_sele(active_p, 0);

    GLES2_refresh_common();
    left_panel2->mtx.unlock();
}

