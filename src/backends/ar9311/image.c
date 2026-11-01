// Source AE/AWB services shipped by fpvOS. They are owned by this capture
// session, started after warm-up, and stopped before capture is torn down.
#include "camera/image.h"
#include "backends/ar9311/camera.h"
#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
unsigned image_max_exposure_us;
static pid_t ae, awb;

static void stop(pid_t *p) {
    if (*p <= 0) return;
    kill(*p, SIGTERM);
    for (int i = 0; i < 100; i++) {
        int r = waitpid(*p, NULL, WNOHANG);
        if (r == *p || (r < 0 && errno == ECHILD)) { *p = 0; return; }
        usleep(10000);
    }
    kill(*p, SIGKILL);
    while (waitpid(*p, NULL, 0) < 0 && errno == EINTR) { }
    *p = 0;
}
void image_pause(void) { stop(&awb); stop(&ae); }
static int gain(const char *key, int *out) {
    const char *v = getenv(key);
    char *end;
    if (!v || !*v) return -1;
    long n = strtol(v, &end, 10);
    if (*end || n < 128 || n > 1024) return -1;
    *out = (int)n;
    return 0;
}
int image_start(int fps, int angle) {
    (void)fps; (void)angle;
    const char *t = getenv("KA_CAMERA_TUNING"), *order = getenv("KA_WB_ORDER");
    int r, b;
    if (!t || access(t, R_OK) || gain("KA_WB_RED", &r) || gain("KA_WB_BLUE", &b) ||
        !order || (strcmp(order,"grbg") && strcmp(order,"rggb") && strcmp(order,"bggr") && strcmp(order,"gbrg"))) {
        fprintf(stderr, "camera: launcher must supply tuning and verified white-balance settings\n");
        return -1;
    }
    if (image_max_exposure_us) {
        fprintf(stderr, "camera: --max-exposure-us is not supported by this backend\n");
        return -1;
    }
    puts("camera: source AE/AWB; remote ISP tuning and rotation are not implemented");
    return 0;
}
int ar9311_controls_start(void) {
    char sensor[128];
    int red, blue;
    if (ae || awb) return ar9311_controls_alive();
    if (ar9311_node("sc231ai", sensor, sizeof(sensor)) ||
        gain("KA_WB_RED", &red) || gain("KA_WB_BLUE", &blue)) return -1;
    FILE *f = fopen("/sys/kernel/debug/ar-isp/white_balance", "w");
    if (!f) return -1;
    int ok = fprintf(f, "%u\n", (unsigned)red * 65536u + (unsigned)blue) > 0;
    if (fclose(f)) ok = 0;
    if (!ok) return -1;
    char *a[] = {"ar9311-aed", "--sensor-device", sensor, "--tuning",
        getenv("KA_CAMERA_TUNING"), "--stats", "300", NULL};
    char *b[] = {"ar9311-awbd", getenv("KA_WB_ORDER"), getenv("KA_WB_RED"), getenv("KA_WB_BLUE"), NULL};
    int e = posix_spawnp(&ae, a[0], NULL, NULL, a, environ);
    if (e) { ae = 0; errno = e; perror("start AE"); return -1; }
    e = posix_spawnp(&awb, b[0], NULL, NULL, b, environ);
    if (e) { awb = 0; image_pause(); errno = e; perror("start AWB"); return -1; }
    return 0;
}
int ar9311_controls_alive(void) {
    if (ae <= 0 || awb <= 0) return -1;
    pid_t r;
    do { r = waitpid(ae, NULL, WNOHANG); } while (r < 0 && errno == EINTR);
    if (r != 0) { ae = 0; return -1; }
    do { r = waitpid(awb, NULL, WNOHANG); } while (r < 0 && errno == EINTR);
    if (r != 0) { awb = 0; return -1; }
    return 0;
}
void image_restart(int fps, int angle) { (void)fps; (void)angle; }
void image_get_info(image_isp_info *i) { memset(i, 0, sizeof(*i)); }
void image_apply_all(void) { }
#define UNSUPPORTED(name) void image_set_##name(int v) { (void)v; fprintf(stderr,"camera: " #name " control unsupported\n"); }
UNSUPPORTED(scene)
UNSUPPORTED(ev)
UNSUPPORTED(sat)
UNSUPPORTED(sharpness)
UNSUPPORTED(awb)
UNSUPPORTED(contrast)
UNSUPPORTED(3dnr)
UNSUPPORTED(2dnr)
void image_low_power(int on) { (void)on; } // RF standby does not alter the validated sensor mode
