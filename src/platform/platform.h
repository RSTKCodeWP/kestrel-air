// Board/SoC operations that must never cross hardware backends.
#pragma once
void platform_sensor_boot(void);
void platform_record_init(void);
void platform_format(int *w, int *h, int *fps);
int platform_sensor_fps(int fps);
const char *platform_fc_device(void);
int platform_cpu_temp_x100(void); // INT_MIN when unavailable
int platform_hw_ver_mv(void);    // -1 when unavailable
int platform_key_open(void);
int platform_key_read(int fd, char *value); // logical 0 pressed, 1 released
int platform_key_upgrade(void); // legacy long-hold upgrade action supported
void platform_led_set(int red, int on);

int platform_radio_board_controls(void); // Verified project GPIO/ADC commands
