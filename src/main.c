#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "resample.h"
#include "args.h"
#include "hrpt_reader.h"

#include "defines.h"

int main(int argc, char* argv[])
{
    // Argument parsing
    arguments_t arguments;

    args_parse(argc, argv, &arguments);

    // set up the reader
    reader_t reader;
    reader_init(&reader, *arguments.reader);

    char* input_file = arguments.input_file;

    if(reader.open(reader.ctx, input_file))
    {
        fprintf(stderr, "Failed to open input file: %s\n", strerror(errno));
        exit(1);
    }

    FILE* fo = fopen(arguments.output_file_raw, "wb+");

    if(!fo)
    {
        fprintf(stderr, "Failed to open output file: %s\n", strerror(errno));
        reader.close(reader.ctx);
        exit(1);
    }

    uint8_t apt_channel_a = arguments.channel_a;
    uint8_t apt_channel_b = arguments.channel_b;

    // Video
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

    while(reader.readline(reader.ctx) != -1)
    {
        // if(dundee_mode) dundee_repack(data_buffer, 14800, frame);

        // resample AVHRR line
        uint16_t avhrr_line_resample[909];
        uint16_t avhrr_offset = 0;

        for(uint8_t ch=0; ch<5; ch++)
        {
            reasample_apt(avhrr_line_resample, reader.channel_get(reader.ctx, ch));

            // add to average 
            for(int i=0; i < 909; i++)
            {
                apt_line_avg[ch][i] += avhrr_line_resample[i];
            }
        }
        average_count++;

        // generate APT line
        if(!(reader.frame_counter_get(reader.ctx) % 3))
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

            uint16_t* prts = reader.prts_get(reader.ctx, 1);
            apt_telemetry[9] =  prts[0] >> 2;
            apt_telemetry[10] = prts[1] >> 2;
            apt_telemetry[11] = prts[2] >> 2;
            apt_telemetry[12] = prts[3] >> 2;
            apt_telemetry[13] = reader.patch_temp_get(reader.ctx) >> 2;

            // APT minute marker
            uint8_t apt_marker_state = apt_line_counter % 120;
            uint8_t space_marker;

            // Channel A
            
            // sync A
            memcpy(apt_line + APT_SYNCA, apt_sync_a, APT_SYNC_LENGTH);

            // Space Data/Minute Marker A
            space_marker = reader.spacev_get(reader.ctx, apt_channel_a) >> 2;
            if(apt_marker_state < 4) space_marker = apt_marker[apt_marker_state];
            memset(apt_line + APT_SPACEA, space_marker, APT_SPACE_LENGTH);

            // Video A
            memcpy(apt_line + APT_VIDEOA, apt_video_a, APT_VIDEO_LENGTH);

            // Telemetry A
            apt_telemetry[14] = reader.calibv_get(reader.ctx, apt_channel_a) >> 2;; 
            apt_telemetry[15] =  apt_telemetry[apt_channel_a]; // Channel 2
            memset(apt_line + APT_TELEMA, apt_telemetry[apt_telemetry_bar], APT_TELEM_LENGTH);

            // Channel B

            // Sync B
            memcpy(apt_line + APT_SYNCB, apt_sync_b, APT_SYNC_LENGTH);

            // Space Data/Minute Marker B
            space_marker = reader.spacev_get(reader.ctx, apt_channel_b) >> 2;
            if(apt_marker_state < 4) space_marker = apt_marker[apt_marker_state];
            memset(apt_line + APT_SPACEB, space_marker, APT_SPACE_LENGTH);

            // Video B
            memcpy(apt_line + APT_VIDEOB, apt_video_b, APT_VIDEO_LENGTH);

            // Telemetry B
            apt_telemetry[14] = reader.calibv_get(reader.ctx, apt_channel_a) >> 2;; 
            apt_telemetry[15] =  apt_telemetry[apt_channel_b]; // Channel 4
            memset(apt_line + APT_TELEMB, apt_telemetry[apt_telemetry_bar], APT_TELEM_LENGTH);

            // clear channel average
            memset(apt_line_avg, 0, sizeof(apt_line_avg));

            fwrite(apt_line, 1, APT_FRAMESIZE, fo);
            apt_line_counter++;
        }
    }

    printf("%u APT lines\n", apt_line_counter);
    
    reader.close(reader.ctx);
    reader_free(&reader);

    fclose(fo);
}
