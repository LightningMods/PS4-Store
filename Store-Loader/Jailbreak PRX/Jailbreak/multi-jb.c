#include "multi-jb.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>


uint16_t g_firmware = 0;

int kexec(void* func, void *user_arg) { return syscall(11, func, user_arg); }


static inline __attribute__((always_inline)) uint64_t __readmsr(unsigned long __register) {
  unsigned long __edx;
  unsigned long __eax;
  __asm__("rdmsr"
          : "=d"(__edx), "=a"(__eax)
          : "c"(__register));
  return (((uint64_t)__edx) << 32) | (uint64_t)__eax;
}



char* (*sceKernelGetFsSandboxRandomWord_1)() = NULL;

uint16_t get_firmware() {
  if (g_firmware) {
    return g_firmware;
  }
  uint16_t ret;            // Numerical representation of the firmware version. ex: 505 for 5.05, 702 for 7.02, etc
  uint32_t offset;         // Offset for ealier firmware's version location
  char binary_fw[2] = {0}; // 0x0000
  char string_fw[5] = {0}; // "0000\0"
  char sandbox_path[33];   // `/XXXXXXXXXX/common/lib/libc.sprx` [Char count of 32 + nullterm]
  int sys;

  
  sys = sceKernelLoadStartModule("libkernel_sys.sprx", 0, 0, 0, 0, 0);
  ret = sceKernelDlsym(sys, "sceKernelGetFsSandboxRandomWord", (void**)&sceKernelGetFsSandboxRandomWord_1);
  if (ret)
  {

     sys = sceKernelLoadStartModule("libkernel.sprx", 0, 0, 0, 0, 0);
     ret = sceKernelDlsym(sys, "sceKernelGetFsSandboxRandomWord", (void**)&sceKernelGetFsSandboxRandomWord_1);
     if (ret) return -2;
		
   }



  snprintf(sandbox_path, sizeof(sandbox_path), "/%s/common/lib/libc.sprx", sceKernelGetFsSandboxRandomWord_1());

  int fd = open(sandbox_path, O_RDONLY, 0);
  if (fd < 0) {
    // Assume it's currently jailbroken
    fd = open("/system/common/lib/libc.sprx", O_RDONLY, 0);
    if (fd < 0) {
      // It's really broken
      return -1;
    }
  }

  lseek(fd, 0x240, SEEK_SET); // 0x240 for 1.01 -> ?.??, 0x2B0 for ?.?? (5.05) -> ???
  read(fd, &offset, sizeof(offset));

  if (offset == 0x50E57464) { // "Påtd"
    lseek(fd, 0x334, SEEK_SET);
  } else {
    lseek(fd, 0x374, SEEK_SET);
  }

  read(fd, &binary_fw,  sizeof(binary_fw));
  close(fd);

  snprintf_s(string_fw, sizeof(string_fw), "%02x%02x", binary_fw[1], binary_fw[0]);

  ret = atoi(string_fw);

  g_firmware = ret;
  return ret;
}


int kpayload_dump(struct thread *td, struct kpayload_dump_args *args) {
  UNUSED(td);
  void *kernel_base;

  int (*copyout)(const void *kaddr, void *uaddr, size_t len);

  uint16_t fw_version = args->kpayload_dump_info->fw_version;

  // NOTE: This is a C preprocessor macro
  build_kpayload(fw_version, copyout_macro);

  uint64_t kaddr = args->kpayload_dump_info->kaddr;
  uint64_t uaddr = args->kpayload_dump_info->uaddr;
  size_t size = args->kpayload_dump_info->size;

  int ret = copyout((uint64_t *)kaddr, (uint64_t *)uaddr, size);

  if (ret == -1) {
    memset((uint64_t *)uaddr, 0, size);
  }

  return ret;
}


int kpayload_kbase(struct thread *td, struct kpayload_kbase_args *args) {
  UNUSED(td);
  void *kernel_base;

  int (*copyout)(const void *kaddr, void *uaddr, size_t len);

  uint16_t fw_version = args->kpayload_kbase_info->fw_version;

  // NOTE: This is a C preprocessor macro
  build_kpayload(fw_version, copyout_macro);

  uint64_t uaddr = args->kpayload_kbase_info->uaddr;
  copyout(&kernel_base, (uint64_t *)uaddr, 8);

  return 0;
}





uint64_t get_kernel_base() {
  uint64_t kernel_base;
  uint64_t *kernel_base_ptr = mmap(NULL, 8, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0); // Allocate a buffer in userland
  struct kpayload_kbase_info kpayload_kbase_info;
  kpayload_kbase_info.fw_version = get_firmware();
  kpayload_kbase_info.uaddr = (uint64_t)kernel_base_ptr;
  kexec(&kpayload_kbase, &kpayload_kbase_info);
  memcpy(&kernel_base, kernel_base_ptr, 8);
  munmap(kernel_base_ptr, 8);
  return kernel_base;
}

int get_memory_dump(uint64_t kaddr, uint64_t *uaddr, size_t size) {
  struct kpayload_dump_info kpayload_dump_info;
  kpayload_dump_info.fw_version = get_firmware();
  kpayload_dump_info.kaddr = kaddr;
  kpayload_dump_info.uaddr = (uint64_t)uaddr;
  kpayload_dump_info.size = size;
  kexec(&kpayload_dump, &kpayload_dump_info);
  return 0;
}

int kpayload_jailbreak(struct thread *td, struct kpayload_firmware_args *args) {
  struct filedesc *fd;
  struct ucred *cred;
  fd = td->td_proc->p_fd;
  cred = td->td_proc->p_ucred;

  void *kernel_base;
  uint8_t *kernel_ptr;
  void **prison0;
  void **rootvnode;

  uint16_t fw_version = args->kpayload_firmware_info->fw_version;

  // NOTE: This is a C preprocessor macro
  build_kpayload(fw_version, jailbreak_macro);

  cred->cr_uid = 0;
  cred->cr_ruid = 0;
  cred->cr_rgid = 0;
  cred->cr_groups[0] = 0;

  cred->cr_prison = *prison0;
  fd->fd_rdir = fd->fd_jdir = *rootvnode;

  void *td_ucred = *(void **)(((char *)td) + 304);

  uint64_t *sonyCred = (uint64_t *)(((char *)td_ucred) + 96);
  *sonyCred = 0xffffffffffffffff;

  uint64_t *sceProcessAuthorityId = (uint64_t *)(((char *)td_ucred) + 88);
  *sceProcessAuthorityId = 0x3801000000000013;

  uint64_t *sceProcCap = (uint64_t *)(((char *)td_ucred) + 104);
  *sceProcCap = 0xffffffffffffffff;

  return 0;
}



int jailbreak_multi() {
  struct kpayload_firmware_info kpayload_firmware_info;
  kpayload_firmware_info.fw_version = get_firmware();
  if(kexec(&kpayload_jailbreak, &kpayload_firmware_info) == 0);
       printf("^^^^^^^^^^^ Jailbroke! FW: %i\n", kpayload_firmware_info.fw_version);

  return 0;
}

