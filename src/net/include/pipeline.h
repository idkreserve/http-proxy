#ifndef PIPELINE_H
#define PIPELINE_H

#include "http.h"

void pipeline(const int clientfd, const int remotefd);
void http_pipeline(const int clientfd, const int remotefd, struct http *tr);

#endif
