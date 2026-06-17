#include <alib/afile.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void on_timeout(int sig) {
    (void)sig;
    fprintf(stderr, "test_afile_domain timed out\n");
    _exit(124);
}

static void expect_clean(const char* phase) {
    if(aErrOccur()) {
        fprintf(stderr, "%s failed: exception=%d\n", phase, aErrGet());
    }
    assert(!aErrOccur());
}

static void test_example_com_tcp(void) {
    char buf[1024] = {0};
    const char request[] =
        "HEAD / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    RAII(AFile) client = aSocketTcpClientOpen("example.com", 80);
    expect_clean("example.com tcp open");

    uint32_t write_len = client.f->write(&client, sizeof(request) - 1, (void*)request);
    expect_clean("example.com tcp write");
    assert(write_len == sizeof(request) - 1);

    uint32_t read_len = client.f->read(&client, sizeof(buf) - 1, buf);
    expect_clean("example.com tcp read");
    assert(read_len > 0);
    assert(strncmp(buf, "HTTP/", sizeof("HTTP/") - 1) == 0);
}

static void test_opendns_udp(void) {
    RAII(AFile) client = aSocketUdpClientOpen("resolver1.opendns.com", 53);
    expect_clean("resolver1.opendns.com udp open");
    assert(Afd_exist(client.fd));
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGALRM, on_timeout);
    signal(SIGPIPE, SIG_IGN);
    alarm(30);

    test_example_com_tcp();
    test_opendns_udp();

    printf("AFile domain parsing test passed.\n");
    return EXIT_SUCCESS;
}
