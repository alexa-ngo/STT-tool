#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

/*
    The headers for the demux_decode.c program
*/

AVCodecContext* audio_dec_context = NULL;
AVStream* audio_stream = NULL;
AVFormatContext* format_context = NULL;

AVFrame* frame = NULL;
AVPacket* packet = NULL;

FILE* audio_final_file = NULL;
char* audio_final_filename = NULL;
int audio_frame_count = 0;
int audio_stream_idx = -1;
char* input_filename = NULL;


