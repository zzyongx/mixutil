/* author: iamzhengzhiyong@gmail.com
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <mixutil/nbnet.h>

#define self()    pth_self()

#define sysfd(ptr, fd)            \
  ((nbnet_mode == NBNET_BLOCK) || \
   ((ptr = (void *)index_hash_get(nbnet_fdtbl, fd)) == (void *)INDEX_HASH_VAL_NIL))

#define yield(ptr, ev) do {                \
  if (NBNET_AUTO_NONBLOCK == nbnet_mode) { \
    pth_yield(NULL);                       \
  } else {                                 \
    ptr->event |= (ev);                    \
    pth_yield(NULL);                       \
  }                                        \
} while (0)

#define yield_read(ptr) yield(ptr, NBNET_EVENT_READ)
#define yield_write(ptr) yield(ptr, NBNET_EVENT_WRITE)

index_hash_t *nbnet_fdtbl = 0;
typedef struct {
  uint32_t event;
  pth_t    tid;
} nbnet_io_event_part;

size_t nbnet_events_size()
{
  return index_hash_size(nbnet_fdtbl);
}

struct nbnet_io_event *nbnet_events(struct nbnet_io_event *events)
{
  uint32_t i = 0;
  for (index_hash_ite ite = index_hash_begin(nbnet_fdtbl);
       ite != index_hash_end(nbnet_fdtbl);
       ite = index_hash_next(nbnet_fdtbl, ite)) {
    events[i].fd = index_hash_ite_key(nbnet_fdtbl, ite);
    nbnet_io_event_part *val =
      (nbnet_io_event_part *) index_hash_ite_val(nbnet_fdtbl, ite);
    events[i].event = val->event;
    events[i].tid   = val->tid;
    i++;
  }
  return events; 
}

int nbnet_mode = NBNET_AUTO_NONBLOCK;
int nbnet_mode_ctl(int mode)
{
  int old = nbnet_mode;
  nbnet_mode = mode;
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

/* BSDs and Linux return 0, and set a pending error in err
 * Solaris return -1, and sets errno */
#define CONNECT_FINISH(sockfd)  do {  \
  int err = 0;                        \
  socklen_t len = sizeof(err);        \
  if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *) &err, &len) == -1) { \
    err = errno;                      \
  }                                   \
  if (err) {                          \
    errno = err;                      \
    return -1;                        \
  } else {                            \
    return 0;                         \
  }                                   \
} while (0)

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  nbnet_io_event_part *ptr = 0;  
  if (nbnet_mode != NBNET_BLOCK) {
    int flag = 1;
    ptr = calloc(1, sizeof(nbnet_io_event_part));
    ptr->tid = self();
    index_hash_put(nbnet_fdtbl, sockfd, (uintptr_t) ptr);
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

  if (nbnet_mode == NBNET_AUTO_NONBLOCK) {
    pth_yield(NULL);

    for ( ;; ) {
      struct pollfd fds[] = {
        {sockfd, POLLOUT | POLLIN, 0}
      };

      int n = poll(fds, 1, 0);
      if (n > 0) {
        CONNECT_FINISH(sockfd);
      } else if ( n < 0) {
        return -1;
      }
    
      pth_yield(NULL);
    }
  } else {
    yield(ptr, NBNET_EVENT_READ | NBNET_EVENT_WRITE);
    ptr->event = 0;
    CONNECT_FINISH(sockfd);
  }    
}

ssize_t read(int fd, void *buf, size_t count)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, fd)) {
    return sys_read(fd, buf, count);
  }

  for ( ;; ) {
    ssize_t nn = sys_read(fd, buf, count);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_read(ptr);
  }
}
      
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, fd)) {
    return sys_readv(fd, iov, iovcnt);
  }

  for ( ;; ) {
    ssize_t nn = sys_readv(fd, iov, iovcnt);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_read(ptr);
  }
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, sockfd)) {
    return sys_recv(sockfd, buf, len, flags);
  }

  for ( ;; ) {
    ssize_t nn = sys_recv(sockfd, buf, len, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_read(ptr);
  }
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, sockfd)) {
    return sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
  }
  
  for ( ;; ) {
    ssize_t nn = sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_read(ptr);
  }
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, sockfd)) {
    return sys_recvmsg(sockfd, msg, flags);
  }
  
  for ( ;; ) {
    ssize_t nn = sys_recvmsg(sockfd, msg, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_read(ptr);
  }
}

      
ssize_t write(int fd, const void *buf, size_t count)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, fd)) {
    return sys_write(fd, buf, count);
  }

  for ( ;; ) {
    ssize_t nn = sys_write(fd, buf, count);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_write(ptr);
  }
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, fd)) {
    return sys_writev(fd, iov, iovcnt);
  }

  for ( ;; ) {
    ssize_t nn = sys_writev(fd, iov, iovcnt);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_write(ptr);
  }
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, sockfd)) {
    return sys_send(sockfd, buf, len, flags);
  }

  for ( ;; ) {
    ssize_t nn = sys_send(sockfd, buf, len, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_write(ptr);
  }
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, sockfd)) {
    return sys_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
  }

  for ( ;; ) {
    ssize_t nn = sys_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    if (nn <  0) {
      if (errno != EAGAIN) return nn;
    } else {
      ptr->event = 0;
      return nn;
    }
    yield_write(ptr);
  }
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  nbnet_io_event_part *ptr;
  if (sysfd(ptr, sockfd)) {
    return sys_sendmsg(sockfd, msg, flags);
  }

  for ( ;; ) {
    ssize_t nn = sys_sendmsg(sockfd, msg, flags);
    if (nn < 0) {
      if (errno != EAGAIN) return nn;
    } else {
      return nn;
    }
    yield_write(ptr);
  }
}

int close(int fd)
{
  nbnet_io_event_part *ptr = (void *) index_hash_del(nbnet_fdtbl, fd);
  if (ptr != (void *) INDEX_HASH_VAL_NIL) {
    free(ptr);
  }
  
  return sys_close(fd);
}
