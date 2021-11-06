// Copyright (C) 2013       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/gpl-2.0.txt


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // lmalloc, qsort, lfree
#include <unistd.h>  // close
#include <sys/signal.h>
#include <errno.h>
#include <sig_handler.h>

#if defined (__ORBIS__)

#include <ps4sdk.h>
#include <debugnet.h>
#include <orbislink.h>
#include <libkernel.h>
#include <sys/mman.h>
#include <ImeDialog.h>
#include <utils.h>
#include <MsgDialog.h>
#include <CommonDialog.h>
#include <dump_and_decrypt.h>

#define  fprintf  debugNetPrintf
#define  ERROR    DEBUGNET_ERROR
#define  DEBUG    DEBUGNET_DEBUG
#define  INFO     DEBUGNET_INFO

#else // on linux

#include <stdio.h>
#define  debugNetPrintf  fprintf
#define  ERROR           stderr
#define  DEBUG           stdout
#define  INFO            stdout

#endif

#include <memory.h>
#include "unpfs.h"

char notify_buf[1024];

int pfs;
size_t pfs_size, pfs_copied;
struct pfs_header_t *header;
struct di_d32 *inodes;

#define BUFFER_SIZE 0x100000

char *copy_buffer;

uint32_t p_progress = 10;

void memcpy_to_file(const char *fname, uint64_t ptr, uint64_t size)
{
  size_t bytes;
  size_t ix = 0;

  int fd = sceKernelOpen(fname, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  log_info("---- name: %s, ptr %p, sz %zu, FD: %i", fname, ptr, size, fd);
  if (fd > 0)
  {
    while (size > 0)
    {
      bytes = (size > BUFFER_SIZE) ? BUFFER_SIZE : size;
      lseek(pfs, ptr + ix * BUFFER_SIZE, SEEK_SET);
      read(pfs, copy_buffer, bytes);
      write(fd, copy_buffer, bytes);
      size -= bytes;
      ix++;
      pfs_copied += bytes;
      if (pfs_copied > pfs_size) pfs_copied = pfs_size;
         p_progress = (uint32_t)(((float)pfs_copied / pfs_size) * 100.f);
    
    }
    log_info("Close(fd): %x", sceKernelClose(fd));
  }
  else
  {
    msgok(WARNING, "Cannot copy file |%s|\n\nError: %x!", fname, fd);
  }
}

static void parse_directory(int ino, int lev, char *parent_name, bool dry_run, char* gtitle, char* title_id)
{
  for (uint32_t z = 0; z < inodes[ino].blocks; z++) 
  {
    uint32_t db = inodes[ino].db[0] + z;
    uint64_t pos = (uint64_t)header->blocksz * db;
    uint64_t size = inodes[ino].size;
    uint64_t top = pos + size;

    while (pos < top)
    {
      struct dirent_t *ent = lmalloc (sizeof(struct dirent_t));
      lseek(pfs, pos, SEEK_SET);
      read(pfs, ent, sizeof(struct dirent_t));

      if (ent->type == 0)
      {
        lfree(ent);
        break;
      }

      char *name = lmalloc(ent->namelen + 1);
      memset(name, 0, ent->namelen + 1);
      if (lev > 0)
      {
        lseek(pfs, pos + sizeof(struct dirent_t), SEEK_SET);
        read(pfs, name, ent->namelen);
      }


      char* fname = lmalloc(strlen(parent_name) + ent->namelen + 2);
      if (parent_name != NULL)
          sprintf(fname, "%s/%s", parent_name, name);
      else
          sprintf(fname, "%s", name);

      if ((ent->type == 2) && (lev > 0))
      {
         log_info("%s -- prog %llx", fname, p_progress);
         
        if(strstr(fname, "-app"))
	           ProgSetMessagewText(p_progress, "Dump info\nApp name: %s\nTITLE_ID %s\n\nExtracting Base Game files File %s to USB...\n", gtitle, title_id, fname);
        else if(strstr(fname, "patch"))
               ProgSetMessagewText(p_progress, "Dump info\nApp name: %s\nTITLE_ID %s\n\nExtracting Game Patch File %s to USB...\n", gtitle, title_id, fname);

        if (dry_run)
          pfs_size += inodes[ent->ino].size;
        else
          memcpy_to_file(fname, (uint64_t)header->blocksz * inodes[ent->ino].db[0], inodes[ent->ino].size);
      }
      else
      if (ent->type == 3)
      {
        log_info(">scan dir %s", name);
        mkdir(fname, 0777);
        parse_directory(ent->ino, lev + 1, fname, dry_run, gtitle, title_id);
      }

      pos += ent->entsize;

      lfree(ent);
      lfree(name);
      lfree(fname);
    }
  }
}

int unpfs(char *pfsfn, char *tidpath, char* gtitle, char* title_id)
{
  copy_buffer = lmalloc(BUFFER_SIZE);

  p_progress = 11;

  mkdir(tidpath, 0777);

  pfs = open(pfsfn, O_RDONLY, 0);
  if (pfs < 0) return -1;

  header = lmalloc(sizeof(struct pfs_header_t));
  lseek(pfs, 0, SEEK_SET);
  read(pfs, header, sizeof(struct pfs_header_t));

  inodes = lmalloc(sizeof(struct di_d32) * header->ndinode);

  uint32_t ix = 0;

  for (uint32_t i = 0; i < header->ndinodeblock; i++)
  {		
    for (uint32_t j = 0; (j < (header->blocksz / sizeof(struct di_d32))) && (ix < header->ndinode); j++)
    {
      lseek(pfs, (uint64_t)header->blocksz * (i + 1) + sizeof(struct di_d32) * j, SEEK_SET);
      read(pfs, &inodes[ix], sizeof(struct di_d32));


 
	   ProgSetMessagewText(20, "Dump info\n\nApp name: %s\nTITLE_ID %s\n\n Processing ino 0x%x...", gtitle, title_id, ix);

      ix++;       
    }
  }

  pfs_size = 0;
  pfs_copied = 0;

  parse_directory(header->superroot_ino, 0, tidpath, 1, gtitle, title_id);
  parse_directory(header->superroot_ino, 0, tidpath, 0, gtitle, title_id);

  notify_buf[0] = '\0';

  lfree(header);
  lfree(inodes);
  close(pfs);
  lfree(copy_buffer);
	
  return 0;
}
