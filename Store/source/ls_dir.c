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
#include <errno.h>

bool if_exists(const char* path)
{
    int dfd = open(path, O_RDONLY, 0); // try to open dir
    if (dfd < 0) {
        log_info("path %s, errno %s", path, strerror(errno));
        return false;
    }
    else
      close(dfd);
    

    return true;
}

bool filter_entry_on_IDs(const char* entry)
{
    if (strlen(entry) != 9) return false;

    unsigned int nid;
    char id[5],
        tmp[32];

    int ret = sscanf(entry, "%c%c%c%c%5u",
        &id[0], &id[1], &id[2], &id[3], &nid);
    id[4] = '\0';
    if (ret == 5)
    {
        int res = strlen(id);      // ABCD...
        if (res != 4
            || nid > 99999) goto fail; // 01234

        snprintf(&tmp[0], 31, "%s%.5u", id, nid);

        if (strlen(tmp) != 9) goto fail;
        // passed
//      log_info("%s - %s looks valid", entry, tmp);

        return true;
    }
fail:
    return false;
}

void CV_Create(struct CVec** cv, uint32_t size)
{
    if (cv) {
        if (!(*cv))	*cv = (struct CVec*)malloc(sizeof(struct CVec));

        (*cv)->cur = 0;
        (*cv)->sz = (size + 4095) & ~4095;		// round to page size
        log_info("Allocating %llx", (*cv)->sz);
        (*cv)->ptr = malloc((*cv)->sz);
    }
}

void CV_Destroy(struct CVec** cv) {
    if (cv) {
        if ((*cv)) {
            if ((*cv)->ptr) free((*cv)->ptr);
            free(*cv);
        }
        *cv = NULL;
    }
}

void CV_Clear(struct CVec* cv) {
    if (cv) { cv->cur = 0; memset(cv->ptr, 0, cv->sz); }
}

void CV_Append(struct CVec* cv, void* data, uint32_t size)
{
    if (!cv) { log_error("Error, null cv!\n"); return; }

    if ((cv->cur + size) > cv->sz)
    {
        cv->sz = (cv->sz + size + 4095) & ~4095;
        cv->ptr = realloc(cv->ptr, cv->sz);
    }

    if ((cv->cur + size) <= cv->sz)
    {
        memcpy(((uint8_t*)cv->ptr + cv->cur), data, size);
        cv->cur += size;
    }
}

bool getEntries(const char* path, struct CVec* cv)
{
    struct dirent* pDirent;
    DIR* pDir = NULL;

    if (!strlen(path))
        return false;

    pDir = opendir(path);
    if (pDir == NULL)
        return false;

    while ((pDirent = readdir(pDir)) != NULL) {


        if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
            continue;

        if (pDirent->d_type == DT_DIR) {

            // Dir is title_id, it WILL go through 3 filters
            // so lets assume they are vaild until the filters say otherwise
            struct _ent bse;
            strcpy(bse.fname, pDirent->d_name);    // dir + '/' + dName ?
            bse.sz = pDirent->d_namlen + 2;
            CV_Append(cv, &bse, sizeof(bse));
        }
        else 
            continue;
        
    }
    if (closedir(pDir) != 0) {
        return false;
    }
    return true;
}

#include <time.h>
int HDD_count = -1;

const char* SPECIAL_XMB_ICON_FOLDERS[] = { APP_HOME_DATA_FOLDER };
const char* SPECIAL_XMB_ICON_TID[] = { APP_HOME_DATA_TID };

void init_xmb_special_icons(const char* XMB_ICON_PATH)
{
    char buf[255];

    if (!if_exists(XMB_ICON_PATH))
    {
        log_info("[XMB OP] Creating %s for XMB", XMB_ICON_PATH);
        mkdir(XMB_ICON_PATH, 0777);
        snprintf(buf, 254, "%s/app.pkg", XMB_ICON_PATH);
        touch_file(buf);
    }
    else
        log_info("[XMB OP] %s already exists", XMB_ICON_PATH);
}

void info_for_xmb_special_icons(item_t* item, const char* TITLE, const char* TID, const char* VER)
{

    item_idx_t* t = &item->token_d[0];

    t[NAME].off = strdup(TITLE);
    t[NAME].len = strlen(t[NAME].off);
    log_info("[XMB OP] SFO Name: %s:%i", t[NAME].off, t[NAME].len);
    t[ID].off = strdup(TID);
    t[ID].len = strlen(t[ID].off);
    log_info("[XMB OP] SFO ID: %s", t[ID].off);
    t[VERSION].off = strdup(VER);
    t[VERSION].len = strlen(t[VERSION].off);
    log_info("[XMB OP] SFO VERSION: %s", t[VERSION].off);
}



