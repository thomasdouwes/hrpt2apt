#include "hrpt_reader.h"
#include <stdlib.h>

#define HRPT_FRAME_SIZE 11090

reader_t hrpt_reader_interface = (reader_t) {
    READER_IFACE_OPEN           hrpt_reader_open,
    READER_IFACE_CLOSE          hrpt_reader_close,
    READER_IFACE_READLINE       hrpt_reader_readline,
    READER_IFACE_PATCH_TEMP_GET hrpt_reader_patch_temp_get,
    READER_IFACE_PRTS_GET       hrpt_reader_prts_get,
    READER_IFACE_CALIBV_GET     hrpt_reader_calibv_get,
    READER_IFACE_SPACEV_GET     hrpt_reader_spacev_get,
    READER_IFACE_CHANNEL_GET    hrpt_reader_channel_get,
    READER_IFACE_FRAME_COUNTER  hrpt_reader_frame_counter_get
};

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

static int hrpt_reader_init_common(hrpt_reader_t* reader)
{
    reader->frame_counter = 0;
    reader->prt_offset = 0;
}

int hrpt_reader_init_raw16(hrpt_reader_t* reader)
{
    reader->mode = HRPT_READER_RAW16;
    reader->packed_buf = (unsigned char*)reader->frame;
    reader->read_size = HRPT_FRAME_SIZE*sizeof(uint16_t);

    hrpt_reader_init_common(reader);
}

const reader_info_t hrpt_reader_raw16 = {
    READER_INFO_IFACE       &hrpt_reader_interface,
    READER_INFO_CTX_SIZE    sizeof(hrpt_reader_t),
    READER_INFO_INIT        hrpt_reader_init_raw16
};

int hrpt_reader_init_dundee(hrpt_reader_t* reader)
{
    reader->mode = HRPT_READER_DUNDEE;
    reader->packed_buf = malloc(HRPT_DUNDEE_BLOCK_SIZE);
    reader->read_size = HRPT_DUNDEE_BLOCK_SIZE;

    hrpt_reader_init_common(reader);
}

const reader_info_t hrpt_reader_dundee = {
    READER_INFO_IFACE       &hrpt_reader_interface,
    READER_INFO_CTX_SIZE    sizeof(hrpt_reader_t),
    READER_INFO_INIT        hrpt_reader_init_dundee
};

static int hrpt_reader_parse(hrpt_reader_t* reader, uint16_t* frame)
{
    // // Time
    // {
    //     uint16_t day_of_year = frame[8] >> 1;
    //     uint64_t milliseconds = (frame[9] & 0x7F) << 20 | (frame[10] << 10) | frame[11];
    // }

    // Patch Temp
    reader->patch_temp = frame[20];

    // PRTs
    {
        uint8_t current_prt = (reader->frame_counter - reader->prt_offset) % 5;
        bool prt_sync = !current_prt;

        uint16_t prt1 = frame[17];
        uint16_t prt2 = frame[18];
        uint16_t prt3 = frame[19];

        if(!prt_sync)
        {
            reader->prts[0][current_prt-1] = prt1;
            reader->prts[1][current_prt-1] = prt2;
            reader->prts[2][current_prt-1] = prt3;
        }

        // PRT sync
        if((prt1 | prt2 | prt3) == 0) // PRT blanking
        {
            // printf("%i\n", reader->prts[0][2]);
            // desync
            if(!prt_sync)
            {
                reader->prt_offset = reader->frame_counter % 5;
                fprintf(stderr, "PRT desync! resetting to %u\n", reader->prt_offset);
            }
        }
        else if(prt_sync) // desync during PRT data
        {
            // printf("PRT sync error at %u\n", reader->frame_counter);
        }
    }

    // Calib View
    {
        uint32_t calib_views_avg[3] = {0,0,0};
        for(uint8_t word=0; word < 30; word++)
        {
            uint8_t ch = word % 3;

            calib_views_avg[ch] += frame[22+word];
        }
        for(uint8_t ch=0; ch < 3; ch++)
        {
            reader->calib_views[ch] = calib_views_avg[ch] / 10;
        }
    }

    // Space View
    {
        uint32_t space_views_avg[5] = {0,0,0,0,0};
        for(uint8_t word=0; word < 50; word++)
        {
            uint8_t ch = word % 5;

            space_views_avg[ch] += frame[52+word];
        }
        for(uint8_t ch=0; ch < 5; ch++)
        {
            reader->space_views[ch] = space_views_avg[ch] / 10;
        }
    }

    // AVHRR data
    {
        for(size_t word=0; word < 10240; word++)
        {
            uint8_t ch = word % 5;

            reader->channels[ch][word/5] = frame[750+word];
        }
    }

    reader->frame_counter++;
    return 0;
}

// reader interface
int hrpt_reader_open(hrpt_reader_t* reader, char* filename)
{
    // Not much to do for raw HRPT with no headers
    reader->file = fopen(filename, "rb");
    if(!reader->file)
        return -1;

    return 0;
}

int hrpt_reader_close(hrpt_reader_t* reader)
{
    fclose(reader->file);
    if(reader->mode != HRPT_READER_RAW16)
    {
        free(reader->packed_buf);
    }
}

int hrpt_reader_readline(hrpt_reader_t* reader)
{
    if(fread(reader->packed_buf, 1, reader->read_size, reader->file) != reader->read_size)
    {
        return -1;
    }

    // unpack
    if(reader->mode == HRPT_READER_DUNDEE)
    {
        dundee_repack(reader->packed_buf, HRPT_DUNDEE_BLOCK_SIZE, reader->frame);
    }

    hrpt_reader_parse(reader, reader->frame);

    return 0;
}

// return uint16_t of patch_temp
uint16_t hrpt_reader_patch_temp_get(hrpt_reader_t* reader)
{
    return reader->patch_temp;
}

// provide uint16_t[4] pointer of synchronised PRTs for selected channel
uint16_t* hrpt_reader_prts_get(hrpt_reader_t* reader, uint8_t channel)
{
    return reader->prts[channel];
}

// return uint16_t of average calib view
uint16_t hrpt_reader_calibv_get(hrpt_reader_t* reader, uint8_t channel)
{
    if(channel < 2)
    {
        return 0;
    }
    else
    {
        return reader->calib_views[channel - 2];
    }
}

// return uint16_t of average space view
uint16_t hrpt_reader_spacev_get(hrpt_reader_t* reader, uint8_t channel)
{
    return reader->space_views[channel];
}

// provide uint16_t[2048] pointer of selected channel
uint16_t* hrpt_reader_channel_get(hrpt_reader_t* reader, uint8_t channel)
{
    return reader->channels[channel];
}

size_t hrpt_reader_frame_counter_get(hrpt_reader_t* reader)
{
    return reader->frame_counter;
}