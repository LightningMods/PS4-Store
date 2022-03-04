#include "multi-jb.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

static uintptr_t prison0;
static uintptr_t rootvnode;


enum { EINVAL = 22 };

struct iovec
{
    const void* ptr;
    size_t size;
};


SYSCALL(12, static int chdir(char* path))
SYSCALL(22, static int unmount(const char* path, int flags))
SYSCALL(136, static int mkdir(const char* path, int mode))
SYSCALL(137, static int rmdir(const char* path))
SYSCALL(326, static int getcwd(char* buf, size_t sz))
SYSCALL(378, static int nmount(struct iovec* iov, unsigned int niov, int flags))


void jbc_run_as_root(void(*fn)(void* arg), void* arg, int cwd_mode)
{
    struct jbc_cred cred;
    jbc_get_cred(&cred);
    struct jbc_cred root_cred = cred;
    jbc_jailbreak_cred(&root_cred);
    switch (cwd_mode)
    {
    case CWD_KEEP:
    default:
        root_cred.cdir = cred.cdir;
        break;
    case CWD_ROOT:
        root_cred.cdir = cred.rdir;
        break;
    case CWD_RESET:
        root_cred.cdir = root_cred.rdir;
        break;
    }
    jbc_set_cred(&root_cred);
    fn(arg);
    jbc_set_cred(&cred);
}


static void do_mount_in_sandbox(void* op)
{
    struct mount_in_sandbox_param* p = op;
    char path[MAX_PATH + 1];
    path[MAX_PATH] = 0;
    int error;
    if ((error = getcwd(path, MAX_PATH)))
        goto err_out;
    size_t l1 = 0;
    while (path[l1])
        l1++;
    size_t l2 = 0;
    while (p->name[l2])
        l2++;
    if (l1 + l2 + 1 >= MAX_PATH)
        goto invalid;
    path[l1] = '/';
    int dots = 1;
    for (size_t i = 0; i <= l2; i++)
    {
        if (p->name[i] == '/')
            goto invalid;
        else if (p->name[i] != '.')
            dots = 0;
        path[l1 + 1 + i] = p->name[i];
    }
    if (dots && l2 <= 2)
        goto invalid;
    if (p->path) //mount operation
    {
        if ((error = mkdir(path, 0777)))
            goto err_out;
        size_t l3 = 0;
        while (p->path[l3])
            l3++;
        struct iovec data[6] = {
            {"fstype", 7}, {"nullfs", 7},
            {"fspath", 7}, {path, l1 + l2 + 2},
            {"target", 7}, {p->path, l3 + 1},
        };
        if ((error = nmount(data, 6, 0)))
        {
            rmdir(path);
            goto err_out;
        }
    }
    else //unmount operation
    {
        if ((error = unmount(path, 0)))
            goto err_out;
        if ((error = rmdir(path)))
            goto err_out;
    }
    return;
invalid:
    error = EINVAL;
err_out:
    p->ans = error;
}

int jbc_mount_in_sandbox(const char* system_path, const char* mnt_name)
{
    struct mount_in_sandbox_param op = {
        .path = system_path,
        .name = mnt_name,
        .ans = 0,
    };
    jbc_run_as_root(do_mount_in_sandbox, &op, CWD_ROOT);
    return op.ans;
}

int jbc_unmount_in_sandbox(const char* mnt_name)
{
    return jbc_mount_in_sandbox(0, mnt_name);
}

static int k_kcall(void* td, uint64_t** uap)
{
    uint64_t* args = uap[1];
    args[0] = ((uint64_t(*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t))args[0])(args[1], args[2], args[3], args[4], args[5], args[6]);
    return 0;
}

asm("kexec:\nmov $11, %rax\nmov %rcx, %r10\nsyscall\nret");
void kexec(void*, void*);

