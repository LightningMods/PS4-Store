#ifndef FW_DEFINES_H
#define FW_DEFINES_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



struct auditinfo_addr {
  char useless[184];
};

struct ucred {
  uint32_t useless1;
  uint32_t cr_uid;
  uint32_t cr_ruid;
  uint32_t useless2;
  uint32_t useless3;
  uint32_t cr_rgid;
  uint32_t useless4;
  void *useless5;
  void *useless6;
  void *cr_prison;
  void *useless7;
  uint32_t useless8;
  void *useless9[2];
  void *useless10;
  struct auditinfo_addr useless11;
  uint32_t *cr_groups;
  uint32_t useless12;
};

struct filedesc {
  void *useless1[3];
  void *fd_rdir;
  void *fd_jdir;
};

struct proc {
  char useless[64];
  struct ucred *p_ucred;
  struct filedesc *p_fd;
};

struct thread {
  void *useless;
  struct proc *td_proc;
};

struct kpayload_get_fw_version_info {
  uint64_t uaddr;
};

struct kpayload_get_fw_version_args {
  void *syscall_handler;
  struct kpayload_get_fw_version_info *kpayload_get_fw_version_info;
};

struct kpayload_jailbreak_info {
  uint64_t fw_version;
};

struct kpayload_jailbreak_args {
  void *syscall_handler;
  struct kpayload_jailbreak_info *kpayload_jailbreak_info;
};


#define K405_XFAST_SYSCALL         0x0030EB30
#define K455_XFAST_SYSCALL         0x003095D0
#define K474_XFAST_SYSCALL         0x0030B7D0
#define K501_XFAST_SYSCALL         0x000001C0
#define K503_XFAST_SYSCALL         0x000001C0
#define K505_XFAST_SYSCALL         0x000001C0
#define K672_XFAST_SYSCALL         0x000001C0

#define K405_PRISON_0              0x00F26010
#define K455_PRISON_0              0x010399B0
#define K474_PRISON_0              0x01042AB0
#define K501_PRISON_0              0x010986A0
#define K503_PRISON_0              0x010986A0
#define K505_PRISON_0              0x010986A0
#define K672_PRISON_0              0x0113E518

#define K405_ROOTVNODE             0x0206D250
#define K455_ROOTVNODE             0x021AFA30
#define K474_ROOTVNODE             0x021B89E0
#define K501_ROOTVNODE             0x022C19F0
#define K503_ROOTVNODE             0x022C1A70
#define K505_ROOTVNODE             0x022C1A70
#define K672_ROOTVNODE             0x02300320

#define K405_PRINTF                0x00347580
#define K455_PRINTF                0x00017F30
#define K474_PRINTF                0x00017F30
#define K501_PRINTF                0x00435C70
#define K503_PRINTF                0x00436000
#define K505_PRINTF                0x00436040
#define K672_PRINTF                0x00123280

#define K405_COPYIN                0x00286DF0
#define K455_COPYIN                0x0014A890
#define K474_COPYIN                0x00149F20
#define K501_COPYIN                0x001EA600
#define K503_COPYIN                0x001EA710
#define K505_COPYIN                0x001EA710
#define K672_COPYIN                0x003C17A0

#define K405_COPYOUT               0x00286D70
#define K455_COPYOUT               0x0014A7B0
#define K474_COPYOUT               0x00149E40
#define K501_COPYOUT               0x001EA520
#define K503_COPYOUT               0x001EA630
#define K505_COPYOUT               0x001EA630
#define K672_COPYOUT               0x003C16B0



uint64_t get_fw_version(void);
int jailbreak_multi(uint64_t fw_version);
int ioctl(int fd, unsigned long com, void* data);



#endif