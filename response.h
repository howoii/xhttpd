//
// Created by Haiwei Zhang on 2021/10/25.
//

#ifndef XHTTPD_RESPONSE_H
#define XHTTPD_RESPONSE_H

#include "common.h"
#include "request.h"
#include "status.h"
#include "util.h"

#define EMPTY_LINE  "\r\n"

// declaration, defined in request.h
struct request;

void response_set_header(struct request *r, char *name, char *value);

void response(struct request *r);
void response_error(struct request *r, int code);
void response_body(struct request *r, char *body, int body_len);

#endif //XHTTPD_RESPONSE_H