// each token_data is filled by dynalloc'd mem by strdup!
item_t* index_items_from_dir(const char* dirpath, const char* dirpath2)
{

    uint64_t start = sceKernelGetProcessTime();

    log_info("Starting index_items_from_dir ...");

   // init_xmb_special_icons(APP_HOME_DATA_FOLDER);

    struct CVec* cvEntries = NULL;
    int ext_count = -1;
    CV_Create(&cvEntries, 666666 ^ 222);

    if (!getEntries(dirpath, cvEntries))
        msgok(FATAL, "Unable to Open %s", dirpath);
    else
        log_debug("[Dir 1] Found %i Apps in: %s", (HDD_count = cvEntries->cur / sizeof(struct _ent)), dirpath);

    if (if_exists(dirpath2))
    {
        if (!getEntries(dirpath2, cvEntries))
            log_warn("Unable to locate Apps on %s", dirpath2);
        else
        {
            ext_count = (cvEntries->cur / sizeof(struct _ent) - HDD_count);
            log_debug("[Dir 2] Found %i Apps in: %s", ext_count, dirpath2);
        }
    }

    uint32_t entCt = cvEntries->cur / sizeof(struct _ent);
    log_debug("Have %d entries:", entCt);
#if 0
    for (int ix = 0; ix < entCt; ix++) {
        struct _ent* e = &((struct _ent*)cvEntries->ptr)[ix];
        log_debug("ent[%06d] size: %d\t\"%s\"", ix, e->sz, e->fname);
    }
#endif
    // output
    item_t* ret = calloc(entCt + 1, sizeof(item_t));

    char tmp[256];
    // skip first reserved: take care
    int i = 1, j;
    for (j = 1; j < entCt + 1; j++)
    {   // we use just those token_data

        struct _ent* e = &((struct _ent*)cvEntries->ptr)[j - 1];

        snprintf(&tmp[0], 255, "%s", e->fname);

        // for Installed_Apps
        {
            if (!filter_entry_on_IDs(tmp))
            {
                log_debug("[F1] Filtered out %s", tmp);
                continue;
            }

            //extra filter: can open app.pkg
            char fullpath[128];
            snprintf(&fullpath[0], 127, "%s/%.9s/app.pkg", dirpath, tmp);

            if (!if_exists(fullpath))
                snprintf(&fullpath[0], 127, "%s/%.9s/app.pkg", dirpath2, tmp);

            if (!if_exists(fullpath))
            {
                log_debug("[F2] Filtered out %s, %s doesnt exist", tmp, fullpath);
                continue;
            }
        }
        // dynalloc for user tokens
        ret[i].token_c = NUM_OF_USER_TOKENS;
        ret[i].token_d = calloc(ret[i].token_c, sizeof(item_idx_t));

        ret[i].token_d[ID].off = strdup(tmp);
        ret[i].token_d[ID].len = strlen(tmp);
        // replicate for now
        ret[i].token_d[NAME].off = strdup(tmp),
        ret[i].token_d[NAME].len = strlen(tmp);

#if defined (__ORBIS__)

        snprintf(&tmp[0], 255, "/user/appmeta/%s/icon0.png", e->fname);

        if (!if_exists(tmp))
            snprintf(&tmp[0], 255, "/user/appmeta/external/%s/icon0.png", e->fname);

#else // on pc
        snprintf(&tmp[0], 255, "./storedata/%s/icon0.png", e->fname);
#endif
        ret[i].token_d[PICPATH].off = strdup(tmp),
            ret[i].token_d[PICPATH].len = strlen(tmp);
        // don't try lo load
        ret[i].texture = 0;// XXX load_png_asset_into_texture(tmp);

#if defined (__ORBIS__)

        snprintf(&tmp[0], 255, "/system_data/priv/appmeta/%s/param.sfo", e->fname);

        if (!if_exists(tmp))
            snprintf(&tmp[0], 255, "/system_data/priv/appmeta/external/%s/param.sfo", e->fname);
#else // on pc
        snprintf(&tmp[0], 255, "./storedata/%s/param.sfo", e->fname);
#endif  

        // read sfo
     //   for (int i = 0; sizeof SPECIAL_XMB_ICON_TID / sizeof SPECIAL_XMB_ICON_TID[0] -1; i++)
      //  {


            if (strstr(e->fname, APP_HOME_DATA_TID) != NULL) {


                if (strstr(e->fname, APP_HOME_DATA_TID))
                {
                    ret[i].token_d[PICPATH].off = strdup("/data/APP_HOME.png"),
                    ret[i].token_d[PICPATH].len = strlen("/data/APP_HOME.png");
                   // info_for_xmb_special_icons(&ret[i], "APP_HOME(Data)", APP_HOME_DATA_TID, "0.00");
                }
                    
            }
            else
               index_token_from_sfo(&ret[i], &tmp[0]);
      //  }
        i++;
    }


    // save path and counted items in first (reserved) index
    ret[0].token_d = calloc(1, sizeof(item_idx_t));
    ret[0].token_d[0].off = strdup(dirpath);
    ret[0].token_c = i - 1;// updated count;


    log_info("valid count: %d", i - 1);
    HDD_count = i;
    HDD_count -= ext_count + 1;
    log_info("HDD_count: %i", HDD_count);

    ret = realloc(ret, i * sizeof(item_t));

    // report back
    if (0) {
        for (int i = 1; i < ret[0].token_c; i++)
            log_info("%3d: %s", i, ret[i].token_d[ID].off);
    }
    CV_Destroy(&cvEntries);

    log_debug("Took %lld microsecs to load Apps", (sceKernelGetProcessTime() - start));

    return ret;
}

// from https://elixir.bootlin.com/busybox/0.39/source/df.c
#include <stdio.h>
#include <sys/mount.h>

int df(char* out, const char* mountPoint)
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
            (long)(s.f_blocks * (s.f_bsize / 1024.0)),
            (long)((s.f_blocks - s.f_bfree) * (s.f_bsize / 1024.0)),
            (long)(s.f_bavail * (s.f_bsize / 1024.0)),
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

void get_stat_from_file(char* out, const char* filepath)
{
    struct stat fileinfo;

    if (!stat(filepath, &fileinfo))
    {
        // https://man7.org/linux/man-pages/man2/stat.2.html#EXAMPLES
        switch (fileinfo.st_mode & S_IFMT) {
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

int check_stat(const char* filepath)
{
    struct stat fileinfo;
    if (!stat(filepath, &fileinfo))
    {
        switch (fileinfo.st_mode & S_IFMT)
        {
        case S_IFDIR: return S_IFDIR;
        case S_IFREG: return S_IFREG;
        }
    }
    return 0;
}

