#pragma once
#include <stddef.h>
#include <stdio.h>

int editor_process(
    const unsigned char *image, size_t image_len,
    const char *yaml, size_t yaml_len,
    FILE *outfile
);
