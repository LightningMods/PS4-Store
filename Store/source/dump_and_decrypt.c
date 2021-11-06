#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ps4sdk.h>
#include <stdlib.h>  // malloc, qsort, free
#include <unistd.h>  // close
#include <dump_and_decrypt.h>
#include <errno.h>
#include "log.h"
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
typedef struct {
    int index;
    uint64_t fileoff;
    size_t bufsz;
    size_t filesz;
    int enc;
} SegmentBufInfo;


#define TRUE 1
#define FALSE 0

bool is_self(const char* fn)
{
    struct stat st;
    bool res = false;
    int fd = sceKernelOpen(fn, O_RDONLY, 0);
    if (fd > 0) {
        stat(fn, &st);
        void* addr = mmap(0, 0x4000, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (addr != MAP_FAILED) {
            log_info("mmap %s : %p fd %i", fn, addr, fd);
            if (st.st_size >= 4)
            {
                uint32_t selfMagic = *(uint32_t*)((uint8_t*)addr + 0x00);
                if (selfMagic == SELF_MAGIC)
                {
                    uint16_t snum = *(uint16_t*)((uint8_t*)addr + 0x18);
                    if (st.st_size >= (0x20 + snum * 0x20 + 4))
                    {
                        uint32_t elfMagic = *(uint32_t*)((uint8_t*)addr + 0x20 + snum * 0x20);
                        if ((selfMagic == SELF_MAGIC) && (elfMagic == ELF_MAGIC))
                            res = true;
                    }
                }
            }
            munmap(addr, 0x4000);
        }
        else {
            log_error("mmap file %s err : %s", fn, strerror(errno));
            if(strstr(fn, "eboot.bin") != NULL)
                return false;
        }
        log_info("Close(fd): %x", sceKernelClose(fd));

    }
    else {
        log_error("open %s err : %x", fn, fd);
    }

    return res;
}

#define DECRYPT_SIZE 0x100000

bool write_decrypt_segment_to(int fdr, int fdw, uint64_t index, uint64_t offset, size_t size)
{
   
    uint64_t outSize = size;
    uint64_t realOffset = (index << 32) | offset;
    while (outSize > 0)
    {
        size_t bytes = (outSize > DECRYPT_SIZE) ? DECRYPT_SIZE : outSize;
        uint8_t* addr = (uint8_t*)mmap(0, bytes, PROT_READ, MAP_PRIVATE | 0x80000, fdr, realOffset);
        if (addr != MAP_FAILED)
        {
            write(fdw, addr, bytes);
            munmap(addr, bytes);
        }
        else
        {
            log_error("mmap segment [%lu] err(%d) : %s", index, errno, strerror(errno));
            return false;
        }
    
        outSize -= bytes;
        realOffset += bytes;
    }
    return true;
}

int is_segment_in_other_segment(Elf64_Phdr* phdr, int index, Elf64_Phdr* phdrs, int num) {
    for (int i = 0; i < num; i += 1) {
        Elf64_Phdr* p = &phdrs[i];
        if (i != index) {
            if (p->p_filesz > 0) {
                if ((phdr->p_offset >= p->p_offset) && ((phdr->p_offset + phdr->p_filesz) <= (p->p_offset + p->p_filesz))) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}


SegmentBufInfo* parse_phdr(Elf64_Phdr* phdrs, int num, int* segBufNum) {
    SegmentBufInfo* infos = (SegmentBufInfo*)malloc(sizeof(SegmentBufInfo) * num);
    int segindex = 0;
    for (int i = 0; i < num; i += 1) {
        Elf64_Phdr* phdr = &phdrs[i];
        

        if (phdr->p_filesz > 0) {
            if ((!is_segment_in_other_segment(phdr, i, phdrs, num)) || (phdr->p_type == 0x6fffff01)) {
                SegmentBufInfo* info = &infos[segindex];
                segindex += 1;
                info->index = i;
                info->bufsz = (phdr->p_filesz + (phdr->p_align - 1)) & (~(phdr->p_align - 1));
                info->filesz = phdr->p_filesz;
                info->fileoff = phdr->p_offset;
                info->enc = (phdr->p_type != 0x6fffff01) ? TRUE : FALSE;

            }
        }
    }
    *segBufNum = segindex;
    return infos;
}

bool do_dump(char* saveFile, int fd, SegmentBufInfo* segBufs, int segBufNum, Elf64_Ehdr* ehdr) {

    unlink(saveFile);

    int sf = sceKernelOpen(saveFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (sf > 0) {
        size_t elfsz = 0x40 + ehdr->e_phnum * sizeof(Elf64_Phdr);
        write(sf, ehdr, elfsz);

        for (int i = 0; i < segBufNum; i += 1) {
         

            if (segBufs[i].enc)
            {
               lseek(sf, segBufs[i].fileoff, SEEK_SET);
               if (!write_decrypt_segment_to(fd, sf, segBufs[i].index, 0, segBufs[i].filesz))
               {
                   log_error("write_decrypt_segment_to failed for %s", saveFile);
                   sceKernelClose(sf);
                   return true;
               }
            }
            else
            {

                if (segBufs[i].bufsz > MB(10))
                    log_error( "segBufs[i].bufsz > MB(10)");

                uint8_t *buf = (uint8_t*)malloc(segBufs[i].bufsz);
                memset(buf, 0, segBufs[i].bufsz);

                lseek(fd, -segBufs[i].filesz, SEEK_END);
                read(fd, buf, segBufs[i].filesz);
                lseek(sf, segBufs[i].fileoff, SEEK_SET);
                write(sf, buf, segBufs[i].filesz);

                free(buf);
            }
        }
        sceKernelClose(sf);
    }
    else 
       log_error("open %s err : %x", saveFile, sf);

    

   return false;
}

int decrypt_and_dump_self(char* selfFile, char* saveFile) {

    bool flag = false;

    int fd = sceKernelOpen(selfFile, O_RDONLY, 0);
    if (fd > 0) {
        void* addr = mmap(0, 0x4000, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (addr != MAP_FAILED) {
            log_info("mmap %s : %p, fd: %i", selfFile, addr, fd);

            uint16_t snum = *(uint16_t*)((uint8_t*)addr + 0x18);
            Elf64_Ehdr* ehdr = (Elf64_Ehdr*)((uint8_t*)addr + 0x20 + snum * 0x20);

            // shdr fix
            ehdr->e_shoff = ehdr->e_shentsize = ehdr->e_shnum = ehdr->e_shstrndx = 0;

            Elf64_Phdr* phdrs = (Elf64_Phdr*)((uint8_t*)ehdr + 0x40);


            int segBufNum = 0;
            SegmentBufInfo* segBufs = parse_phdr(phdrs, ehdr->e_phnum, &segBufNum);
            bool flag = do_dump(saveFile, fd, segBufs, segBufNum, ehdr);
            log_info("flag: %i", flag);
            free(segBufs);
            munmap(addr, 0x4000);

            sceKernelClose(fd);
            return flag;
        }
        else {
            log_fatal( "mmap file %s err : %s", selfFile, strerror(errno));
            sceKernelClose(fd);
            return true;
        }
        sceKernelClose(fd);
    }
    else {
        log_fatal( "open %s err : %s", selfFile, strerror(errno));
        return true;
    }
}
