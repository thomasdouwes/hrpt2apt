#pragma once
#include <stdint.h>
#include <stddef.h>

// Standard reader interface
// This could do with being more instrument independant
// I should just have made this program in C++
typedef struct {
    int (*open)(void* context, char* filename);
    int (*close)(void* context);
    int (*readline)(void* context);
    uint16_t (*patch_temp_get)(void* context);
    uint16_t* (*prts_get)(void* context, uint8_t channel);
    uint16_t (*calibv_get)(void* context, uint8_t channel);
    uint16_t (*spacev_get)(void* context, uint8_t channel);
    uint16_t* (*channel_get)(void* context, uint8_t channel);
    size_t (*frame_counter_get)(void* context);
    void* ctx;
} reader_t;

typedef struct {
    reader_t* interface;
    size_t context_size;
    int (*init)(void* context);
} reader_info_t;

// evil stuff
#define READER_IFACE_OPEN .open = (int(*)(void*, char*))
#define READER_IFACE_CLOSE .close = (int(*)(void*))
#define READER_IFACE_READLINE .readline = (int(*)(void*))
#define READER_IFACE_PATCH_TEMP_GET .patch_temp_get = (uint16_t(*)(void*))
#define READER_IFACE_PRTS_GET .prts_get = (uint16_t*(*)(void*, uint8_t))
#define READER_IFACE_CALIBV_GET .calibv_get = (uint16_t(*)(void*, uint8_t))
#define READER_IFACE_SPACEV_GET .spacev_get = (uint16_t(*)(void*, uint8_t))
#define READER_IFACE_CHANNEL_GET .channel_get = (uint16_t*(*)(void*, uint8_t))
#define READER_IFACE_FRAME_COUNTER .frame_counter_get = (size_t(*)(void*))

#define READER_INFO_IFACE .interface =
#define READER_INFO_CTX_SIZE .context_size =
#define READER_INFO_INIT .init = (int(*)(void*))

int reader_init(reader_t* reader, reader_info_t info);
int reader_free(reader_t* reader);