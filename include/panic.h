#ifndef PANIC_H
#define PANIC_H

#define PANIC_NO_PERROR 0x00
#define PANIC_PERROR    0x01
#define PANIC_EXIT      0x02

int panic(int flags, int code, char *format, ...);

#endif