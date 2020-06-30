#define robust_list_head __robust_list_head
#include <errno.h>
#undef robust_list_head
#include <asm/unistd_64.h>
#include <linux/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <asm/prctl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/times.h>
#include <sys/capability.h>
#include <utime.h>
#include <ustat.h>
#include <sys/vfs.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/timex.h>
#include <asm/ldt.h>
#include <linux/aio_abi.h>
#include <time.h>
#include <sys/epoll.h>
#include <mqueue.h>
#include <linux/perf_event.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/futex.h>
#include <stdatomic.h>

#include <hash.h>
#include <htable.h>

#include <shim_table.h>
#include <syscall_shim.h>
#include <sgx.h>
#include <sgx_signal.h>
#include "enclave_mman.h"
#undef ENCLAVE_START
#define ENCLAVE_START enclave_params.base
#undef ENCLAVE_END
#define ENCLAVE_END (ENCLAVE_START + enclave_params.size)

#define IN_ENCLAVE(addr) ((unsigned long)addr >= ENCLAVE_START && \
        (unsigned long)addr < ENCLAVE_END)
#define MAX_ERRNO	4095
#define IS_ERR_VALUE(x) (x >= (unsigned long)-MAX_ERRNO)
#define BAD_ADDR_OR_NULL(x) (!x || IS_ERR_VALUE(x))

void WARN(const char *err_msg) {
        __shim_write(2, (long)err_msg, strlen(err_msg));
}
void HCF(const char *err_msg) {
    WARN(err_msg);
    while (1) __builtin_trap();
}


void NO_SYSCALL_FAIL(int nbr) {
    char buf[16];
    __itoa(buf, nbr);
    WARN(buf);
    HCF(" UNSUPPORTED SYSCALL\n");
}
long accept_page(unsigned long vaddr, unsigned long flags, PageType pt)
{

    long ret;
    SecInfo align64 secinfo = {0};
    SecInfo *secinfop = &secinfo;

    secinfo.flags = flags | (pt << SEC_PT_OFFSET);

    asm volatile (
            "movq %1, %%rbx\n\t"
            "movq %2, %%rcx\n\t"
            "movq %3, %%rax\n\t"
            "enclu\n\t"
            "movq %%rax, %0\n\t" : "=m"(ret)
                                 : "m"(secinfop), "m"(vaddr), "i"(EACCEPT)
                                 : "memory", "cc", "rax", "rbx", "rcx");

    /*
    secinfo.flags = sgx_flags;

    SETM(RBX, secinfop);
    SETM(RCX, vaddr);
    ENCLU(EMODPE);
    */
    return ret;
}

long accept_region(unsigned long addr, unsigned long size, unsigned long prot,
        PageType pt)
{
    long ret = 0;
    addr &= ~0xFFFUL;
    size = ROUND_UP(size);
    while (size > 0) {
        ret = accept_page(addr, prot, pt);
        if (ret) {
            break;
        }
        addr += PG_SIZE;
        size -= PG_SIZE;
    }
    return ret;
}

SYSCALL_SHIM_DEF_3(read, int, fd, void *, buf, size_t, size)
    ssize_t ret;
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    ret = PASS_THROUGH_SYSCALL(read, 3, fd, _buf, size);
    if (ret > 0)
        memcpy(buf, _buf, ret);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END

void __itoa(char *dest, size_t num) {
    int j, i = 0;
    if (num == 0) {
        dest[0] = '0';
        dest[1] = '\0';
        return;
    }
    while (num != 0) {
        dest[i] = num % 16;
        if (dest[i] > 9){
            dest[i] = dest[i] - 10 + 'a';
        } else {
            dest[i] = dest[i] + '0';
        }
        num /= 16;
        i++;
    }
    dest[i] = '\0';
    for (j = 0; j < i/2; j++) {
        dest[j] ^= dest[i - j - 1];
        dest[i - j - 1] ^= dest[j];
        dest[j] ^= dest[i - j - 1];
    }
}


SYSCALL_SHIM_DEF_3(write, int, fd, const char *, buf, size_t, size)
    long ret = -EBUFFUL;
    char *_buf = get_syscall_buffer(size);
    if (!_buf) {
        goto done;
    }
    memcpy(_buf, buf, size);
    ret = PASS_THROUGH_SYSCALL(write, 3, fd, _buf, size);
done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(open, const char *, filename, int, flags, int, mode)
    long ret, size = strlen(filename) + 1;
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, filename, size);
    ret = PASS_THROUGH_SYSCALL(open, 3, _buf, flags, mode);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(close, int, fd)
    return PASS_THROUGH_SYSCALL(close, 1, fd);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(stat, const char *, filename, struct stat *, statbuf)
    long ret, len = strlen(filename) + 1;
    long size = len + sizeof(struct stat);
    void *_buf_filename = get_syscall_buffer(size);
    if (!_buf_filename) {
        return -EBUFFUL;
    }
    void *_buf_stat = _buf_filename + len;

    memcpy(_buf_filename, filename, len);

    ret = PASS_THROUGH_SYSCALL(stat, 2, _buf_filename, _buf_stat);

    memcpy(statbuf, _buf_stat, sizeof(struct stat));

    free_syscall_buffer(_buf_filename);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(fstat, unsigned int, fd, struct stat *, statbuf)
    long ret, size = sizeof(struct stat);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }

    ret = PASS_THROUGH_SYSCALL(fstat, 2, fd, _buf);

    memcpy(statbuf, _buf, size);

    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(lstat, const char *, filename, struct stat *, statbuf)
    long ret, len = strlen(filename) + 1;
    long size = len + sizeof(struct stat);
    void *_buf_filename = get_syscall_buffer(size);
    if (!_buf_filename) {
        return -EBUFFUL;
    }
    void *_buf_stat = _buf_filename + len;

    memcpy(_buf_filename, filename, len);

    ret = PASS_THROUGH_SYSCALL(lstat, 2, _buf_filename, _buf_stat);

    memcpy(statbuf, _buf_stat, sizeof(struct stat));

    free_syscall_buffer(_buf_filename);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(poll, struct pollfd *, ufds, unsigned int, nfds,
        long, timeout_msecs)
    long ret, size = sizeof(struct pollfd) * nfds;
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }

    memcpy(_buf, ufds, size);

    ret = PASS_THROUGH_SYSCALL(poll, 3, _buf, nfds, timeout_msecs);

    memcpy(ufds, _buf, size);

    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(lseek, unsigned int, fd, off_t, offset,
        unsigned int, origin)
    return PASS_THROUGH_SYSCALL(lseek, 3, fd, offset, origin);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_6(mmap, unsigned long, addr, unsigned long, len,
        unsigned long, prot, unsigned long, flags, long, fd,
        unsigned long, off)
    bool need_file_backing = false;
    long initial_prot;
    if (flags & MAP_UNTRUST) {
        flags &= ~MAP_UNTRUST;
        return PASS_THROUGH_SYSCALL(mmap, 6, addr, len, prot, flags, fd, off);
    }

    if (off & 0xfff)
        return -EINVAL;
    if (flags & MAP_FIXED && addr < enclave_params.mmap_line) {
        /* A FIXED mapping requested outside of memory we don't control */
        return PASS_THROUGH_SYSCALL(mmap, 6, addr, len, prot, flags, fd, off);
    }

    sgx_lock(&mm_lock);

    if (flags & MAP_ANONYMOUS || fd < 0) {
        need_file_backing = false;
        initial_prot = prot;
    } else {
        need_file_backing = true;
        initial_prot = PROT_READ|PROT_WRITE;
    }
    if (flags & MAP_SHARED) {
        /* We can't do shared mappings */
        flags &= ~MAP_SHARED;
    }



    /* we map everything read/write at first because we may have to
       fake a file backed mapping */
    if (!addr || !(flags & MAP_FIXED)) {
        addr = allocate_region(ROUND_UP(len), initial_prot, false);
    } else {
        addr = allocate_region_at(addr, ROUND_UP(len), initial_prot);
    }

    if (IS_ERR_VALUE(addr)) {
        goto done;
    }

    if (need_file_backing) {
        void *tmp_mapping;
        long ret;
        ret = PASS_THROUGH_SYSCALL(mmap, 6, NULL, len, PROT_READ,
                MAP_PRIVATE, fd, off);
        if (ret <= 0) {
            DIE("Outside mmap failed\n");
        }
        tmp_mapping = (void *)ret;
        /* When mapping in files ldso maps enough memory for the entire so
           (holes and all) we get this weird bus error when we try to sream
           over the edge of the actual file so we have to check where that is
           here */
        off_t f_len;
        {/* live range of this buffer */
           struct stat *_stat = get_syscall_buffer(sizeof(struct stat));
           ret = PASS_THROUGH_SYSCALL(fstat, 2, fd, _stat);
           f_len = _stat->st_size - off;
           free_syscall_buffer(_stat);
        }
        if (ret) {
           DIE("fstat failed");
        }

        if (f_len)
            memcpy((void *)addr, tmp_mapping, (len > f_len ? f_len : len));

        /* why not clean up? */
        ret = PASS_THROUGH_SYSCALL(munmap, 2, tmp_mapping, len);
        if (ret) {
            /* actually it doesn't really matter if this fails */
        }

        /* and fix up prot */
        ret = update_prot(addr, ROUND_UP(len), prot);
        if (ret) {
            DIE("update prot failed somehow");
        }
    }
done:
    sgx_unlock(&mm_lock);
    return addr;
SYSCALL_SHIM_END

void *mprotect_region_start;
size_t mprotect_region_len;
SYSCALL_SHIM_DEF_3(mprotect, unsigned long, start, size_t, len,
        unsigned long, prot)
    unsigned long ret;
    if (mprotect_region_start) {
        /* TODO COST OF EMODPE, one for each page */
       if ((void *)start > mprotect_region_start &&
           (void *)start < mprotect_region_start + mprotect_region_len) {
          size_t pages = len / PAGE_SIZE;
          return PASS_THROUGH_SYSCALL(mprotect, 3, start, len, prot);
       }
    }
    sgx_lock(&mm_lock);
    ret = update_prot(ROUND_DOWN(start), ROUND_UP(len), prot);
    sgx_unlock(&mm_lock);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(munmap, unsigned long, addr, size_t, length)
    unsigned long ret;
    sgx_lock(&mm_lock);
    ret = free_region(ROUND_DOWN(addr), ROUND_UP(length));
    sgx_unlock(&mm_lock);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_1(brk, unsigned long, brk)
    static unsigned long current_brk;
    unsigned long ret, oldbrk, newbrk;
    if (!in_sgx) {
        return PASS_THROUGH_SYSCALL(brk, 1, brk);
    }

    if (!current_brk) {
        current_brk = enclave_params.brk;
    }
    if (brk <= current_brk)
        return current_brk;

    newbrk = ROUND_UP(brk);
    oldbrk = ROUND_UP(current_brk);

    if (oldbrk == newbrk) {
        return current_brk = brk;
    }

    if (!IN_ENCLAVE(brk)) {
        DIE("requested brk outside of enclave");
    }

    ret = PASS_THROUGH_SYSCALL(enclave_aug, 5, oldbrk, newbrk - oldbrk,
            PROT_READ|PROT_WRITE, -1, 0);
    if (BAD_ADDR_OR_NULL(ret)) {
        return current_brk;
    }
    accept_region(oldbrk, newbrk - oldbrk, PROT_READ|PROT_WRITE|SGX_PENDING,
            PT_REG);

    return current_brk = brk;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(rt_sigaction, int, sig, const struct kernel_sigaction *, act,
        struct kernel_sigaction *, oact, size_t, sigsize)
    if (!in_sgx) {
        return PASS_THROUGH_SYSCALL(rt_sigaction, 4, sig, act, oact, sigsize);
    }
    return do_sigaction(sig, act, oact, sigsize);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(rt_sigprocmask, int, how, const sigset_t *, set,
        sigset_t *, oset, size_t, sigsetsize)
    if (!in_sgx)
        return PASS_THROUGH_SYSCALL(rt_sigprocmask, 4, how, set, oset,
                    sigsetsize);
    long ret = -EBUFFUL;
    sigset_t *_set = NULL;
    sigset_t *_oset = NULL;
    if (set) {
        _set = get_syscall_buffer(sigsetsize);
       if (!_set) goto done;
        memcpy(_set, set, sigsetsize);
    }
    if (oset) {
        _oset = get_syscall_buffer(sigsetsize);
       if (!_oset) goto done;
    }

    ret = PASS_THROUGH_SYSCALL(rt_sigprocmask, 4, how, _set, _oset, sigsetsize);
    if (oset) {
        memcpy(oset, _oset, sigsetsize);
    }

 done:
    free_syscall_buffer(_set);
    free_syscall_buffer(_oset);
    return ret;
SYSCALL_SHIM_END


extern long do_sigreturn(void);
SYSCALL_SHIM_DEF_1(rt_sigreturn, unsigned long, __unused)
    /* FIX UP THE STACK */
    if (!in_sgx)
        return PASS_THROUGH_SYSCALL(rt_sigreturn, 1, __unused);
    return do_sigreturn();
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(ioctl, unsigned int, fd, unsigned int, cmd, char *, arg)
    long ret;
    unsigned long size = _IOC_SIZE(cmd);
    char *_buf = GET_BUFFER_IF(size, arg);
    if (!_buf && arg) return -EBUFFUL;
    if (_buf) memcpy(_buf, arg, size);
    ret = PASS_THROUGH_SYSCALL(ioctl, 3, fd, cmd, _buf);
    if (_buf) memcpy(arg, _buf, size);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(pread64, int, fd, void *, buf, size_t, count, off_t, offset)
    ssize_t ret = -EBUFFUL;
    void *_buf;
    _buf = GET_BUFFER_IF(count, count);
    if (!_buf && count && buf) {
        goto done;
    }
    ret = PASS_THROUGH_SYSCALL(pread64, 4, fd, _buf, count, offset);
    if (ret > 0) memcpy(buf, _buf, count);
done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(pwrite64, int, fd, const void *, buf, size_t, count,
        off_t, offset)
    long ret;
    void *_buf = get_syscall_buffer(count);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, buf, count);
    ret = PASS_THROUGH_SYSCALL(pwrite64, 4, fd, _buf, count, offset);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(readv, int, fd, struct iovec *, iov, int, iovcnt)
    long ret;
    struct iovec *_iov = alloc_iovec(iov, iovcnt);
    if (!_iov) {
        return -EBUFFUL;
    }
    ret = PASS_THROUGH_SYSCALL(readv, 3, fd, _iov, iovcnt);
    cpy_iovec(iov, _iov, iovcnt);
    free_iovec(_iov, iovcnt);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(writev, int, fd, struct iovec *, iov, int, iovcnt)
    long ret;
    struct iovec *_iov = alloc_iovec(iov, iovcnt);
    if (!_iov) {
        return -EBUFFUL;
    }
    cpy_iovec(_iov, iov, iovcnt);
    ret = PASS_THROUGH_SYSCALL(writev, 3, fd, _iov, iovcnt);
    free_iovec(_iov, iovcnt);
    return ret;
SYSCALL_SHIM_END



SYSCALL_SHIM_DEF_2(access, const char *, filename, int, mode)
    long ret, size = strlen(filename) + 1;
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, filename, size);
    ret = PASS_THROUGH_SYSCALL(access, 2, _buf, mode);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(pipe, int *, filedes)
    long ret, size = 2 * sizeof(int);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, filedes, size);
    ret = PASS_THROUGH_SYSCALL(pipe, 1, filedes);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END

