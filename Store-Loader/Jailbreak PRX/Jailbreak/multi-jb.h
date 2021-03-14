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



struct kpayload_kbase_info {
  uint16_t fw_version;
  uint64_t uaddr;
};

struct kpayload_kbase_args {
  void *syscall_handler;
  struct kpayload_kbase_info *kpayload_kbase_info;
};

struct kpayload_dump_info {
  uint16_t fw_version;
  uint64_t kaddr;
  uint64_t uaddr;
  size_t size;
};

struct kpayload_dump_args {
  void *syscall_handler;
  struct kpayload_dump_info *kpayload_dump_info;
};

struct kpayload_firmware_info {
  uint16_t fw_version;
};

struct kpayload_firmware_args {
  void *syscall_handler;
  struct kpayload_firmware_info *kpayload_firmware_info;
};




#define K405_XFAST_SYSCALL         0x0030EB30
#define K455_XFAST_SYSCALL         0x003095D0
#define K474_XFAST_SYSCALL         0x0030B7D0
#define K501_XFAST_SYSCALL         0x000001C0
#define K503_XFAST_SYSCALL         0x000001C0
#define K505_XFAST_SYSCALL         0x000001C0
#define K672_XFAST_SYSCALL         0x000001C0
#define K750_XFAST_SYSCALL         0x000001C0
#define K751_XFAST_SYSCALL         0x000001C0
#define K755_XFAST_SYSCALL         0x000001C0
#define K800_XFAST_SYSCALL         0x000001C0

// Used in every payload that uses jailbreak();
#define K300_XFAST_SYSCALL         0x0
#define K310_XFAST_SYSCALL         0x0
#define K311_XFAST_SYSCALL         0x0
#define K315_XFAST_SYSCALL         0x0
#define K350_XFAST_SYSCALL         0x003A1AD0
#define K355_XFAST_SYSCALL         0x003A1F10
#define K370_XFAST_SYSCALL         0x003A2000
#define K400_XFAST_SYSCALL         0x0030EA00
#define K401_XFAST_SYSCALL         0x0030EA00
#define K405_XFAST_SYSCALL         0x0030EB30
#define K406_XFAST_SYSCALL         0x0030EB40
#define K407_XFAST_SYSCALL         0x0030EB40
#define K450_XFAST_SYSCALL         0x003095D0
#define K455_XFAST_SYSCALL         0x003095D0
#define K470_XFAST_SYSCALL         0x0030B840
#define K471_XFAST_SYSCALL         0x0030B7D0
#define K472_XFAST_SYSCALL         0x0030B7D0
#define K473_XFAST_SYSCALL         0x0030B7D0
#define K474_XFAST_SYSCALL         0x0030B7D0
#define K500_XFAST_SYSCALL         0x000001C0
#define K501_XFAST_SYSCALL         0x000001C0
#define K503_XFAST_SYSCALL         0x000001C0
#define K505_XFAST_SYSCALL         0x000001C0
#define K507_XFAST_SYSCALL         0x000001C0
#define K550_XFAST_SYSCALL         0x000001C0
#define K553_XFAST_SYSCALL         0x000001C0
#define K555_XFAST_SYSCALL         0x000001C0
#define K556_XFAST_SYSCALL         0x000001C0
#define K600_XFAST_SYSCALL         0x000001C0
#define K602_XFAST_SYSCALL         0x000001C0
#define K620_XFAST_SYSCALL         0x000001C0
#define K650_XFAST_SYSCALL         0x000001C0
#define K651_XFAST_SYSCALL         0x000001C0
#define K670_XFAST_SYSCALL         0x000001C0
#define K671_XFAST_SYSCALL         0x000001C0
#define K672_XFAST_SYSCALL         0x000001C0
#define K700_XFAST_SYSCALL         0x000001C0
#define K701_XFAST_SYSCALL         0x000001C0
#define K702_XFAST_SYSCALL         0x000001C0
#define K750_XFAST_SYSCALL         0x000001C0
#define K751_XFAST_SYSCALL         0x000001C0
#define K755_XFAST_SYSCALL         0x000001C0
#define K800_XFAST_SYSCALL         0x000001C0
#define K801_XFAST_SYSCALL         0x0
#define K803_XFAST_SYSCALL         0x0

