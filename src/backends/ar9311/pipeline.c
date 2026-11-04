// Linux owns sensor clocks, reset, CSI, ISP and capture buffers. The launcher
// loads those drivers; this backend verifies the supported camera before use.
#include "camera/pipeline.h"
#include "backends/ar9311/camera.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int ar9311_node(const char *name, char *path, size_t size) {
    DIR *d = opendir("/sys/class/video4linux");
    struct dirent *e;
    int n = 0;
    if (!d) return -1;
    while ((e = readdir(d))) {
        char p[512], v[128];
        snprintf(p, sizeof(p), "/sys/class/video4linux/%s/name", e->d_name);
        FILE *f = fopen(p, "r");
        if (!f) continue;
        if (fgets(v, sizeof(v), f)) {
            v[strcspn(v, "\r\n")] = 0;
            if (!strcmp(v, name) || (!strcmp(name, "sc231ai") && !strncmp(v, "sc231ai ", 8))) {
                if (snprintf(path, size, "/dev/%s", e->d_name) >= (int)size) n = 2;
                else n++;
            }
        }
        fclose(f);
    }
    closedir(d);
    if (n != 1) fprintf(stderr, "camera: expected one %s node, found %d\n", name, n);
    return n == 1 ? 0 : -1;
}
int camera_sensor_probe(void) { char p[128]; return ar9311_node("sc231ai", p, sizeof(p)); }
int camera_pipeline_create(char sensor, short w, short h, char rate, int slices) {
    char p[128];
    (void)sensor; (void)slices;
    if (w != 1920 || h != 1080 || rate != 60) return -1;
    return camera_sensor_probe() || ar9311_node("ar-cvisp", p, sizeof(p)) ||
        ar9311_node("C&M Wave5 VPU encoder", p, sizeof(p)) ? -1 : 0;
}
void camera_pipeline_destroy(void) { }
void camera_system_deinit(void) { }
void camera_set_sensor_angle(int angle) { if (angle) fprintf(stderr, "camera: rotation unsupported on P1\n"); }
void camera_set_low_delay_lines(int lines) { (void)lines; }
const char *camera_sensor_name(void) { return "sc231ai"; }
int camera_sensor_type(void) { return 0; } // No verified stock protocol sensor ID
int camera_sensor_angle_tuning(void) { return 0; }
void camera_capture_size(int *w, int *h) { *w = 1920; *h = 1080; }
int camera_has_864p120(void) { return 0; }
int camera_has_864p144(void) { return 0; }
uint32_t camera_readout_us(void) { return 0; } // Not measured; do not advertise timing
