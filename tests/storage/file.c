#define _GNU_SOURCE
#include "common/file.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int sync_calls, fail_sync, fail_rename, renamed;
int __real_fsync(int fd);
int __real_rename(const char *, const char *);
int __wrap_fsync(int fd) {
    sync_calls++;
    if (sync_calls == 1) assert(!renamed);
    if (sync_calls == 2) assert(renamed);
    if (sync_calls == fail_sync) { errno = EIO; return -1; }
    return __real_fsync(fd);
}
int __wrap_rename(const char *old, const char *new) {
    assert(sync_calls == 1);
    if (fail_rename) { errno = EIO; return -1; }
    renamed = 1;
    return __real_rename(old, new);
}
static void reset(void) { sync_calls = fail_sync = fail_rename = renamed = 0; }
static void write_text(FILE *f, const void *text) { fputs(text, f); }
static void write_error(FILE *f, const void *unused) { (void)unused; fputs("partial", f); close(fileno(f)); }
static void killed_writer(FILE *f, const void *unused) {
    (void)unused; fputs("partial", f); fflush(f); kill(getpid(), SIGKILL);
}
static void check_text(const char *path, const char *expected) {
    char b[64] = {0}; FILE *f = fopen(path, "r"); assert(f);
    size_t n = fread(b, 1, sizeof b, f); assert(n == strlen(expected));
    assert(!memcmp(b, expected, n)); fclose(f);
}
int main(void) {
    assert(file_replace("", write_text, "bad") == -1);
    assert(file_replace(NULL, write_text, "bad") == -1);
    char dir[] = "/tmp/ka-storage-XXXXXX", path[128], link[128];
    assert(mkdtemp(dir)); snprintf(path, sizeof path, "%s/settings.json", dir);
    snprintf(link, sizeof link, "%s/link", dir);
    assert(file_replace(path, write_text, "old") == 0);
    struct stat st; assert(!stat(path, &st) && (st.st_mode & 0777) == 0600);
    assert(sync_calls == 2);
    assert(!chmod(path, 0640));
    reset(); fail_sync = 1;
    assert(file_replace(path, write_text, "bad") == -1); check_text(path, "old");
    reset(); fail_rename = 1;
    assert(file_replace(path, write_text, "bad") == -1); check_text(path, "old");
    reset();
    assert(file_replace(path, write_error, NULL) == -1); check_text(path, "old");
    reset(); fail_sync = 2;
    assert(file_replace(path, write_text, "whole") == -1); check_text(path, "whole");
    reset();
    int oldfd = open(path, O_RDONLY); assert(oldfd >= 0);
    assert(file_replace(path, write_text, "new") == 0); check_text(path, "new");
    char old[6] = {0}; assert(read(oldfd, old, 5) == 5 && !strcmp(old, "whole")); close(oldfd);
    assert(!stat(path, &st) && (st.st_mode & 0777) == 0640);
    assert(!symlink(path, link)); reset();
    assert(file_replace(link, write_text, "bad") == -1); check_text(path, "new");
    assert(file_replace(dir, write_text, "bad") == -1);
    pid_t child = fork(); assert(child >= 0);
    if (!child) { reset(); file_replace(path, killed_writer, NULL); _exit(2); }
    int status; assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL); check_text(path, "new");
    // A killed writer can leave a harmless temp file; it is never loaded as settings.
    DIR *d = opendir(dir); assert(d); struct dirent *e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
        char p[512]; snprintf(p, sizeof p, "%s/%s", dir, e->d_name); assert(!unlink(p));
    }
    closedir(d); assert(!rmdir(dir));
    puts("atomic storage: complete replacement, errors and killed writer passed");
}
