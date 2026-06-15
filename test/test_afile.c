#include <alib/afile.h>

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void on_timeout(int sig) {
    (void)sig;
    fprintf(stderr, "test_afile timed out\n");
    _exit(124);
}

static void test_mark(const char* name) {
    printf("[afile] %s\n", name);
}

static void expect_clean(void) {
    if(aErrOccur()) {
        fprintf(stderr, "unexpected exception: %d\n", aErrGet());
    }
    assert(!aErrOccur());
}

static void expect_error(void) {
    assert(aErrOccur());
    aTry((void)0;)aExc{}
}

static void make_path(char* out, size_t out_size, const char* dir, const char* name) {
    int n = snprintf(out, out_size, "%s/%s", dir, name);
    assert(n > 0 && (size_t)n < out_size);
}

static void write_text_posix(const char* path, const char* text) {
    FILE* fp = fopen(path, "wb");
    assert(fp != NULL);
    assert(fwrite(text, 1, strlen(text), fp) == strlen(text));
    assert(fclose(fp) == 0);
}

static void assert_file_text(const char* path, const char* expected) {
    char buf[256] = {0};
    FILE* fp = fopen(path, "rb");
    assert(fp != NULL);
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    assert(ferror(fp) == 0);
    assert(fclose(fp) == 0);
    assert(n == strlen(expected));
    assert(strcmp(buf, expected) == 0);
}

static int line_has_path(ALine(AStr)* list, const char* path) {
    forEach(it, *list) {
        if(it.p != NULL && strcmp(it.p->s, path) == 0) {
            return 1;
        }
    }
    return 0;
}

static void test_file_tools(const char* root) {
    test_mark("file tools");
    char nested[PATH_MAX];
    char file[PATH_MAX];
    char copy[PATH_MAX];
    char moved[PATH_MAX];
    char subdir[PATH_MAX];
    char subfile[PATH_MAX];
    char recursive_copy[PATH_MAX];
    char recursive_file[PATH_MAX];

    make_path(nested, sizeof(nested), root, "a/b");
    test_mark("file tools: mkdir");
    af_mkdir(nested);
    expect_clean();
    assert(af_isdir(nested));

    make_path(file, sizeof(file), nested, "file.txt");
    test_mark("file tools: touch");
    af_touch(file);
    expect_clean();
    assert(af_isfile(file));
    assert(!af_isdir(file));
    assert(!af_isdev(file));

    RAII(AStr) dir = af_dir_extract(file);
    expect_clean();
    printf("[afile] dir_extract actual='%s' expected='%s'\n", dir.s, nested);
    assert(strcmp(dir.s, nested) == 0);

    RAII(AStr) abs = af_path_absolute(file);
    expect_clean();
    assert(abs.s != NULL);
    assert(abs.s[0] == '/');

    af_chmod(file, 0600);
    expect_clean();
    struct stat st;
    assert(stat(file, &st) == 0);
    assert((st.st_mode & S_IRUSR) != 0);
    assert((st.st_mode & S_IWUSR) != 0);

    write_text_posix(file, "tool-data");
    make_path(copy, sizeof(copy), nested, "copy.txt");
    test_mark("file tools: copy file");
    af_cp(file, copy);
    expect_clean();
    assert(af_isfile(copy));
    assert_file_text(copy, "tool-data");

    make_path(moved, sizeof(moved), nested, "moved.txt");
    test_mark("file tools: move file");
    af_mv(copy, moved);
    expect_clean();
    assert(!af_isfile(copy));
    assert(af_isfile(moved));

    test_mark("file tools: ls");
    RAII(ALine(AStr)) names = af_ls(nested);
    expect_clean();
    assert(names.f->getNumber(&names) >= 2);
    assert(line_has_path(&names, file));
    assert(line_has_path(&names, moved));

    AFileInfo info = af_get_info(file);
    expect_clean();
    assert(info.ast_size == strlen("tool-data"));

    make_path(subdir, sizeof(subdir), nested, "sub");
    af_mkdir(subdir);
    expect_clean();
    make_path(subfile, sizeof(subfile), subdir, "child.txt");
    write_text_posix(subfile, "child-data");

    make_path(recursive_copy, sizeof(recursive_copy), root, "copy-tree");
    test_mark("file tools: copy tree");
    af_cp_r(nested, recursive_copy);
    expect_clean();
    make_path(recursive_file, sizeof(recursive_file), recursive_copy, "sub/child.txt");
    assert(af_isfile(recursive_file));
    assert_file_text(recursive_file, "child-data");

    test_mark("file tools: chmod tree");
    af_chmod_r(nested, 0700);
    expect_clean();
    assert(stat(subfile, &st) == 0);
    assert((st.st_mode & S_IRUSR) != 0);

    test_mark("file tools: remove moved");
    af_rm(moved);
    expect_clean();
    assert(!af_isfile(moved));

    af_rm(nullptr);
    expect_error();

    test_mark("file tools: remove copy tree");
    af_rm_r(recursive_copy);
    expect_clean();
    assert(!af_isdir(recursive_copy));
}

