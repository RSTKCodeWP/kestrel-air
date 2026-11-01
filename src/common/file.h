#pragma once
#include <stdio.h>

// Complete temp file + fsync + rename + directory fsync; preserve existing mode.
// Failure before rename preserves the old file. A directory-sync failure after
// rename is reported because durability is uncertain, although the file is whole.
int file_replace(const char *path, void (*write_file)(FILE *, const void *), const void *data);
