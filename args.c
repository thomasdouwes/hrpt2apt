#include "args.h"
#include <argp.h>
#include <string.h>
#include <stdlib.h>
#include "defines.h"

// Stupid strcmp stack, but only called a few times
int channel_from_string(char* buf)
{
    if(!strcmp(buf, "ch1")) {
        return AVHRR_CH1;
    } else if(!strcmp(buf, "ch2"))
    {
        return AVHRR_CH2;
    } else if(!strcmp(buf, "ch3a"))
    {
        return AVHRR_CH3a;
    } else if(!strcmp(buf, "ch3b"))
    {
        return AVHRR_CH3b;
    } else if(!strcmp(buf, "ch4"))
    {
        return AVHRR_CH4;
    } else if(!strcmp(buf, "ch5"))
    {
        return AVHRR_CH5;
    } 
    return -1;
}

error_t parse_arg(int key, char* arg, struct argp_state* state)
{
    arguments_t* arguments = (arguments_t*)state->input;

    switch(key)
    {
        case ARGP_KEY_INIT:
            memset(arguments, 0, sizeof(arguments_t));
            arguments->channel_a = AVHRR_CH2;
            arguments->channel_b = AVHRR_CH4;
            break;
        case ARGP_KEY_END:
            if(!arguments->input_file)
            {
                argp_error(state, "No input file specified!");
            }
            if(!(arguments->output_file_raw || arguments->output_file_png || arguments->output_file_wav || arguments->output_file_iq))
            {
                argp_error(state, "At least one output type must be specified");
            }
            break;
        case 'i':
            arguments->input_file = arg;
            break;
        case 'r':
            arguments->output_file_raw = arg;
            break;
        case 'p':
            arguments->output_file_png = arg;
            argp_error(state, "PNG output not currently implemented");
            break;
        case 'w':
        case 'o':
            arguments->output_file_wav = arg;
            argp_error(state, "WAV output not currently implemented");
            break;
        case 'q':
            arguments->output_file_iq = arg;
            argp_error(state, "IQ output not currently implemented");
            break;
        case 'd':
            arguments->dundee_format = true;
            break;
        case 't':
            char* end;
            int i = strtol(arg, &end, 10);
            if(*end != 0)
            {
                argp_error(state, "Year format error");
            }
            if(i < 0)
            {
                argp_error(state, "Year cannot be negative (I don't think ancient rome had any weather satellites)");
            }
            arguments->year = i;
            argp_error(state, "WXtoIMG timestamp not currently implemented");
            break;
        case 'a':
                {
                    int ch = channel_from_string(arg);
                    if(ch == -1)
                    {
                        argp_error(state, "Invalid channel A");
                    }
                    arguments->channel_a = ch;
                }
            break;
        case 'b':
                {
                    int ch = channel_from_string(arg);
                    if(ch == -1)
                    {
                        argp_error(state, "Invalid channel B");
                    }
                    arguments->channel_b = ch;
                }
            break;
        case 'c':
            argp_error(state, "Terminator switching not currently implemented");
            break;
        default:
            break;
    }
    return 0;
}

static struct argp_option argp_options[] = {
    {"input", 'i', "INPUT_FILE", 0, "Input HRPT file"},
    {"raw", 'r', "OUTPUT_RAW", 0, "Raw data output file"},
    {"png", 'p', "OUTPUT_PNG", 0, "PNG output file"},
    {"wav", 'w', "OUTPUT_WAV", 0, "WAV output file"},
    {"output", 'o', "OUTPUT_WAV", OPTION_ALIAS, "Output file"},
    {"iq", 'q', "OUTPUT_IQ", 0, "IQ output file"},
    {"dundee", 'd', 0, OPTION_ARG_OPTIONAL, "Decode input file as dundee HRPT format, this is a switch, it takes no arguments"},
    {"wxtoimg_timestamp", 't', "YEAR", 0, "Timestamp WAV file with end of pass from HRPT time for WXtoIMG. Takes year as argument"},
    {"channel_a", 'a', "CHANNEL", 0, "Channel to put on APT Channel A (default ch2)"},
    {"channel_b", 'b', "CHANNEL", 0, "Channel to put on APT Channel B (default ch4)"},
    {"channel_c", 'c', "CHANNEL", 0, "Channel to switch to when passing over daylight terminator, no switch if not specified"},
    { 0 }
};

static char argp_documentation[] =
    "Converts NOAA HRPT to NOAA APT with correct space/calibration data from HRPT.\n"
    "Takes raw16(default) or packed dundee format HRPT.\n"
    "Can output APT as:\n"
    "   raw data: raw 8 bit values for each sample for use with GNU radio flowchart\n"
    "   PNG: output the APT image as a PNG without any signal processing\n"
    "   WAV: outputs the APT as if it were decoded with an FM demod, can be used in most APT decoding software\n"
    "   IQ: generate an FM modulated APT signal following the specifications of actual transmission\n"
    "Multiple output options can be specified\n"
    "Available AVHRR channels are:\n"
    "   ch1\n"
    "   ch2\n"
    "   ch3a\n"
    "   ch3b\n"
    "   ch4\n"
    "   ch5\n"
    "\n"
    "Only RAW output currently implemented!!!";

static struct argp argp_parser = { argp_options, parse_arg, 0, argp_documentation };

// Little wrapper to keep arg parsing logic out of main.c
void args_parse(int argc, char* argv[], arguments_t* arguments)
{
    argp_parse(&argp_parser, argc, argv, 0, 0, arguments);
}