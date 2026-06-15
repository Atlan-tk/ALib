#include <alib/afile.h>

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void on_timeout(int sig) {
    (void)sig;
    fprintf(stderr, "test_afile_open_write timed out\n");
    _exit(124);
}

static void expect_clean_at(const char* phase, int iter) {
    if(aErrOccur()) {
        fprintf(stderr, "[afile-open-write] iter=%d phase=%s exception=%d\n",
                iter, phase, aErrGet());
    }
    assert(!aErrOccur());
}

static void make_path(char* out, size_t out_size, const char* dir, int iter) {
    int n = snprintf(out, out_size, "%s/open-write-%04d.txt", dir, iter);
    assert(n > 0 && (size_t)n < out_size);
}

static void assert_file_text(const char* path, const char* expected) {
    char buf[128] = {0};
    FILE* fp = fopen(path, "rb");
    assert(fp != NULL);
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    assert(ferror(fp) == 0);
    assert(fclose(fp) == 0);
    assert(n == strlen(expected));
    assert(strcmp(buf, expected) == 0);
}

static void test_wrappers_once(const char* path, int iter) {
    {
        RAII(AFile) out = aFileOutOpen(path);
        expect_clean_at("aFileOutOpen", iter);
        assert(out.fd >= 0);
        assert(out.f->write(&out, 5, "hello") == 5);
        expect_clean_at("write hello", iter);
    }
    expect_clean_at("close out", iter);
    assert_file_text(path, "hello");

    {
        RAII(AStr) abs = af_path_absolute(path);
        expect_clean_at("absolute append", iter);

        RAII(AFile) end = A_INIT(AFile);
        AFile_open(&end, __aftype_file, abs,
                   __afmod_write | __afmod_appent | __afmod_creat);
        expect_clean_at("AFile_open append", iter);
        assert(end.fd >= 0);
        assert(end.f->write(&end, 2, "++") == 2);
        expect_clean_at("append ++", iter);
    }
    expect_clean_at("close append", iter);
    assert_file_text(path, "hello++");

    {
        char buf[16] = {0};
        RAII(AFile) in = aFileInOpen(path);
        expect_clean_at("aFileInOpen", iter);
        assert(in.fd >= 0);
        assert(in.f->read(&in, sizeof(buf), buf) == 7);
        expect_clean_at("read full", iter);
        assert(strcmp(buf, "hello++") == 0);
    }
    expect_clean_at("close input", iter);
}

static void test_direct_open_once(const char* path, int iter) {
    {
        RAII(AStr) abs = af_path_absolute(path);
        expect_clean_at("absolute direct", iter);

        RAII(AFile) file = A_INIT(AFile);
        AFile_open(&file, __aftype_file, abs,
                   __afmod_write | __afmod_creat | __afmod_truncate | __afmod_exclusive);
        expect_clean_at("AFile_open direct write", iter);
        assert(file.fd >= 0);
        assert(file.f->write(&file, 1, "Z") == 1);
        expect_clean_at("direct write Z", iter);
    }
    expect_clean_at("close direct write", iter);
    assert_file_text(path, "Z");

    {
        RAII(AStr) abs = af_path_absolute(path);
        expect_clean_at("absolute read", iter);

        char buf[4] = {0};
        RAII(AFile) file = A_INIT(AFile);
        AFile_open(&file, __aftype_file, abs, __afmod_read);
        expect_clean_at("AFile_open direct read", iter);
        assert(file.fd >= 0);
        assert(file.f->read(&file, sizeof(buf), buf) == 1);
        expect_clean_at("direct read Z", iter);
        assert(strcmp(buf, "Z") == 0);
    }
    expect_clean_at("close direct read", iter);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGALRM, on_timeout);
    alarm(60);

    char tmpl[] = "/tmp/alib-afile-open-write-XXXXXX";
    char* root = mkdtemp(tmpl);
    assert(root != NULL);

    for(int i = 0; i < 1000; i++) {
        char path[PATH_MAX];
        make_path(path, sizeof(path), root, i);
        test_wrappers_once(path, i);
        test_direct_open_once(path, i);
        {
            RAII(AFile) probe = aFileOutOpen(path);
            expect_clean_at("probe reopen after direct read", i);
            assert(probe.fd >= 0);
        }
        expect_clean_at("close probe", i);
        af_rm(path);
        expect_clean_at("remove file", i);
    }

    af_rm_r(root);
    expect_clean_at("remove root", -1);

    printf("AFile open/write directed tests passed.\n");
    return 0;
}
