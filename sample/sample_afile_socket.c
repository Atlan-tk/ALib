#include <alib/afile.h>
#include <alib/athrd.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SERVER_READY = 1,
    SERVER_FAILED = -1,
};

typedef struct {
    mtx_t mutex;
    cnd_t cond;
    int server_state;
} SocketTestState;

static void fatal_if_exception(const char* phase) {
    if(aExcOccur()) {
        fprintf(stderr, "%s failed: exception=%d\n", phase, aExcGet());
        exit(EXIT_FAILURE);
    }
}

static void notify_server_state(SocketTestState* state, int server_state) {
    assert(mtx_lock(&state->mutex) == thrd_success);
    state->server_state = server_state;
    assert(cnd_broadcast(&state->cond) == thrd_success);
    assert(mtx_unlock(&state->mutex) == thrd_success);
}

static int server_thread(void* arg) {
    SocketTestState* state = arg;
    char buf[32] = {0};

    RAII(AFile) server = aSocketTcpServerOpen("127.0.0.1", 8290);
    if(aExcOccur()) {
        fprintf(stderr, "server open failed: exception=%d\n", aExcGet());
        aExcClean();
        notify_server_state(state, SERVER_FAILED);
        return 1;
    }

    notify_server_state(state, SERVER_READY);
    printf("[server] listening on 127.0.0.1:8290\n");

    RAII(AFile) client = aSocketTcpAccept(server);
    fatal_if_exception("server accept");
    printf("[server] client connected\n");

    for(;;) {
        memset(buf, 0, sizeof(buf));
        uint32_t read_len = client.f->read(&client, strlen("hello"), buf);
        fatal_if_exception("server read");

        if(read_len == 0) {
            fprintf(stderr, "[server] client closed\n");
            return 1;
        }

        printf("[server] recv: %.*s\n", (int)read_len, buf);
        assert(read_len == strlen("hello"));
        assert(memcmp(buf, "hello", strlen("hello")) == 0);

        uint32_t write_len = client.f->write(&client, strlen("yes"), "yes");
        fatal_if_exception("server write");
        assert(write_len == strlen("yes"));
        printf("[server] send: yes\n");
    }

    return 0;
}

static int client_thread(void* arg) {
    SocketTestState* state = arg;
    char buf[32] = {0};

    assert(mtx_lock(&state->mutex) == thrd_success);
    while(state->server_state == 0) {
        assert(cnd_wait(&state->cond, &state->mutex) == thrd_success);
    }
    int server_state = state->server_state;
    assert(mtx_unlock(&state->mutex) == thrd_success);

    if(server_state == SERVER_FAILED) {
        return 1;
    }

    RAII(AFile) client = aSocketTcpClientOpen("127.0.0.1", 8290);
    fatal_if_exception("client open");
    printf("[client] connected to 127.0.0.1:8290\n");

    for(;;) {
        uint32_t write_len = client.f->write(&client, strlen("hello"), "hello");
        fatal_if_exception("client write");
        assert(write_len == strlen("hello"));
        printf("[client] send: hello\n");

        memset(buf, 0, sizeof(buf));
        uint32_t read_len = client.f->read(&client, strlen("yes"), buf);
        fatal_if_exception("client read");

        if(read_len == 0) {
            fprintf(stderr, "[client] server closed\n");
            return 1;
        }

        printf("[client] recv: %.*s\n", (int)read_len, buf);
        assert(read_len == strlen("yes"));
        assert(memcmp(buf, "yes", strlen("yes")) == 0);
    }

    return 0;
}

int main(void) {
    SocketTestState state = {0};
    thrd_t server = {0};
    thrd_t client = {0};
    int server_ret = 0;
    int client_ret = 0;

    signal(SIGPIPE, SIG_IGN);

    assert(mtx_init(&state.mutex, mtx_plain) == thrd_success);
    assert(cnd_init(&state.cond) == thrd_success);

    assert(thrd_create(&server, server_thread, &state) == thrd_success);
    assert(thrd_create(&client, client_thread, &state) == thrd_success);

    assert(thrd_join(client, &client_ret) == thrd_success);
    assert(thrd_join(server, &server_ret) == thrd_success);

    cnd_destroy(&state.cond);
    mtx_destroy(&state.mutex);

    return client_ret == 0 && server_ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
