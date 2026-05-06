#ifndef POLLFD_SET_H
#define POLLFD_SET_H

#include <stdlib.h>
#include <poll.h>

#define DEFAULT_CAPACITY 10

struct pollfd_set {
    size_t len;
    size_t capacity;
    struct pollfd *data;
};

#define fd_init (struct pollfd_set) {0}

struct pollfd *fd_add(struct pollfd_set *set, int fd, short events, short revents);
struct pollfd *fd_isset(struct pollfd_set *set, struct pollfd *v);
void fd_clr(struct pollfd_set *set, struct pollfd *v);

#endif