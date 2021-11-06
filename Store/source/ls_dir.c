/*
    listing local folder using getdents()
*/

#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close


#include "defines.h"
/*
  Glibc does not provide a wrapper for
  getdents, so we call it by syscall()
*/
#include <sys/syscall.h> // SYS_getdents
#include <sys/param.h>   // MIN


char *entryName(int entryType)
{
    switch(entryType)
    {
        case 4:  return "DIR";
        case 8:  return "FILE";
        default: return "OTHER";
    }
}

#if 1
/*
    minimal folder listing:
    ps4 uses getdents(),
    glibc doesn't have so use syscall.
    test implementation, now unused (see below)

    THIS_WAS_OLD_VER
*/
int ls_dir(char *dirpath)
{
    int dfd = open(dirpath, O_RDONLY, 0); // try to open dir
    if(dfd < 0)
        { log_debug( "Invalid directory. (%s)", dirpath); return -1; }
    else 
        { log_debug( "open(%s)", dirpath); }

    struct dirent *dent;
    char           buffer[512]; // fixed buffer

    memset(buffer, 0, sizeof(buffer));

    // get directory entries
#if defined (__ORBIS__)
    while(getdents(dfd, buffer, sizeof(buffer)) != 0)
#else
    while(syscall(SYS_getdents, dfd, buffer, sizeof(buffer)) != 0)
#endif
    {
        dent = (struct dirent *)buffer;

        while(dent->d_fileno)
        {
            log_debug( "[%p]: %lx %2d, type: %4d, '%s'", // report entry
                dent, 
                dent->d_fileno, dent->d_reclen, 
                dent->d_type,   dent->d_name /* on pc we step back by -1 */);

            dent = (struct dirent *)((void *)dent + dent->d_reclen);

            if(dent == (void*) &buffer[512]) break; // refill buffer
        }
        memset(buffer, 0, sizeof(buffer));
    }
    close(dfd);

    return 0;
}
#endif

bool if_exists(const char *path)
{
    int dfd = open(path, O_RDONLY, 0); // try to open dir

    if(dfd < 0)
        return false;
    else
      close(dfd); 
    
    return true;
}


/* extended: a two-pass folder-parser */

/* qsort struct comparison function (C-string field) */
static int struct_cmp_by_name(const void *a, const void *b)
{
    entry_t *ia = (entry_t *)a;
    entry_t *ib = (entry_t *)b;
    return strcmp(ia->name, ib->name);
/* strcmp functions works exactly as expected from comparison function */
}

int num = 0; // total item count

int get_item_count(void)
{
    return num;
}

void free_item_entries(entry_t *e)
{
    for (int i = 0; i < num; ++i)
    {
        if(e->name) free(e->name), e->name = NULL;
    }
    free(e), e = NULL;
}

entry_t *get_item_entries(const char *dirpath, int *count)
{   
    struct dirent *dent;
    char           buffer[512]; // fixed buffer

    int dfd    = 0,
        n, r   = 1;    // item counter, rounds to loop
    entry_t *p = NULL; // we fill this struct with items

loop:
    n = 0;
    log_debug( "loop: %d, count:%d", r, *count);

    // try to open dir
    dfd = open(dirpath, O_RDONLY, 0);
    if(dfd < 0)
        { log_debug( "Invalid directory. (%s)", dirpath); *count = -1; return NULL; }
    else
        { log_debug( "open(%s)", dirpath); }

    memset(buffer, 0, sizeof(buffer));

    // get directory entries
#if defined (__ORBIS__)
    while(getdents(dfd, buffer, sizeof(buffer)) != 0)
#else
    while(syscall(SYS_getdents, dfd, buffer, sizeof(buffer)) != 0)
#endif
    {
        dent = (struct dirent *)buffer;

        while(dent->d_fileno)
        {   // skip `.` and `..`
            if( !strncmp(dent->d_name, "..", 2)
            ||  !strncmp(dent->d_name, ".",  1)) goto skip_dent;

            // deal with filtering outside of this function, we just skip .., .

            switch(r)
            {   // first round: just count items
                case 1: 
                {
                    // skip special cases
                    if(dent->d_fileno == 0) goto skip_dent;
                #if 0
                    log_debug( "[%p]: %8lx %2d, type: %4d, '%s'", // report entry
                        dent,
                        dent->d_fileno, dent->d_reclen,
                        dent->d_type,   dent->d_name /* on pc we step back by -1 */);
                #endif
                    break;
                }
                // second round: store filenames
                case 0: p[n].name = strdup(dent->d_name); break;
            }
            n++;

skip_dent:

            dent = (struct dirent *)((void *)dent + dent->d_reclen);

            if(dent == (void*) &buffer[512]) break; // refill buffer
        }
        memset(buffer, 0, sizeof(buffer));
    }
    close(dfd);

    // on first round, calloc for our list
    if(!p)
    {   // now n holds total item count, note it
             p = calloc(n, sizeof(entry_t));
        *count = n;
    }

    // first round passed, loop
    r--; if(!r) goto loop;

    // report count
    log_info( "%d items at %p, from 1-%d", *count, (void*)p, *count);

    /* resort using custom comparision function */
    qsort(p, *count, sizeof(entry_t), struct_cmp_by_name);

    // report items
    //for (int i = 0; i < num; ++i) log_error( "%s", p[i].name);

    return p;
}


