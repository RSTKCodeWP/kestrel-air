#include "file.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int file_replace(const char *path, void (*write_file)(FILE *, const void *), const void *data) {
    if (!path || !*path || !write_file) { errno = EINVAL; return -1; }
    struct stat st;
    mode_t mode = 0600;
    if (lstat(path, &st) == 0) {
        // Replacing a symlink or special file would change fopen's old meaning.
        if (!S_ISREG(st.st_mode)) { errno = EINVAL; return -1; }
        mode = st.st_mode & 0777;
    } else if (errno != ENOENT) return -1;
    char *parent = strdup(path), *temp = malloc(strlen(path) + sizeof ".tmp.XXXXXX");
    if (!parent || !temp) { free(parent); free(temp); return -1; }
    char *slash = strrchr(parent, '/');
    if (slash) { if (slash == parent) slash[1] = 0; else *slash = 0; }
    else strcpy(parent, ".");
    sprintf(temp, "%s.tmp.XXXXXX", path);
    int dir = open(parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC), fd = -1, rc = -1, created = 0;
    if (dir < 0) goto out;
    fd = mkstemp(temp);
    if (fd < 0) goto out;
    created = 1;
    if (fchmod(fd, mode)) goto out;
    FILE *f = fdopen(fd, "w");
    if (!f) goto out;
    fd = -1;
    write_file(f, data);
    int failed = ferror(f) || fflush(f) || fsync(fileno(f));
    if (fclose(f)) failed = 1;
    if (failed || rename(temp, path) || fsync(dir)) goto out;
    rc = 0;
out: {
    int error = errno;
    if (fd >= 0) close(fd);
    if (dir >= 0) close(dir);
    if (rc && created) unlink(temp);
    free(parent);
    free(temp);
    errno = error;
    return rc;
    }
}
