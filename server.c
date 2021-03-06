#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <uv.h>

// uv_handle_t > uv_stream_t > uv_tcp_t

const int backlog = 128;
const int buffer_size = 1024;
uv_fs_t open_req;
uv_fs_t read_req;
uv_tcp_t *client;

void on_new_connection(uv_stream_t *server, int status);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void on_client_read(uv_stream_t *client, ssize_t nread, uv_buf_t *buf);
void on_client_write(uv_write_t *req, int status);
void on_file_open(uv_fs_t *req);
void on_file_read(uv_fs_t *req);

void on_new_connection(uv_stream_t *server, int status) {
    fprintf(stderr, "on_new_connection\n");
    if (status == -1) {
        fprintf(stderr, "error on_new_connection");
        uv_close((uv_handle_t*) client, NULL);
        return;
    }

    client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), client);

    // accept
    int result = uv_accept(server, (uv_stream_t*) client);

    if (result == 0) { // success
        uv_read_start((uv_stream_t*) client, alloc_buffer, on_client_read);
    } else { // error
        uv_close((uv_handle_t*) client, NULL);
    }
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    fprintf(stderr, "alloc_buffer\n");
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}


int i = 1;
//static void f1(struct uv__work* w) {
void f1(uv_work_t* w) {
    fprintf(stderr, "f1\n");
    if (i>0) {
        i--;
        sleep(10);
    }
}

//static void f2(struct uv__work* w, int status) {
void f2(uv_work_t* w, int status) {
    fprintf(stderr, "f2\n");
    uv_write_t *write_req = (uv_write_t *) malloc(sizeof(uv_write_t));

    char *message = (char *)malloc(6);
    snprintf(message, 6, "helle");

    uv_buf_t buf = uv_buf_init(message, sizeof(message));
    buf.len = 6;
    buf.base = message;
    int buf_count = 1;

    write_req->data = (void*) message;

    uv_write(write_req, (uv_stream_t*) client, &buf, buf_count, on_client_write);

}

void dispatch() {
    //struct uv__work w;
    //uv__work_submit(uv_default_loop(), &w, f1, f2);
    //int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb)
    uv_work_t w;
    uv_queue_work(uv_default_loop(), &w, f1, f2);
}

void on_client_read(uv_stream_t *_client, ssize_t nread, uv_buf_t *buf) {
    fprintf(stderr, "on_client_read\n");
    // other client can not be accepted again
    // sleep(3600);
    if (nread == -1) {
        fprintf(stderr, "error on_client_read");
        uv_close((uv_handle_t*) client, NULL);
        return;
    }

    char *filename = buf->base;
    int mode = 0;

    //test threadpool
    dispatch();
    return;

    uv_fs_open(uv_default_loop(), &open_req, filename, O_RDONLY, mode, on_file_open);
}

void on_client_write(uv_write_t *req, int status) {
    fprintf(stderr, "on_client_write\n");
    if (status == -1) {
        fprintf(stderr, "error on_client_write");
        uv_close((uv_handle_t*) client, NULL);
        return;
    }

    char *buffer = (char*) req->data;
    free(buffer);
    free(req);

    uv_close((uv_handle_t*) client, NULL);
}

void on_file_open(uv_fs_t *req) {
    fprintf(stderr, "on_file_open\n");
    if (req->result == -1) {
        fprintf(stderr, "error on_file_open");
        uv_close((uv_handle_t*) client, NULL);
        return;
    }

    char *buffer = (char *) malloc(sizeof(char) * buffer_size);

    int offset = -1;
    read_req.data = (void*) buffer;

    uv_buf_t iov = uv_buf_init(buffer, buffer_size);
    uv_fs_read(uv_default_loop(), &read_req, req->result, &iov, 1, -1, on_file_read);
    uv_fs_req_cleanup(req);
}

void on_file_read(uv_fs_t *req) {
    fprintf(stderr, "on_file_read\n");
    // req->result: data length
    if (req->result < 0) {
        fprintf(stderr, "error on_file_read: %s\n", strerror(-req->result));
        uv_close((uv_handle_t*) client, NULL);
    } else if (req->result == 0) {
        uv_fs_t close_req;
        uv_fs_close(uv_default_loop(), &close_req, open_req.result, NULL);
        uv_close((uv_handle_t*) client, NULL);
    } else {
        uv_write_t *write_req = (uv_write_t *) malloc(sizeof(uv_write_t));

        char *message = (char*) req->data;

        uv_buf_t buf = uv_buf_init(message, sizeof(message));
        buf.len = req->result;
        buf.base = message;
        int buf_count = 1;

        write_req->data = (void*) message;

        uv_write(write_req, (uv_stream_t*) client, &buf, buf_count, on_client_write);
    }
    uv_fs_req_cleanup(req);
}

int main(void) {
    uv_tcp_t server;
    uv_tcp_init(uv_default_loop(), &server);
    struct sockaddr_in bind_addr;
    uv_ip4_addr("0.0.0.0", 7000, &bind_addr);

    // bind
    uv_tcp_bind(&server, &bind_addr, 0);

    // listen
    int r = uv_listen((uv_stream_t*) &server, backlog, on_new_connection);
    if (r) {
        fprintf(stderr, "error uv_listen");
        return 1;
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    return 0;
}