// Used in every payload that uses jailbreak();
#define K300_PRISON_0              0x0
#define K310_PRISON_0              0x0
#define K311_PRISON_0              0x0
#define K315_PRISON_0              0x0
#define K350_PRISON_0              0x00EF5A00
#define K355_PRISON_0              0x00EF5A00
#define K370_PRISON_0              0x00EFEF10
#define K400_PRISON_0              0x00F26010
#define K401_PRISON_0              0x00F26010
#define K405_PRISON_0              0x00F26010
#define K406_PRISON_0              0x00F26010
#define K407_PRISON_0              0x00F2A010
#define K450_PRISON_0              0x010399B0
#define K455_PRISON_0              0x010399B0
#define K470_PRISON_0              0x01042AB0
#define K471_PRISON_0              0x01042AB0
#define K472_PRISON_0              0x01042AB0
#define K473_PRISON_0              0x01042AB0
#define K474_PRISON_0              0x01042AB0
#define K500_PRISON_0              0x010986A0
#define K501_PRISON_0              0x010986A0
#define K503_PRISON_0              0x010986A0
#define K505_PRISON_0              0x010986A0
#define K507_PRISON_0              0x010986A0
#define K550_PRISON_0              0x01134180
#define K553_PRISON_0              0x01134180
#define K555_PRISON_0              0x01139180
#define K556_PRISON_0              0x01139180
#define K600_PRISON_0              0x01139458
#define K602_PRISON_0              0x01139458
#define K620_PRISON_0              0x0113D458
#define K650_PRISON_0              0x0113D4F8
#define K651_PRISON_0              0x0113D4F8
#define K670_PRISON_0              0x0113E518
#define K671_PRISON_0              0x0113E518
#define K672_PRISON_0              0x0113E518
#define K700_PRISON_0              0x0113E398
#define K701_PRISON_0              0x0113E398
#define K702_PRISON_0              0x0113E398
#define K750_PRISON_0              0x0113B728
#define K751_PRISON_0              0x0113B728
#define K755_PRISON_0              0x0113B728
#define K800_PRISON_0              0x0111A7D0
#define K801_PRISON_0              0x0
#define K803_PRISON_0              0x0

// Used in every payload that uses jailbreak();
#define K300_ROOTVNODE             0x0
#define K310_ROOTVNODE             0x0
#define K311_ROOTVNODE             0x0
#define K315_ROOTVNODE             0x0
#define K350_ROOTVNODE             0x01963000
#define K355_ROOTVNODE             0x01963040
#define K370_ROOTVNODE             0x0196F040
#define K400_ROOTVNODE             0x0206D250
#define K401_ROOTVNODE             0x0206D250
#define K405_ROOTVNODE             0x0206D250
#define K406_ROOTVNODE             0x0206D250
#define K407_ROOTVNODE             0x02071250
#define K450_ROOTVNODE             0x021AFA30
#define K455_ROOTVNODE             0x021AFA30
#define K470_ROOTVNODE             0x021B89E0
#define K471_ROOTVNODE             0x021B89E0
#define K472_ROOTVNODE             0x021B89E0
#define K473_ROOTVNODE             0x021B89E0
#define K474_ROOTVNODE             0x021B89E0
#define K500_ROOTVNODE             0x022C19F0
#define K501_ROOTVNODE             0x022C19F0
#define K503_ROOTVNODE             0x022C1A70
#define K505_ROOTVNODE             0x022C1A70
#define K507_ROOTVNODE             0x022C1A70
#define K550_ROOTVNODE             0x022EF570
#define K553_ROOTVNODE             0x022EF570
#define K555_ROOTVNODE             0x022F3570
#define K556_ROOTVNODE             0x022F3570
#define K600_ROOTVNODE             0x021BFAC0
#define K602_ROOTVNODE             0x021BFAC0
#define K620_ROOTVNODE             0x021C3AC0
#define K650_ROOTVNODE             0x02300320
#define K651_ROOTVNODE             0x02300320
#define K670_ROOTVNODE             0x02300320
#define K671_ROOTVNODE             0x02300320
#define K672_ROOTVNODE             0x02300320
#define K700_ROOTVNODE             0x022C5750
#define K701_ROOTVNODE             0x022C5750
#define K702_ROOTVNODE             0x022C5750
#define K750_ROOTVNODE             0x01B463E0
#define K751_ROOTVNODE             0x01B463E0
#define K755_ROOTVNODE             0x01B463E0
#define K800_ROOTVNODE             0x01B8C730
#define K801_ROOTVNODE             0x0
#define K803_ROOTVNODE             0x0

