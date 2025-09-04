#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "resample.h"
#include "args.h"

#include "defines.h"

// Taken from my SatDump fork
int dundee_repack(uint8_t *bytes, int byte_length, uint16_t *words)
{
    int bpos = 0;
    int wpos = 0;

    for (int i = 0; i < byte_length; i += 4)
    {
        words[wpos++] = bytes[bpos + 0] << 2 | bytes[bpos + 1] >> 6;
        words[wpos++] = (bytes[bpos + 1] % 64) << 4 | bytes[bpos + 2] >> 4;
        words[wpos++] = (bytes[bpos + 2] % 16) << 6 | bytes[bpos + 3] >> 2;
        bpos += 4;
    }

    return wpos;
}

int main(int argc, char* argv[])
{
    // Argument parsing
    arguments_t arguments;

    args_parse(argc, argv, &arguments);

    bool dundee_mode = arguments.dundee_format;

    char* input_file = arguments.input_file;

    FILE* fi = fopen(input_file, "rb");

    if(!fi)
    {
        fprintf(stderr, "Failed to open input file: %s\n", strerror(errno));
        exit(1);
    }

    FILE* fo = fopen(arguments.output_file_raw, "wb+");

    if(!fo)
    {
        fprintf(stderr, "Failed to open output file: %s\n", strerror(errno));
        exit(1);
    }

    uint8_t apt_channel_a = arguments.channel_a;
    uint8_t apt_channel_b = arguments.channel_b;

    uint16_t frame[11090 + 10];
    unsigned char* data_buffer = (unsigned char*)frame;

    // fread size
    size_t read_size = 11090*2;
    if(dundee_mode)
    {
        read_size = 14800;
        data_buffer = malloc(14800);
    }

    size_t frame_counter = 0;
    size_t prt_offset = 0;

    // Calibration data
    uint16_t prts[3][4];
    uint16_t calib_views[3];
    uint16_t space_views[5];

    // Video
    uint16_t avhrr_line[5][2048];
    uint16_t apt_line_avg[5][909];
    uint8_t average_count;

    // APT format
    uint8_t apt_line[APT_FRAMESIZE];
    size_t apt_line_counter = 0;
    size_t apt_frame_counter = 0;

    uint8_t apt_telemetry[16] = {
        32,     // WEDGE #1
        64,     // WEDGE #2
        96,     // WEDGE #3
        128,    // WEDGE #4
        160,    // WEDGE #5
        192,    // WEDGE #6
        224,    // WEDGE #7
        255,    // WEDGE #8
        0,      // Zero Modulation Reference
        0,      // Thermistor Temp #1 (set by PRT1)
        0,      // Thermistor Temp #2 (set to PRT2)
        0,      // Thermistor Temp #3 (set to PRT3)
        0,      // Thermistor Temp #4 (set to PRT4)
        0,      // Patch Temp (set to Patch temp)
        0,      // Back Scan (set to Calibration Target View)
        0,      // Channel I.D. Wedge (set bases on user input)
    };

    uint8_t apt_marker[4] = {
        0,
        0,
        255,
        255
    };

    memset(apt_line, 127, APT_FRAMESIZE);

    uint8_t apt_sync_a[APT_SYNC_LENGTH] = {0, 0, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0, 0};
    uint8_t apt_sync_b[APT_SYNC_LENGTH] = {0, 0, 0, 0, 255, 255, 255, 0, 0, 255, 255, 255, 0, 0, 255, 255, 255, 0, 0, 255, 255, 255, 0, 0, 255, 255, 255, 0, 0, 255, 255, 255, 0, 0, 255, 255, 255, 0, 0};

    while(fread(data_buffer, 1, read_size, fi) > 0)
    {
        if(dundee_mode) dundee_repack(data_buffer, 14800, frame);

        uint16_t prt1 = frame[17];
        uint16_t prt2 = frame[18];
        uint16_t prt3 = frame[19];
        uint16_t patch_temp = frame[20];
        uint16_t sync = frame[102];

        uint8_t current_prt = (frame_counter - prt_offset) % 5;
        bool prt_sync = !current_prt;

        uint16_t day_of_year = frame[8] >> 1;
        uint64_t milliseconds = (frame[9] & 0x7F) << 20 | (frame[10] << 10) | frame[11];

        // printf("frame: %u, day: %u, ms: %lu\n", frame_counter, day_of_year, milliseconds);

        // printf("frame: %llu prt_sync: %u cprt: %u prt: %u %u %u patch: %u sync: %016b\n", frame_counter, prt_sync, current_prt, prt1, prt2, prt3, patch_temp, sync);

        // Set PRTs
        if(!prt_sync)
        {
            prts[0][current_prt-1] = prt1;
            prts[1][current_prt-1] = prt2;
            prts[2][current_prt-1] = prt3;
        }

        // get Space scan and Calibration View
        uint32_t calib_views_avg[3];
        memset(calib_views_avg, 0, sizeof(calib_views_avg));
        for(uint8_t word=0; word < 30; word++)
        {
            uint8_t ch = word % 3;

            calib_views_avg[ch] += frame[22+word];
        }
        for(uint8_t ch=0; ch < 3; ch++)
        {
            calib_views[ch] = calib_views_avg[ch] / 10;
        }
    
        uint32_t space_views_avg[5];
        memset(space_views_avg, 0, sizeof(space_views_avg));
        for(uint8_t word=0; word < 50; word++)
        {
            uint8_t ch = word % 5;

            space_views_avg[ch] += frame[52+word];
        }
        for(uint8_t ch=0; ch < 5; ch++)
        {
            space_views[ch] = space_views_avg[ch] / 10;
            // printf("ch %u space: %u\n", ch+1, space_views[ch]);
        }

        // PRT sync
        if((prt1 | prt2 | prt3) == 0) // PRT blanking
        {
            // desync
            if(!prt_sync)
            {
                    prt_offset = frame_counter % 5;
                    printf("PRT desync! resetting to %u\n", prt_offset);
            }
            // printf("PRTs: ch3b: %u %u %u %u, ch4: %u %u %u %u, ch5: %u %u %u %u\n", prts[0][0], prts[0][1], prts[0][2], prts[0][3], prts[1][0], prts[1][1], prts[1][2], prts[1][3], prts[2][0], prts[2][1], prts[2][2], prts[2][3]);
        }
        else if(prt_sync) // non-blanking desync
        {
            // printf("PRT sync error at %u\n", frame_counter);
        }

        // unpack AVHRR line
        for(size_t word=0; word < 10240; word++)
        {
            uint8_t ch = word % 5;

            avhrr_line[ch][word/5] = frame[750+word];
        }

        // resample AVHRR line
        uint16_t avhrr_line_resample[909];
        uint16_t avhrr_offset = 0;

        for(uint8_t ch=0; ch<5; ch++)
        {
            reasample_apt(avhrr_line_resample, avhrr_line[ch]);

            // add to average 
            for(int i=0; i < 909; i++)
            {
                apt_line_avg[ch][i] += avhrr_line_resample[i];
            }
        }
        average_count++;

        // generate APT line
        if(!(frame_counter % 3))
        {
            // channel average
            for(uint8_t ch=0; ch < 5; ch++)
            {
                for(int i=0; i < 909; i++)
                {
                    apt_line_avg[ch][i] /= average_count;
                }
            }
            average_count = 0;

            // Convert Channel bit depth
            uint8_t apt_video_a[909];
            uint8_t apt_video_b[909];

            for(int i=0; i < 909; i++)
            {
                apt_video_a[i] = apt_line_avg[apt_channel_a][i] >> 2;
            }
            for(int i=0; i < 909; i++)
            {
                apt_video_b[i] = apt_line_avg[apt_channel_b][i] >> 2;
            }

            // APT telemtry
            size_t apt_frame = apt_line_counter / 128;
            uint8_t apt_frame_line = apt_line_counter % 128;
            uint8_t apt_telemetry_bar = apt_frame_line / 8;

            apt_telemetry[9] = prts[1][0] >> 2;
            apt_telemetry[10] = prts[1][1] >> 2;
            apt_telemetry[11] = prts[1][2] >> 2;
            apt_telemetry[12] = prts[1][3] >> 2;
            apt_telemetry[13] = patch_temp >> 2;

            // APT minute marker
            uint8_t apt_marker_state = apt_line_counter % 120;
            uint8_t space_marker;

            // Channel A
            
            // sync A
            memcpy(apt_line + APT_SYNCA, apt_sync_a, APT_SYNC_LENGTH);

            // Space Data/Minute Marker A
            space_marker = space_views[apt_channel_a] >> 2;
            if(apt_marker_state < 4) space_marker = apt_marker[apt_marker_state];
            memset(apt_line + APT_SPACEA, space_marker, APT_SPACE_LENGTH);

            // Video A
            memcpy(apt_line + APT_VIDEOA, apt_video_a, APT_VIDEO_LENGTH);

            // Telemetry A
            // Visible channel doesn't have calib view
            if(apt_channel_a < 2)
            {
                apt_telemetry[14] = 0; 
            }
            else
            {
                apt_telemetry[14] = calib_views[apt_channel_a - 2] >> 2;; 
            }
            apt_telemetry[15] =  apt_telemetry[apt_channel_a]; // Channel 2
            memset(apt_line + APT_TELEMA, apt_telemetry[apt_telemetry_bar], APT_TELEM_LENGTH);

            // Channel B

            // Sync B
            memcpy(apt_line + APT_SYNCB, apt_sync_b, APT_SYNC_LENGTH);

            // Space Data/Minute Marker B
            space_marker = space_views[apt_channel_b] >> 2;
            if(apt_marker_state < 4) space_marker = apt_marker[apt_marker_state];
            memset(apt_line + APT_SPACEB, space_marker, APT_SPACE_LENGTH);

            // Video B
            memcpy(apt_line + APT_VIDEOB, apt_video_b, APT_VIDEO_LENGTH);

            // Telemetry B
            // Visible channel doesn't have calib view
            if(apt_channel_b < 2)
            {
                apt_telemetry[14] = 0; 
            }
            else
            {
                apt_telemetry[14] = calib_views[apt_channel_b - 2] >> 2;; 
            }
            apt_telemetry[15] =  apt_telemetry[apt_channel_b]; // Channel 4
            memset(apt_line + APT_TELEMB, apt_telemetry[apt_telemetry_bar], APT_TELEM_LENGTH);

            // clear channel average
            memset(apt_line_avg, 0, sizeof(apt_line_avg));

            fwrite(apt_line, 1, APT_FRAMESIZE, fo);
            apt_line_counter++;
        }

        frame_counter++;
    }

    printf("%u APT lines\n", apt_line_counter);

    if(dundee_mode) free(data_buffer);
    
    fclose(fo);
    fclose(fi);
}