uint64_t jbc_krw_kcall(uint64_t fn, ...)
{
    va_list v;
    va_start(v, fn);
    uint64_t uap[7] = { fn };
    for (int i = 1; i <= 6; i++)
        uap[i] = va_arg(v, uint64_t);
    kexec(k_kcall, uap);
    return uap[0];
}

asm("k_get_td:\nmov %gs:0, %rax\nret");
extern char k_get_td[];

uintptr_t jbc_krw_get_td(void)
{
    return jbc_krw_kcall((uintptr_t)k_get_td);
}

static int have_mira = -1;
static int mira_socket[2];

static int do_check_mira(void)
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, mira_socket))
        return 0;
    if (write(mira_socket[1], (void*)jbc_krw_get_td(), 1) == 1)
    {
        char c;
        read(mira_socket[0], &c, 1);
        return 1;
    }
    return 0;
}

static inline bool check_mira(void)
{
    if (have_mira < 0)
        have_mira = do_check_mira();
    return (bool)have_mira;
}

static inline bool check_ptr(uintptr_t p, KmemKind kind)
{
    if (kind == USERSPACE)
        return p < 0x800000000000;
    else if (kind == KERNEL_HEAP)
        return p >= 0xffff800000000000 && p < 0xffffffff00000000;
    else if (kind == KERNEL_TEXT)
        return p >= 0xffffffff00000000 && p < 0xfffffffffffff000;
    else
        return false;
}

static int kcpy_mira(uintptr_t dst, uintptr_t src, size_t sz)
{
    while (sz > 0)
    {
        size_t chk = (sz > 64 ? 64 : sz);
        if (write(mira_socket[1], (void*)src, chk) != chk)
            return -1;
        if (read(mira_socket[0], (void*)dst, chk) != chk)
            return -1;
        dst += chk;
        src += chk;
        sz -= chk;
    }
    return 0;
}

asm("k_kcpy:\nmov %rdx, %rcx\nrep movsb\nret");
extern char k_kcpy[];

int jbc_krw_memcpy(uintptr_t dst, uintptr_t src, size_t sz, KmemKind kind)
{
    if (sz == 0)
        return 0;
    bool u1 = check_ptr(dst, USERSPACE) && check_ptr(dst + sz - 1, USERSPACE);
    bool ok1 = check_ptr(dst, kind) && check_ptr(dst + sz - 1, kind);
    bool u2 = check_ptr(src, USERSPACE) && check_ptr(src + sz - 1, USERSPACE);
    bool ok2 = check_ptr(src, kind) && check_ptr(src + sz - 1, kind);
    if (!((u1 || ok1) && (u2 || ok2)))
        return -1;
    if (u1 && u2)
        return -1;
    if (check_mira())
        return kcpy_mira(dst, src, sz);
    jbc_krw_kcall((uintptr_t)k_kcpy, dst, src, sz);
    return 0;
}

uint64_t jbc_krw_read64(uintptr_t p, KmemKind kind)
{
    uint64_t ans;
    if (jbc_krw_memcpy((uintptr_t)&ans, p, sizeof(ans), kind))
        return -1;
    return ans;
}

int jbc_krw_write64(uintptr_t p, KmemKind kind, uintptr_t val)
{
    return jbc_krw_memcpy(p, (uintptr_t)&val, sizeof(val), kind);
}


static int resolve(void)
{
restart:;
    uintptr_t td = jbc_krw_get_td();
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    for (;;)
    {
        int pid;
        if (jbc_krw_memcpy((uintptr_t)&pid, proc + 0xb0, sizeof(pid), KERNEL_HEAP))
            goto restart;
        if (pid == 1)
            break;
        uintptr_t proc2 = jbc_krw_read64(proc, KERNEL_HEAP);
        uintptr_t proc1 = jbc_krw_read64(proc2 + 8, KERNEL_HEAP);
        if (proc1 != proc)
            goto restart;
        proc = proc2;
    }
    uintptr_t pid1_ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    uintptr_t pid1_fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);
    if (jbc_krw_memcpy((uintptr_t)&prison0, pid1_ucred + 0x30, sizeof(prison0), KERNEL_HEAP))
        return -1;
    if (jbc_krw_memcpy((uintptr_t)&rootvnode, pid1_fd + 0x18, sizeof(rootvnode), KERNEL_HEAP))
    {
        prison0 = 0;
        return -1;
    }
    return 0;
}

