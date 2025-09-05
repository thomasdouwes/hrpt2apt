#include "reader.h"
#include <stdlib.h>

int reader_init(reader_t* reader, reader_info_t info)
{
    *reader = *info.interface;

    reader->ctx = malloc(info.context_size);

    info.init(reader->ctx);
}

int reader_free(reader_t* reader)
{
    free(reader->ctx);
}