bool filter_entry_on_IDs(const char *entry)
{
    if(strlen(entry) != 9) return false;

    unsigned int nid;
    char id[ 5],
        tmp[32];

    int ret = sscanf( entry, "%c%c%c%c%5u",
                      &id[0],  &id[1],  &id[2],  &id[3], &nid );
    id[4] = '\0';
    if(ret == 5)
    {
        int res = strlen(id);      // ABCD
        if(res != 4
        || nid > 99999) goto fail; // 01234

        snprintf(&tmp[0], 31, "%s%.5u", id, nid);

        if(strlen(tmp) != 9) goto fail;
        // passed
//      log_info("%s - %s looks valid", entry, tmp);

        return true;
    }
fail:
    return false;
}

// each token_data is filled by dynalloc'd mem by strdup!
item_t *index_items_from_dir(const char *dirpath)
{
    // grab, count and sort entries by name
    int   count = -1;
    entry_t  *e = get_item_entries(dirpath, &count);
    // output
    item_t *ret = calloc(count +1, sizeof(item_t));

    char tmp[256];
    // skip first reserved: take care
    int i = 1, j;
    for(j = 1; j < count +1; j++)
    {   // we use just those token_data
        snprintf(&tmp[0], 255, "%s", e[ j -1 ].name);

        // for Installed_Apps
      {  
        if( ! filter_entry_on_IDs(tmp) ) continue;

        char fullpath[128];
        snprintf(&fullpath[0], 127, "%s/%.9s", dirpath, tmp);
        // passed
//      log_info( "%3d, %d: %s", i, j, fullpath);

        if( check_stat(fullpath) != S_IFDIR ) continue;
      }
        // dynalloc for user tokens
        ret[i].token_c           = NUM_OF_USER_TOKENS;
        ret[i].token_d           = calloc(ret[i].token_c, sizeof(item_idx_t));

        ret[i].token_d[ ID ].off = strdup(tmp);
        ret[i].token_d[ ID ].len = strlen(tmp);
        // replicate for now
        ret[i].token_d[ NAME ].off = strdup(tmp),
        ret[i].token_d[ NAME ].len = strlen(tmp);

#if defined (__ORBIS__)
        snprintf(&tmp[0], 255, "/user/appmeta/%s/icon0.png", e[ j -1 ].name);
#else // on pc
        snprintf(&tmp[0], 255, "./storedata/%s/icon0.png", e[ j -1 ].name);
#endif

        ret[i].token_d[ PICPATH ].off = strdup(tmp),
        ret[i].token_d[ PICPATH ].len = strlen(tmp);
        // don't try lo load
        ret[i].texture = 0;// XXX load_png_asset_into_texture(tmp);

#if defined (__ORBIS__)
        snprintf(&tmp[0], 255, "/system_data/priv/appmeta/%s/param.sfo", e[ j -1 ].name);
#else // on pc
        snprintf(&tmp[0], 255, "./storedata/%s/param.sfo", e[ j -1 ].name);
#endif

        // read sfo
        index_token_from_sfo(&ret[i], &tmp[0]);
        i++;
    }
    // save path and counted items in first (reserved) index
    ret[0].token_d        = calloc(1, sizeof(item_idx_t));
    ret[0].token_d[0].off = strdup(dirpath);
    ret[0].token_c        = i -1;// updated count;

    ret = realloc(ret, i * sizeof(item_t));

    log_info("valid count: %d", i -1);
    // all done, release!
    free_item_entries(e);

    // report back
    if(0){
        for(int i = 1; i < ret[0].token_c +1 /* off by one, skip 0 reserved! */; i++)
            log_info( "%3d: %s", i, ret[i].token_d[ ID ].off);
    }

    return ret;
}

