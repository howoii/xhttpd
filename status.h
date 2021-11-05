//
// Created by Haiwei Zhang on 2021/10/25.
//

#ifndef XHTTPD_STATUS_H
#define XHTTPD_STATUS_H

// when add new code should change function @status_text together
enum http_code {
    HTTP_OK                 = 200,
    HTTP_BAD_REQUEST        = 401,
    HTTP_NOT_FOUND          = 404,
    HTTP_METHOD_NOT_ALLOW   = 405, // should return allowed methods
    HTTP_INTERNAL_ERROR     = 500,
};

#define HTTP_GET        (1<<0)
#define HTTP_POST       (1<<1)

char* status_text(enum http_code code);

#endif //XHTTPD_STATUS_H
