#include <alib/afile.h>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void expect_clean(void) {
    assert(!aExcOccur());
}

static void make_path(char* out, size_t out_size, const char* dir, const char* name) {
    int n = snprintf(out, out_size, "%s/%s", dir, name);
    assert(n > 0 && (size_t)n < out_size);
}

static void test_file_tools(const char* root) {
    char nested[PATH_MAX];
    char file[PATH_MAX];
    char copy[PATH_MAX];
    char moved[PATH_MAX];

    make_path(nested, sizeof(nested), root, "a/b");
    af_mkdir_p(nested);
    expect_clean();
    assert(af_isdir(nested));

    make_path(file, sizeof(file), nested, "file.txt");
    af_touch(file);
    expect_clean();
    assert(af_isfile(file));
    assert(!af_isdir(file));
    assert(!af_isdev(file));

    RAII(AText) dir = af_dir_extract(file);
    expect_clean();
    assert(strcmp(dir.s, nested) == 0);

    RAII(AText) abs = af_path_absolute(file);
    expect_clean();
    assert(abs.s != NULL);
    assert(abs.s[0] == '/');

    af_chmod(file, 6);
    expect_clean();
    struct stat st;
    assert(stat(file, &st) == 0);
    assert((st.st_mode & S_IRUSR) != 0);
    assert((st.st_mode & S_IWUSR) != 0);

    make_path(copy, sizeof(copy), nested, "copy.txt");
    af_cp(file, copy);
    expect_clean();
    assert(af_isfile(copy));

    make_path(moved, sizeof(moved), nested, "moved.txt");
    af_mv(copy, moved);
    expect_clean();
    assert(!af_isfile(copy));
    assert(af_isfile(moved));

    RAII(ALine(AText)) names = af_ls(nested);
    expect_clean();
    assert(names.f->getNumber(&names) >= 2);

    RAII(ALine(AText)) names_a = af_ls_a(nested);
    expect_clean();
    assert(names_a.f->getNumber(&names_a) >= names.f->getNumber(&names));

    RAII(ALine(AText)) names_A = af_ls_A(nested);
    expect_clean();
    assert(names_A.f->getNumber(&names_A) >= names.f->getNumber(&names));

    AFileInfo info = af_get_info(file);
    expect_clean();
    assert(info.st_size == 0);

    af_rm(moved);
    expect_clean();
    assert(!af_isfile(moved));
}

static void test_file_object_io(const char* root) {
    char file[PATH_MAX];
    char missing[PATH_MAX];
    char buf[32] = {0};

    make_path(file, sizeof(file), root, "io.txt");

    {
        RAII(AFile) out = aFileOutOpen(file);
        expect_clean();
        assert(out.f->write(&out, 5, "hello") == 5);
        expect_clean();
    }

    {
        RAII(AFile) app = aFileEnOpen(file);
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
        assert(aExcOccur());
        aExcClean();
        assert(in.fd < 0);
    }
    assert(!af_isfile(missing));
}

int main(void) {
    char tmpl[] = "/tmp/alib-afile-XXXXXX";
    char* root = mkdtemp(tmpl);
    assert(root != NULL);

    test_file_tools(root);
    test_file_object_io(root);

    af_rm_r(root);
    expect_clean();
    assert(!af_isdir(root));

    printf("AFile tests passed.\n");
    return 0;
}