/** fd Set size calculation from kernel code**/
#define FDS_BITPERLONG  (8*sizeof(long))
#define FDS_LONGS(nr)   (((nr)+FDS_BITPERLONG-1)/FDS_BITPERLONG)
#define FDS_BYTES(nr)   (FDS_LONGS(nr)*sizeof(long))
SYSCALL_SHIM_DEF_5(select, int, nfds, fd_set *, readfds, fd_set *, writefds,
        fd_set *, exceptfds, struct timeval *, timeout)
    long ret = -EBUFFUL;
    unsigned long size = FDS_BYTES(nfds);
    void *_readfds = GET_BUFFER_IF(size, readfds);
    void *_writefds = GET_BUFFER_IF(size, writefds);
    void *_exceptfds = GET_BUFFER_IF(size, exceptfds);
    void *_timeout = GET_BUFFER_IF(sizeof(struct timeval), timeout);

    if (readfds) {
        if (!_readfds) goto done;
        memcpy(_readfds, readfds, size);
    }
    if (writefds) {
        if (!_writefds) goto done;
        memcpy(_writefds, writefds, size);
    }
    if (exceptfds) {
        if (!_exceptfds) goto done;
        memcpy(_exceptfds, exceptfds, size);
    }
    if (timeout) {
        if (!_timeout) goto done;
        memcpy(_timeout, timeout, sizeof(struct timeval));
    }


    ret = PASS_THROUGH_SYSCALL(select, 5, nfds, _readfds, _writefds,
            _exceptfds, _timeout);

    if (readfds) memcpy(readfds, _readfds, size);
    if (writefds) memcpy(writefds, _writefds, size);
    if (exceptfds) memcpy(exceptfds, _exceptfds, size);
    if (timeout) memcpy(timeout, _timeout, sizeof(struct timeval));

 done:
    free_syscall_buffer(_readfds);
    free_syscall_buffer(_writefds);
    free_syscall_buffer(_exceptfds);
    free_syscall_buffer(_timeout);
    return ret;
SYSCALL_SHIM_END
#undef FDS_BITPERLONG
#undef FDS_LONGS
#undef FDS_BYTES


SYSCALL_SHIM_DEF_0(sched_yield)
    return PASS_THROUGH_SYSCALL(sched_yield, 0);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_5(mremap, int, dont, int, care, int, about, int, these, int, args)
    return -ENOSYS;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(mincore, void *, addr, size_t, length,
        unsigned char *, vec)
    long ret;
    unsigned long size = (length + PG_SIZE - 1) / PG_SIZE;
    void *buf = get_syscall_buffer(size);
    if (!buf) {
        return -EBUFFUL;
    }

    ret = PASS_THROUGH_SYSCALL(mincore, 3, addr, length, buf);

    memcpy(vec, buf, size);
    free_syscall_buffer(buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(madvise, void *, addr, size_t, length, int, advice)
    return 0; /* Doesn't affect application behavior? DON'T CARE */
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(shmget, key_t, key, size_t, size, int, shmflg)
    return PASS_THROUGH_SYSCALL(shmget, 3, key, size, shmflg);
SYSCALL_SHIM_END


#if 0
SYSCALL_SHIM_DEF_3(shmat, int, shmid, const void *, shmaddr, int, shmflg)
    if (!shmaddr || !enclave_overlap((unsigned long)shmaddr, 1)) {
        /* request is for normal memory let the OS handle it */
        return PASS_THROUGH_SYSCALL(msync, 3, shmid, shmaddr, shmflg);
    }
    return -EINVAL;
SYSCALL_SHIM_END
#endif


SYSCALL_SHIM_DEF_3(shmctl, int, shmid, int, cmd, struct shmid_ds *, buf)
    long ret;
    unsigned long size = sizeof(struct shmid_ds);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }

    memcpy(_buf, buf, size);

    ret = PASS_THROUGH_SYSCALL(shmctl, 3, shmid, cmd, _buf);

    memcpy(buf, _buf, size); /* XXX Needed? */
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(dup, int, oldfd)
    return PASS_THROUGH_SYSCALL(dup, 1, oldfd);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(dup2, int, oldfd, int, newfd)
    return PASS_THROUGH_SYSCALL(dup2, 2, oldfd, newfd);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(pause)
    return PASS_THROUGH_SYSCALL(pause, 0);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(nanosleep, const struct timespec *, req,
        struct timespec *, rem)
    long ret;
    unsigned long size = sizeof(struct timespec);
    void *_req = get_syscall_buffer(size);
    void *_rem = rem ? get_syscall_buffer(size) : NULL;
    if (!_req) {
        return -EBUFFUL;
    }
    if (!_rem && rem) {
        free_syscall_buffer(_req);
        return -EBUFFUL;
    }

    memcpy(_req, req, size);
    if (_rem) memcpy(_rem, rem, size);
    ret = PASS_THROUGH_SYSCALL(nanosleep, 2, _req, _rem);
    if (_rem) memcpy(rem, _rem, size);

    free_syscall_buffer(_req);
    free_syscall_buffer(_rem);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(getitimer, int, which, struct itimerval *, curr_value)
    long ret;
    unsigned long size = sizeof(struct itimerval);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }

    memcpy(_buf, curr_value, size);
    ret = PASS_THROUGH_SYSCALL(getitimer, 2, which, _buf);
    memcpy(curr_value, _buf, size);

    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_1(alarm, unsigned long, seconds)
    return PASS_THROUGH_SYSCALL(alarm, 1, seconds);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(setitimer, int, which, const struct itimerval *, new_value,
        struct itimerval *, old_value)
    long ret;
    unsigned long size = sizeof(struct itimerval);
    void *_new = NULL, *_old = NULL;
    if (new_value) {
       _new = get_syscall_buffer(size);
       if (!_new)
           return -EBUFFUL;
       memcpy(_new, new_value, size);
    }
    if (old_value) {
       _old = get_syscall_buffer(size);
       if (!_old) {
           free_syscall_buffer(_new);
           return -EBUFFUL;
       }
    }

    ret = PASS_THROUGH_SYSCALL(setitimer, 3, which, _new, _old);
    if (old_value)
       memcpy(old_value, _old, size);

    free_syscall_buffer(_new);
    free_syscall_buffer(_old);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_0(getpid)
    return PASS_THROUGH_SYSCALL(getpid, 0);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_4(sendfile, int, out_fd, int, in_fd, off_t *, offset,
        size_t, count)
    long ret;
    off_t *buf = NULL;
    if (offset) {
        buf = get_syscall_buffer(sizeof(off_t));
        if (!buf) {
            return -EBUFFUL;
        }
        *buf = *offset;
    }
    ret = PASS_THROUGH_SYSCALL(sendfile, 4, out_fd, in_fd, buf, count);
    if (offset) {
        *offset = *buf;
        free_syscall_buffer(buf);
    }
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(socket, int, family, int, type, int, protocol)
    return PASS_THROUGH_SYSCALL(socket, 3, family, type, protocol);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(connect, int, sockfd, const struct sockaddr *, addr,
        socklen_t, addrlen)
    long ret;
    void *_buf = get_syscall_buffer(addrlen);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, addr, addrlen);
    ret = PASS_THROUGH_SYSCALL(connect, 3, sockfd, _buf, addrlen);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(accept, int, sockfd, struct sockaddr *, addr,
        socklen_t *, addrlen)
    long ret;
    void *_addr = NULL;
    socklen_t *_addrlen = NULL;

    if (addr) {
        _addr = get_syscall_buffer(*addrlen);
        if (!_addr) {
            return -EBUFFUL;
        }
        _addrlen = get_syscall_buffer(sizeof(socklen_t));
        if (!_addrlen) {
            free_syscall_buffer(_addr);
            return -EBUFFUL;
        }
        *_addrlen = *addrlen;
    }

    ret = PASS_THROUGH_SYSCALL(accept, 3, sockfd, _addr, _addrlen);
    if (addr) {
        memcpy(addr, _addr, *addrlen);
        *addrlen = *_addrlen;
        free_syscall_buffer(_addr);
        free_syscall_buffer(_addrlen);
    }
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_6(sendto, int, sockfd, const void *, buf, size_t, len,
        int, flags, const struct sockaddr *, dest_addr, socklen_t, addrlen)
    long ret;
    void *_buf = NULL;
    void *_dest_addr = NULL;
    if (buf){
        _buf = get_syscall_buffer(len);
        if (!_buf) {
            return -EBUFFUL;
        }
        memcpy(_buf, buf, len);
    }
    if (dest_addr){
        _dest_addr = get_syscall_buffer(addrlen);
        if (!_dest_addr) {
            free_syscall_buffer(_buf);
            return -EBUFFUL;
        }
        memcpy(_dest_addr, dest_addr, addrlen);
    }
    ret = PASS_THROUGH_SYSCALL(sendto, 6, sockfd, _buf, len, flags, _dest_addr,
            addrlen);
    free_syscall_buffer(_dest_addr);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_6(recvfrom, int, sockfd, void *, buf, size_t, len, int,
        flags, struct sockaddr *, src_addr, socklen_t *, addrlen)
    long ret = -EBUFFUL;
    void *_buf = NULL, *_src_addr = NULL;
    socklen_t *_addrlen = NULL;

    if (buf && len) {
        _buf = get_syscall_buffer(len);
        if (!_buf) goto done;
    }

    if (src_addr && addrlen && *addrlen) {
        _src_addr = get_syscall_buffer(*addrlen);
        if (!_src_addr) goto done;

        _addrlen = get_syscall_buffer(sizeof(socklen_t));
        if (!_addrlen) goto done;

        memcpy(_src_addr, _src_addr, *addrlen);
        *_addrlen = *addrlen;
    }

    ret = PASS_THROUGH_SYSCALL(recvfrom, 6, sockfd, _buf, len, flags,
            _src_addr, _addrlen);
    if (buf) {
        memcpy(buf, _buf, len);
    }
    if (src_addr) {
        memcpy(src_addr, _src_addr, *addrlen);
        *addrlen = *_addrlen;
    }
 done:
    free_syscall_buffer(_buf);
    free_syscall_buffer(_src_addr);
    free_syscall_buffer(_addrlen);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(sendmsg, int, sockfd, const struct msghdr *, msg,
        int, flags)
    int ret = -EBUFFUL;
    struct msghdr *_msg = alloc_msghdr(msg);
    if (!_msg) goto done;

    cpy_msghdr(_msg, msg);
    ret = PASS_THROUGH_SYSCALL(sendmsg, 3, sockfd, _msg, flags);

 done:
    free_msghdr(_msg);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(recvmsg, int, sockfd, struct msghdr *, msg, int, flags)
    int ret = -EBUFFUL;
    struct msghdr *_msg = alloc_msghdr(msg);
    if (!_msg) goto done;

    cpy_msghdr(_msg, msg);
    ret = PASS_THROUGH_SYSCALL(recvmsg, 3, sockfd, _msg, flags);
    cpy_msghdr(msg, _msg);

 done:
    free_msghdr(_msg);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(shutdown, int, sockfd, int, how)
    return PASS_THROUGH_SYSCALL(shutdown, 2, sockfd, how);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(bind, int, sockfd, const struct sockaddr *, addr,
        socklen_t, addrlen)
    long ret = -EBUFFUL;
    void *_buf = get_syscall_buffer(addrlen);
    if (!_buf) goto done;

    memcpy(_buf, addr, addrlen);
    ret = PASS_THROUGH_SYSCALL(bind, 3, sockfd, _buf, addrlen);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(listen, int, sockfd, int, backlog)
    return PASS_THROUGH_SYSCALL(listen, 2, sockfd, backlog);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(getsockname, int, sockfd, struct sockaddr *, addr,
        socklen_t *, addrlen)
    long ret = -EBUFFUL;
    void *_addr = NULL;
    socklen_t *_addrlen = NULL;

    if (addr && addrlen && *addrlen) {
        _addr = get_syscall_buffer(*addrlen);
        if (!_addr) goto done;
        _addrlen = get_syscall_buffer(sizeof(socklen_t));
        if (!_addrlen) goto done;
        *_addrlen = *addrlen;
    }

    ret = PASS_THROUGH_SYSCALL(getsockname, 3, sockfd, _addr, _addrlen);
    if (addr && addrlen) {
        memcpy(addr, _addr, *addrlen);
        *addrlen = *_addrlen;
    }
 done:
    free_syscall_buffer(_addr);
    free_syscall_buffer(_addrlen);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(getpeername, int, sockfd, struct sockaddr *, addr,
        socklen_t *, addrlen)
    long ret = -EBUFFUL;
    void *_addr = NULL;
    socklen_t *_addrlen = NULL;

    if (addr && addrlen && *addrlen) {
        _addr = get_syscall_buffer(*addrlen);
        if (!_addr) goto done;
        _addrlen = get_syscall_buffer(sizeof(socklen_t));
        if (!_addrlen) goto done;
        *_addrlen = *addrlen;
    }

    ret = PASS_THROUGH_SYSCALL(getpeername, 3, sockfd, _addr, _addrlen);
    if (addr && addrlen) {
        memcpy(addr, _addr, *addrlen);
        *addrlen = *_addrlen;
    }
 done:
    free_syscall_buffer(_addr);
    free_syscall_buffer(_addrlen);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(socketpair, int, domain, int, type, int, protocol,
        int *, sv)
    long ret = -EBUFFUL;
    void *_buf = get_syscall_buffer(2);
    if (!_buf) goto done;

    ret = PASS_THROUGH_SYSCALL(socketpair, 4, domain, type, protocol, _buf);
    memcpy(sv, _buf, sizeof(int) * 2);

 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(setsockopt, int, sockfd, int, level, int, optname,
        const void *, optval, socklen_t, optlen)
    long ret = -EBUFFUL;
    void *_buf = NULL;
    if (optval && optlen) {
        _buf = get_syscall_buffer(optlen);
        if (!_buf) goto done;
        memcpy(_buf, optval, optlen);
    }

    ret = PASS_THROUGH_SYSCALL(setsockopt, 5, sockfd, level, optname, _buf,
            optlen);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(getsockopt, int, sockfd, int, level, int, optname,
        void *, optval, socklen_t*, optlen)
    long ret = -EBUFFUL;
    void *_buf = NULL;
    socklen_t *_optlen = NULL;

    if (optval && optlen && *optlen) {
        _buf = get_syscall_buffer(*optlen);
        if (!_buf) goto done;
        _optlen = get_syscall_buffer(sizeof(socklen_t));
        if (!_optlen) goto done;
        *_optlen = *optlen;
    }

    ret = PASS_THROUGH_SYSCALL(getsockopt, 5, sockfd, level, optname, _buf,
            _optlen);

    if (optval) {
        memcpy(optval, _buf, *optlen);
        *optlen = *_optlen;
    }
 done:
    free_syscall_buffer(_buf);
    free_syscall_buffer(_optlen);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(clone, unsigned long, flags, void *, child_stack,
        int *, ptid, int *, ctid, unsigned long, tls)
    long ret;
    SGX_tcs *new_tcs;
    int *_ptid, *_ctid;
    if (!in_sgx)
        return PASS_THROUGH_SYSCALL(clone, 5, flags, child_stack, ptid, ctid,
                tls);
    _ptid = get_syscall_buffer(sizeof(int));
    _ctid = get_syscall_buffer(sizeof(int));
    if (!_ctid || !_ptid) {
        ret = -EBUFFUL;
        goto done;
    }
    /* these are just the flags used by libpthread, and I haven't considered
       other cases yet */
    if (!(CLONE_VM & flags))            DIE("Clone needs CLONE_VM");
    if (!(CLONE_FS & flags))            DIE("Clone needs CLONE_FS");
    if (!(CLONE_FILES & flags))         DIE("Clone needs CLONE_FILES");
    if (!(CLONE_SIGHAND & flags))       DIE("Clone needs CLONE_SIGHAND");
    if (!(CLONE_THREAD & flags))        DIE("Clone needs CLONE_THREAD");
    if (!(CLONE_SYSVSEM & flags))       DIE("Clone needs CLONE_SYSVSEM");
    if (!(CLONE_SETTLS & flags))        DIE("Clone needs CLONE_SETTLS");
    if (!(CLONE_PARENT_SETTID & flags)) DIE("Clone needs CLONE_PARENT_SETTID");
    if (!(CLONE_CHILD_CLEARTID & flags))DIE("Clone needs CLONE_CLILD_CLEARTID");

    new_tcs = alloc_new_thread((unsigned long)child_stack, tls);
    if (!new_tcs) {
        return -ENOMEM;
    }
    if (!IN_ENCLAVE(ptid)) {
        DIE("ptid not in enclave");
    }
    if (!IN_ENCLAVE(ctid)) {
        DIE("ctid not in enclave");
    }
    ret = do_clone_exit(flags & ~CLONE_SETTLS, _ptid, _ctid, new_tcs);
    if (!ret) {
        if (ptid) *ptid = *_ptid;
        if (ctid) *ctid = *_ctid;
    }
