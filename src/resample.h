#pragma once
#include <stdint.h>
#include <stddef.h>

void resample_zone1(uint16_t* output, uint16_t* input, size_t count);

void resample_zone2(uint16_t* output, uint16_t* input, size_t count);

void resample_zone3(uint16_t* output, uint16_t* input, size_t count);

void resample_zone4(uint16_t* output, uint16_t* input, size_t count);

void resample_zone5(uint16_t* output, uint16_t* input, size_t count);

void reasample_apt(uint16_t* resampled, uint16_t* original);