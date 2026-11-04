// The whole air side: start-up and the pipeline's size and rate.
#pragma once
#include <signal.h>

int  app_run(volatile sig_atomic_t *keep_running);
void app_ch0_remap(int *w, int *h, int *fps);
int  app_set_sensor_res(int w, int h, int fps, int angle);
