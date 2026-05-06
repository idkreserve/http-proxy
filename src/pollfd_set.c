#include <stdlib.h>
#include <poll.h>
#include "pollfd_set.h"

struct pollfd *fd_add(struct pollfd_set *set, int fd, short events, short revents)
{
    size_t new_capacity;
    struct pollfd *tmp;

    if (set->len == set->capacity) {
        new_capacity = set->capacity ? set->capacity * 2 : DEFAULT_CAPACITY;
        if ((tmp = realloc(set->data, new_capacity * sizeof *set->data)) == NULL)
            return NULL;
        set->data = tmp;
        set->capacity = new_capacity;
    }
    set->data[set->len] = (struct pollfd) {
        .fd = fd, .events = events, .revents = revents,
    };
    return set->data + set->len++;
}

struct pollfd *fd_isset(struct pollfd_set *set, struct pollfd *v)
{
    for (size_t p = 0; p < set->len; p++)
        if (set->data + p == v)
            return set->data + p;
    return NULL;
}

void fd_clr(struct pollfd_set *set, struct pollfd *v)
{
    struct pollfd *p;

    if ((p = fd_isset(set, v)) == NULL) return;

    for (size_t i = p - set->data; i < set->len - 1; i++) {
        set->data[i] = set->data[i + 1];
    }

    set->len--;
}