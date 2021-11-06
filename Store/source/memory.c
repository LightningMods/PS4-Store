
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <dump_and_decrypt.h>
#include <errno.h>
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
#include <elf_common.h>
#include <elf64.h>

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

#define SCE_KERNEL_PROT_CPU_RW    0x02
#define SCE_KERNEL_MAP_FIXED      0x0010


#include <memory.h>

void* pHeap = NULL;
off_t phyAddr = 0;

uint64_t ssbrk=0;

void* lmalloc(unsigned long size)
{
//  log_info("zmalloc(%d) ", size);

  if(NULL==pHeap)
  {
    int32_t res = 0;
    if (0 != (res = sceKernelAllocateMainDirectMemory(MB(512), KB(64), 0, &phyAddr))) {
      log_info("sceKernelAllocateMainDirectMemory() Failed with 0x%08lX ", (uint64_t)res);
      return NULL;
    }

    if (0 != (res = sceKernelMapDirectMemory(&pHeap,MB(512),SCE_KERNEL_PROT_CPU_RW,0,phyAddr,KB(64)))) {
      log_info("sceKernelMapDirectMemory() Failed with 0x%08lX ", (uint64_t)res);
      return NULL;
    }

    log_info("@@@@@@@@@@@@@@@ HEAP INIT - %p ####################",pHeap);
  }

  void* res = (void*)((uint64_t)pHeap+ssbrk);

  ssbrk+=size;

 // log_info("lmalloc(%lu) @ %p :: total %lu", size, res, ssbrk);
 // log_info("res pointer %p", res);

  return res;
}


void lfree(void* ptr)
{
  //sceKernelMunmap(ptr, 1024*1024*4);  // who cares atm, fucking choke on it
}

size_t getsize(void * p) {
    size_t * in = p;
    if (in) { --in; return *in; }
    return -1;
}

void *lrealloc(void *ptr,size_t size) {
    int msize;

    if(ptr==NULL)
      return lmalloc(size);

    msize = getsize(ptr);
    log_info("msize=%d", msize);
    if (size <= msize)
        return ptr;
    void * newptr = lmalloc(size);
    memcpy(newptr, ptr, msize);
    lfree(ptr);
    return newptr;
}