// Used in Kernel Dumper
#define K300_COPYOUT               0x0
#define K310_COPYOUT               0x0
#define K311_COPYOUT               0x0
#define K315_COPYOUT               0x00480BB0
#define K350_COPYOUT               0x003B9220
#define K355_COPYOUT               0x003B9660
#define K370_COPYOUT               0x003B9750
#define K400_COPYOUT               0x00286C40
#define K401_COPYOUT               0x00286C40
#define K405_COPYOUT               0x00286D70
#define K406_COPYOUT               0x00286D70
#define K407_COPYOUT               0x00286D70
#define K450_COPYOUT               0x0014A7B0
#define K455_COPYOUT               0x0014A7B0
#define K470_COPYOUT               0x00149E40
#define K471_COPYOUT               0x00149E40
#define K472_COPYOUT               0x00149E40
#define K473_COPYOUT               0x00149E40
#define K474_COPYOUT               0x00149E40
#define K500_COPYOUT               0x001EA520
#define K501_COPYOUT               0x001EA520
#define K503_COPYOUT               0x001EA630
#define K505_COPYOUT               0x001EA630
#define K507_COPYOUT               0x001EA630
#define K550_COPYOUT               0x00405AC0
#define K553_COPYOUT               0x004059C0
#define K555_COPYOUT               0x00405D80
#define K556_COPYOUT               0x00405D80
#define K600_COPYOUT               0x00114800
#define K602_COPYOUT               0x00114800
#define K620_COPYOUT               0x00114800
#define K650_COPYOUT               0x003C1300
#define K651_COPYOUT               0x003C1300
#define K670_COPYOUT               0x003C16B0
#define K671_COPYOUT               0x003C16B0
#define K672_COPYOUT               0x003C16B0
#define K700_COPYOUT               0x0002F140
#define K701_COPYOUT               0x0002F140
#define K702_COPYOUT               0x0002F140
#define K750_COPYOUT               0x0028F900
#define K751_COPYOUT               0x0028F900
#define K755_COPYOUT               0x0028F900
#define K800_COPYOUT               0x0025E2C0
#define K801_COPYOUT               0x0
#define K803_COPYOUT               0x0



#define UNUSED(x) (void)(x)

#define copyout_macro(x)                                                    \
  kernel_base = &((uint8_t *)__readmsr(0xC0000082))[-K##x##_XFAST_SYSCALL]; \
  copyout = (void *)(kernel_base + K##x##_COPYOUT);

#define jailbreak_macro(x)                                                  \
  kernel_base = &((uint8_t *)__readmsr(0xC0000082))[-K##x##_XFAST_SYSCALL]; \
  kernel_ptr = (uint8_t *)kernel_base;                                      \
  prison0 = (void **)&kernel_ptr[K##x##_PRISON_0];                          \
  rootvnode = (void **)&kernel_ptr[K##x##_ROOTVNODE];


#define caseentry(id, macro) \
  case id:                   \
    macro(id);               \
    break;

#define build_kpayload(id, macro) \
  switch (id) {                   \
    caseentry(400, macro);        \
    caseentry(401, macro);        \
    caseentry(405, macro);        \
    caseentry(406, macro);        \
    caseentry(407, macro);        \
    caseentry(450, macro);        \
    caseentry(455, macro);        \
    caseentry(470, macro);        \
    caseentry(471, macro);        \
    caseentry(472, macro);        \
    caseentry(473, macro);        \
    caseentry(474, macro);        \
    caseentry(500, macro);        \
    caseentry(501, macro);        \
    caseentry(503, macro);        \
    caseentry(505, macro);        \
    caseentry(507, macro);        \
    caseentry(550, macro);        \
    caseentry(553, macro);        \
    caseentry(555, macro);        \
    caseentry(556, macro);        \
    caseentry(600, macro);        \
    caseentry(602, macro);        \
    caseentry(620, macro);        \
    caseentry(650, macro);        \
    caseentry(651, macro);        \
    caseentry(670, macro);        \
    caseentry(671, macro);        \
    caseentry(672, macro);        \
    caseentry(700, macro);        \
    caseentry(701, macro);        \
    caseentry(702, macro);        \
    caseentry(750, macro);        \
    caseentry(751, macro);        \
    caseentry(755, macro);        \
    caseentry(800, macro);        \
  default:                        \
    __asm__("ret");               \
    /* raise(SIGSEGV); */         \
  }

uint16_t get_firmware();
int jailbreak_multi();


#endif
