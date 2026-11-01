// P1 wiring belongs to Linux's device tree, not HiSilicon MMIO or GPIO numbers.
#include "platform/platform.h"
#include <limits.h>
#include <stdio.h>

void platform_sensor_boot(void) { /* Clock/reset owned by the sensor driver. */ }
void platform_record_init(void) { /* Storage is mounted by the OS. */ }
void platform_format(int *w, int *h, int *fps) { *w = 1920; *h = 1080; *fps = 60; }
int platform_sensor_fps(int fps) { (void)fps; return 60; }
const char *platform_fc_device(void) { return NULL; } // P1 FC UART not validated yet
int platform_cpu_temp_x100(void) { return INT_MIN; } // No validated thermal provider yet
int platform_hw_ver_mv(void) { return -1; }
const char *platform_key_path(void) { return NULL; }
int platform_red_gpio(void) { return -1; }
int platform_green_gpio(void) { return -1; }

int platform_radio_board_controls(void) { return 0; }
