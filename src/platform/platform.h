// Board/SoC operations that must never cross hardware backends.
#pragma once
void platform_sensor_boot(void);
void platform_record_init(void);
void platform_format(int *w, int *h, int *fps);
int platform_sensor_fps(int fps);
const char *platform_fc_device(void);
int platform_cpu_temp_x100(void); // INT_MIN when unavailable
int platform_hw_ver_mv(void);    // -1 when unavailable
const char *platform_key_path(void);
int platform_red_gpio(void);
int platform_green_gpio(void);

int platform_radio_board_controls(void); // Verified project GPIO/ADC commands
