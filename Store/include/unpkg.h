#ifndef UNPKG_H
#define UNPKG_H

#define PS4_PKG_MAGIC 0x544E437F // .CNT

enum PS4_PKG_ENTRY_TYPES
{
  PS4_PKG_ENTRY_TYPE_DIGEST_TABLE = 0x0001,
  PS4_PKG_ENTRY_TYPE_0x800        = 0x0010,
  PS4_PKG_ENTRY_TYPE_0x200        = 0x0020,
  PS4_PKG_ENTRY_TYPE_0x180        = 0x0080,
  PS4_PKG_ENTRY_TYPE_META_TABLE   = 0x0100,
  PS4_PKG_ENTRY_TYPE_NAME_TABLE   = 0x0200,
  PS4_PKG_ENTRY_TYPE_LICENSE      = 0x0400,
  PS4_PKG_ENTRY_TYPE_FILE1        = 0x1000,
  PS4_PKG_ENTRY_TYPE_FILE2        = 0x1200
};

// CNT/PKG structures.
struct cnt_pkg_main_header
{
  uint32_t magic;
  uint32_t type;
  uint32_t unk_0x08;
  uint32_t unk_0x0C;
  uint16_t unk1_entries_num;
  uint16_t table_entries_num;
  uint16_t system_entries_num;
  uint16_t unk2_entries_num;
  uint32_t file_table_offset;
  uint32_t main_entries_data_size;
  uint32_t unk_0x20;
  uint32_t body_offset;
  uint32_t unk_0x28;
  uint32_t body_size;
  uint8_t  unk_0x30[0x10];
  uint8_t  content_id[0x30];
  uint32_t unk_0x70;
  uint32_t unk_0x74;
  uint32_t unk_0x78;
  uint32_t unk_0x7C;
  uint32_t date;
  uint32_t time;
  uint32_t unk_0x88;
  uint32_t unk_0x8C;
  uint8_t  unk_0x90[0x70];
  uint8_t  main_entries1_digest[0x20];
  uint8_t  main_entries2_digest[0x20];
  uint8_t  digest_table_digest[0x20];
  uint8_t  body_digest[0x20];
} __attribute__((packed));

struct cnt_pkg_content_header
{
  uint32_t unk_0x400;
  uint32_t unk_0x404;
  uint32_t unk_0x408;
  uint32_t unk_0x40C;
  uint32_t unk_0x410;
  uint32_t content_offset;
  uint32_t unk_0x418;
  uint32_t content_size;
  uint32_t unk_0x420;
  uint32_t unk_0x424;
  uint32_t unk_0x428;
  uint32_t unk_0x42C;
  uint32_t unk_0x430;
  uint32_t unk_0x434;
  uint32_t unk_0x438;
  uint32_t unk_0x43C;
  uint8_t  content_digest[0x20];
  uint8_t  content_one_block_digest[0x20];
} __attribute__((packed));

struct cnt_pkg_table_entry
{
  uint32_t type;
  uint32_t unk1;
  uint32_t flags1;
  uint32_t flags2;
  uint32_t offset;
  uint32_t size;
  uint32_t unk2;
  uint32_t unk3;
} __attribute__((packed));

// Internal structure.
struct file_entry
{
  int offset;
  int size;
  char *name;
};

int unpkg(char *pkgfn, char *tidpath);

#endif
