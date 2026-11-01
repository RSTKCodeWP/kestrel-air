#include "video/internal.h"
#include "video/video.h"
#include <stdio.h>
#include <unistd.h>

static int send_cmd(int enable) {
    if (cmd_req) {
        printf("video: send %s, warning: request busy\n", enable ? "start" : "stop");
        return -1;
    }
    cmd_enable = enable;
    cmd_req = 1;
    for (int i = 0; i < 100 && cmd_req; i++) usleep(20000);
    if (cmd_req) printf("video: send %s, fail\n", enable ? "start" : "stop");
    return 0;
}

int video_send_start(void) { return send_cmd(1); }
int video_send_stop(void) { return send_cmd(0); }

void video_set_format(int w, int h, int f, int sensor_fps) {
    width = w;
    height = h;
    fps = f;
    src_fps = sensor_fps;
}

