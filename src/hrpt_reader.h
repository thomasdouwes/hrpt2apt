#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "reader.h"

#define HRPT_FRAME_SIZE 11090
#define HRPT_DUNDEE_BLOCK_SIZE 14800

typedef enum {
    HRPT_READER_RAW16,
    HRPT_READER_DUNDEE
} hrpt_reader_mode_t;

// "This could have been a class"
typedef struct {
    FILE* file;
    size_t prt_offset;
    size_t frame_counter;
    uint16_t frame[HRPT_FRAME_SIZE + 10];
    unsigned char* packed_buf;
    hrpt_reader_mode_t mode;
    size_t read_size;

    // line data
    uint16_t patch_temp;
    uint16_t prts[3][4];
    uint16_t calib_views[3];
    uint16_t space_views[5];
    uint16_t channels[5][2048];
} hrpt_reader_t;


int hrpt_reader_open(hrpt_reader_t* reader, char* filename);
int hrpt_reader_close(hrpt_reader_t* reader);
int hrpt_reader_readline(hrpt_reader_t* reader);
uint16_t hrpt_reader_patch_temp_get(hrpt_reader_t* reader);
uint16_t* hrpt_reader_prts_get(hrpt_reader_t* reader, uint8_t channel);
uint16_t hrpt_reader_calibv_get(hrpt_reader_t* reader, uint8_t channel);
uint16_t hrpt_reader_spacev_get(hrpt_reader_t* reader, uint8_t channel);
uint16_t* hrpt_reader_channel_get(hrpt_reader_t* reader, uint8_t channel);
size_t hrpt_reader_frame_counter_get(hrpt_reader_t* reader);

const extern reader_info_t hrpt_reader_raw16;
const extern reader_info_t hrpt_reader_dundee;