// from https://elixir.bootlin.com/busybox/0.39/source/df.c
#include <stdio.h>
#include <sys/mount.h>

int df(char *out, const char *mountPoint)
{
    struct statfs s;
    long blocks_used = 0;
    long blocks_percent_used = 0;
    //struct fstab* fstabItem;

    if (statfs(mountPoint, &s) != 0) {
        log_error("error %s", mountPoint);
        return 0;
    }

    if (s.f_blocks > 0)
    {
        blocks_used = s.f_blocks - s.f_bfree;
        blocks_percent_used = (long)
        (blocks_used * 100.0 / (blocks_used + s.f_bavail) + 0.5);
#if 0
    log_info("%-20s %9ld %9ld %9ld %3ld%% %s",
           out,
           (long) (s.f_blocks * (s.f_bsize / 1024.0)),
           (long) ((s.f_blocks - s.f_bfree) * (s.f_bsize / 1024.0)),
           (long) (s.f_bavail * (s.f_bsize / 1024.0)),
           blocks_percent_used, mountPoint);
#endif
    }
    double gb_free = ((long)(s.f_bavail * (s.f_bsize / 1024.0))
                   / 1024);
    gb_free /= 1024.;
//  log_info("%s, %3ld%%, %.1f GB free", mountPoint, blocks_percent_used, gb_free);
    snprintf(out, 127, "%s, %3ld%%, %.1f GB free", mountPoint, blocks_percent_used, gb_free);

    return blocks_percent_used;
}

#include <time.h> // ctime

void get_stat_from_file(char *out, const char *filepath)
{
    struct stat fileinfo;

    if( ! stat(filepath, &fileinfo) )
    {
        // https://man7.org/linux/man-pages/man2/stat.2.html#EXAMPLES
        switch(fileinfo.st_mode & S_IFMT) {
            case S_IFBLK:  snprintf(out, 127, "Block device");                              break;
            case S_IFCHR:  snprintf(out, 127, "Character device");                          break;
            case S_IFDIR:  snprintf(out, 127, "Directory, %s", ctime(&fileinfo.st_mtime));  break;
            case S_IFIFO:  snprintf(out, 127, "FIFO/Pipe");                                 break;
            case S_IFLNK:  snprintf(out, 127, "Symlink");                                   break;
            case S_IFREG:  snprintf(out, 127, "Size: %ld bytes, %s", fileinfo.st_size, ctime(&fileinfo.st_mtime));  break;
            case S_IFSOCK: snprintf(out, 127, "Socket");                                    break;
            default:       snprintf(out, 127, "Unknown, Mode %o", fileinfo.st_mode);        break;
        }

    }
    else // can't be reached by logic, but in case...
        snprintf(out, 127, "Error stat(%s)", filepath);

    // clean string, works for LF, CR, CRLF, LFCR, ...
    out[strcspn(out, "\r")] = 0;
}

int check_stat(const char *filepath)
{
    struct stat fileinfo;
    if( ! stat(filepath, &fileinfo) )
    {
        switch(fileinfo.st_mode & S_IFMT)
        {
            case S_IFDIR: return S_IFDIR;
            case S_IFREG: return S_IFREG;
        }
    }
    return 0;
}