done:
    free_syscall_buffer(_ptid);
    free_syscall_buffer(_ctid);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_0(vfork)
   return -ENOSYS;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(execve, const char *, filename, char **const, argv,
                   char **const, envp)
    long result = -EBUFFUL;
    char *_filename = string_buffer_out(filename);
    char ** _argv = string_array_buffer_out(argv);
    char ** _envp = string_array_buffer_out(envp);

    if (!_filename || !_argv || !_envp)
        goto done;
    result = PASS_THROUGH_SYSCALL(execve, 3, _filename, _argv, _envp);

done:
    free_string_buffer(_filename);
    free_string_array_buffer(_argv);
    free_string_array_buffer(_envp);
    return result;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_1(exit, int, status)
    return PASS_THROUGH_SYSCALL(exit, 1, status);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_4(wait4, pid_t, pid, int *, status, int, options,
        struct rusage *, rusage)
    long ret = -EBUFFUL;
    int *_status = NULL;
    struct rusage *_rusage = NULL;

    if (status) {
        _status = get_syscall_buffer(sizeof(int));
        if (!_status) goto done;
    }
    if (rusage) {
        _rusage = get_syscall_buffer(sizeof(struct rusage));
        if (!_rusage) goto done;
    }
    ret = PASS_THROUGH_SYSCALL(wait4, 4, pid, _status, options, _rusage);
    if (status) {
        *status = *_status;
    }
    if (rusage) {
        *rusage = *_rusage;
    }
 done:
    free_syscall_buffer(_status);
    free_syscall_buffer(_rusage);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(kill, pid_t, pid, int, sig)
    return PASS_THROUGH_SYSCALL(kill, 2, pid, sig);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(uname, struct new_utsname *, name)
    long ret;
    unsigned long size = sizeof(struct new_utsname);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }

    ret = PASS_THROUGH_SYSCALL(uname, 1, _buf);
    memcpy(name, _buf, size);
    free_syscall_buffer(_buf);

    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(semget, key_t, key, int, nsems, int, semflg)
    return PASS_THROUGH_SYSCALL(semget, 3, key, nsems, semflg);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(semop, int, semid, struct sembuf *, sops, size_t, nsops)
    long ret = -EBUFFUL;
    void *_sops = get_syscall_buffer(sizeof(struct sembuf) * nsops);
    if (!_sops) goto done;
    memcpy(_sops, sops, sizeof(struct sembuf) * nsops);
    ret = PASS_THROUGH_SYSCALL(semop, 3, semid, _sops, nsops);
    memcpy(sops, _sops, sizeof(struct sembuf) * nsops); /* XXX necessary? */
 done:
    free_syscall_buffer(_sops);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(semctl, int, semid, int, semnum, int, cmd, void *, semun)
    long ret = -EBUFFUL;
    void *_buf = NULL;
    unsigned long size;
    switch (cmd) {
        case IPC_STAT:
        case IPC_SET:
            size = sizeof(struct semid_ds);
            break;
        case GETALL:
        case SETALL:
            size = sizeof(unsigned short) * 128; /*SEMMSL; */
            break;
        /*case IPC_INFO:
            size = sizeof(struct seminfo);
            break;
            */
        case SETVAL:
        default:
            size = 0;
    }
    if (size) {
        _buf = get_syscall_buffer(size);
        if (!_buf) goto done;
        memcpy(_buf, semun, size);
    }
    ret = PASS_THROUGH_SYSCALL(semctl, 4, semid, semnum, cmd, _buf ?: semun);
    if (size) {
        memcpy(semun, _buf, size);
    }
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(shmdt, void *, shmaddr)
    return PASS_THROUGH_SYSCALL(shmdt, 1, shmaddr);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(msgget, key_t, key, int, msgflg)
    return PASS_THROUGH_SYSCALL(msgget, 2, key, msgflg);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_4(msgsnd, int, msgid, const void *, msgp, size_t, msgsz,
        int, msgflg)
    long ret = -EBUFFUL;
    unsigned long size = msgsz + sizeof(long);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) goto done;
    memcpy(_buf, msgp, size);
    ret = PASS_THROUGH_SYSCALL(msgsnd, 4, msgid, _buf, msgsz, msgflg);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(msgrcv, int, msgid, void *, msgp, size_t, msgsz,
        long, msgtyp, int, msgflg)
    long ret = -EBUFFUL;
    unsigned long size = msgsz + sizeof(long);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) goto done;
    ret = PASS_THROUGH_SYSCALL(msgrcv, 5, msgid, _buf, msgsz, msgtyp, msgflg);
    memcpy(msgp, _buf, size);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(msgctl, int, msgid, int, cmd, struct msqid_ds *, buf)
    long ret = -EBUFFUL;
    unsigned long size = sizeof(struct msqid_ds);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) goto done;
    memcpy(_buf, buf, size);
    ret = PASS_THROUGH_SYSCALL(msgctl, 3, msgid, cmd, _buf);
    memcpy(buf, _buf, size);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(fcntl, int, fd, int, cmd, void *, arg)
    long ret = -EBUFFUL;
    unsigned long size;
    void *_buf = NULL;
    switch (cmd) {
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        //case F_OFD_SETLK:
        //case F_OFD_SETLKW:
        //case F_OFD_GETLK:
            size = sizeof(struct flock);
            break;
        //case F_GETOWN_EX:
        //case F_SETOWN_EX:
        //    size = sizeof(struct f_owner_ex);
        //    break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETFL:
        case F_GETFD:
        case F_GETFL:
        case F_GETOWN:
        case F_SETOWN:
        //case F_GETSIG:
        //case F_SETSIG:
        //case F_SETLEASE:
        //case F_GETLEASE:
        //case F_NOTIFY:
        //case F_SETPIPE_SZ:
        //case F_GETPIPE_SZ:
        default:
            size = 0;
    }
    if (size) {
        _buf = get_syscall_buffer(size);
        if (!_buf) goto done;
    }
    ret = PASS_THROUGH_SYSCALL(fcntl, 3, fd, cmd, _buf ?: arg);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(flock, int, fd, int, operation)
    return PASS_THROUGH_SYSCALL(flock, 2, fd, operation);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(fsync, int, fd)
    return PASS_THROUGH_SYSCALL(fsync, 1, fd);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(fdatasync, int, fd)
    return PASS_THROUGH_SYSCALL(fdatasync, 1, fd);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(truncate, const char *, pathname, off_t, length)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(truncate, 2, _pathname, length);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(ftruncate, int, fd, off_t, length)
    return PASS_THROUGH_SYSCALL(ftruncate, 2, fd, length);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(getdents, int, fd, void *, dirp, unsigned int, count)
    long ret = -EBUFFUL;
    size_t size = count;
    char *_buf = get_syscall_buffer(size);
    if (!_buf) goto done;


    ret = PASS_THROUGH_SYSCALL(getdents, 3, fd, _buf, count);
    memcpy(dirp, _buf, size);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(getcwd, char *, buf, size_t, size)
    long ret = -EBUFFUL;
    char *_buf = get_syscall_buffer(size);
    if (!_buf) goto done;

    ret = PASS_THROUGH_SYSCALL(getcwd, 2, _buf, size);
    memcpy(buf, _buf, size);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(chdir, const char *, pathname)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(chdir, 1, _pathname);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(fchdir, int, fd)
    return PASS_THROUGH_SYSCALL(fchdir, 1, fd);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(rename, const char *, oldpath, const char *, newpath)
    long ret = -EBUFFUL;
    unsigned long oldlen = strlen(oldpath) + 1;
    unsigned long newlen = strlen(newpath) + 1;
    char *_oldpath = get_syscall_buffer(oldlen);
    char *_newpath = get_syscall_buffer(newlen);
    if (!_oldpath) goto done;
    if (!_newpath) goto done;

    memcpy(_oldpath, oldpath, oldlen);
    memcpy(_newpath, newpath, newlen);

    ret = PASS_THROUGH_SYSCALL(rename, 2, _oldpath, _newpath);
 done:
    free_syscall_buffer(_oldpath);
    free_syscall_buffer(_newpath);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(mkdir, const char *, pathname, mode_t, mode)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(mkdir, 2, _pathname, mode);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(rmdir, const char *, pathname)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(rmdir, 1, _pathname);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(creat, const char *, pathname, mode_t, mode)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(creat, 2, _pathname, mode);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(link, const char *, oldpath, const char *, newpath)
    long ret = -EBUFFUL;
    unsigned long oldlen = strlen(oldpath) + 1;
    unsigned long newlen = strlen(newpath) + 1;
    char *_oldpath = get_syscall_buffer(oldlen);
    char *_newpath = get_syscall_buffer(newlen);
    if (!_oldpath) goto done;
    if (!_newpath) goto done;

    memcpy(_oldpath, oldpath, oldlen);
    memcpy(_newpath, newpath, newlen);

    ret = PASS_THROUGH_SYSCALL(link, 2, _oldpath, _newpath);
 done:
    free_syscall_buffer(_oldpath);
    free_syscall_buffer(_newpath);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(unlink, const char *, pathname)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(unlink, 1, _pathname);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(symlink, const char *, target, const char *, linkpath)
    long ret = -EBUFFUL;
    unsigned long tarlen = strlen(target) + 1;
    unsigned long linklen = strlen(linkpath) + 1;
    char *_target = get_syscall_buffer(tarlen);
    char *_linkpath = get_syscall_buffer(linklen);
    if (!_target) goto done;
    if (!_linkpath) goto done;

    memcpy(_target, target, tarlen);
    memcpy(_linkpath, linkpath, linklen);

    ret = PASS_THROUGH_SYSCALL(symlink, 2, _target, _linkpath);
 done:
    free_syscall_buffer(_target);
    free_syscall_buffer(_linkpath);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(readlink, const char *, path, char *, buf, int, bufsize)
    long ret;
    unsigned long len = strlen(path) + 1;
    char *path_buf = get_syscall_buffer(len);
    if (!path_buf) {
        return -EBUFFUL;
    }
    char *_buf = get_syscall_buffer(bufsize);
    if (!_buf) {
        free_syscall_buffer(path_buf);
        return -EBUFFUL;
    }

    memcpy(path_buf, path, len);
    ret = PASS_THROUGH_SYSCALL(readlink, 3, path_buf, _buf, bufsize);
    memcpy(buf, _buf, bufsize);

    free_syscall_buffer(path_buf);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(chmod, const char *, pathname, mode_t, mode)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(chmod, 2, _pathname, mode);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(fchmod, int, fd, mode_t, mode)
    return PASS_THROUGH_SYSCALL(fchmod, 2, fd, mode);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(chown, const char *, pathname, uid_t, owner, gid_t, group)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(chown, 3, _pathname, owner, group);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(fchown, int, fd, uid_t, owner, gid_t, group)
    return PASS_THROUGH_SYSCALL(fchown, 3, fd, owner, group);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(lchown, const char *, pathname, uid_t, owner, gid_t, group)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(lchown, 3, _pathname, owner, group);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(umask, mode_t, mask)
    return PASS_THROUGH_SYSCALL(umask, 1, mask);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(gettimeofday, struct timeval *, tv, struct timezone *, tz)
    long ret = -EBUFFUL;
    struct timeval *_tv = GET_BUFFER_IF(sizeof(struct timeval), tv);
    struct timezone *_tz = GET_BUFFER_IF(sizeof(struct timezone), tz);
    if (!_tv && tv) goto done;
    if (!_tz && tz) goto done;

    ret = PASS_THROUGH_SYSCALL(gettimeofday, 2, _tv, _tz);
    if (tv) *tv = *_tv;
    if (tz) *tz = *_tz;
 done:
    free_syscall_buffer(_tv);
    free_syscall_buffer(_tz);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(getrlimit, int, resource, struct rlimit *, rlim)
    long ret = -EBUFFUL;
    struct rlimit *_rlim = get_syscall_buffer(sizeof(struct rlimit));
    if (!_rlim) goto done;

    ret = PASS_THROUGH_SYSCALL(getrlimit, 2, resource, _rlim);
    *rlim = *_rlim;

 done:
    free_syscall_buffer(_rlim);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(getrusage, int, who, struct rusage *, usage)
    long ret = -EBUFFUL;
    struct rusage *_usage = get_syscall_buffer(sizeof(struct rusage));
    if (!_usage) goto done;

    ret = PASS_THROUGH_SYSCALL(getrusage, 2, who, _usage);
    *usage = *_usage;

 done:
    free_syscall_buffer(_usage);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(sysinfo, struct sysinfo *, info)
    long ret = -EBUFFUL;
    struct sysinfo *_info = get_syscall_buffer(sizeof(struct sysinfo));
    if (!_info) goto done;

    ret = PASS_THROUGH_SYSCALL(sysinfo, 1, _info);
    *info = *_info;

 done:
    free_syscall_buffer(_info);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(times, struct tms *, buf)
    long ret = -EBUFFUL;
    struct tms *_buf = get_syscall_buffer(sizeof(struct tms));
    if (!_buf) goto done;

    ret = PASS_THROUGH_SYSCALL(times, 1, _buf);
    *buf = *_buf;

 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(getuid)
    return PASS_THROUGH_SYSCALL(getuid, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(syslog, int, type, char *, bufp, int, len)
    long ret;
    void *_buf = get_syscall_buffer(len);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, bufp, len);
    ret = PASS_THROUGH_SYSCALL(syslog, 3, type, _buf, len);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(getgid)
    return PASS_THROUGH_SYSCALL(getgid, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(setuid, uid_t, uid)
    return PASS_THROUGH_SYSCALL(setuid, 1, uid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(setgid, gid_t, gid)
    return PASS_THROUGH_SYSCALL(setgid, 1, gid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(geteuid)
    return PASS_THROUGH_SYSCALL(geteuid, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(getegid)
    return PASS_THROUGH_SYSCALL(getegid, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(setpgid, pid_t, pid, pid_t, pgid)
    return PASS_THROUGH_SYSCALL(getpgid, 2, pid, pgid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(getppid)
    return PASS_THROUGH_SYSCALL(getppid, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(getpgrp)
    return PASS_THROUGH_SYSCALL(getpgrp, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(setsid)
    return PASS_THROUGH_SYSCALL(setsid, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(setreuid, uid_t, ruid, uid_t, euid)
    return PASS_THROUGH_SYSCALL(setreuid, 2, ruid, euid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(setregid, gid_t, rgid, gid_t, egid)
    return PASS_THROUGH_SYSCALL(setregid, 2, rgid, egid);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(getgroups, int, size, gid_t *, list)
    long ret = -EBUFFUL;
    unsigned long len = sizeof(gid_t) * size;
    gid_t *_list = get_syscall_buffer(len);
    if (!_list) goto done;
    ret = PASS_THROUGH_SYSCALL(getgroups, 2, size, _list);
    memcpy(list, _list, len);
 done:
    free_syscall_buffer(_list);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(setgroups, size_t, size, const gid_t *, list)
    long ret = -EBUFFUL;
    unsigned long len = sizeof(gid_t) * size;
    gid_t *_list = get_syscall_buffer(len);
    if (!_list) goto done;
    memcpy(_list, list, len);
    ret = PASS_THROUGH_SYSCALL(setgroups, 2, size, _list);
 done:
    free_syscall_buffer(_list);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(setresuid, uid_t, ruid, uid_t, euid, uid_t, suid)
    return PASS_THROUGH_SYSCALL(setresuid, 3, ruid, euid, suid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(getresuid, uid_t *, ruid, uid_t *, euid, uid_t *, suid)
    long ret = -EBUFFUL;
    uid_t *_list = get_syscall_buffer(sizeof(uid_t) * 3);
    if (!_list) goto done;
    ret = PASS_THROUGH_SYSCALL(getresuid, 3, _list, _list+1, _list+2);
    *ruid = _list[0];
    *euid = _list[1];
    *suid = _list[2];
 done:
    free_syscall_buffer(_list);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(setresgid, uid_t, ruid, uid_t, euid, uid_t, suid)
    return PASS_THROUGH_SYSCALL(setresgid, 3, ruid, euid, suid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(getresgid, gid_t *, rgid, gid_t *, egid, gid_t *, sgid)
    long ret = -EBUFFUL;
    gid_t *_list = get_syscall_buffer(sizeof(gid_t) * 3);
    if (!_list) goto done;
    ret = PASS_THROUGH_SYSCALL(getresgid, 3, _list, _list+1, _list+2);
    *rgid = _list[0];
    *egid = _list[1];
    *sgid = _list[2];
 done:
    free_syscall_buffer(_list);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(getpgid, pid_t, pid)
    return PASS_THROUGH_SYSCALL(getpgid, 1, pid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(setfsuid, uid_t, fsuid)
    return PASS_THROUGH_SYSCALL(setfsuid, 1, fsuid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(setfsgid, uid_t, fsgid)
    return PASS_THROUGH_SYSCALL(setfsgid, 1, fsgid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(getsid, pid_t, pid)
    return PASS_THROUGH_SYSCALL(getsid, 1, pid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(capget, cap_user_header_t, hdrp, cap_user_data_t, datap)
    long ret = -EBUFFUL;
    unsigned long hdrsz, datasz;
    cap_user_header_t _hdrp = NULL;
    cap_user_data_t _datap = NULL;

    hdrsz = sizeof(struct __user_cap_header_struct);
    _hdrp = get_syscall_buffer(hdrsz);
    if (!_hdrp) goto done;
    datasz = sizeof(struct __user_cap_data_struct);
    _datap = get_syscall_buffer(datasz);
    if (!_datap) goto done;

    *_hdrp = *hdrp;
    ret = PASS_THROUGH_SYSCALL(capget, 2, _hdrp, _datap);
    *hdrp = *_hdrp;
    *datap = *_datap;
  done:
    free_syscall_buffer(_hdrp);
    free_syscall_buffer(_datap);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(capset, cap_user_header_t, hdrp,
        const cap_user_data_t, datap)
    long ret = -EBUFFUL;
    unsigned long hdrsz, datasz;
    cap_user_header_t _hdrp = NULL;
    cap_user_data_t _datap = NULL;

    hdrsz = sizeof(struct __user_cap_header_struct);
    _hdrp = get_syscall_buffer(hdrsz);
    if (!_hdrp) goto done;
    datasz = sizeof(struct __user_cap_data_struct);
    _datap = get_syscall_buffer(datasz);
    if (!_datap) goto done;

    *_hdrp = *hdrp;
    *_datap = *datap;
    ret = PASS_THROUGH_SYSCALL(capset, 2, _hdrp, _datap);
    *hdrp = *_hdrp;
  done:
    free_syscall_buffer(_hdrp);
    free_syscall_buffer(_datap);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(rt_sigpending, sigset_t *, set, size_t, sigsetsize)
    if (!in_sgx)
        return  PASS_THROUGH_SYSCALL(rt_sigpending, 2, set, sigsetsize);
    DIE("rt_sigpending not implemented");
    long ret = -EBUFFUL;
    void *_set = get_syscall_buffer(sigsetsize);
    if (!_set) goto done;
    ret = PASS_THROUGH_SYSCALL(rt_sigpending, 2, _set, sigsetsize);
    memcpy(set, _set, sigsetsize);
 done:
    free_syscall_buffer(_set);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(rt_sigtimedwait, sigset_t *, uthese,
        siginfo_t *, uinfo, struct timespec *, uts, size_t, sigsetsize)
    if (!in_sgx)
        return  PASS_THROUGH_SYSCALL(rt_sigtimedwait, 4, uthese, uinfo, uts,
                    sigsetsize);
    DIE("rt_sigtimedwait not implemented");
    long ret = -EBUFFUL;
    siginfo_t *_uinfo = NULL;
    struct timespec *_uts;
    void *_uthese;
    _uthese = GET_BUFFER_IF(sigsetsize, uthese);
    _uts = GET_BUFFER_IF(sizeof(struct timespec), uts);
    _uinfo = GET_BUFFER_IF(sizeof(siginfo_t), uinfo);
    if (!_uthese && uthese) goto done;
    if (!_uts && uts) goto done;
    if (!_uinfo && uinfo) goto done;

    if (uthese) memcpy(_uthese, uthese, sigsetsize);
    if (uts)    memcpy(_uts, uts, sizeof(struct timespec));
    if (uinfo)  memcpy(_uinfo, uinfo, sizeof(siginfo_t));
    ret = PASS_THROUGH_SYSCALL(rt_sigtimedwait, 4, _uthese, _uinfo, _uts,
            sigsetsize);
    if (uthese) memcpy(uthese, _uthese, sigsetsize);
    if (uts)    memcpy(uts, _uts, sizeof(struct timespec));
    if (uinfo)  memcpy(uinfo, _uinfo, sizeof(siginfo_t));
 done:
    free_syscall_buffer(_uinfo);
    free_syscall_buffer(_uts);
    free_syscall_buffer(_uthese);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(rt_sigqueueinfo, int, pid, int, sig, siginfo_t *, uinfo)
    if (!in_sgx)
        return PASS_THROUGH_SYSCALL(rt_sigqueueinfo, 3, pid, sig, uinfo);
    DIE("rt_sigtimedwait not implemented");
    long ret = -EBUFFUL;
    siginfo_t *_uinfo = get_syscall_buffer(sizeof(siginfo_t));
    if (!_uinfo) goto done;
    *_uinfo = *uinfo;
    ret = PASS_THROUGH_SYSCALL(rt_sigqueueinfo, 3, pid, sig, _uinfo);
    *uinfo = *_uinfo;
 done:
    free_syscall_buffer(_uinfo);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(rt_sigsuspend, const sigset_t *, mask, size_t, sigsetsize)
    if (!in_sgx)
        return PASS_THROUGH_SYSCALL(rt_sigsuspend, 2, mask, sigsetsize);
    DIE("rt_sigsuspend not implemented");
    long ret = -EBUFFUL;
    void *_mask = get_syscall_buffer(sigsetsize);
    if (!_mask) goto done;
    memcpy(_mask, _mask, sigsetsize);
    ret = PASS_THROUGH_SYSCALL(rt_sigsuspend, 2, _mask, sigsetsize);
 done:
    free_syscall_buffer(_mask);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(sigaltstack, const stack_t *, ss, stack_t *, oss)
    long ret = -EBUFFUL;
    stack_t *_ss = NULL;
    stack_t *_oss = NULL;
    if (in_sgx) {
        return do_sigaltstack(ss, oss);
    }

    if (ss) {
        _ss = get_syscall_buffer(sizeof(stack_t));
        if (!_ss) goto done;
        *_ss = *ss;
    }
    if (oss) {
        _oss = get_syscall_buffer(sizeof(stack_t));
        if (!_oss) goto done;
    }

    ret = PASS_THROUGH_SYSCALL(sigaltstack, 2, _ss, _oss);

    if (oss) {
        *oss = *_oss;
    }
 done:
    free_syscall_buffer(_ss);
    free_syscall_buffer(_oss);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(utime, const char*, filename, const struct utimbuf *, times)
    long ret = -EBUFFUL;
    char *_filename;
    struct utimbuf *_times;
    unsigned long len = strlen(filename) + 1;
    _filename = get_syscall_buffer(len);
    _times = GET_BUFFER_IF(sizeof(struct utimbuf), times);
    if (!_filename) goto done;
    if (!_times && times) goto done;

    if (times) *_times = *times;
    memcpy(_filename, filename, len);
    ret = PASS_THROUGH_SYSCALL(utime, 2, _filename, _times);

 done:
    free_syscall_buffer(_filename);
    free_syscall_buffer(_times);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(mknod, const char *, pathname, mode_t, mode, dev_t, dev)
    long ret = -EBUFFUL;
    char *_pathname;
    unsigned long len = strlen(pathname) + 1;
    _pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(mknod, 3, _pathname, mode, dev);

 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(uselib, const char *, library)
    long ret = -EBUFFUL;
    char *_library;
    unsigned long len = strlen(library) + 1;
    _library = get_syscall_buffer(len);
    if (!_library) goto done;

    memcpy(_library, library, len);

    ret = PASS_THROUGH_SYSCALL(uselib, 1, _library);

 done:
    free_syscall_buffer(_library);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(personality, int, persona)
    return PASS_THROUGH_SYSCALL(personality, 1, persona);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(ustat, dev_t, dev, struct ustat *, ubuf)
    long ret = -EBUFFUL;
    struct ustat *_ubuf;
    _ubuf = get_syscall_buffer(sizeof(struct ustat));
    if (!_ubuf) goto done;

    ret = PASS_THROUGH_SYSCALL(ustat, 2, dev, _ubuf);
    *ubuf = *_ubuf;
 done:
    free_syscall_buffer(_ubuf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(statfs, const char *, path, struct statfs *, buf)
    /* XXX if we allow this syscall nacl tries to use /dev/shm
       by disallowing it nacl uses /tmp/ instead NEAT */
    return -ENOSYS;
    /*
    long ret = -EBUFFUL;
    struct statfs *_buf;
    char *_path;
    unsigned long len = strlen(path) + 1;
    _buf = get_syscall_buffer(sizeof(struct statfs));
    _path = get_syscall_buffer(len);
    if (!_buf) goto done;
    if (!_path) goto done;

    memcpy(_path, path, len);

    ret = PASS_THROUGH_SYSCALL(statfs, 2, _path, _buf);
    *buf = *_buf;
 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_buf);
    return ret;
    */
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(fstatfs, int, fd, struct statfs *, buf)
    long ret = -EBUFFUL;
    struct statfs *_buf;
    _buf = get_syscall_buffer(sizeof(struct statfs));
    if (!_buf) goto done;

    ret = PASS_THROUGH_SYSCALL(fstatfs, 2, fd, _buf);
    *buf = *_buf;
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(getpriority, int, which, id_t, who)
    return PASS_THROUGH_SYSCALL(getpriority, 2, which, who);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(setpriority, int, which, int, who, int, prio)
    return PASS_THROUGH_SYSCALL(setpriority, 3, which, who, prio);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(sched_setparam, pid_t, pid, const struct sched_param *, param)
    long ret = -EBUFFUL;
    struct sched_param *_param;
    _param = get_syscall_buffer(sizeof(struct sched_param));
    if (!_param) goto done;

    *_param = *param;
    ret = PASS_THROUGH_SYSCALL(sched_setparam, 2, pid, _param);
 done:
    free_syscall_buffer(_param);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(sched_getparam, pid_t, pid, struct sched_param *, param)
    long ret = -EBUFFUL;
    struct sched_param *_param;
    _param = get_syscall_buffer(sizeof(struct sched_param));
    if (!_param) goto done;

    ret = PASS_THROUGH_SYSCALL(sched_getparam, 2, pid, _param);
    *param = *_param;
 done:
    free_syscall_buffer(_param);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(sched_setscheduler, pid_t, pid, int, policy,
        const struct sched_param*, param)
    long ret = -EBUFFUL;
    struct sched_param *_param;
    _param = get_syscall_buffer(sizeof(struct sched_param));
    if (!_param) goto done;

    *_param = *param;
    ret = PASS_THROUGH_SYSCALL(sched_setscheduler, 3, pid, policy, _param);
 done:
    free_syscall_buffer(_param);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(sched_getscheduler, pid_t, pid)
    return PASS_THROUGH_SYSCALL(sched_getscheduler, 1, pid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(sched_get_priority_max, int, policy)
    return PASS_THROUGH_SYSCALL(sched_get_priority_max, 1, policy);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(sched_get_priority_min, int, policy)
    return PASS_THROUGH_SYSCALL(sched_get_priority_min, 1, policy);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(sched_rr_get_interval, pid_t, pid, struct timespec *, tp)
    long ret = -EBUFFUL;
    struct timespec *_tp;
    _tp = get_syscall_buffer(sizeof(struct timespec));
    if (!_tp) goto done;

    ret = PASS_THROUGH_SYSCALL(sched_rr_get_interval, 2, pid, _tp);
    *tp = *_tp;
 done:
    free_syscall_buffer(_tp);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(mlock, const void *, addr, size_t, len)
    return PASS_THROUGH_SYSCALL(mlock, 2, addr, len);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(munlock, const void *, addr, size_t, len)
    return PASS_THROUGH_SYSCALL(munlock, 2, addr, len);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(mlockall, int, flags)
    return PASS_THROUGH_SYSCALL(mlockall, 1, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(munlockall)
    return PASS_THROUGH_SYSCALL(munlockall, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(vhangup)
    return PASS_THROUGH_SYSCALL(vhangup, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(modify_ldt, int, func, void *, ptr, unsigned long, bytecount)
    long ret = -EBUFFUL;
    void *_ptr;
    _ptr = get_syscall_buffer(bytecount);
    if (!_ptr) goto done;

    if (func == 1) {
        memcpy(_ptr, ptr, bytecount);
    }
    ret = PASS_THROUGH_SYSCALL(modify_ldt, 3, func, _ptr, bytecount);
    if (func == 0) {
        memcpy(ptr, _ptr, bytecount);
    }
 done:
    free_syscall_buffer(_ptr);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(pivot_root, const char *, new_root, const char *, put_old)
    long ret = -EBUFFUL;
    char *_new_root = NULL, *_put_old = NULL;
    unsigned long new_len = strlen(new_root) + 1;
    unsigned long old_len = strlen(put_old) + 1;
    _new_root = get_syscall_buffer(new_len);
    if (!_new_root) goto done;
    _put_old = get_syscall_buffer(old_len);
    if (!_put_old) goto done;

    memcpy(_new_root, new_root, new_len);
    memcpy(_put_old, put_old, old_len);

    ret = PASS_THROUGH_SYSCALL(pivot_root, 2, _new_root, _put_old);
 done:
    free_syscall_buffer(_new_root);
    free_syscall_buffer(_put_old);
    return ret;
SYSCALL_SHIM_END

#if 0
SYSCALL_SHIM_DEF_5(prctl, int, option, unsigned long, arg2, unsigned long, arg3,
        unsigned long, arg4, unsigned long, arg5)
    long ret = -EBUFFUL;
    void *_arg2 = (void *)arg2;

    char *buf_arg2 = NULL;

    unsigned long arg2_sz = 0;
    switch (option) {
        case PR_GET_TID_ADDRESS:
            arg2_sz = sizeof(int*);
            break;
        case PR_GET_NAME:
            arg2_sz = 16;
            break;
        case PR_SET_NAME:
            buf_arg2 = get_syscall_buffer(16);
            if(!buf_arg2) goto done;
            strncpy(buf_arg2, _arg2, 16);
            _arg2 = buf_arg2;
            break;
        case PR_GET_UNALIGN:
        case PR_GET_TSC:
        case PR_GET_FPEXC:
        case PR_GET_ENDIAN:
        case PR_GET_FPEMU:
        case PR_GET_PDEATHSIG:
            arg2_sz = sizeof(int);
            break;
        case PR_SET_MM:
        case PR_MCE_KILL_GET:
        case PR_MCE_KILL:
        case PR_SET_UNALIGN:
        case PR_SET_TSC:
        case PR_TASK_PERF_EVENTS_ENABLE:
        case PR_TASK_PERF_EVENTS_DISABLE:
        case PR_GET_TIMING:
        case PR_SET_TIMING:
        case PR_GET_TIMERSLACK:
        case PR_SET_TIMERSLACK:
/*
        case PR_GET_THP_DISABLE:
        case PR_SET_THP_DISABLE:
*/
        case PR_GET_SECUREBITS:
        case PR_SET_SECUREBITS:
        case PR_GET_SECCOMP:
        case PR_SET_SECCOMP:
        case PR_SET_PTRACER:
        case PR_SET_PDEATHSIG:
        case PR_GET_NO_NEW_PRIVS:
        case PR_SET_NO_NEW_PRIVS:
        case PR_GET_KEEPCAPS:
        case PR_SET_KEEPCAPS:
        case PR_SET_FPEXC:
        case PR_SET_FPEMU:
        case PR_SET_ENDIAN:
        case PR_GET_DUMPABLE:
        case PR_SET_DUMPABLE:
        case PR_SET_CHILD_SUBREAPER:
        case PR_CAPBSET_DROP:
        case PR_CAPBSET_READ:
            /* No marshalling to do; just let it happen */
        default:
            break;
    }
    if (arg2_sz) {
        buf_arg2 = get_syscall_buffer(arg2_sz);
        if(!buf_arg2) goto done;
        _arg2 = buf_arg2;
    }
    ret = PASS_THROUGH_SYSCALL(prctl, 5, option, _arg2, arg3, arg4, arg5);
    if (arg2_sz) {
        _arg2 = (void *)arg2;
        memcpy(_arg2, buf_arg2, arg2_sz);
    }
 done:
    free_syscall_buffer(buf_arg2);
    return ret;
SYSCALL_SHIM_END
#endif


SYSCALL_SHIM_DEF_2(arch_prctl, int, code, unsigned long, addr)
    /* modifies the value of fs/gs */
    if (in_sgx) {
        /* not allowed in enclave */
        DIE("Attempted arch_prctl in enclave");
    }
    return PASS_THROUGH_SYSCALL(arch_prctl, 2, code, addr);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(adjtimex, struct timex *, buf)
    long ret = -EBUFFUL;
    struct timex *_buf = get_syscall_buffer(sizeof(struct timex));
    if (!_buf) goto done;

    *_buf = *buf;
    ret = PASS_THROUGH_SYSCALL(adjtimex, 1, _buf);
    *buf = *_buf;

 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(setrlimit, int, resource, const struct rlimit *, rlim)
    long ret = -EBUFFUL;
    struct rlimit *_rlim = get_syscall_buffer(sizeof(struct rlimit));
    if (!_rlim) goto done;

    *_rlim = *rlim;
    ret = PASS_THROUGH_SYSCALL(setrlimit, 2, resource, _rlim);

 done:
    free_syscall_buffer(_rlim);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(chroot, const char *, path)
    long ret;
    unsigned long size = strlen(path) + 1;
    void *_path = get_syscall_buffer(size);
    if (!_path) {
        return -EBUFFUL;
    }
    memcpy(_path, path, size);
    ret = PASS_THROUGH_SYSCALL(chroot, 1, _path);
    free_syscall_buffer(_path);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(sync)
    return PASS_THROUGH_SYSCALL(sync, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(acct, const char *, filename)
    long ret;
    unsigned long size = strlen(filename) + 1;
    void *_filename = get_syscall_buffer(size);
    if (!_filename) {
        return -EBUFFUL;
    }
    memcpy(_filename, filename, size);
    ret = PASS_THROUGH_SYSCALL(chroot, 1, _filename);
    free_syscall_buffer(_filename);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(settimeofday, const struct timeval *, tv,
        const struct timezone *, tz)
    long ret = -EBUFFUL;
    struct timeval *_tv = get_syscall_buffer(sizeof(struct timeval));
    struct timezone *_tz = get_syscall_buffer(sizeof(struct timezone));
    if (!_tv) goto done;
    if (!_tz) goto done;

    *_tv = *tv;
    *_tz = *tz;
    ret = PASS_THROUGH_SYSCALL(settimeofday, 2, _tv, _tz);
 done:
    free_syscall_buffer(_tv);
    free_syscall_buffer(_tz);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(umount2, const char *, target, int, flags)
    long ret;
    unsigned long size = strlen(target) + 1;
    void *_target = get_syscall_buffer(size);
    if (!_target) {
        return -EBUFFUL;
    }
    memcpy(_target, target, size);
    ret = PASS_THROUGH_SYSCALL(umount2, 2, _target, flags);
    free_syscall_buffer(_target);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(swapon, const char *, path, int, swapflags)
    long ret;
    unsigned long size = strlen(path) + 1;
    void *_path = get_syscall_buffer(size);
    if (!_path) {
        return -EBUFFUL;
    }
    memcpy(_path, path, size);
    ret = PASS_THROUGH_SYSCALL(swapon, 2, _path, swapflags);
    free_syscall_buffer(_path);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(swapoff, const char *, path)
    long ret;
    unsigned long size = strlen(path) + 1;
    void *_path = get_syscall_buffer(size);
    if (!_path) {
        return -EBUFFUL;
    }
    memcpy(_path, path, size);
    ret = PASS_THROUGH_SYSCALL(swapoff, 1, _path);
    free_syscall_buffer(_path);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(sethostname, const char *, name, size_t, len)
    long ret;
    void *_name = get_syscall_buffer(len);
    if (!_name) {
        return -EBUFFUL;
    }
    memcpy(_name, name, len);
    ret = PASS_THROUGH_SYSCALL(sethostname, 2, _name, len);
    free_syscall_buffer(_name);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(setdomainname, const char *, name, size_t, len)
    long ret;
    void *_name = get_syscall_buffer(len);
    if (!_name) {
        return -EBUFFUL;
    }
    memcpy(_name, name, len);
    ret = PASS_THROUGH_SYSCALL(setdomainname, 2, _name, len);
    free_syscall_buffer(_name);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(iopl, int, level)
    return PASS_THROUGH_SYSCALL(iopl, 1, level);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(ioperm, unsigned long, form, unsigned long, num,
        int, turn_on)
    return PASS_THROUGH_SYSCALL(ioperm, 3, form, num, turn_on);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(create_module, const char *, name, size_t, size)
    long ret;
    void *_name = get_syscall_buffer(size);
    if (!_name) {
        return -EBUFFUL;
    }
    memcpy(_name, name, size);
    ret = PASS_THROUGH_SYSCALL(create_module, 2, _name, size);
    free_syscall_buffer(_name);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(init_module, void *, module_image, unsigned long, len,
        const char *, param_values)
    long ret = -EBUFFUL;
    unsigned long param_len = strlen(param_values) + 1;
    void *_module_image = get_syscall_buffer(len);
    void *_param_values = get_syscall_buffer(param_len);
    if (!_module_image) goto done;
    memcpy(_module_image, module_image, len);
    memcpy(_param_values, param_values, param_len);
    ret = PASS_THROUGH_SYSCALL(init_module, 3, _module_image, len,
            _param_values);
 done:
    free_syscall_buffer(_module_image);
    free_syscall_buffer(_param_values);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(delete_module, const char *, name, int, flags)
    long ret;
    unsigned long size = strlen(name) + 1;
    void *_name = get_syscall_buffer(size);
    if (!_name) {
        return -EBUFFUL;
    }
    memcpy(_name, name, size);
    ret = PASS_THROUGH_SYSCALL(delete_module, 2, _name, flags);
    free_syscall_buffer(_name);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(gettid)
    return PASS_THROUGH_SYSCALL(gettid, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(readahead, int, fd, off_t, offset, size_t, count)
    return PASS_THROUGH_SYSCALL(readahead, 3, fd, offset, count);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(setxattr, const char *, path, const char *, name,
        const void *, value, size_t, size, int, flags)
    long ret = -EBUFFUL;
    void *_path = NULL, *_name = NULL, *_value = NULL;
    unsigned long path_sz = strlen(path) + 1;
    unsigned long name_sz = strlen(name) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;
    _value = get_syscall_buffer(size);
    if (!_value) goto done;

    memcpy(_path, path, path_sz);
    memcpy(_name, name, name_sz);
    memcpy(_value, value, size);

    ret = PASS_THROUGH_SYSCALL(setxattr, 5, _path, _name, _value, size, flags);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_name);
    free_syscall_buffer(_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(lsetxattr, const char *, path, const char *, name,
        const void *, value, size_t, size, int, flags)
    long ret = -EBUFFUL;
    void *_path = NULL, *_name = NULL, *_value = NULL;
    unsigned long path_sz = strlen(path) + 1;
    unsigned long name_sz = strlen(name) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;
    _value = get_syscall_buffer(size);
    if (!_value) goto done;

    memcpy(_path, path, path_sz);
    memcpy(_name, name, name_sz);
    memcpy(_value, value, size);

    ret = PASS_THROUGH_SYSCALL(lsetxattr, 5, _path, _name, _value, size, flags);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_name);
    free_syscall_buffer(_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(fsetxattr, int, fd, const char *, name, const void *, value,
        size_t, size, int, flags)
    long ret = -EBUFFUL;
    unsigned long name_sz = strlen(name) + 1;
    void *_name = NULL, *_value = NULL;

    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;
    _value = get_syscall_buffer(size);
    if (!_value) goto done;

    memcpy(_name, name, name_sz);
    memcpy(_value, value, size);

    ret = PASS_THROUGH_SYSCALL(fsetxattr, 5, fd, _name, _value, size, flags);

 done:
    free_syscall_buffer(_name);
    free_syscall_buffer(_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(getxattr, const char *, path, const char *, name,
        void *, value, size_t, size)
    long ret = -EBUFFUL;
    void *_path = NULL, *_name = NULL, *_value = NULL;
    unsigned long path_sz = strlen(path) + 1;
    unsigned long name_sz = strlen(name) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;
    _value = get_syscall_buffer(size);
    if (!_value) goto done;

    memcpy(_path, path, path_sz);
    memcpy(_name, name, name_sz);

    ret = PASS_THROUGH_SYSCALL(getxattr, 4, _path, _name, _value, size);
    memcpy(value, _value, size);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_name);
    free_syscall_buffer(_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(lgetxattr, const char *, path, const char *, name,
        void *, value, size_t, size)
    long ret = -EBUFFUL;
    void *_path = NULL, *_name = NULL, *_value = NULL;
    unsigned long path_sz = strlen(path) + 1;
    unsigned long name_sz = strlen(name) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;
    _value = get_syscall_buffer(size);
    if (!_value) goto done;

    memcpy(_path, path, path_sz);
    memcpy(_name, name, name_sz);

    ret = PASS_THROUGH_SYSCALL(lgetxattr, 4, _path, _name, _value, size);
    memcpy(value, _value, size);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_name);
    free_syscall_buffer(_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(fgetxattr, int, fd, const char *, name, void *, value,
        size_t, size)
    long ret = -EBUFFUL;
    void *_name = NULL, *_value = NULL;
    unsigned long name_sz = strlen(name) + 1;

    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;
    _value = get_syscall_buffer(size);
    if (!_value) goto done;

    memcpy(_name, name, name_sz);

    ret = PASS_THROUGH_SYSCALL(fgetxattr, 4, fd, _name, _value, size);
    memcpy(value, _value, size);

 done:
    free_syscall_buffer(_name);
    free_syscall_buffer(_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(listxattr, const char *, path, char *, list, size_t, size)
    long ret = -EBUFFUL;
    void *_path = NULL, *_list = NULL;
    unsigned long path_sz = strlen(path) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _list = get_syscall_buffer(size);
    if (!_list) goto done;

    memcpy(_path, path, path_sz);

    ret = PASS_THROUGH_SYSCALL(listxattr, 3, _path, _list, size);
    memcpy(list, _list, size);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_list);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(llistxattr, const char *, path, char *, list, size_t, size)
    long ret = -EBUFFUL;
    void *_path = NULL, *_list = NULL;
    unsigned long path_sz = strlen(path) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _list = get_syscall_buffer(size);
    if (!_list) goto done;

    memcpy(_path, path, path_sz);

    ret = PASS_THROUGH_SYSCALL(llistxattr, 3, _path, _list, size);
    memcpy(list, _list, size);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_list);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(flistxattr, int, fd, char *, list, size_t, size)
    long ret = -EBUFFUL;
    void *_list = NULL;

    _list = get_syscall_buffer(size);
    if (!_list) goto done;

    ret = PASS_THROUGH_SYSCALL(flistxattr, 3, fd, _list, size);
    memcpy(list, _list, size);

 done:
    free_syscall_buffer(_list);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(removexattr, const char *, path, const char *, name)
    long ret = -EBUFFUL;
    void *_path = NULL, *_name = NULL;
    unsigned long path_sz = strlen(path) + 1;
    unsigned long name_sz = strlen(name) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;

    memcpy(_path, path, path_sz);
    memcpy(_name, name, name_sz);

    ret = PASS_THROUGH_SYSCALL(removexattr, 2, _path, _name);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_name);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(lremovexattr, const char *, path, const char *, name)
    long ret = -EBUFFUL;
    void *_path = NULL, *_name = NULL;
    unsigned long path_sz = strlen(path) + 1;
    unsigned long name_sz = strlen(name) + 1;

    _path = get_syscall_buffer(path_sz);
    if (!_path) goto done;
    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;

    memcpy(_path, path, path_sz);
    memcpy(_name, name, name_sz);

    ret = PASS_THROUGH_SYSCALL(lremovexattr, 2, _path, _name);

 done:
    free_syscall_buffer(_path);
    free_syscall_buffer(_name);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(fremovexattr, int, fd, const char *, name)
    long ret = -EBUFFUL;
    void *_name = NULL;
    unsigned long name_sz = strlen(name) + 1;

    _name = get_syscall_buffer(name_sz);
    if (!_name) goto done;

    memcpy(_name, name, name_sz);

    ret = PASS_THROUGH_SYSCALL(fremovexattr, 2, fd, _name);

 done:
    free_syscall_buffer(_name);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(tkill, int, tid, int, sig)
    return PASS_THROUGH_SYSCALL(tkill, 2, tid, sig);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(time, time_t *, t)
    long ret = -EBUFFUL;
    time_t *_t = NULL;

    if (t) {
        _t = get_syscall_buffer(sizeof(time_t));
        if (!_t) goto done;
    }

    ret = PASS_THROUGH_SYSCALL(time, 1, _t);
    if (t) {
        *t = *_t;
        free_syscall_buffer(_t);
    }
 done:
    return ret;
SYSCALL_SHIM_END

struct futex_map {
    uint32_t *in;
    uint32_t *out;
    sgx_mutex lock;
};

static size_t futex_hash(const void *e, void *unused)
{
    return hash_pointer(((struct futex_map*)e)->in, 0);
}
static bool futexcmp(const void *e, void *addr)
{
    return ((struct futex_map *)e)->in == addr;
}

bool sgx_malloc = false;
static
long do_wait_on_futex(struct futex_map *ftx, uint32_t val,
        const struct timespec *timeout)
{
    long ret;
    struct timespec *t = NULL;
    if (timeout) {
        t = get_syscall_buffer(sizeof(struct timespec));
        if (!t) return -EBUFFUL;
        memcpy(t, timeout, sizeof(struct timespec));
    }
    ret = PASS_THROUGH_SYSCALL(futex, 6, ftx->out, FUTEX_WAIT_PRIVATE, val, t,
                               NULL, 0);
    free_syscall_buffer(t);
    return ret;
}
static long do_wake_on_pseudo_futex(uint32_t *ftx, int val)
{
    atomic_fetch_add(ftx, 1);
    return PASS_THROUGH_SYSCALL(futex, 6, ftx, FUTEX_WAKE_PRIVATE, val, NULL,
                                NULL, 0);
}

static uint32_t *pool_base;
static uint32_t total_futexes;
struct futex_map *create_entry(struct htable *ht, size_t key, uint32_t *addr)
{
    static sgx_mutex futex_mutex;
    static uint32_t next_futex_idx;
    struct futex_map *entry;


    sgx_lock(&futex_mutex);
    entry = htable_get(ht, key, futexcmp, addr);
    if (!entry) {
        if (next_futex_idx > total_futexes) DIE("Used up all my futexes!");
        sgx_malloc = true;
        entry = malloc(sizeof(struct futex_map));
        entry->out = pool_base + next_futex_idx++;
        entry->in = addr;
        entry->lock = 0;
        htable_add(ht, key, entry);
    }
    sgx_unlock(&futex_mutex);
    return entry;
}
static
struct futex_map *futex_map_lookup(uint32_t *addr)
{
    /* futex creation mutex */
    bool ht_inited;

    static struct htable ht;
    struct futex_map *entry;
    size_t key = hash_pointer(addr, 0);

    if (!pool_base) {
        pool_base = enclave_params.futex_pool;
        total_futexes = enclave_params.n_futexes;
        htable_init(&ht, futex_hash, NULL);
    }

    entry = htable_get(&ht, key, futexcmp, addr);
    if (!entry) {
        entry = create_entry(&ht, key, addr);
    }
    return entry;
}

static uint32_t *futex_translate(uint32_t *addr)
{
    return futex_map_lookup(addr)->out;
}

static void release_lock_pair(struct futex_map *map1, struct futex_map *map2)
{
    sgx_unlock(&map1->lock);
    sgx_unlock(&map2->lock);
}
static void grab_lock_pair(struct futex_map *map1, struct futex_map *map2)
{
    if (map1->in < map2->in) {
        sgx_lock(&map1->lock);
        sgx_lock(&map2->lock);
    } else {
        sgx_lock(&map2->lock);
        sgx_lock(&map1->lock);
    }
}

long do_futex_wake_op(struct futex_map *map, int val, int val2,
        struct futex_map* map2, int val3)
{
    long ret = 0;
    int op, cmp, oparg, cmparg, oldval = 0;
    uint32_t *out, *out2;
    op = (val3 >> 28) & 0xf;
    cmp = (val3 >> 24) & 0xf;
    oparg = (val3 >> 12) & 0xfff;
    cmparg = val3 & 0xfff;
    bool cmp_res = false;

    switch(op) {
        case FUTEX_OP_SET:
            oldval = atomic_exchange(map2->in, oparg);
            break;
        case FUTEX_OP_ADD:
            oldval = atomic_fetch_add(map2->in, oparg);
            break;
        case FUTEX_OP_OR:
            oldval = atomic_fetch_or(map2->in, oparg);
            break;
        case FUTEX_OP_ANDN:
            oldval = atomic_fetch_and(map2->in, ~oparg);
            break;
        case FUTEX_OP_XOR:
            oldval = atomic_fetch_xor(map2->in, oparg);
            break;
        default:
            DIE("BAD WAKE OP");
    }
    switch (cmp) {
        case FUTEX_OP_CMP_EQ:
            cmp_res = oldval == cmparg;
            break;
        case FUTEX_OP_CMP_NE:
            cmp_res = oldval != cmparg;
            break;
        case FUTEX_OP_CMP_LT:
            cmp_res = oldval < cmparg;
            break;
        case FUTEX_OP_CMP_LE:
            cmp_res = oldval <= cmparg;
            break;
        case FUTEX_OP_CMP_GT:
            cmp_res = oldval > cmparg;
            break;
        case FUTEX_OP_CMP_GE:
            cmp_res = oldval >= cmparg;
            break;
        default:
            DIE("BAD CMP");
    }
    ret = do_wake_on_pseudo_futex(map->out, val);
    if (ret < 0)
        DIE("WAKE failed!");
    if (cmp_res)
        ret += do_wake_on_pseudo_futex(map2->out, val2);
    return ret;
}

SYSCALL_SHIM_DEF_6(futex, uint32_t *, uaddr, int, op, int, val,
        const struct timespec *, timeout, uint32_t *, uaddr2, int, val3)
    long ret = -EINVAL;
    int base_op;
    struct futex_map *map, *map2;
    if (!in_sgx) {
        ret = PASS_THROUGH_SYSCALL(futex, 6, uaddr, op, val, timeout, uaddr2,
                val3);
    } else {
        if (!(op & FUTEX_PRIVATE_FLAG)) {
            DIE("FUTEXES should be PRIVATE");
        }
        if (!IN_ENCLAVE(uaddr)) {
            DIE("FUTEX for outside address....that shouldn't happen");
        }
        switch (op & FUTEX_CMD_MASK) {
            case FUTEX_WAKE_BITSET:
                DIE("FUTEX_WAKE_BITSET");
            case FUTEX_WAKE:
                map = futex_map_lookup(uaddr);
                sgx_lock(&map->lock);
                ret = do_wake_on_pseudo_futex(map->out, val);
                sgx_unlock(&map->lock);
                break;
            case FUTEX_WAIT_BITSET:
            case FUTEX_WAIT:
                map = futex_map_lookup(uaddr);
                if (atomic_load(map->in) != val)
                    ret = -EAGAIN;
                else {
                    uint32_t out_val;
                    sgx_lock(&map->lock);
                    out_val = atomic_load(map->out);
                    sgx_unlock(&map->lock);
                    ret = do_wait_on_futex(map, out_val, timeout);
                }
                break;
            case FUTEX_WAKE_OP:
                map = futex_map_lookup(uaddr);
                map2 = futex_map_lookup(uaddr2);
                grab_lock_pair(map, map2);
                do_futex_wake_op(map, val, (int)(long)timeout, map2, val3);
                release_lock_pair(map, map2);
                break;
            case FUTEX_CMP_REQUEUE:
                DIE("UNEXPECTED FUTEX_CMP_REQUEUE");
                break;
            case FUTEX_REQUEUE:
                DIE("UNEXPECTED FUTEX_REQUEUE");
                break;
            case FUTEX_FD:
                DIE("UNEXPECTED FUTEX_FD");
                break;
            default:
                DIE("UNEXPECTED FUTEX OP");
        }
    }

    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(sched_setaffinity, pid_t, pid, size_t, cpusetsize,
        const void*, mask)
    long ret = -EBUFFUL;
    void *_mask = NULL;

    _mask = get_syscall_buffer(cpusetsize);
    if (!_mask) goto done;

    memcpy(_mask, mask, cpusetsize);
    ret = PASS_THROUGH_SYSCALL(sched_setaffinity, 3, pid, cpusetsize, _mask);

    free_syscall_buffer(_mask);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(sched_getaffinity, pid_t, pid, size_t, cpusetsize,
        void*, mask)
    long ret = -EBUFFUL;
    void *_mask = NULL;

    _mask = get_syscall_buffer(cpusetsize);
    if (!_mask) goto done;

    ret = PASS_THROUGH_SYSCALL(sched_getaffinity, 3, pid, cpusetsize, _mask);
    memcpy(mask, _mask, cpusetsize);

    free_syscall_buffer(_mask);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(set_thread_area, struct user_desc *, u_info)
    long ret = -EBUFFUL;
    struct user_desc *_u_info = NULL;

    _u_info = get_syscall_buffer(sizeof(struct user_desc));
    if (!_u_info) goto done;

    *_u_info = *u_info;
    ret = PASS_THROUGH_SYSCALL(set_thread_area, 1, _u_info);
    *u_info = *_u_info;

    free_syscall_buffer(_u_info);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(io_setup, unsigned, nr_events, aio_context_t *, ctx_idp)
    long ret = -EBUFFUL;
    aio_context_t *_ctx_idp = NULL;

    _ctx_idp = get_syscall_buffer(sizeof(aio_context_t));
    if (!_ctx_idp) goto done;

    ret = PASS_THROUGH_SYSCALL(io_setup, 2, nr_events, _ctx_idp);
    *ctx_idp = *_ctx_idp;

    free_syscall_buffer(_ctx_idp);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(io_destroy, aio_context_t, ctx_id)
    return PASS_THROUGH_SYSCALL(io_destroy, 1, ctx_id);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(io_getevents, aio_context_t, ctx_id, long, min_nr, long, nr,
        struct io_event *, events, struct timespec *, timeout)
    long ret = -EBUFFUL;
    struct io_event *_events = NULL;
    struct timespec *_timeout = NULL;

    _events = get_syscall_buffer(sizeof(struct io_event));
    if (!_events) goto done;
    if (timeout) {
        _timeout = get_syscall_buffer(sizeof(struct timespec));
        if (!_timeout) goto done;
        *_timeout = *timeout;
    }

    ret = PASS_THROUGH_SYSCALL(io_getevents, 5, ctx_id, min_nr, nr, _events,
            _timeout);
    *events = *_events;
    if (timeout) {
        *timeout = *_timeout;
    }

 done:
    free_syscall_buffer(_events);
    free_syscall_buffer(_timeout);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(get_thread_area, struct user_desc *, u_info)
    long ret = -EBUFFUL;
    struct user_desc *_u_info = NULL;

    _u_info = get_syscall_buffer(sizeof(struct user_desc));
    if (!_u_info) goto done;

    *_u_info = *u_info;
    ret = PASS_THROUGH_SYSCALL(get_thread_area, 1, _u_info);
    *u_info = *_u_info;

    free_syscall_buffer(_u_info);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(lookup_dcookie, uint64_t, cookie, char *, buffer,
        size_t, len)
    long ret = -EBUFFUL;
    char *_buffer = NULL;

    _buffer = get_syscall_buffer(len);
    if (!_buffer) goto done;

    ret = PASS_THROUGH_SYSCALL(lookup_dcookie, 3, cookie, _buffer, len);
    memcpy(buffer, _buffer, len);

    free_syscall_buffer(_buffer);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(epoll_create, int, size)
    return PASS_THROUGH_SYSCALL(epoll_create, 1, size);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(getdents64, int, fd, void *, dirp, unsigned int, count)
    long ret = -EBUFFUL;
    char *_buf = get_syscall_buffer(count);
    if (!_buf) goto done;


    ret = PASS_THROUGH_SYSCALL(getdents64, 3, fd, _buf, count);
    memcpy(dirp, _buf, count);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(set_tid_address, int *, tidptr)
    long ret = -EBUFFUL;
    int *_buf = get_syscall_buffer(sizeof(int));
    if (!_buf) goto done;


    ret = PASS_THROUGH_SYSCALL(set_tid_address, 1, _buf);
    *tidptr = *_buf;
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(restart_syscall)
    return PASS_THROUGH_SYSCALL(restart_syscall, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(semtimedop, int, semid, struct sembuf *, sops, size_t, nsops,
        struct timespec *, timeout)
    long ret = -EBUFFUL;
    unsigned long size = nsops * sizeof(struct sembuf);
    struct sembuf *_sops = NULL;
    struct timespec* _timeout = NULL;
    _sops = get_syscall_buffer(size);
    if (!_sops) goto done;

    if (timeout) {
        _timeout = get_syscall_buffer(sizeof(struct timespec));
        if (!_timeout) goto done;
        *_timeout = *timeout;
    }

    memcpy(_sops, sops, size);
    ret = PASS_THROUGH_SYSCALL(semtimedop, 4, semid, _sops, nsops, _timeout);
    memcpy(sops, _sops, size);
    if (timeout) {
        *timeout = *_timeout;
    }
 done:
    free_syscall_buffer(_sops);
    free_syscall_buffer(_timeout);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(fadvise64, int, fd, off_t, offset, off_t, len,
        off_t, advice)
    return PASS_THROUGH_SYSCALL(fadvise64, 4, fd, offset, len, advice);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(timer_create, clockid_t, clockid, struct sigevent *, sevp,
        timer_t *, timerid)
    long ret = -EBUFFUL;
    struct sigevent *_sevp = NULL;
    timer_t* _timerid = NULL;

    _sevp = get_syscall_buffer(sizeof(struct sigevent));
    if (!_sevp) goto done;

    _timerid = get_syscall_buffer(sizeof(timer_t));
    if (!_timerid) goto done;

    *_sevp = *sevp;

    ret = PASS_THROUGH_SYSCALL(timer_create, 3, clockid, _sevp, _timerid);
    *timerid = *timerid;
 done:
    free_syscall_buffer(_sevp);
    free_syscall_buffer(_timerid);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(timer_settime, timer_t, timerid, int, flags,
        const struct itimerspec *, new_value, struct itimerspec *, old_value)
    long ret = -EBUFFUL;
    struct itimerspec *_new_value = NULL;
    struct itimerspec *_old_value = NULL;

    _new_value = get_syscall_buffer(sizeof(struct itimerspec));
    if (!_new_value) goto done;

    if (old_value) {
        _old_value = get_syscall_buffer(sizeof(struct itimerspec));
        if (!_old_value) goto done;
    }

    *_new_value = *new_value;

    ret = PASS_THROUGH_SYSCALL(timer_settime, 4, timerid, flags, _new_value,
            _old_value);
    if (old_value) {
        *old_value = *_old_value;
    }
 done:
    free_syscall_buffer(_new_value);
    free_syscall_buffer(_old_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(timer_gettime, timer_t, timerid,
        struct itimerspec *, cur_value)
    long ret = -EBUFFUL;
    struct itimerspec *_cur_value = NULL;

    _cur_value = get_syscall_buffer(sizeof(struct itimerspec));
    if (!_cur_value) goto done;

    ret = PASS_THROUGH_SYSCALL(timer_settime, 2, timerid, _cur_value);
    *cur_value = *_cur_value;
    free_syscall_buffer(_cur_value);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(timer_getoverrun, timer_t, timerid)
    return PASS_THROUGH_SYSCALL(timer_getoverrun, 1, timerid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(timer_delete, timer_t, timerid)
    return PASS_THROUGH_SYSCALL(timer_delete, 1, timerid);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(clock_settime, clockid_t, clk_id,
        const struct timespec *, tp)
    long ret = -EBUFFUL;
    struct timespec *_tp = NULL;

    _tp = get_syscall_buffer(sizeof(struct timespec));
    if (!_tp) goto done;

    *_tp = *tp;
    ret = PASS_THROUGH_SYSCALL(clock_settime, 2, clk_id, _tp);
    free_syscall_buffer(_tp);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(clock_gettime, clockid_t, clk_id, struct timespec *, tp)
    long ret = -EBUFFUL;
    struct timespec *_tp = NULL;

    _tp = get_syscall_buffer(sizeof(struct timespec));
    if (!_tp) goto done;

    ret = PASS_THROUGH_SYSCALL(clock_gettime, 2, clk_id, _tp);
    *tp = *_tp;
    free_syscall_buffer(_tp);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(clock_getres, clockid_t, clk_id, struct timespec *, res)
    long ret = -EBUFFUL;
    struct timespec *_res = NULL;

    if (res) {
      _res = get_syscall_buffer(sizeof(struct timespec));
      if (!_res) goto done;
    }

    ret = PASS_THROUGH_SYSCALL(clock_getres, 2, clk_id, _res);
    if (res)
      *res = *_res;
    free_syscall_buffer(_res);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(clock_nanosleep, clockid_t, clk_id, int, flags,
        const struct timespec *, request, struct timespec*, remain)
    long ret = -EBUFFUL;
    struct timespec *_request = NULL;
    struct timespec *_remain = NULL;

    _request = get_syscall_buffer(sizeof(struct timespec));
    if (!_request) goto done;
    _remain = get_syscall_buffer(sizeof(struct timespec));
    if (!_remain) goto done;

    *_request = *request;
    ret = PASS_THROUGH_SYSCALL(clock_getres, 4, clk_id, flags, _request,
            _remain);
    *remain = *_remain;
 done:
    free_syscall_buffer(_request);
    free_syscall_buffer(_remain);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(exit_group, int, status);
    return PASS_THROUGH_SYSCALL(exit_group, 1, status);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(epoll_wait, int, epfd, struct epoll_event *, events,
        int, maxevents, int, timeout)
    long ret;
    unsigned long size = sizeof(struct epoll_event) * maxevents;
    void *_events = get_syscall_buffer(size);
    if (!_events) {
        return -EBUFFUL;
    }
    memcpy(_events, events, size);
    ret = PASS_THROUGH_SYSCALL(epoll_wait, 4, epfd, _events, maxevents,
            timeout);
    memcpy(events, _events, size);
    free_syscall_buffer(_events);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(epoll_ctl, int, epfd, int, op, int, fd,
        struct epoll_event *, event)
    long ret;
    struct epoll_event *_event = get_syscall_buffer(sizeof(struct epoll_event));
    if (!_event) {
        return -EBUFFUL;
    }
    *_event = *event;
    ret = PASS_THROUGH_SYSCALL(epoll_ctl, 4, epfd, op, fd, _event);
    *event = *_event;

    free_syscall_buffer(_event);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(tgkill, int, tgid, int, tid, int, sig)
    return PASS_THROUGH_SYSCALL(tgkill, 3, tgid, tid, sig);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(utimes, const char *, filename,
        const struct timeval *, times)
    long ret = -EBUFFUL;
    char *_filename;
    struct timeval *_times;
    unsigned long len = strlen(filename) + 1;
    _filename = get_syscall_buffer(len);
    _times = times ? get_syscall_buffer(sizeof(struct timeval) * 2) : NULL;
    if (!_filename) goto done;
    if (times && !_times) goto done;

    memcpy(_filename, filename, len);
    if (_times) memcpy(_times, times, sizeof(struct timeval) * 2);
    ret = PASS_THROUGH_SYSCALL(utimes, 2, _filename, _times);

 done:
    free_syscall_buffer(_filename);
    free_syscall_buffer(_times);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_6(mbind, void *, addr, unsigned long, len, int, mode,
      const unsigned long *, nodemask, unsigned long, maxnode, unsigned, flags)
     return -ENOSYS;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(mq_open, const char *, name, int, oflag, mode_t, mode,
        struct mq_attr *, attr)
    long ret = -EBUFFUL;
    long size = strlen(name) + 1;
    char *_buf = get_syscall_buffer(size);
    struct mq_attr *_attr = NULL;
    if (!_buf) {
        return -EBUFFUL;
    }
    if (attr) {
        _attr = get_syscall_buffer(sizeof(struct mq_attr));
        if (_attr) goto done;
        *_attr = *attr;
    }
    memcpy(_buf, name, size);
    ret = PASS_THROUGH_SYSCALL(mq_open, 4, _buf, oflag, mode, _attr);
    if (attr) {
        *attr = *_attr;
    }
 done:
    free_syscall_buffer(_attr);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(mq_unlink, const char *, name)
    long ret, size = strlen(name) + 1;
    char *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, name, size);
    ret = PASS_THROUGH_SYSCALL(mq_unlink, 1, _buf);
 done:
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(mq_timedsend, mqd_t, mqdes, const char *, msg_ptr,
        size_t, msg_len, unsigned int, msg_prio,
        const struct timespec *, abs_timeout)
    long ret = -EBUFFUL;
    char *_buf = get_syscall_buffer(msg_len);
    struct timespec *_abs_timeout = NULL;
    if (!_buf) {
        return -EBUFFUL;
    }
    if (abs_timeout) {
        _abs_timeout = get_syscall_buffer(sizeof(struct timespec));
        if (!_abs_timeout) goto done;
        *_abs_timeout = *abs_timeout;
    }
    memcpy(_buf, msg_ptr, msg_len);
    ret = PASS_THROUGH_SYSCALL(mq_unlink, 5, mqdes, _buf, msg_len, msg_prio,
            _abs_timeout);
 done:
    free_syscall_buffer(_buf);
    free_syscall_buffer(_abs_timeout);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(mq_timedreceive, mqd_t, mqdes, char *, msg_ptr,
        size_t, msg_len, unsigned int *, msg_prio,
        const struct timespec *, abs_timeout)
    long ret = -EBUFFUL;
    char *_buf = get_syscall_buffer(msg_len);
    struct timespec *_abs_timeout = NULL;
    if (!_buf) {
        return -EBUFFUL;
    }
    if (abs_timeout) {
        _abs_timeout = get_syscall_buffer(sizeof(struct timespec));
        if (!_abs_timeout) goto done;
        *_abs_timeout = *abs_timeout;
    }
    ret = PASS_THROUGH_SYSCALL(mq_unlink, 5, mqdes, _buf, msg_len, msg_prio,
            _abs_timeout);
    memcpy(msg_ptr, _buf, msg_len);
 done:
    free_syscall_buffer(_buf);
    free_syscall_buffer(_abs_timeout);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(mq_notify, mqd_t, mqdes, const struct sigevent *, sevp)
    long ret = -EBUFFUL;
    struct sigevent *_sevp = NULL;
    _sevp = get_syscall_buffer(sizeof(struct sigevent));
    if (!_sevp) goto done;
    *_sevp = *sevp;

    ret = PASS_THROUGH_SYSCALL(mq_notify, 2, mqdes, _sevp);
 done:
    free_syscall_buffer(_sevp);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_5(waitid, idtype_t, id_type, id_t, id, siginfo_t *, infop,
      int, options, struct rusage *, usage)
     long ret = -EBUFFUL;
     struct rusage *_usage = NULL;
     siginfo_t *_infop = get_syscall_buffer(sizeof(siginfo_t));
     if (!_infop) goto done;
     if (infop)
        *_infop = *infop;
     if (usage) {
         _usage = get_syscall_buffer(sizeof(struct rusage));
         if (!usage) goto done;
         *_usage = *usage;
     }
     ret = PASS_THROUGH_SYSCALL(waitid, 5, id_type, id, _infop, options, _usage);
     if (usage)
         *usage = *_usage;
     if (infop)
         *infop = *_infop;
done:
     free_syscall_buffer(_infop);
     free_syscall_buffer(_usage);
     return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(ioprio_set, int, which, int, who, int, ioprio)
     return PASS_THROUGH_SYSCALL(ioprio_set, 3, which, who, ioprio);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(ioprio_get, int, which, int, who)
     return PASS_THROUGH_SYSCALL(ioprio_get, 2, which, who);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_0(inotify_init)
    return PASS_THROUGH_SYSCALL(inotify_init, 0);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(inotify_add_watch, int, fd, const char*, pathname,
        uint32_t, mask)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = NULL;
    _pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;
    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(inotify_add_watch, 3, fd, _pathname, mask);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(inotify_rm_watch, int, fd, int, wd)
    return PASS_THROUGH_SYSCALL(inotify_rm_watch, 2, fd, wd);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(openat, int, dirfd, const char *, filename, int, flags,
        int, mode)
    long ret, size = strlen(filename) + 1;
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, filename, size);
    ret = PASS_THROUGH_SYSCALL(openat, 4, dirfd, _buf, flags, mode);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(mkdirat, int, dirfd, const char *, pathname, mode_t, mode)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(mkdirat, 3, dirfd, _pathname, mode);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(mknodat, int, dirfd, const char *, pathname, mode_t, mode,
        dev_t, dev)
    long ret = -EBUFFUL;
    char *_pathname;
    unsigned long len = strlen(pathname) + 1;
    _pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(mknodat, 4, dirfd, _pathname, mode, dev);

 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(fchownat, int, dirfd, const char *, pathname, uid_t, owner,
        gid_t, group, int, flags)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(fchownat, 5, dirfd, _pathname, owner, group,
            flags);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(futimesat, int, dirfd, const char*, filename,
        const struct timeval *, times)
    long ret = -EBUFFUL;
    char *_filename = NULL;
    struct timeval *_times = NULL;
    unsigned long len = strlen(filename) + 1;

    _filename = get_syscall_buffer(len);
    if (!_filename) goto done;
    if (times) {
       _times = get_syscall_buffer(sizeof(struct timeval) * 2);
       if (!_times)
          goto done;
       _times[0] = times[0];
       _times[1] = times[1];
    }

    memcpy(_filename, filename, len);

    ret = PASS_THROUGH_SYSCALL(futimesat, 3, dirfd, _filename, _times);

 done:
    free_syscall_buffer(_filename);
    free_syscall_buffer(_times);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(newfstatat, int, dfd, char *, filename, struct stat *, buf,
        int, flag)
    long ret, len = strlen(filename) + 1;
    long size = len + sizeof(struct stat);
    void *_buf_filename = get_syscall_buffer(size);
    if (!_buf_filename) {
        return -EBUFFUL;
    }
    void *_buf_stat = _buf_filename + len;

    memcpy(_buf_filename, filename, len);

    ret = PASS_THROUGH_SYSCALL(newfstatat, 4, dfd, _buf_filename, _buf_stat,
            flag);

    memcpy(buf, _buf_stat, sizeof(struct stat));

    free_syscall_buffer(_buf_filename);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(unlinkat, int, dfd, const char *, pathname, int, flags)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(unlinkat, 3, dfd, _pathname, flags);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(renameat, int, olddirfd, const char *, oldpath,
        int, newdirfd, const char *, newpath)
    long ret = -EBUFFUL;
    char *_oldpath = NULL, *_newpath = NULL;
    unsigned long oldlen = strlen(oldpath) + 1;
    unsigned long newlen = strlen(newpath) + 1;

    _oldpath = get_syscall_buffer(oldlen);
    _newpath = get_syscall_buffer(newlen);
    memcpy(_oldpath, oldpath, oldlen);
    memcpy(_newpath, newpath, newlen);
    ret =  PASS_THROUGH_SYSCALL(renameat, 4, olddirfd, _oldpath, newdirfd,
            _newpath);
 done:
    free_syscall_buffer(_oldpath);
    free_syscall_buffer(_newpath);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(linkat, int, olddirfd, const char *, oldpath, int, newdirfd,
        const char *, newpath, int, flags)
    long ret = -EBUFFUL;
    unsigned long oldlen = strlen(oldpath) + 1;
    unsigned long newlen = strlen(newpath) + 1;
    char *_oldpath = NULL, *_newpath = NULL;

    _oldpath = get_syscall_buffer(oldlen);
    if (!_oldpath) goto done;
    _newpath = get_syscall_buffer(newlen);
    if (!_newpath) goto done;

    memcpy(_oldpath, oldpath, oldlen);
    memcpy(_newpath, newpath, newlen);

    ret = PASS_THROUGH_SYSCALL(linkat, 5, olddirfd, _oldpath, newdirfd,
            _newpath, flags);
 done:
    free_syscall_buffer(_oldpath);
    free_syscall_buffer(_newpath);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(symlinkat, const char *, target, int, newdirfd,
        const char *, linkpath)
    long ret = -EBUFFUL;
    unsigned long tarlen = strlen(target) + 1;
    unsigned long linklen = strlen(linkpath) + 1;
    char *_target = NULL, *_linkpath = NULL;
    _target = get_syscall_buffer(tarlen);
    if (!_target) goto done;
    _linkpath = get_syscall_buffer(linklen);
    if (!_linkpath) goto done;

    memcpy(_target, target, tarlen);
    memcpy(_linkpath, linkpath, linklen);

    ret = PASS_THROUGH_SYSCALL(symlinkat, 3, _target, newdirfd, _linkpath);
 done:
    free_syscall_buffer(_target);
    free_syscall_buffer(_linkpath);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(readlinkat, int, dirfd, const char *, path, char *, buf,
        size_t, bufsize)
    long ret;
    unsigned long len = strlen(path) + 1;
    char *path_buf = NULL, *_buf = NULL;
    path_buf = get_syscall_buffer(len);
    if (!path_buf) {
        return -EBUFFUL;
    }
    _buf = get_syscall_buffer(bufsize);
    if (!_buf) {
        free_syscall_buffer(path_buf);
        return -EBUFFUL;
    }

    memcpy(path_buf, path, len);
    ret = PASS_THROUGH_SYSCALL(readlinkat, 4, dirfd, path_buf, _buf, bufsize);
    memcpy(buf, _buf, bufsize);

    free_syscall_buffer(path_buf);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(fchmodat, int, dirfd, const char *, pathname, mode_t, mode,
        int, flags)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    char *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);

    ret = PASS_THROUGH_SYSCALL(fchmodat, 4, dirfd, _pathname, mode, flags);
 done:
    free_syscall_buffer(_pathname);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(faccessat, int, dirfd, const char *, filename, int, mode,
        int, flags)
    long ret, size = strlen(filename) + 1;
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, filename, size);
    ret = PASS_THROUGH_SYSCALL(faccessat, 4, dirfd, _buf, mode, flags);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_6(pselect6, int, nfds, fd_set *, readfds, fd_set *, writefds,
      fd_set *, exceptfds, struct timespec *, timeout, void*, __sigmask)
   long ret = -EBUFFUL;
   struct sigmask_struct{
      const sigset_t *ss;
      size_t ss_len;
   };
   struct sigmask_struct *sigmask = __sigmask, *_sigmask = NULL;
   fd_set *_readfds = NULL;
   fd_set *_writefds = NULL;
   fd_set *_exceptfds = NULL;
   struct timespec *_timeout = NULL;

   if (readfds) {
      _readfds = get_syscall_buffer(sizeof(fd_set));
      if (!_readfds)
         goto done;
      *_readfds = *readfds;
   }
   if (writefds) {
      _writefds = get_syscall_buffer(sizeof(fd_set));
      if (!_writefds)
         goto done;
      *_writefds = *writefds;
   }
   if (exceptfds) {
      _exceptfds = get_syscall_buffer(sizeof(fd_set));
      if (!_exceptfds)
         goto done;
      *_exceptfds = *exceptfds;
   }

   if (timeout) {
      _timeout = get_syscall_buffer(sizeof(struct timespec));
      if (!_timeout)
         goto done;
      *_timeout = *timeout;
   }

   if (sigmask) {
      _sigmask = get_syscall_buffer(sizeof(struct sigmask_struct));
      if (!_sigmask)
         goto done;
      _sigmask->ss_len = sigmask->ss_len;
      _sigmask->ss = get_syscall_buffer(sigmask->ss_len);
      if (!_sigmask->ss)
         goto done;
      memcpy(_sigmask->ss, sigmask->ss, sigmask->ss_len);
   }

   ret = PASS_THROUGH_SYSCALL(pselect6, 6, nfds, _readfds, _writefds, _exceptfds, _timeout, _sigmask);

   if (_sigmask)
      free_syscall_buffer(_sigmask->ss);
   if (_timeout)
      *timeout = *_timeout;
   if (_exceptfds)
      *exceptfds = *_exceptfds;
   if (_writefds)
      *writefds = *_writefds;
   if (_readfds)
      *readfds = *_readfds;

done:
   free_syscall_buffer(_sigmask);
   free_syscall_buffer(_timeout);
   free_syscall_buffer(_exceptfds);
   free_syscall_buffer(_writefds);
   free_syscall_buffer(_readfds);
   return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(unshare, int, flags)
    return PASS_THROUGH_SYSCALL(unshare, 1, flags);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_6(splice, int, fd_in, off_t *, off_in, int, fd_out,
        off_t *, off_out, size_t, len, unsigned, flags)
    long ret = -EBUFFUL;
    off_t *_off_in = NULL, *_off_out = NULL;
    if (off_in) {
        _off_in = get_syscall_buffer(sizeof(off_t));
        if (!_off_in) goto done;
        *_off_in = *off_in;
    }
    if (off_out) {
        _off_out = get_syscall_buffer(sizeof(off_t));
        if (!_off_out) goto done;
        *_off_out = *off_out;
    }
    ret = PASS_THROUGH_SYSCALL(splice, 6, fd_in, _off_in, fd_out, _off_out, len,
            flags);
    if (off_in) {
        *off_in = *_off_in;
    }
    if (off_out) {
        *off_out = *_off_out;
    }
 done:
    free_syscall_buffer(_off_in);
    free_syscall_buffer(_off_out);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(tee, int, fd_in, int, fd_out, size_t, len, unsigned, flags)
    return PASS_THROUGH_SYSCALL(tee, 4, fd_in, fd_out, len, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(sync_file_range, int, fd, off_t, offset, off_t, nbytes,
        unsigned int, flags)
    return PASS_THROUGH_SYSCALL(sync_file_range, 4, fd, offset, nbytes, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(epoll_pwait, int, epfd, struct epoll_event *, events,
        int, maxevents, int, timeout, const sigset_t *, sigmask)
    long ret = EBUFFUL;
    unsigned long size = sizeof(struct epoll_event) * maxevents;
    sigset_t *_sigmask;
    void *_events = get_syscall_buffer(size);
    if (!_events) {
        return -EBUFFUL;
    }
    _sigmask = get_syscall_buffer(sizeof(sigset_t));
    if (!_sigmask) goto done;
    *_sigmask = *sigmask;
    memcpy(_events, events, size);
    ret = PASS_THROUGH_SYSCALL(epoll_pwait, 5, epfd, _events, maxevents,
            timeout, _sigmask);
    memcpy(events, _events, size);
 done:
    free_syscall_buffer(_events);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(signalfd, int, fd, const sigset_t *, mask, int, flags)
    long ret = -EBUFFUL;
    sigset_t *_sigmask = get_syscall_buffer(sizeof(sigset_t));
    if (!_sigmask) goto done;
    *_sigmask = *mask;
    ret = PASS_THROUGH_SYSCALL(signalfd, 3, fd, _sigmask, flags);
 done:
    free_syscall_buffer(_sigmask);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(timerfd_create, int, clockid, int, flags)
    return PASS_THROUGH_SYSCALL(timerfd_create, 2, clockid, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(eventfd, int, interval, int, flags)
    return PASS_THROUGH_SYSCALL(eventfd, 2, interval, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(fallocate, int, fd, int, mode, off_t, offset, off_t, len)
    return PASS_THROUGH_SYSCALL(fallocate, 4, fd, mode, offset, len);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(timerfd_settime, int, fd, int, flags,
        const struct itimerspec *, new_value, struct itimerspec *, old_value)
    long ret = -EBUFFUL;
    struct itimerspec *_new_value = NULL;
    struct itimerspec *_old_value = NULL;

    _new_value = get_syscall_buffer(sizeof(struct itimerspec));
    if (!_new_value) goto done;

    if (old_value) {
        _old_value = get_syscall_buffer(sizeof(struct itimerspec));
        if (!_old_value) goto done;
    }

    *_new_value = *new_value;

    ret = PASS_THROUGH_SYSCALL(timerfd_settime, 4, fd, flags, _new_value,
            _old_value);
    if (old_value) {
        *old_value = *_old_value;
    }
 done:
    free_syscall_buffer(_new_value);
    free_syscall_buffer(_old_value);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(timerfd_gettime, int, fd, struct itimerspec *, cur_value)
    long ret = -EBUFFUL;
    struct itimerspec *_cur_value = NULL;

    _cur_value = get_syscall_buffer(sizeof(struct itimerspec));
    if (!_cur_value) goto done;

    ret = PASS_THROUGH_SYSCALL(timer_settime, 2, fd, _cur_value);
    *cur_value = *_cur_value;
    free_syscall_buffer(_cur_value);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(accept4, int, sockfd, struct sockaddr *, addr,
        socklen_t *, addrlen, int, flags)
    long ret;
    void *_addr = NULL;
    socklen_t *_addrlen = NULL;

    if (addr) {
        _addr = get_syscall_buffer(*addrlen);
        if (!_addr) {
            return -EBUFFUL;
        }
        _addrlen = get_syscall_buffer(sizeof(socklen_t));
        if (!_addrlen) {
            free_syscall_buffer(_addr);
            return -EBUFFUL;
        }
        *_addrlen = *addrlen;
    }

    ret = PASS_THROUGH_SYSCALL(accept, 4, sockfd, _addr, _addrlen, flags);
    if (addr) {
        memcpy(addr, _addr, *addrlen);
        *addrlen = *_addrlen;
        free_syscall_buffer(_addr);
        free_syscall_buffer(_addrlen);
    }
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(eventfd2, int, initval, int, flags)
    return PASS_THROUGH_SYSCALL(epoll_create, 2, initval, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(epoll_create1, int, flags)
    return PASS_THROUGH_SYSCALL(epoll_create1, 1, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(dup3, int, oldfd, int, newfd, int, flags)
    return PASS_THROUGH_SYSCALL(dup3, 3, oldfd, newfd, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(pipe2, int *, filedes, int, flags)
    long ret, size = 2 * sizeof(int);
    void *_buf = get_syscall_buffer(size);
    if (!_buf) {
        return -EBUFFUL;
    }
    memcpy(_buf, filedes, size);
    ret = PASS_THROUGH_SYSCALL(pipe2, 2, filedes, flags);
    free_syscall_buffer(_buf);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(inotify_init1, int, flags)
    return PASS_THROUGH_SYSCALL(inotify_init1, 1, flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_4(rt_tgsigqueueinfo, pid_t, tgid, pid_t, tid, int, sig,
        siginfo_t *, uinfo)
    if (!in_sgx)
        return PASS_THROUGH_SYSCALL(rt_sigqueueinfo, 4, tgid, tid, sig, uinfo);
    DIE("rt_tgsigqueueinfo not implemented");
    long ret = -EBUFFUL;
    siginfo_t *_uinfo = get_syscall_buffer(sizeof(siginfo_t));
    if (!_uinfo) goto done;
    *_uinfo = *uinfo;
    ret = PASS_THROUGH_SYSCALL(rt_sigqueueinfo, 4, tgid, tid, sig, _uinfo);
    *uinfo = *_uinfo;
 done:
    free_syscall_buffer(_uinfo);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(perf_event_open, struct perf_event_attr *, attr, pid_t, pid,
        int, cpu, int, group_fd, unsigned long, flags)
    long ret = -EBUFFUL;
    struct perf_event_attr *_attr =
            get_syscall_buffer(sizeof(struct perf_event_attr));
    if (!_attr) goto done;
    *_attr = *attr;
    ret = PASS_THROUGH_SYSCALL(perf_event_open, 5, _attr, pid, cpu, group_fd,
            flags);
    *attr = *_attr;
 done:
    free_syscall_buffer(_attr);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_2(fanotify_init, unsigned int, flags,
        unsigned int, event_f_flags)
    return PASS_THROUGH_SYSCALL(fanotify_init, 2, flags, event_f_flags);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(fanotify_mark, int, fd, unsigned int, flags, uint64_t, mask,
        int, dirfd, const char *, pathname)
    long ret = -EBUFFUL;
    unsigned long len = strlen(pathname) + 1;
    void *_pathname = get_syscall_buffer(len);
    if (!_pathname) goto done;

    memcpy(_pathname, pathname, len);
    ret = PASS_THROUGH_SYSCALL(fanotify_mark, 5, fd, flags, mask, dirfd,
            _pathname);
    free_syscall_buffer(_pathname);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_1(syncfs, int, fd)
    return PASS_THROUGH_SYSCALL(syncfs, 1, fd);
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_4(sendmmsg, int, sockfd, struct mmsghdr *, msgvec,
      unsigned int, vlen, unsigned int, flags)
    return -ENOMEM;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(setns, int, fd, int, nstype)
    return PASS_THROUGH_SYSCALL(setns, 2, fd, nstype);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(getcpu, unsigned *, cpu, unsigned *, node,
        struct getcpu_cache *, cache)
    long ret = -EBUFFUL;
    unsigned *_cpu = NULL, *_node = NULL;

    if (cpu) {
        _cpu = get_syscall_buffer(sizeof(unsigned));
        if (!_cpu) goto done;
    }
    if (node) {
        _node = get_syscall_buffer(sizeof(unsigned));
        if (!_node) goto done;
    }

    ret = PASS_THROUGH_SYSCALL(getcpu, 3, _cpu, _node, cache);

    if (cpu) {
        *cpu = *_cpu;
    }
    if (node) {
        *node = *_node;
    }
 done:
    free_syscall_buffer(_cpu);
    free_syscall_buffer(_node);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(kcmp, pid_t, pid1, pid_t, pid2, int, type,
        unsigned long, idx1, unsigned long, idx2)
    return PASS_THROUGH_SYSCALL(kcmp, 5, pid1, pid2, type, idx1, idx2);
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(finit_module, int, fd, const char *, param_values,
        int, flags)
    long ret = -EBUFFUL;
    unsigned long param_len = strlen(param_values) + 1;
    void *_param_values = get_syscall_buffer(param_len);
    if (!_param_values) goto done;

    memcpy(_param_values, param_values, param_len);
    ret = PASS_THROUGH_SYSCALL(finit_module, 3, fd, _param_values, flags);
    free_syscall_buffer(_param_values);
 done:
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(sched_setattr, pid_t, pid, const struct sched_attr *, attr,
        unsigned int, flags)
    long ret = -EBUFFUL;
    struct sched_attr *_attr;

    _attr = get_syscall_buffer(sizeof(struct sched_attr));
    if (!_attr) goto done;

    *_attr = *attr;
    ret =  PASS_THROUGH_SYSCALL(sched_setattr, 3, pid, _attr, flags);
 done:
    free_syscall_buffer(_attr);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_3(sched_getattr, pid_t, pid, const struct sched_attr *, attr,
        unsigned int, flags)
    long ret = -EBUFFUL;
    struct sched_attr *_attr;

    _attr = get_syscall_buffer(sizeof(struct sched_attr));
    if (!_attr) goto done;

    *_attr = *attr;
    ret =  PASS_THROUGH_SYSCALL(sched_getattr, 3, pid, _attr, flags);
 done:
    free_syscall_buffer(_attr);
    return ret;
SYSCALL_SHIM_END


SYSCALL_SHIM_DEF_5(renameat2, int, olddirfd, const char *, oldpath,
        int, newdirfd, const char *, newpath, unsigned int, flags)
    long ret = -EBUFFUL;
    char *_oldpath = NULL, *_newpath = NULL;
    unsigned long oldlen = strlen(oldpath) + 1;
    unsigned long newlen = strlen(newpath) + 1;

    _oldpath = get_syscall_buffer(oldlen);
    if (!_oldpath) goto done;
    _newpath = get_syscall_buffer(newlen);
    if (!_newpath) goto done;

    memcpy(_oldpath, oldpath, oldlen);
    memcpy(_newpath, newpath, newlen);
    ret =  PASS_THROUGH_SYSCALL(renameat2, 5, olddirfd, _oldpath, newdirfd,
            _newpath, flags);
 done:
    free_syscall_buffer(_oldpath);
    free_syscall_buffer(_newpath);
    return ret;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_3(getrandom, uint8_t *, buf, size_t, len, unsigned int, flags)
    unsigned long ret;
    size_t total_len = len;
    int i = 0;
    if (no_rdrand) {
        uint8_t *buffer = get_syscall_buffer(len);
        if (!buffer) return -EBUFFUL;
        ret = PASS_THROUGH_SYSCALL(getrandom, 3, buffer, len, flags);
        memcpy(buf, buffer, len);
        free_syscall_buffer(buffer);
        return ret;
    }

    for (; len >= 8; len -= 8, i += 8) {
        ret = __builtin_ia32_rdrand64_step((long long unsigned int*)(buf + i));
        if (!ret) DIE("HW randomnes failed us");
    }
    for (; len >= 4; len -= 4, i += 4) {
        ret = __builtin_ia32_rdrand32_step((unsigned int*)(buf + i));
        if (!ret) DIE("HW randomnes failed us");
    }
    for (; len >= 2; len -= 2, i += 2) {
        ret = __builtin_ia32_rdrand16_step((short unsigned int*)(buf + i));
        if (!ret) DIE("HW randomnes failed us");
    }
    if (len) {
        short unsigned int tmp;
        if (len != 1) DIE("IMPOSSIBLE");
        ret = __builtin_ia32_rdrand16_step(&tmp);
        if (!ret) DIE("HW randomnes failed us");
        buf[i] = (uint8_t)tmp;
    }
    return total_len;
SYSCALL_SHIM_END

SYSCALL_SHIM_DEF_2(lockmprotect, void *, start, size_t, len)
   mprotect_region_start = start;
   mprotect_region_len = len;
   return 0;
SYSCALL_SHIM_END

#if 0
SYSCALL_SHIM_DEF_3(nodelaymprotect, unsigned long, start, size_t, len,
        unsigned long, prot)
   return SGX_PASS_THROUGH_SYSCALL_NODELAY(nodelaymprotect, 3, start, len, prot);
SYSCALL_SHIM_END
#endif
