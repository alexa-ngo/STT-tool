#include "demux_decode.h"

/*

This program reads the frames from an input file, decodes the file, and writes the decoded
audio frame to a raw audio file called audio_output file.

Run this program:
    ./demux-decode your_input_file your_output_audio

*/

/* The audio frame is output in the first plane. This works for output planar audio
	which uses a separate channel like AV_SAMPLE_FMT_S16P.
	This code will only write the first audio channel.
	We should use libswresample or libavfilter to convert the frame to packed data.
*/
int output_audio_frame(AVFrame* frame) {
    size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(frame->format);
    printf("The audio frame number: %d Number Samples:%d pts:%s\n", audio_frame_count++, frame->nb_samples, av_ts2timestr(frame->pts, &audio_dec_context->time_base));
    fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_final_file);

    return 0;
}

// Decode each packet
int decode_packet(AVCodecContext* decode, const AVPacket* packet) {
    int is_keep_running = 0;

    // Send the packet to the decoder
    is_keep_running = avcodec_send_packet(decode, packet);
    if (is_keep_running < 0) {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(is_keep_running));
        exit(1);
    }

    // get all the available frames from the decoder
    while (is_keep_running >= 0) {
        is_keep_running = avcodec_receive_frame(decode, frame);
        if (is_keep_running < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (is_keep_running == AVERROR_EOF || is_keep_running == AVERROR(EAGAIN)) {
                return 0;
			}
            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(is_keep_running));
            return is_keep_running;
        }
        // write the frame data to output file
        is_keep_running = output_audio_frame(frame);
        av_frame_unref(frame);
    }
    return is_keep_running;
}

int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_context, AVFormatContext *format_context, enum AVMediaType type) {
    int is_keep_running, stream_index;
    AVStream *st;
    const AVCodec *dec = NULL;

    is_keep_running = av_find_best_stream(format_context, type, -1, -1, NULL, 0);
    if (is_keep_running < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n", av_get_media_type_string(type), input_filename);
        exit(1);
    } else {
        stream_index = is_keep_running;
        st = format_context->streams[stream_index];

        // Find the decoder for the stream
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(type));
            exit(1);
        }

        // Allocate the context for the stream
        *dec_context = avcodec_alloc_context3(dec);
        if (!*dec_context) {
			fprintf(stderr, "Failed to allocate the %s codec context\n", av_get_media_type_string(type));
            exit(1);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((is_keep_running = avcodec_parameters_to_context(*dec_context, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n", av_get_media_type_string(type));
            exit(1);
        }

        /* Init the decoders */
        if ((is_keep_running = avcodec_open2(*dec_context, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return is_keep_running;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

// Get the format for the sample
int get_format_from_sample_format(const char **format, enum AVSampleFormat sample_format) {

	struct sample_format_entry {
        enum AVSampleFormat sample_format;
        const char *format_be, *format_le;
	}

    sample_format_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *format = NULL;

    for (int i = 0; i < FF_ARRAY_ELEMS(sample_format_entries); i++) {
        struct sample_format_entry *entry = &sample_format_entries[i];
        if (sample_format == entry->sample_format) {
            *format = AV_NE(entry->format_be, entry->format_le);
            return 0;
        }
    }
    fprintf(stderr, "sample format %s is not supported as output format\n", av_get_sample_fmt_name(sample_format));
    return -1;
}

int main (int argc, char* argv[]) {

	int is_found = 0;

    if (argc != 3) {
	fprintf(stderr, "Usage: %s your_input_file your_audio_output_file\n", argv[0]);
        exit(1);
    }
    input_filename = argv[1];
    audio_final_filename = argv[2];

    // Open the file input
    if (avformat_open_input(&format_context, input_filename, NULL, NULL) < is_found) {
        fprintf(stderr, "Can't open input file %s\n", input_filename);
        exit(1);
    }
    // Get the stream from the file
    if (avformat_find_stream_info(format_context, NULL) < is_found) {
        fprintf(stderr, "Can't find the stream information\n");
        exit(1);
    }
    // Open the content within the final file
    if (open_codec_context(&audio_stream_idx, &audio_dec_context, format_context, AVMEDIA_TYPE_AUDIO) >= is_found) {
        audio_stream = format_context->streams[audio_stream_idx];
        audio_final_file = fopen(audio_final_filename, "wb");
        if (!audio_final_file) {
            fprintf(stderr, "Can't open the final desintation %s\n", audio_final_filename);
            exit(1);
        }
    }
    // Output input filename information
    av_dump_format(format_context, 0, input_filename, 0);
    // Check if there is an audio stream
    if (!audio_stream) {
        fprintf(stderr, "There is no audio stream\n");
        exit(1);
    }
    // Get each frame
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        exit(1);
    }
	// Check if there is a packet
    packet = av_packet_alloc();
    if (!packet) {
        fprintf(stderr, "Could not allocate packet\n");
        exit(1);
    }
    if (audio_stream) {
        printf("Demultiplexing the audio from file '%s' into '%s'\n", input_filename, audio_final_filename);
	}
    int is_keep_running = 0;
    // Read the frames from the file
    while (av_read_frame(format_context, packet) >= is_keep_running) {
        // check if the packet belongs to a stream we are interested in, otherwise skip it
        if (packet->stream_index == audio_stream_idx) {
            is_keep_running = decode_packet(audio_dec_context, packet);
		}
        av_packet_unref(packet);
        if (is_keep_running < 0)
            break;
    }

    // Flush the decoders
    if (audio_dec_context) {
        decode_packet(audio_dec_context, NULL);
    }

    printf("Demuxing succeeded.\n");

    if (audio_stream) {
        enum AVSampleFormat sformat = audio_dec_context->sample_fmt;
        int num_of_channels = audio_dec_context->ch_layout.nb_channels;
        const char *format;

        if (av_sample_fmt_is_planar(sformat)) {
            const char *packed = av_get_sample_fmt_name(sformat);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            sformat = av_get_packed_sample_fmt(sformat);
            num_of_channels = 1;
        }

        if ((is_keep_running = get_format_from_sample_format(&format, sformat)) < 0) {
			exit(1);
		}

        printf("Play the output audio file with the command:\n"
               "ffplay -f %s -ar %d %s\n",
               format, audio_dec_context->sample_rate,
               audio_final_filename);
    }

    avcodec_free_context(&audio_dec_context);
    avformat_close_input(&format_context);

    // Close the audio file
    if (audio_final_file)
        fclose(audio_final_file);
    av_packet_free(&packet);
    av_frame_free(&frame);

    return is_keep_running < 0;
}
