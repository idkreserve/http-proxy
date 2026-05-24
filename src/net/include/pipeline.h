#ifndef PIPELINE_H
#define PIPELINE_H

#include "http.h"

void pipeline(const int clientfd, const int remotefd, const char *c2s_append, const size_t c2s_appendlen);

#endif
