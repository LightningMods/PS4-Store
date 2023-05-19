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
#include <sys/stat.h>
#include <sys/vfs.h>
#include "utils.h"


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

// from https://elixir.bootlin.com/busybox/0.39/source/df.c
#include <stdio.h>
#include <sys/mount.h>
#ifndef __ORBIS__
#include <sys/vfs.h>
#endif

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
        log_info("Allocating %i", (*cv)->sz);
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
            bse.sz = strlen(bse.fname) + 2;
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

int vaild_count = 0;

int number_of_iapps(const char *path) {

#ifndef __ORBIS__
    return 4; // chosen by fair dice roll
    // guaranteed to be random
#else
    struct CVec* cvEntries = NULL;
    char tmp[255];
    int count = -1;
    bool does_app_exist = false;

    if(vaild_count > 0)
        return vaild_count;

    CV_Create(&cvEntries, 666666 ^ 222);

    if (getEntries(path, cvEntries))
        log_debug("[Dir 1] Found %i Apps in: %s", (count = cvEntries->cur / sizeof(struct _ent)), path);
    else
        log_error("[Dir 1] Failed to get entries in: %s", path);

    for (int j = 1; j < count + 1; j++)
    {   // we use just those token_data
        struct _ent* e = &((struct _ent*)cvEntries->ptr)[j - 1];
        snprintf(&tmp[0], 254, "%s", e->fname);

        if (!filter_entry_on_IDs(&tmp[0])) {
             log_debug("[F1] Filtered out %s", &tmp[0]);
             continue;
        }
        //extra filter: see if sonys pkg api can vaildate it
        if (app_inst_util_is_exists(&tmp[0], &does_app_exist)) {
            if (!does_app_exist) {
                 log_debug("[F2] Filtered out %s, the App doesnt exist", &tmp[0]);
                continue;
           }
        } 
         vaild_count++;
    }

    CV_Destroy(&cvEntries);

    log_info("[COUNT] Found %i Vaild Apps", vaild_count);
#endif

    return vaild_count;
}

int oo_statfs(const char *path, struct statfs *buf)
{
#ifdef __ORBIS__
  return syscall_alt(396, path, buf);
#else
    return statfs(path, buf);
#endif
}

int df(std::string mountPoint, std::string &out)
{
    struct statfs s;
    long blocks_used = 0;
    long blocks_percent_used = 0;
    // struct fstab* fstabItem;
#ifndef __ORBIS__
    if (statfs(mountPoint.c_str(), &s) != 0)
#else
    if (oo_statfs(mountPoint.c_str(), &s) != 0)
#endif
    {
        log_error("df cannot open %s", mountPoint.c_str());
        return 0;
    }

    if (s.f_blocks > 0)
    {
        blocks_used = s.f_blocks - s.f_bfree;
        blocks_percent_used = (long)(blocks_used * 100.0 / (blocks_used + s.f_bavail) + 0.5);
#if 0
        log_info("%-20s %9ld %9ld %9ld %3ld%% %s",
            out,
            (long)(s.f_blocks * (s.f_bsize / 1024.0)),
            (long)((s.f_blocks - s.f_bfree) * (s.f_bsize / 1024.0)),
            (long)(s.f_bavail * (s.f_bsize / 1024.0)),
            blocks_percent_used, mountPoint);
#endif
    }
    double gb_free = ((long)(s.f_bavail * (s.f_bsize / 1024.0)) / 1024);
    gb_free /= 1024.;

    double disk_space = (long)(s.f_blocks * (s.f_bsize / 1024.0) / 1024);
    disk_space /= 1024.;

    out = fmt::format("{0:}, {1:d}%, {2:.1f}GBs / {3:.1f}GBs", mountPoint, blocks_percent_used, disk_space - gb_free, disk_space);
    return blocks_percent_used;
}



int check_free_space(const char* mountPoint)
{
    struct statfs s;

    if (oo_statfs(mountPoint, &s) != 0) {
        log_error("error %s, strerror %s", mountPoint, strerror(errno));
        return 0;
    }

    double gb_free = ((long)(s.f_bavail * (s.f_bsize / 1024.0)) / 1024);
    gb_free /= 1024.;
    log_info("%s has %.1f GB free", mountPoint, gb_free);

    return (int)floor(gb_free);
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