uintptr_t jbc_get_prison0(void)
{
    if (!prison0)
        resolve();
    return prison0;
}

uintptr_t jbc_get_rootvnode(void)
{
    if (!rootvnode)
        resolve();
    return rootvnode;
}

static inline int ppcopyout(void* u1, void* u2, uintptr_t k)
{
    return jbc_krw_memcpy((uintptr_t)u1, k, (uintptr_t)u2 - (uintptr_t)u1, KERNEL_HEAP);
}

static inline int ppcopyin(const void* u1, const void* u2, uintptr_t k)
{
    return jbc_krw_memcpy(k, (uintptr_t)u1, (uintptr_t)u2 - (uintptr_t)u1, KERNEL_HEAP);
}
int jbc_get_cred(struct jbc_cred* ans)
{
    uintptr_t td = jbc_krw_get_td();
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    uintptr_t ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    uintptr_t fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);

    if (ppcopyout(&ans->uid, 1 + &ans->svuid, ucred + 4)
        || ppcopyout(&ans->rgid, 1 + &ans->svgid, ucred + 20)
        || ppcopyout(&ans->prison, 1 + &ans->prison, ucred + 0x30)
        || ppcopyout(&ans->cdir, 1 + &ans->jdir, fd + 0x10)
|| ppcopyout(&ans->rdir, 1 + &ans->jdir, fd + 0x10)
|| ppcopyout(&ans->jdir, 1 + &ans->jdir, fd + 0x10)
        || ppcopyout(&ans->sceProcType, 1 + &ans->sceProcType, ucred + 88)
        || ppcopyout(&ans->sonyCred, 1 + &ans->sonyCred, ucred + 88)
        || ppcopyout(&ans->sceProcCap, 1 + &ans->sceProcCap, ucred + 96))
        return -1;

    return 0;
}//

int jbc_set_cred(const struct jbc_cred* ans)
{
    uintptr_t td = jbc_krw_get_td();
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    uintptr_t ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    uintptr_t fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);


    if (ppcopyin(&ans->uid, 1 + &ans->svuid, ucred + 4)
        || ppcopyin(&ans->rgid, 1 + &ans->svgid, ucred + 20)
        || ppcopyin(&ans->prison, 1 + &ans->prison, ucred + 0x30)
        || ppcopyin(&ans->cdir, 1 + &ans->jdir, fd + 0x10)
        || ppcopyin(&ans->sceProcType, 1 + &ans->sceProcCap, ucred + 88))
        return -1;
    return 0;
}


int jbc_jailbreak_cred(struct jbc_cred* ans)
{
    uintptr_t prison0 = jbc_get_prison0();
    if (!prison0)
        return -1;

    uintptr_t rootvnode = jbc_get_rootvnode();
    if (!rootvnode)
        return -1;

    //without some modules wont load like Apputils
    ans->sceProcCap = 0xffffffffffffffff;
    ans->sceProcType = 0x3801000000000013;
    ans->sonyCred = 0xffffffffffffffff;

    ans->uid = ans->ruid = ans->svuid = ans->rgid = ans->svgid = 0;
    ans->prison = prison0;
    ans->cdir = ans->rdir = ans->jdir = rootvnode;
    return 1;
}

struct jbc_cred rejail;

int jailbreak_multi() {

  struct jbc_cred ans;
  jbc_get_cred(&rejail);
  jbc_get_cred(&ans);
  jbc_jailbreak_cred(&ans);
  return jbc_set_cred(&ans);
}

int rejail_multi() {
  return jbc_set_cred(&rejail);
}

