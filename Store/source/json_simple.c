#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

#include "jsmn.h"
#include "json.h"
#include <user_mem.h> 

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


int count_availables_json(void)
{
    int   count = 0;
    char  json_file[128];

    do{   // json page_num starts from 1 !!!
        count++;
        snprintf(&json_file[0], 127, "/user/app/NPXS39041/pages/homebrew-page%d.json", count);
        // passed, increase num of available pages
    } while (if_exists(&json_file[0]));
    count--;

    return count;
}

// index (write) all used_tokens
int json_index_used_tokens_v2(layout_t *l, char *json_data)
{
    int r, i, c = 0;
    jsmn_parser p;
    jsmntok_t   t[1024]; /* We expect no more than this tokens */
    //char *json_data = page->json_data;
    jsmn_init(&p);
    r = jsmn_parse(&p, json_data, strlen(json_data), t,
                             sizeof(t) / sizeof(t[0]));
    if (r < 0) { log_info( "Failed to parse JSON: %d", r); return -1; }

    // grab last added index from current item count
    int idx = l->item_c;

    item_idx_t *token = NULL;

    for(i = 1; i < r; i++)
    {   // entry
        l->item_d[idx].token_c = NUM_OF_USER_TOKENS;
        // dynalloc for json tokens
        if(!l->item_d[idx].token_d)
            l->item_d[idx].token_d = calloc(l->item_d[idx].token_c, sizeof(item_idx_t)); /// XXX
//        log_info("%s %p", __FUNCTION__, l->item_d[idx].token_d);
        token = l->item_d[idx].token_d;

        int j;
        for(j = 0; j < NUM_OF_USER_TOKENS; j++)
        {
            if (jsoneq(json_data, &t[i], used_token[j]) == 0)
            {
                /* We may use strndup() to fetch string value */
                token[j].off = strndup(json_data + t[i + 1].start,
                                    t[i + 1].end - t[i + 1].start);
                token[j].len =      t[i + 1].end - t[i + 1].start;
//              klog("token_d[].off:%s", layout->item_d[idx].token[0].off);
                i++; c++;
                if(j==16) idx++; // ugly but increases :facepalm:
            }
        }
        // if less, shrink buffer: realloc
        if(j < NUM_OF_USER_TOKENS)
        {   // shrink
            l->item_d[idx].token_c = j;
            l->item_d[idx].token_d = realloc(l->item_d[idx].token_d,
                                             l->item_d[idx].token_c * sizeof(item_idx_t));
        }
        // save current index from current item count
        l->item_c = idx;
    }

    /* this is the number of all counted items * NUM_OF_USER_TOKENS ! */
    return c;
}

