#ifndef SERVER
#define SERVER

/*
 *  These are the function declarations of the server.
 */

#include <arpa/inet.h>
#include <curl/curl.h>
#include <dirent.h>
#include <errno.h>
#include "json-c/json.h"
#include "minimal_multipart_parser.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

// User custom path
#define USER_CUSTOM_BASE_PATH "/home/alexa/"
// Example of the USER_CUSTOM_ENDPOINT_STR "http://127.0.0.1:8000/v1/chat/completions"
#define USER_CUSTOM_ENDPOINT_STR "http://127.0.0.1:8000/v1/chat/completions";
// Example of the USER_CUSTOM_GGML_VK_VISIBLE_DEVICES_NUM "GGML_VK_VISIBLE_DEVICES=1 ";
// If you have a second GPU, set the variable to 1. If you're unsure, use nvtop to find the integer
#define USER_CUSTOM_GGMLK_VK_VISIBLE_DEVICES_NUM "GGML_VK_VISIBLE_DEVICES=0 "

// API
#define API_ERROR -1
#define API_PING 5
#define API_SUMMARIZE 3
#define API_TRANSCRIBE 2
#define API_UPLOAD 1

// BOOLEAN
#define CURLE_OK 0
#define DOES_NOT_EXIST -1
#define EXTRA_BYTES 2
#define FALSE 0
#define IS_ERROR -1
#define IS_TRUE 0
#define TRUE 1

#define ERROR 0
#define EIGHT_THOUSAND_BYTES 8000
#define ENDING_CHARS 4
#define FOUR_BYTES_FOR_POST 4
#define IDX_ZERO 0
#define ONE_BYTE 1
#define ONE_BYTE_FOR_NULL 1
#define ONE_HUNDRED_BYTES 100
#define ONE_HUNDRED_THOUSAND_BYTES 100000
#define ONE_RUNNING_PROCESS 1
#define SEVEN_NEW_LINES_BEFORE_FILENAME_BODY 7
#define STARTING_API_AT_INDEX_ZERO 0
#define STARTING_BUFF_AT_INDEX_FIVE 5
#define THIRTY_TWO_THOUSAND_BYTES 32000
#define TWELVE_THOUSAND_EIGHT_HUNDRED_BYTES 12800
#define TWENTY_CHAR_FOR_API_BUFF 20
#define TWO_THOUSAND_BYTES 2000

int check_if_post_request(char *buf);
void check_if_video_transcribe_and_wav_directory_exists(int connect_d);
int compare_keys(char* desired_key, char* client_input_key);
void create_send_key_value_to_client(int connect_d, char *key, char *value);
int execute_upload(int connect_d, char *buf);
int execute_transcribe(int connect_d, char *buf);
int execute_summarize(int connect_d, char* buf);
char *get_data_from_curl(char* connect_d);
char *make_a_string(char* beginning, char* middle, char* end);
char *make_demux_decode_bash_command(char* video_unix_time_mp4, char* transcribe_filename_wav);
char *make_ffmpeg_bash_script(char* video_unix_time_mp4, char* transcribe_filename_wav);
char *make_json_data_str(char* transcription_filename_output_txt, char* data_results);
char *make_whisper_transcription_str(char* transcribed_unix_output_wav_result, char* transcribed_unix_output_txt_result);
void send_a_http_response(int connect_d, char* http_code_num_and_comment, char* content_type, char* http_code_message);
int send_200_ok_response(int connect_d);
void send_400_bad_response(int connect_d);
void send_500_internal_server_error(int connect_d);
char *unix_time(void);
char *unix_time_mp4(char* unix_time);

#endif