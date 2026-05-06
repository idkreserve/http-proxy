#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include "panic.h"

int panic(int flags, int code, char *format, ...)
{
  int saved_errno = errno;
  va_list ap;

  fputs("PANIC: ", stderr);

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  if (flags & PANIC_PERROR) {
      if (saved_errno) {
          fputs(": ", stderr);
          errno = saved_errno;
          perror(NULL);
      } else
          fputs(": (errno is not set, perror skipped)\n", stderr);
  }

  if (flags & PANIC_EXIT)
      exit(code);

  return code;
}