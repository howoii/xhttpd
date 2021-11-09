//
// Created by Haiwei Zhang on 2021/10/25.
//
#include "response.h"

static void finish_write_response(struct conn *c) {
    log_debug(__func__ );

    struct request *r = c->data;
    if (conn_buff_flush(c) != 0)
        return;

    conn_close(c);
}

static void send_response_file(struct conn *c) {
    log_debug(__func__ );

    struct request *r = c->data;
    if (conn_buff_flush(c) != 0)
        return;

    r->fs.len = 0;
    int ret = conn_send_file(&r->fs, c);
    if (ret == -1 || (ret == EAGAIN && r->fs.seek < r->res_body_len))
        return;

    conn_close(c);
}

static void write_response_body(struct conn *c) {
    log_debug(__func__ );

    struct request *r = c->data;
    if (r->res_body_len > r->res_body_curr) {
        int buff_len = r->res_body_len-r->res_body_curr;
        int ret = conn_buff_append_data(c, r->response_body+r->res_body_curr, buff_len);
        if (ret == -1)
            return;
        if (ret < buff_len) {
            r->res_body_curr += ret;
            return;
        }
    }

    c->write_callback = finish_write_response;
    finish_write_response(c);
}

static void write_response_headers(struct conn *c) {
    log_debug(__func__ );

    struct request *r = c->data;
    for (int i = r->res_header_curr; i < r->res_header_len; ++i) {
        snprintf(r->line_buf, LINE_SIZE, "%s: %s\r\n", r->res_headers[i].key, r->res_headers[i].value);
        if (conn_buff_append_line(c, r->line_buf, strlen(r->line_buf)) != 0){
            return;
        }
        r->res_header_curr = i+1;
    }

    if (conn_buff_append_line(c, EMPTY_LINE, strlen(EMPTY_LINE)) != 0){
        return;
    }

    if (r->fs.fd > 0) {
        c->write_callback = send_response_file;
        send_response_file(c);
    }else{
        c->write_callback = write_response_body;
        write_response_body(c);
    }
}

static void write_response_line(struct conn *c) {
    log_debug(__func__ );

    struct request *r = c->data;
    snprintf(r->line_buf, LINE_SIZE, "%s %s\r\n", HTTP_VER, status_text(r->status_code));
    if (conn_buff_append_line(c, r->line_buf, strlen(r->line_buf)) != 0)
        return;

    c->write_callback = write_response_headers;
    write_response_headers(c);
}

static void set_common_response_header(struct request *r) {
    response_set_header(r, "Server", "xHttp/1.0");
}

void response_set_header(struct request *r, char *name, char *value) {
    if (r->res_header_len == HEADER_SIZE)
        return;

    struct header *h = &r->res_headers[r->res_header_len++];
    h->key = alloc_copy_string(name);
    h->value = alloc_copy_string(value);
}

void response(struct request *r) {
    // set headers
    set_common_response_header(r);
    if (r->status_code >= 200 && r->status_code != HTTP_NO_CONTENT)
        response_set_header(r, "Content-Length", int_to_string(r->res_body_len));

    // close read callback
    r->c->read_callback = NULL;
    r->c->write_callback = write_response_line;
    conn_listen(r->c, EVENT_WRITE);
}

void response_error(struct request *r, int code) {
    r->status_code = code;
    response(r);
}

void response_body(struct request *r, char *body, int body_len) {
    if (r->response_body) {
        log_warn("response body ignored since body already exist");
    } else {
        r->response_body = alloc_copy_nstring(body, body_len);
        r->res_body_len = body_len;
    }
    response(r);
}

void response_file(struct request *r, char *filename, struct stat st) {
    log_debugf(__func__ , "response file: %s", filename);

    char *ext = strrchr(filename, '.');
    response_set_header(r, "Content-Type", ext_to_content_type(ext));
    if (st.st_size == 0) {
        response(r);
        return;
    }
    int fd = open(filename, O_RDONLY|O_NONBLOCK);
    if (fd == -1) {
        log_warn(strerror(errno));
        r->status_code = HTTP_NOT_FOUND;
    }
    r->fs.fd = fd;
    r->res_body_len = st.st_size;
    response(r);
}