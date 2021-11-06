#ifndef UNPFS_H
#define UNPFS_H

struct pfs_header_t
{
  uint64_t version;
  uint64_t magic;
  uint32_t id[2];
  char fmode;
  char clean;
  char ronly;
  char rsv;
  uint16_t mode;
  uint16_t unk1;
  uint32_t blocksz;
  uint32_t nbackup;
  uint64_t nblock;
  uint64_t ndinode;
  uint64_t ndblock;
  uint64_t ndinodeblock;
  uint64_t superroot_ino;	
} __attribute__((packed));

struct di_d32
{
  uint16_t mode;
  uint16_t nlink;
  uint32_t flags;
  uint64_t size;
  uint64_t size_compressed;
  uint64_t unix_time[4];
  uint32_t time_nsec[4];
  uint32_t uid;
  uint32_t gid;
  uint64_t spare[2];
  uint32_t blocks;
  uint32_t db[12];
  uint32_t ib[5];
} __attribute__((packed));

struct dirent_t
{
  uint32_t ino;
  uint32_t type;
  uint32_t namelen;
  uint32_t entsize;
  //char name[namelen+1];
} __attribute__((packed));


int unpfs(char *pfsfn, char *tidpath, char* gtitle, char* title_id);

#endif
