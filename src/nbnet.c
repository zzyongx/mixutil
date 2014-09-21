#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>

#include <pth.h>
#include <mixutil/index_hash.h>

#define yield() pth_yield(NULL)
#define self()  pth_self()

#define sysfd(fd)       \
  (nbnet_block_flag ||  \
   index_hash_get(nbnet_fdtbl, fd) == INDEX_HASH_VAL_NIL)

index_hash_t *nbnet_fdtbl = 0;
int nbnet_fds_size()
{
  return index_hash_size(nbnet_fdtbl);
}

int *nbnet_fds(int *fds)
{
  return (int *) index_hash_keys(nbnet_fdtbl, (uint32_t *)fds);
}

int nbnet_block_flag = 0;
int nbnet_block_ctl(int block)
{
  int old = nbnet_block_flag;
  nbnet_block_flag = block;
  return old;
}

int (*sys_connect)(int, const struct sockaddr*, socklen_t) = 0;

ssize_t (*sys_read)(int, void*, size_t) = 0;
ssize_t (*sys_readv)(int, const struct iovec*, int) = 0;

ssize_t (*sys_recv)(int, void*, size_t, int) = 0;
ssize_t (*sys_recvfrom)(int, void*, size_t, int, struct sockaddr*, socklen_t*) = 0;
ssize_t (*sys_recvmsg)(int, struct msghdr*, int) = 0;

ssize_t (*sys_write)(int, const void*, size_t) = 0;
ssize_t (*sys_writev)(int, const struct iovec *, int) = 0;

ssize_t (*sys_send)(int, const void*, size_t, int) = 0;
ssize_t (*sys_sendto)(int, const void*, size_t, int, const struct sockaddr*, socklen_t) = 0;
ssize_t (*sys_sendmsg)(int, const struct msghdr*, int) = 0;

int (*sys_close)(int) = 0;

int nbnet_init() __attribute__((constructor));
int nbnet_init()
{
  nbnet_fdtbl = index_hash_init(256);
  
  void *handle = dlopen("libc.so.6", RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "dlopen(libc.so.6) failed, %s", dlerror());
    exit(-1);
  }

  struct { const char *fun; void **pt; } symtbl[] = {
    {"connect",  (void **) &sys_connect},
    
    {"read",     (void **) &sys_read},
    {"readv",    (void **) &sys_readv},
    
    {"recv",     (void **) &sys_recv},
    {"recvfrom", (void **) &sys_recvfrom},
    {"recvmsg",  (void **) &sys_recvmsg},

    {"write",    (void **) &sys_write},
    {"writev",   (void **) &sys_writev},
    
    {"send",     (void **) &sys_send},
    {"sendto",   (void **) &sys_sendto},
    {"sendmsg",  (void **) &sys_sendmsg},

    {"close",    (void **) &sys_close},
    {NULL, NULL}
  };

  for (size_t i = 0; symtbl[i].fun; ++i) {
    if (!(*symtbl[i].pt = dlsym(handle, symtbl[i].fun))) {
      fprintf(stderr, "dlsym(%s) failed, %s", symtbl[i].fun, dlerror());
      exit(-1);
    }
  };

  return 1;
}  

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  if (nbnet_block_flag == 0) {
    int flag = 1;
    index_hash_put(nbnet_fdtbl, sockfd, (uintptr_t) self());
    ioctl(sockfd, FIONBIO, &flag);
  }

  int rc = sys_connect(sockfd, addr, addrlen);
  if (rc != 0) {
    if (errno != EINPROGRESS) {
      return rc;
    }
  } else {
    return rc;
  }

  yield();

  for ( ;; ) {
    struct pollfd fds[] = {
      {sockfd, POLLOUT | POLLIN, 0}
    };

    int n = poll(fds, 1, 0);
    if (n > 0) {
      int err = 0;
      socklen_t len = sizeof(err);

      /* BSDs and Linux return 0, and set a pending error in err
       * Solaris return -1, and sets errno */
      if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *) &err, &len) == -1) {
        err = errno;
      }

      if (err) {
        errno = err;
        return -1;
      } else {
        return 0;
      }
    } else if ( n < 0) {
      return -1;
    }
    
    yield();
  }
}

ssize_t read(int fd, void *buf, size_t count)
{
  if (sysfd(fd)) {
    return sys_read(fd, buf, count);
  }

  for ( ;; ) {
    ssize_t nn = sys_read(fd, buf, count);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }
    
    yield();
  }
}
      
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
  if (sysfd(fd)) {
    return sys_readv(fd, iov, iovcnt);
  }

  for ( ;; ) {
    ssize_t nn = sys_readv(fd, iov, iovcnt);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
  if (sysfd(sockfd)) {
    return sys_recv(sockfd, buf, len, flags);
  }

  for ( ;; ) {
    ssize_t nn = sys_recv(sockfd, buf, len, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
  if (sysfd(sockfd)) {
    return sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
  }
  
  for ( ;; ) {
    ssize_t nn = sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
  if (sysfd(sockfd)) {
    return sys_recvmsg(sockfd, msg, flags);
  }
  
  for ( ;; ) {
    ssize_t nn = sys_recvmsg(sockfd, msg, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

      
ssize_t write(int fd, const void *buf, size_t count)
{
  if (sysfd(fd)) {
    return sys_write(fd, buf, count);
  }

  for ( ;; ) {
    ssize_t nn = sys_write(fd, buf, count);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
  if (sysfd(fd)) {
    return sys_writev(fd, iov, iovcnt);
  }

  for ( ;; ) {
    ssize_t nn = sys_writev(fd, iov, iovcnt);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
  if (sysfd(sockfd)) {
    return sys_send(sockfd, buf, len, flags);
  }

  for ( ;; ) {
    ssize_t nn = sys_send(sockfd, buf, len, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
  if (sysfd(sockfd)) {
    return sys_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
  }

  for ( ;; ) {
    ssize_t nn = sys_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    if (nn <  0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  if (sysfd(sockfd)) {
    return sys_sendmsg(sockfd, msg, flags);
  }

  for ( ;; ) {
    ssize_t nn = sys_sendmsg(sockfd, msg, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }

    yield();
  }
}

int close(int fd)
{
  index_hash_del(nbnet_fdtbl, fd);
  return sys_close(fd);
}