static void test_file_object_io(const char* root) {
    test_mark("file object io");
    char file[PATH_MAX];
    char missing[PATH_MAX];
    char second[PATH_MAX];
    char buf[32] = {0};

    make_path(file, sizeof(file), root, "io.txt");

    {
        RAII(AFile) out = aFileOutOpen(file);
        expect_clean();
        assert(out.f->write(&out, 5, "hello") == 5);
        expect_clean();
    }

    {
        RAII(AFile) app = aFileEndOpen(file);
        expect_clean();
        assert(app.f->write(&app, 2, "++") == 2);
        expect_clean();
    }

    {
        RAII(AFile) in = aFileInOpen(file);
        expect_clean();
        assert(in.f->read(&in, sizeof(buf), buf) == 7);
        expect_clean();
        assert(strcmp(buf, "hello++") == 0);

        memset(buf, 0, sizeof(buf));
        assert(in.f->read_pos(&in, 5, 2, buf) == 2);
        expect_clean();
        assert(strcmp(buf, "++") == 0);
    }

    {
        RAII(AFile) rw = aDevInOutOpen(file, false, false);
        expect_clean();
        assert(rw.f->write_pos(&rw, 0, 2, "HE") == 2);
        expect_clean();
    }

    {
        RAII(AFile) in = aFileInOpen(file);
        expect_clean();
        memset(buf, 0, sizeof(buf));
        assert(in.f->read(&in, sizeof(buf), buf) == 7);
        expect_clean();
        assert(strcmp(buf, "HEllo++") == 0);
    }

    make_path(missing, sizeof(missing), root, "missing.txt");
    {
        RAII(AFile) in = aFileInOpen(missing);
        assert(aErrOccur());
        aTry((void)0;)aExc{}
        assert(in.fd < 0);
    }
    assert(!af_isfile(missing));

    make_path(second, sizeof(second), root, "second.txt");
    {
        RAII(AFile) out = aFileOutOpen(second);
        expect_clean();
        assert(out.f->write(&out, 3, "abc") == 3);
        expect_clean();
    }
    {
        RAII(AFile) out = aFileOutOpen(second);
        expect_clean();
        assert(out.f->write(&out, 1, "Z") == 1);
        expect_clean();
    }
    assert_file_text(second, "Z");
}

static void test_invalid_inputs(const char* root) {
    test_mark("invalid inputs");
    char missing[PATH_MAX];
    make_path(missing, sizeof(missing), root, "does-not-exist");

    assert(!af_path_exist(missing));

    af_rm(missing);
    expect_error();

    af_cp(missing, root);
    expect_error();

    af_chmod(missing, 0600);
    expect_error();

    RAII(ALine(AStr)) list = af_ls(missing);
    (void)list;
    expect_error();
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGALRM, on_timeout);
    alarm(30);

    char tmpl[] = "/tmp/alib-afile-XXXXXX";
    char* root = mkdtemp(tmpl);
    assert(root != NULL);

    test_file_tools(root);
    test_file_object_io(root);
    test_invalid_inputs(root);

    af_rm_r(root);
    expect_clean();
    assert(!af_isdir(root));

    printf("AFile tests passed.\n");
    return 0;
}
