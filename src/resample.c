#include "resample.h"
#include <string.h>
#include "defines.h"

// APT resample defines
#define ZONE1_AVHRR_SAMPLES 628
#define ZONE2_AVHRR_SAMPLES 330
#define ZONE3_AVHRR_SAMPLES 166
#define ZONE4_AVHRR_SAMPLES 93
#define ZONE5_AVHRR_SAMPLES 121

#define ZONE1_APT_SAMPLES 157
#define ZONE2_APT_SAMPLES 110
#define ZONE3_APT_SAMPLES 83
#define ZONE4_APT_SAMPLES 62
#define ZONE5_APT_SAMPLES 121

#define ZONE5L_APT 0
#define ZONE4L_APT (ZONE5L_APT + ZONE5_APT_SAMPLES)
#define ZONE3L_APT (ZONE4L_APT + ZONE4_APT_SAMPLES)
#define ZONE2L_APT (ZONE3L_APT + ZONE3_APT_SAMPLES)
#define ZONE1_APT (ZONE2L_APT + ZONE2_APT_SAMPLES)
#define ZONE2R_APT (ZONE1_APT + ZONE1_APT_SAMPLES)
#define ZONE3R_APT (ZONE2R_APT + ZONE2_APT_SAMPLES)
#define ZONE4R_APT (ZONE3R_APT + ZONE3_APT_SAMPLES)
#define ZONE5R_APT (ZONE4R_APT + ZONE4_APT_SAMPLES)
#define ZONE_APT_TOTAL (ZONE5R_APT + ZONE5_APT_SAMPLES)

#define ZONE5L_AVHRR 0
#define ZONE4L_AVHRR (ZONE5L_AVHRR + ZONE5_AVHRR_SAMPLES)
#define ZONE3L_AVHRR (ZONE4L_AVHRR + ZONE4_AVHRR_SAMPLES)
#define ZONE2L_AVHRR (ZONE3L_AVHRR + ZONE3_AVHRR_SAMPLES)
#define ZONE1_AVHRR (ZONE2L_AVHRR + ZONE2_AVHRR_SAMPLES)
#define ZONE2R_AVHRR (ZONE1_AVHRR + ZONE1_AVHRR_SAMPLES)
#define ZONE3R_AVHRR (ZONE2R_AVHRR + ZONE2_AVHRR_SAMPLES)
#define ZONE4R_AVHRR (ZONE3R_AVHRR + ZONE3_AVHRR_SAMPLES)
#define ZONE5R_AVHRR (ZONE4R_AVHRR + ZONE4_AVHRR_SAMPLES)
#define ZONE_AVHRR_TOTAL (ZONE5R_AVHRR + ZONE5_AVHRR_SAMPLES)

#if ZONE_APT_TOTAL != APT_VIDEO_LENGTH
#error "APT video size mismatch"
#endif

#if ZONE_AVHRR_TOTAL != 2048
#error "HRPT video size mismatch"
#endif

// average 4 samples
void resample_zone1(uint16_t* output, uint16_t* input, size_t count)
{
    for(int i=0; i < count; i++)
    {
        uint16_t sample;

        sample = (input[i*4] + input[i*4 + 1] + input[i*4 + 2] + input[i*4 + 3]) / 4;
        output[i] = sample;
    }
}

void resample_zone2(uint16_t* output, uint16_t* input, size_t count)
{
    for(int i=0; i < count; i++)
    {
        uint16_t sample;

        sample = (input[i*3] + input[i*3 + 1] + input[i*3 + 2]) / 3;
        output[i] = sample;
    }
}

// average 2 samples
void resample_zone3(uint16_t* output, uint16_t* input, size_t count)
{
    for(int i=0; i < count; i++)
    {
        uint16_t sample;

        sample = (input[i*2] + input[i*2 + 1]) / 2;
        output[i] = sample;
    }
}

// average 1.5 samples
void resample_zone4(uint16_t* output, uint16_t* input, size_t count)
{
    for(int i=0; i < count; i+=2)
    {
        uint16_t sample1, sample2;

        int n = i+i/2;

        sample1 = (input[n] + input[n + 1]) / 2;
        sample2 = (input[n + 1] + input[n + 2]) / 2;
        output[i] = sample1;
        output[i+1] = sample2;
    }
}

// Retain original resolution
void resample_zone5(uint16_t* output, uint16_t* input, size_t count)
{
    memcpy(output, input, count*2);
}

void reasample_apt(uint16_t* resampled, uint16_t* original)
{
        // Zone 5 left
        resample_zone5(&resampled[ZONE5L_APT], &original[ZONE5L_AVHRR], ZONE5_APT_SAMPLES);

        // Zone 4 left
        resample_zone4(&resampled[ZONE4L_APT], &original[ZONE4L_AVHRR], ZONE4_APT_SAMPLES);

        // Zone 3 left
        resample_zone3(&resampled[ZONE3L_APT], &original[ZONE3L_AVHRR], ZONE3_APT_SAMPLES);

        // Zone 2 left
        resample_zone2(&resampled[ZONE2L_APT], &original[ZONE2L_AVHRR], ZONE2_APT_SAMPLES);

        // Zone 1 nadir
        resample_zone1(&resampled[ZONE1_APT], &original[ZONE1_AVHRR], ZONE1_APT_SAMPLES);

        // Zone 2 right
        resample_zone2(&resampled[ZONE2R_APT], &original[ZONE2R_AVHRR], ZONE2_APT_SAMPLES);

        // Zone 3 right
        resample_zone3(&resampled[ZONE3R_APT], &original[ZONE3R_AVHRR], ZONE3_APT_SAMPLES);

        // Zone 4 right
        resample_zone4(&resampled[ZONE4R_APT], &original[ZONE4R_AVHRR], ZONE4_APT_SAMPLES);

        // Zone 5 right
        resample_zone5(&resampled[ZONE5R_APT], &original[ZONE5R_AVHRR], ZONE5_APT_SAMPLES);
}