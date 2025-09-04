#pragma once
#include <stdint.h>

typedef struct {
    char* input_file;
    char* output_file_raw;
    char* output_file_png;
    char* output_file_wav;
    char* output_file_iq;
    bool dundee_format;
    int year;
    uint8_t channel_a;
    uint8_t channel_b;
} arguments_t;

void args_parse(int argc, char* argv[], arguments_t* arguments);