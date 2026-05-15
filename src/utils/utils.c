#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <wait.h>
#include "utils.h"

void sigchild_handler(int)
{
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}
