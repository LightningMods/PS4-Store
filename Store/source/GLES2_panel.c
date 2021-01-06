/*
    GLES2 scene

    my implementation of an interactive menu, from scratch.
    (shaders
     beware)

    2020, masterzorag
*/

#include <stdio.h>
#include <string.h>

#if defined(__ORBIS__)
    #include <debugnet.h>
#endif

#include <freetype-gl.h>
// freetype-gl pass last composed Text_Length in pixel, we use to align text!
extern float tl;

#include "defines.h"

#include "json.h"


// check for pattern in whitelist patterns, if there take count, if not fall in Other
static char *group_label[] =
{   // 0 is reserved index for: (label, total count)
    "HB Game",
    "Emulator",
    "Emulator Add-On",
    "Media",
    "Mira Plugins",
    "Utility",
    "Other"
};

/* pattern from icon tokens, item for Group arrays */
static int check_for_token(char *pattern, item_t *item, int count)
{
    int   i, plen = strlen(pattern);
    item_idx_t *t = NULL;
    /*  loop over, skip very first one (0), reserved
        and (6) for Other used as "unmatched labels" */
    for(i = 1; i < count -1; i++)
    {
        t = &item[i].token_d[0];
        /* check for exact match */
        if( strstr(t->off, pattern)  // hit found +
        && (strlen(t->off) == plen)) // same length
            break;

        /* check for common 'Game' label */
        if(i == 1
        && strstr(pattern, "Game"))  // hit found +
            break;
    }
    // store: if not found before falls into Other, take count
    item[i].token_c += 1;
/*  printf("%d: %s -> %s (%d)\n", i, pattern, item[i].token_d[0].off,
                                              item[i].token_c); */
    return i;
}

// we return an array of indexes and count number, per Group
item_t *analyze_item_t_v2(item_t *items, int item_count)
{
    // same number of group_labels, +1 for reserved main index
    int  i, count = sizeof(group_label) / sizeof(group_label[0]) +1;
    // dynalloc
    item_t   *ret = calloc(count, sizeof(item_t));
    item_idx_t *t = NULL;
    
    // init: fill the Groups labels
    for(i = 1; i < count; i++)
    {   // we expect no more than this number, per Group label
        ret[i].token_c = 128;
        // dynalloc for item_idx_t
        if(!ret[i].token_d)
            ret[i].token_d = calloc(ret[i].token_c, sizeof(item_idx_t));
//        printf("%s %p\n", __FUNCTION__, layout->item_d[idx].token_d);
        // clean count, address token
        t = &ret[i].token_d[0];
             ret[i].token_c = 0;
        // address the label
        t->off = group_label[i -1];
        t->len = 0;
    }
    // reserved index (0)!
    ret[0].token_d = NULL; 
    ret[0].token_c = count -1;

    /*  count for label, iterate all passed items
        index for each Group in token_data[ ].len */
    for(i = 0; i < item_count; i++)
    {
        t = &items[i].token_d[ APPTYPE ];
        // in which Group fall this item?
        int res = check_for_token(t->off, &ret[0], count);
/*      printf("%d %d: %s (%d) %s\n", i, res, ret[ res ].token_d[0].off,
                                              ret[ res ].token_c, t->off); */
        int idx = ret[ res ].token_c;
        // store the index for item related to icon_panel list!
        ret[ res ].token_d[ idx ].len = i;

        // is installed?
        char tmp[128];
        sprintf(&tmp[0], "/user/app/%s", items[i].token_d[ ID ].off);

        if( if_exists(tmp) )
        {
            printf("%3d: %s\n", i, tmp);
        }
/*
p icon_panel->item_d[101]->token_d[APPTYPE]
$18 = {off = 0xbed330 "HB Game", len = 7}

*/
    }
#if 1
    // done building the item_t array, now we check items sum
    int check = 0;
    for(i = 1; i < count; i++)
    {   // report the main reserved index
        printf("%d %s: %d\n", i, ret[i].token_d[0].off,
                                 ret[i].token_c);
        // shrink buffers, remember +1 !!!
        ret[i].token_d = realloc(ret[i].token_d, (ret[i].token_c +1)
                                               * sizeof(item_idx_t));
        check += ret[i].token_c;
    }
    printf("so we have %d items across %d Groups\n", check, ret[0].token_c);
/*
1 HB Game: 3
2 Emulator: 18
3 Emulator Add-On: 3
4 Media: 4
5 Mira Plugins: 0
6 Other: 84
so we have 7 Groups, check:112
*/
#endif

    return ret;
}

/* qsort struct comparison function (C-string field) */
static int struct_cmp_by_token(const void *a, const void *b)
{
    item_idx_t *ia = (item_idx_t *)a;
    item_idx_t *ib = (item_idx_t *)b;
    return strcmp(ia->off, ib->off);
/* strcmp functions works exactly as expected from comparison function */
}

// search for pattern in items tokens
item_idx_t *search_item_t(item_t *items, int item_count, enum token_name TN, char *pattern)
{
    /* store in the very first item_idx_t:
       pattern searched and array length, so +1 !!! */

    // at most same number of items, we realloc then
    item_idx_t *ret = calloc(item_count +1, sizeof(item_idx_t));
    int  curr_count = 1,
                len = strlen(pattern);
    char         *p = NULL;

    // build an array of discovered unique entries
    for(int i = 0; i < item_count; i++)
    {
        item_idx_t *t = &items[i].token_d[ TN ];

        if(TN == NAME)
        {
            p = strstr(t->off, pattern);
            if(p) // hit found?
            {
                printf("hit[%d]:%3d %s\n", curr_count, i, t->off);
                ret[curr_count].off = t->off,
                ret[curr_count].len = i; // store index for icon_panel!
                curr_count++;
            }
        }
        else
        if(TN == APPTYPE)
        {
            int res = memcmp(t->off, pattern, len);
            if(!res // hit found?
            && len == strlen(t->off)) // same length?
            {
                printf("hit[%2d]:%3d %s %d %lu\n", curr_count, i, t->off, res, strlen(pattern));
                ret[curr_count].off = t->off,
                ret[curr_count].len = i; // store index for icon_panel!
                curr_count++;
            }
        }
    }
    // shrink buffer, consider +1 !!!
    ret = realloc(ret, curr_count * sizeof(item_idx_t));
    // save count as very first item_idx_t.len
    ret[0].len = curr_count -1;

    printf("pattern '%s' counts %d hits\n", pattern, ret[0].len);

    ret[0].off = strdup(pattern); // save pattern too

    return ret;
}

// use pointer to pointer to clean the struct
void destroy_item_t(item_idx_t **p)
{
    if(*p)
    {   // clean auxiliary array
        if(*p[0]->off) free(p[0]->off), p[0]->len = 0;
        free(*p), *p = NULL;
    }
}

