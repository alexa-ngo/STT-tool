#include "server.h"

// Represents the listener
int listener_d;

// Uploads the input file as a .mp4 into the video directory (Ex. video/171234.mp4)
int run_data_parser(char* final_filename_output, FILE* input_stream_from_connection) {
    MinimalMultipartParserContext state = {0};  // Reset the state for each upload
    FILE *output = (FILE*) fopen(final_filename_output, "wb");
    if (output == NULL) {
      exit(0);
    }
    int c;
    // Handles incoming stream by each character
    while ((c = getc(input_stream_from_connection)) != EOF) {
        const MultipartParserEvent event = minimal_multipart_parser_process(&state, (char)c);
        // Special Events That Needs Handling
        if (event == MultipartParserEvent_DataBufferAvailable) {
            for (unsigned int j = 0; j < minimal_multipart_parser_get_data_size(&state); j++) {
                const char rx = minimal_multipart_parser_get_data_buffer(&state)[j];
                fputc(rx, output);
            }
        } else if (event == MultipartParserEvent_DataStreamCompleted) {
            // Datastream Finished
            break;
        }
    }
    // Check if file has been received
    if (minimal_multipart_parser_is_file_received(&state)) {
        fclose(output);
    }
}

// Converts an integer to a string
char* num_2_key_str(int num) {
    int idx = 0;
    char* buffer = malloc(sizeof(char) * ONE_HUNDRED_BYTES);
    int quotient = num;
    while (quotient > 0) {
        int digit = quotient % 10;
        char v = '0' + digit;
        buffer[idx] = v;
        idx++;
        quotient = quotient / 10;
    }
    buffer[idx] = '\0';		// Need the null terminating character

    // Reverse the string because the "number" is now backwards.
    int buffer_length = idx;
    for (int i = 0, j = buffer_length -1; i < j; i++, j--) {
        char t = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = t;
    }
    return buffer;
}

// Bind to a port
void bind_to_port(int socket, int port) {
    struct sockaddr_in the_socket;
    the_socket.sin_family = PF_INET;
    the_socket.sin_port = (in_port_t) htons(port);
    the_socket.sin_addr.s_addr = htonl(INADDR_ANY);
    // Reuse the socket to restart the server without a problem
    int reuse = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int)) == -1) {
        fprintf(stderr, "Can't set the reuse option on the socket");
        exit(1);
    }
    // Bind to a socket
    int c = bind(socket, (struct sockaddr *) &the_socket, sizeof(the_socket));
    if (c == -1) {
        fprintf(stderr, "Can't bind to socket");
        exit(1);
    }
}

// Catch the signal handle
int catch_signal(int sig, void (*handler)(int)) {
    struct sigaction action;
    action.sa_handler = handler;
    // Use an empty mask
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    close(listener_d);
    return sigaction(sig, &action, NULL);
}

int check_if_options_request(char* original_buf) {
    char* buf = strdup(original_buf);
    uint16_t len_options = strlen("OPTIONS");
    char* options_request_buffer = malloc(sizeof(char) * (len_options + ONE_BYTE_FOR_NULL));

    int idx = 0;
    for (idx = 0; idx < len_options; idx++) {
        char curr = *(buf + idx);
        *(options_request_buffer + idx) = curr;
    }
    // End the string with a null terminating character
    *(options_request_buffer + idx) = '\0';
    int is_options_request = (strcmp(options_request_buffer, "OPTIONS") == IS_TRUE);

    free(buf);
    free(options_request_buffer);
    return is_options_request;
}

// Checks if the response is a POST request
int check_if_post_request(char* original_buf) {
    char* buf = strdup(original_buf);
    char* post_request_buffer = malloc(sizeof(char) * (FOUR_BYTES_FOR_POST + ONE_BYTE_FOR_NULL));

    int idx = 0;
    for (idx = 0; idx < FOUR_BYTES_FOR_POST; idx++) {
        char curr = *(buf + idx);
        *(post_request_buffer + idx) = curr;
    }
    // End the string with a null terminating character
    *(post_request_buffer + idx) = '\0';
    int is_post_request = (strcmp(post_request_buffer, "POST") == IS_TRUE);

    free(buf);
    free(post_request_buffer);
    return is_post_request;
}

// Create and send a JSON object (as a string) with a key and value
void create_send_key_value_to_client(int connect_d, char* key, char* value) {

    json_object *root_key_result = json_object_new_object();
    if (!root_key_result) {
        fprintf(stderr, "Failed to create root JSON object for upload.\n");
        json_object_put(root_key_result);
        close(connect_d);
        exit(1);
    }
    json_object_object_add(root_key_result, key, json_object_new_string(value));
    char *curl_value = (char *)json_object_to_json_string_ext(root_key_result, JSON_C_TO_STRING_PRETTY);

    // Make the HTTP Response
    char *http_upload_official = malloc(ONE_HUNDRED_THOUSAND_BYTES);
    char *http_str = "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: ";
    char *cors_header = "\nAccess-Control-Allow-Origin: *";
    memset(http_upload_official, '\0', ONE_HUNDRED_THOUSAND_BYTES);
    strcat(http_upload_official, http_str);
    int key_len = strlen(curl_value);
    char *filename_curl_len_as_str = num_2_key_str(key_len);
    strcat(http_upload_official, filename_curl_len_as_str);
    strcat(http_upload_official, cors_header);
    // Add the line break
    char *two_slash_n = "\n\n";
    strcat(http_upload_official, two_slash_n);
    strcat(http_upload_official, curl_value);
    // Add the NULL character
    char *null_char = "\0";
    strcat(http_upload_official, null_char);

    // Send the built HTTP Response to the client
    int send_200_ok_data = send(connect_d, http_upload_official, strlen(http_upload_official), 0);
    if (send_200_ok_data == DOES_NOT_EXIST) {
        fprintf(stderr, "Error in sending the 200 OK data.\n");
        close(connect_d);
        exit(0);
    }
    free(filename_curl_len_as_str);
    free(http_upload_official);
    json_object_put(root_key_result);
}

// Determine between /api/upload, /api/transcribe, /api/summarize
int determine_api_call(char *buf) {
    char* api_call_buffer = malloc(sizeof(char) * (TWENTY_CHAR_FOR_API_BUFF + ONE_BYTE));
    int api_idx = STARTING_API_AT_INDEX_ZERO;
    int buf_idx = STARTING_BUFF_AT_INDEX_FIVE;
    char curr = *(buf + buf_idx);
    while (curr != ' ') {
        curr = *(buf + buf_idx);
        *(api_call_buffer + api_idx) = curr;
        api_idx++;
        buf_idx++;
    }
    // Need to include the null terminating character
    *(api_call_buffer + api_idx) = '\0';
    if (strcmp(api_call_buffer, "/api/upload ") == IS_TRUE) {
        free(api_call_buffer);
        return API_UPLOAD;
    } else if (strcmp(api_call_buffer, "/api/transcribe ") == IS_TRUE) {
        free(api_call_buffer);
        return API_TRANSCRIBE;
    } else if (strcmp(api_call_buffer, "/api/summarize ") == IS_TRUE) {
        free(api_call_buffer);
        return API_SUMMARIZE;
    } else if (strcmp(api_call_buffer, "/api/ping ") == IS_TRUE) {
        free(api_call_buffer);
        return API_PING;
    } else {
        printf("This is the API call: %s\n", api_call_buffer);
        free(api_call_buffer);
        return API_ERROR;
    }
    return 0;
}

// Check if directory exist
bool does_directory_exist(char* directory_name) {

    DIR* dir = opendir(directory_name);
    if (dir) {
        // If directory exists
        closedir(dir);
        return true;
    } else if (ENOENT == errno)
        // Directory doesn't exist
        return false;
}

void check_if_video_transcribe_and_wav_directory_exists(int connect_d) {
    bool video_dir_exists = does_directory_exist("video");
    if (video_dir_exists == false) {
        char* http_code_num_and_comment = "400 Bad Response";
        char* content_type = "text/plain";
        char* http_code_message = "400 Bad Response. The video directory doesn't exist.\n";
        send_a_http_response(connect_d, http_code_num_and_comment, content_type, http_code_message);
        exit(1);
    }
    bool transcribe_dir_exists = does_directory_exist("transcribed");
    if (transcribe_dir_exists == false) {
        char* http_code_num_and_comment = "400 Bad Response";
        char* content_type = "text/plain";
        char* http_code_message = "400 Bad Response. The transcribe directory doesn't exist.\n";
        send_a_http_response(connect_d, http_code_num_and_comment, content_type, http_code_message);
        exit(1);
    }
    bool wav_dir_exists = does_directory_exist("wav-files");
    if (wav_dir_exists == false) {
        char* http_code_num_and_comment = "400 Bad Response";
        char* content_type = "text/plain";
        char* http_code_message = "400 Bad Response. The wav directory doesn't exist.\n";
        send_a_http_response(connect_d, http_code_num_and_comment, content_type, http_code_message);
        exit(1);
    }
}

// Store a copy of the file as an .mp4
// curl -X POST -H 'Content-Type: video/mp4' -F "bob=@/home/jane_doe/Code/STT-tool/i-have-a-dream.mp4" http://localhost:1234/api/upload
void make_a_copy_of_the_original_file(int connect_d, char *unix_time_result) {

    char *mp4 = ".mp4";
    char *video = "video/";
    char *video_unix_time_mp4_result = make_a_string(video,unix_time_result,mp4);
    char *unix_time_mp4_result = make_a_string(unix_time_result,mp4, NULL);

    // Make a copy of the original file
    FILE *input_stream_from_fdopen_connection = (FILE *) fdopen(connect_d, "r");
    run_data_parser(video_unix_time_mp4_result, input_stream_from_fdopen_connection);
    char *key = "filename";
    create_send_key_value_to_client(connect_d, key, unix_time_mp4_result);
    if (input_stream_from_fdopen_connection) {
        fclose(input_stream_from_fdopen_connection);
    }
    free(unix_time_mp4_result);
    free(video_unix_time_mp4_result);
}

// Execute the Upload API
int execute_upload(int connect_d, char* buf)
{
    // Check if the video, transcribe, and wav directories exist
    check_if_video_transcribe_and_wav_directory_exists(connect_d);

    // Generate unix time to be shared
    char *unix_time_result = unix_time();
    char *unix_time_str = malloc(sizeof(char) * strlen(unix_time_result) + EXTRA_BYTES);
    strcpy(unix_time_str, unix_time_result);

    // Make a copy of the .mp4 in the video directory for the client to process at a later time.
    // Send the filename back to the client and close the connection after sending.
    make_a_copy_of_the_original_file(connect_d, unix_time_str);

    if (unix_time_str)
    {
        free(unix_time_str);
    }
    if (buf) {
        free(buf);
    }
    if (connect_d)
    {
        close(connect_d);
    }
    // An if statement is not needed when it's always true
    free(unix_time_result);

    printf("Done uploading\n");
    exit(0);
}

// Get the data from the curl request
// Example sent from the client: {"data": "Hi there"}
char* get_data_from_curl(char* buf) {
    char* client_entered_str = malloc(sizeof(char) * (THIRTY_TWO_THOUSAND_BYTES));
    int count_of_new_lines = 0;
    int filename_idx = 0;
    int len_of_buf = strlen(buf);

    // Iterate to the line to get the filename
    for (int idx = 0; idx < len_of_buf; idx++) {
        char curr = *(buf + idx);
        if (curr == 10) {
            count_of_new_lines++;
        }
        // if the code gets stuck here use have count_of_new_lines == 15 (new lines)
        if (count_of_new_lines == SEVEN_NEW_LINES_BEFORE_FILENAME_BODY) {
          *(client_entered_str + filename_idx) = curr;
            filename_idx++;
        }
    }
    *(client_entered_str + filename_idx) = '\0';
    return client_entered_str;
}

// Check if the value exists
char *get_key(char *client_entered_str) {

    char *final_client_key = malloc(ONE_HUNDRED_BYTES);
    int final_client_key_idx = 0;
    int client_entered_str_idx = 3;
    char curr = *(client_entered_str + client_entered_str_idx);
    while (curr != '\"')
    {
        curr = *(client_entered_str + client_entered_str_idx);
        if (curr != '\"') {
            client_entered_str_idx++;
            *(final_client_key + final_client_key_idx) = curr;
            final_client_key_idx++;
        }
    }
    *(final_client_key + final_client_key_idx) = '\0';
    return final_client_key;
}

// Compares the key in the JSON object
int compare_keys(char* desired_key, char* client_input_key) {

    if (strcmp(desired_key, client_input_key) == IS_TRUE) {
        return TRUE;
    }
    return FALSE;
}

// Get the filename from the curl request
// Example sent from client: {"filename": "1773.mp4"}
char* get_filename_from_curl(char* buf) {

    char* filename_result = malloc(sizeof(char) * (ONE_HUNDRED_BYTES));
    bool beginning_of_data = false;

    int idx_for_buf = 0;
    for (int i = 1; i < strlen(buf); i++) {
        char prev_char = *(buf + i - 1);
        char curr_char = *(buf + i);
        char next_char = *(buf + i + 1);
        if (prev_char == '\r' && curr_char == '\n' && next_char == '{') {
            beginning_of_data = true;
        }
        if (beginning_of_data == true) {
            *(filename_result + idx_for_buf) = curr_char;
            idx_for_buf++;
        }
    }
    // Must end the buf string with a null character
    *(filename_result + idx_for_buf) = '\0';

    // Make a json object for the filename_result using the json-c library
    // Example: {filename:1774097516.mp4"}
    json_object* j_data = json_tokener_parse(filename_result);
    json_object* objname;

    // The key is called "filename"
    char object_key[] = "filename";
    struct json_object *object;

    const char* j_data_result = json_object_get_string(j_data);
    if (j_data == NULL) {
        fprintf(stderr, "Unable to tokenize string\n");
        free(buf);
        free(filename_result);
        exit(1);
    }
    // Get the object
    if (!json_object_object_get_ex(j_data, object_key, &objname)) {
        fprintf(stderr, "Unable to find object '%s'\n", object_key);
        json_object_put(j_data);
        free(filename_result);
        free(buf);
        exit(1);
    }
    // Get the value from the object
    json_object_object_get_ex(j_data, object_key, &object);
    const char *value = json_object_get_string(object);
    char* copied_value = strdup(value);

    if (j_data) {
        json_object_put(j_data);
    }
    if (filename_result) {
        free(filename_result);
    }
    return copied_value;
}

// Makes a string using three arguments
char* make_a_string(char* beginning, char* middle, char* end) {
    char* str_result = malloc(EIGHT_THOUSAND_BYTES * sizeof(char));
    memset(str_result, '\0', EIGHT_THOUSAND_BYTES * sizeof(char));
    char* null_char = "\0";
    strcat(str_result, beginning);
    strcat(str_result, middle);
    if (end != NULL) {
        strcat(str_result, end);
    }
    strcat(str_result, null_char);
    return str_result;
}

// Make bash command for demux decode
char* make_demux_decode_bash_command(char* video_unix_time_mp4, char* transcribe_filename_wav) {

    char* bash_script_buffer_final = malloc( TWELVE_THOUSAND_EIGHT_HUNDRED_BYTES* sizeof(char));
    memset(bash_script_buffer_final, '\0', TWELVE_THOUSAND_EIGHT_HUNDRED_BYTES * sizeof(char));

    // To run the compiled program:
    // gcc demux_decode -I /usr/include/ffmpeg -lavcodec -lavutil -lavformat -o demux_decode.sh
    //                                            [video_input]   [wav_filename]
    // bash                      "demux_decode.sh video/17123.mp4 transcribed/17123.wav";
    char* bash_script_demux = "./demux_decode";
    char* space = " ";
    char* null_char = "\0";

    strcat(bash_script_buffer_final, bash_script_demux);
    strcat(bash_script_buffer_final, space);
    strcat(bash_script_buffer_final, video_unix_time_mp4);
    strcat(bash_script_buffer_final, space);
    strcat(bash_script_buffer_final, transcribe_filename_wav);
    strcat(bash_script_buffer_final, null_char);

    return bash_script_buffer_final;
}

// Create a streaming socket
int open_listener_socket(void) {
    int streaming_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (streaming_socket == IS_ERROR) {
        fprintf(stderr, "Can't open the socket");
        exit(1);
    }
    return streaming_socket;
}

// send an options response with the Access-Control-Allow-Methods header to allow the client to know that the server accepts POST and OPTIONS requests
void send_options_response(int connect_d, char* http_code_num_and_comment, char* content_type, char* http_code_message) {
    char *http_1_dot_1 = "HTTP/1.1 ";
    char *single_new_line_char = "\n";
    char *double_new_line_char = "\n\n";
    char *content_type_label = "Content-Type: ";
    char *content_len_label = "Content-Length: ";
    // should also allow all origins
    char *access_control_allow_methods = "Access-Control-Allow-Methods: POST, OPTIONS";
    char *access_control_allow_origin = "Access-Control-Allow-Origin: *";
    char *access_control_allow_headers = "Access-Control-Allow-Headers: Content-Type";
    char *null_term_char = "\0";
    char *built_http_options_response = malloc(ONE_HUNDRED_THOUSAND_BYTES * sizeof(char));;
    int content_len = strlen(http_code_message);
    char* content_len_as_str = num_2_key_str(content_len);
    memset(built_http_options_response, '\0', ONE_HUNDRED_THOUSAND_BYTES * sizeof(char));
    strcat(built_http_options_response, http_1_dot_1);
    strcat(built_http_options_response, http_code_num_and_comment);
    strcat(built_http_options_response, single_new_line_char);
    strcat(built_http_options_response, content_type_label);
    strcat(built_http_options_response, content_type);
    strcat(built_http_options_response, single_new_line_char);
    strcat(built_http_options_response, content_len_label);
    strcat(built_http_options_response, content_len_as_str);
    strcat(built_http_options_response, single_new_line_char);
    strcat(built_http_options_response, access_control_allow_origin);
    strcat(built_http_options_response, single_new_line_char);
    strcat(built_http_options_response, access_control_allow_methods);
    strcat(built_http_options_response, single_new_line_char);
    strcat(built_http_options_response, access_control_allow_headers);
    strcat(built_http_options_response, double_new_line_char);
    strcat(built_http_options_response, http_code_message);
    strcat(built_http_options_response, single_new_line_char);
    int send_http_options_response = send(connect_d, built_http_options_response, strlen(built_http_options_response), 0);
    if (send_http_options_response == IS_ERROR) {
        fprintf(stderr, "Error in sending HTTP response\n");
        close(connect_d);
        exit(1);
    }
    free(built_http_options_response);
    free(content_len_as_str);
}

// Build the HTTP response
void send_a_http_response(int connect_d, char* http_code_num_and_comment, char* content_type, char* http_code_message) {
    char *http_1_dot_1 = "HTTP/1.1 ";
    char *single_new_line_char = "\n";
    char *double_new_line_char = "\n\n";
    char *content_type_label = "Content-Type: ";
    char *content_len_label = "Content-Length: ";
    char *access_control_allow_origin = "Access-Control-Allow-Origin: *";
    char *null_term_char = "\0";
    char *built_http_ok_response = malloc(EIGHT_THOUSAND_BYTES * sizeof(char));;
    int content_len = strlen(http_code_message);
    char* content_len_as_str = num_2_key_str(content_len);

    // Clear the buffer first
    memset(built_http_ok_response, '\0', EIGHT_THOUSAND_BYTES * sizeof(char));
    strcat(built_http_ok_response, http_1_dot_1);
    strcat(built_http_ok_response, http_code_num_and_comment);
    strcat(built_http_ok_response, single_new_line_char);
    strcat(built_http_ok_response, content_type_label);
    strcat(built_http_ok_response, content_type);
    strcat(built_http_ok_response, single_new_line_char);
    strcat(built_http_ok_response, content_len_label);
    strcat(built_http_ok_response, content_len_as_str);
    strcat(built_http_ok_response, single_new_line_char);
    strcat(built_http_ok_response, access_control_allow_origin);
    strcat(built_http_ok_response, double_new_line_char);
    strcat(built_http_ok_response, http_code_message);
    strcat(built_http_ok_response, single_new_line_char);
    strcat(built_http_ok_response, null_term_char);

    int send_http_response = send(connect_d, built_http_ok_response, strlen(built_http_ok_response), 0);
    if (send_http_response == IS_ERROR) {
        fprintf(stderr, "Error in sending HTTP response\n");
        close(connect_d);
        exit(1);
    }
    free(built_http_ok_response);
    free(content_len_as_str);
}

// Build the ffmpeg command
char* make_ffmpeg_bash_script(char* video_unix_time_mp4, char* transcribe_unix_time_wav) {
    // Build and clear the official bash string
    char* ffmpeg_buf = malloc(EIGHT_THOUSAND_BYTES * sizeof(char));
    memset(ffmpeg_buf, '\0', EIGHT_THOUSAND_BYTES * sizeof(char));
    //char* ffmpeg_front_str = "ffmpeg -i /home/jane_doe/Code/STT-tool/Winner_Audio.mp4 -vn -acodec pcm_s16le /home/jane_doe/Code/STT-tool/transcribed/output.wav";
    char* ffmpeg_front_str = "ffmpeg -f f32le -ar 16000 -ac 2 -i";
    char* space = " ";
    char* null_char = "\0";

    strcat(ffmpeg_buf, ffmpeg_front_str);
    strcat(ffmpeg_buf, space);
    strcat(ffmpeg_buf, video_unix_time_mp4);
    strcat(ffmpeg_buf, space);
    strcat(ffmpeg_buf, transcribe_unix_time_wav);
    strcat(ffmpeg_buf, null_char);

    return ffmpeg_buf;
}

// Sends the transcription data to the client
char* make_json_data_str(char* transcription_filename_output_txt, char* data_results) {

    char* http_OK_filename_str_official = malloc(EIGHT_THOUSAND_BYTES);
    char* http_OK_filename_str = "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: ";
    char* json_data_for_client = malloc(EIGHT_THOUSAND_BYTES);
    memset(json_data_for_client, '\0', EIGHT_THOUSAND_BYTES);
    memset(http_OK_filename_str_official, '\0', EIGHT_THOUSAND_BYTES);

    // Build the official data string
    char* data_label = "{\"data\" : \"";
    char* right_brace = "}";
    char* null_char = "\0";
    char* quote = "\"";
    char* two_slash_n = "\n\n";

    FILE* file = fopen(transcription_filename_output_txt, "r");
    if (file) {
        int idx = 0;
        int c;
        while ((c = getc(file)) != EOF) {
            json_data_for_client[idx] = c;
            idx++;
        }
        fclose(file);
        json_data_for_client[idx] = '\0';
    }

    // Build and send to the client
    // {"data" : "[00:00:.. So even though we face"}
    char data_content_bytes[EIGHT_THOUSAND_BYTES] = "\0";
    strcat(data_content_bytes, data_label);
    strcat(data_content_bytes, json_data_for_client);
    strcat(data_content_bytes, quote);
    strcat(data_content_bytes, right_brace);
    strcat(data_content_bytes, two_slash_n);
    strcat(data_content_bytes, null_char);

    // Build the whole HTTP request
    strcat(http_OK_filename_str_official, http_OK_filename_str);
    int data_len = strlen(data_content_bytes);
    char* data_len_as_str = num_2_key_str(data_len);
    strcat(http_OK_filename_str_official, data_len_as_str);
    strcat(http_OK_filename_str_official, two_slash_n);
    strcat(http_OK_filename_str_official, data_content_bytes);
    strcat(http_OK_filename_str_official, null_char);
    data_results = http_OK_filename_str_official;
    free(data_len_as_str);
    free(json_data_for_client);
    return data_results;
}

char* make_whisper_transcription_str(char* transcribed_unix_time_output_wav_result, char* transcribed_unix_time_output_txt_result) {

    char* whisper_buf = malloc(ONE_HUNDRED_THOUSAND_BYTES * sizeof(char));
    memset(whisper_buf, '\0', ONE_HUNDRED_THOUSAND_BYTES * sizeof(char));

    /*
    First string >> whisper_cli_location = "GGML_VK_VISIBLE_DEVICES=1 /home/jane/whisper.cpp/build/bin/whisper-cli -m ";
    Second string >> char* medium_model = "/home/jane_doe/Code/whisper.cpp/models/ggml-medium.en.bin -f ";
    Third string >>  "/home/jane_doe/Code/STT-tool/";    // builds transcribed/17123-output.wav
    Flag to turn on timestamps >> "-ml 120 ";
    */

    // Find the whsiper_cli_location:
    // The first part of the string: "GGML_VK_VISIBLE_DEVICES=1 /home/jane_doe/Code/whisper.cpp/build/bin/whisper-cli -m ";
    char* base_path = "/home/ango/Code";
    char* GGML_VK_VISIBLE_DEVICES_1 = "GGML_VK_VISIBLE_DEVICES=1 ";
    char* whisper_dot_cpp_base_str = "/whisper.cpp";
    char* build_bin_whisper_cli_dash_m = "/build/bin/whisper-cli -m ";

    // The second part of the string:
    // char* medium_model = "/home/jane_doe/Code/whisper.cpp/models/ggml-medium.en.bin -f ";
    char* models_ggml_medium_en_bin_dash_f = "/models/ggml-medium.en.bin -f ";

    // The third part of the string:
    //char* wav_path = "/home/jane_doe/Code/STT-tool/";    // builds transcribed/17123-output.wav
    char *speech_to_text_translation_tool = "/STT-tool/";
    char *wav_path = make_a_string(base_path, speech_to_text_translation_tool, NULL);

    char* turn_on_timestamps = "-ml 120 ";
    char* redirect = ">";
    char* space = " ";
    char* null_char = "\0";

    strcat(whisper_buf, GGML_VK_VISIBLE_DEVICES_1);
    strcat(whisper_buf, base_path);
    strcat(whisper_buf, whisper_dot_cpp_base_str);
    strcat(whisper_buf, build_bin_whisper_cli_dash_m);

    // Build the medium model component
    strcat(whisper_buf, base_path);
    strcat(whisper_buf, whisper_dot_cpp_base_str);
    strcat(whisper_buf, models_ggml_medium_en_bin_dash_f);

    // Build the destination command of the file
    strcat(whisper_buf, base_path);
    char* speech_to_text_tool = "/STT-tool ";
    strcat(whisper_buf, speech_to_text_tool);
    strcat(whisper_buf, turn_on_timestamps);

    strcat(whisper_buf, transcribed_unix_time_output_wav_result);
    strcat(whisper_buf, space);
    strcat(whisper_buf, turn_on_timestamps);
    strcat(whisper_buf, redirect);
    strcat(whisper_buf, space);
    strcat(whisper_buf, wav_path);
    strcat(whisper_buf, transcribed_unix_time_output_txt_result);
    strcat(whisper_buf, null_char);

    // Free the wav_path
    if (wav_path)
    {
        free(wav_path);
    }

    return whisper_buf;
}

// Read file content into a string
char* read_file(const char* filename) {

    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Couldn't open file '%s'\n", filename);
        return NULL;
    }
    // Get file size
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind(file);

    // Buffer for the content
    char *buffer = malloc((file_size + ONE_BYTE_FOR_NULL) * sizeof(char));
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        exit(1);
    }
    // Read the file into the buffer
    size_t read_bytes = fread(buffer, sizeof(char), file_size, file);
    buffer[read_bytes] = '\0';  //Add the null terminator
    fclose(file);
    return buffer;
}

int send_200_ok_response_options_request(int connect_id) {
    char* http_code_num_and_comment = "200 OK";
    char* content_type = "text/plain";
    char* http_code_message = "This is a 200 OK response to the OPTIONS request\n:";
    send_options_response(connect_id, http_code_num_and_comment, content_type, http_code_message);
}

int send_200_ok_response(int connect_d) {
    char* http_code_num_and_comment = "200 OK";
    char* content_type = "text/plain";
    char* http_code_message = "This is a 200 OK response\n:";
    send_a_http_response(connect_d, http_code_num_and_comment, content_type, http_code_message);
}

void send_400_bad_response(int connect_d) {
    char* http_code_num_and_comment = "400 Bad Response";
    char* content_type = "text/plain";
    char* http_code_message = "This is a 400 ERROR\n";
    send_a_http_response(connect_d, http_code_num_and_comment, content_type, http_code_message);
}

void send_500_internal_server_error(int connect_d) {
    char* http_code_num_and_comment = "500 Internal Server Error";
    char* content_type = "text/plain";
    char* http_code_message = "This is a 500 ERROR\n";
    send_a_http_response(connect_d, http_code_num_and_comment, content_type, http_code_message);
}

char* unix_time(void) {
    int now = time(NULL);
    char* UNIX_time_str = num_2_key_str(now);
    return UNIX_time_str;
}

// Copies data to the buffer
static size_t write_response_data_to_buf(void* data_from_LM_studio, size_t size, size_t nmemb, void* user_data_p) {
    // Get the total size of what's being copied
    size_t total_size = size * nmemb;
    char** response_ptr = (char**)user_data_p;
    strncpy(*response_ptr, data_from_LM_studio, total_size);
    *(*(response_ptr) + total_size) = '\0';
    return total_size;
}

// Handles the shutdown process
void handle_shutdown(int sig) {
    if (listener_d) {
        close(listener_d);
    }
    printf("Bye!\n");
    exit(0);
}

// Execute the transcribe part of the code
int execute_transcribe(int connect_d, char *buf)
{
    char *base_str = "/home/ango/";
    char *mp4 = ".mp4";
    char *output = "-output";
    char *transcribed = "transcribed/";
    char *txt = ".txt";
    char *video = "video/";
    char *wav = ".wav";

    // Checks if the file exists: video/171234.mp4
    char *filename_from_curl = get_filename_from_curl(buf);
    char *user_input_video_unix_time_mp4_path = make_a_string(video, filename_from_curl, NULL);
    FILE *output_file = fopen(user_input_video_unix_time_mp4_path,"r");
    if (output_file == NULL) {
        printf("Can't find the file");
        fclose(output_file);
        free(user_input_video_unix_time_mp4_path);
        free(filename_from_curl);
        close(connect_d);
        free(buf);
        return 0;

    } else {
        // Build the system commands if the file is empty
        int len = strlen(filename_from_curl);
        filename_from_curl[len - ENDING_CHARS] = '\0';
        char* transcribed_unix_time_result = make_a_string(transcribed, filename_from_curl, NULL);
        char* transcribed_unix_time_wav_result = make_a_string(transcribed, filename_from_curl, wav);
        char* transcribed_unix_time_output_txt_result = make_a_string(transcribed_unix_time_result, output,txt);
        char* transcribed_unix_time_output_wav_result = make_a_string(transcribed_unix_time_result, output, wav);
        char* video_unix_time_mp4_result = make_a_string(video, filename_from_curl, mp4);

        // Make a .wav file
        char *wav_files = "wav-files/";
        char *wav_file_path_for_whisper = make_a_string(wav_files, filename_from_curl, wav);

        // This ffmpeg1_str code makes: ffmpeg -i /home/jane_doe/Code/STT-tool/video/
        char *ffmpeg_dash_i = "ffmpeg -i ";
        char *speech_to_text_translation_tool_slash_video = "Code/STT-tool/video/";
        char *ffmpeg1_str = make_a_string(ffmpeg_dash_i, base_str, speech_to_text_translation_tool_slash_video);

        // The ffmpeg_str_final_part_last_chunk makes:
        // ".mp4 -vn -acodec pcm_s16le /home/jane_doe/Code/STT-tool/wav-files/"
        char *mp4_vn_acodec_pcm_s16le = ".mp4 -vn -acodec pcm_s16le ";
        char *speech_to_text_translation_tool_wav_file = "Code/STT-tool/wav-files/";
        char *ffmpeg_str_first_part_last_chunk = make_a_string(mp4_vn_acodec_pcm_s16le, base_str, speech_to_text_translation_tool_wav_file);
        char *ffmpeg_str_first_part = make_a_string(ffmpeg1_str,filename_from_curl, ffmpeg_str_first_part_last_chunk);
        char *ffmpeg_final_str = make_a_string(ffmpeg_str_first_part,filename_from_curl, ".wav");
        system(ffmpeg_final_str);

        // Retrieve the files and output as a translation-output.txt:
        // ./build/bin/whisper-cli -m /home/jane_doe/Code/whisper.cpp/models/ggml-medium.en.bin -f
        // /home/jane_doe/Code/STT-Tool/wav-files/1773967545.wav >
        // /home/jane_doe/Code/STT-Tool/transcribed/1773967545-translation-output.txt
        char* whisper_cli_result = make_whisper_transcription_str(wav_file_path_for_whisper, transcribed_unix_time_output_txt_result);

        // The input from a user is: curl -X POST -H "Content-Type:
        // application/json" -H "Accept: application/json" -d '{"filename":"1769344455.mp4"}' http://localhost:12346/api/transcribe calls
        system(whisper_cli_result);

        // Makes a JSON data string
        char* http_OK_filename_str_official = malloc(ONE_HUNDRED_THOUSAND_BYTES);
        char* json_data_for_client = malloc(ONE_HUNDRED_THOUSAND_BYTES);
        memset(json_data_for_client, '\0', ONE_HUNDRED_THOUSAND_BYTES);
        memset(http_OK_filename_str_official, '\0', ONE_HUNDRED_THOUSAND_BYTES);
        FILE* file = fopen(transcribed_unix_time_output_txt_result, "r");
        if (file) {
            int idx = 0;
            int c;
            while ((c = getc(file)) != EOF) {
                json_data_for_client[idx] = c;
                idx++;}
            fclose(file);
        }

        // Send the data back to the client
        char *key = "data";
        create_send_key_value_to_client(connect_d, key, json_data_for_client);

        if (whisper_cli_result)
        {
            free(whisper_cli_result);
        }
        if (ffmpeg_final_str)
        {
            free(ffmpeg_final_str);
        }
        if (ffmpeg_str_first_part)
        {
            free(ffmpeg_str_first_part);
        }
        if (ffmpeg_str_first_part_last_chunk)
        {
            free(ffmpeg_str_first_part_last_chunk);
        }
        if (ffmpeg1_str)
        {
            free(ffmpeg1_str);
        }
        if (http_OK_filename_str_official)
        {
            free(http_OK_filename_str_official);
        }
        if (json_data_for_client)
        {
            free(json_data_for_client);
        }
        free(wav_file_path_for_whisper);

        free(transcribed_unix_time_result);
        free(transcribed_unix_time_wav_result);
        free(transcribed_unix_time_output_txt_result);
        free(transcribed_unix_time_output_wav_result);
        free(video_unix_time_mp4_result);

        fclose(output_file);
        free(user_input_video_unix_time_mp4_path);
        free(filename_from_curl);
        close(connect_d);
        free(buf);
        printf("At the end of transcribe\n");
        exit(0);
    }
    if (output_file)
    {
        fclose(output_file);
    }
    if (user_input_video_unix_time_mp4_path)
    {
        free(user_input_video_unix_time_mp4_path);
    }
    if (filename_from_curl)
    {
        free(filename_from_curl);
    }
    if (connect_d) {
        close(connect_d);
    }
    if (buf)
    {
        free(buf);
    }
    return 0;
}


int execute_summarize(int connect_d, char* buf) {
    json_object *root = json_object_new_object();
    if (!root) {
        fprintf(stderr, "Failed to create root JSON object.\n");
        exit(1);
    }
    json_object *messages = json_object_new_array();
    if (!messages) {
        fprintf(stderr, "Failed to create message array.\n");
        exit(1);
    }
    json_object *model = json_object_new_object();
    if (!model) {
        fprintf(stderr, "Failed to create model.\n");
        exit(1);
    }
    json_object *msg = json_object_new_object();
    if (!msg) {
        fprintf(stderr, "Failed to create message object.\n");
        exit(1);
    }

    char *data_from_user = get_data_from_curl(buf);
    //... and the server will get back the response from LM studio, with the right call back request, and then
    //.. build up that response from LMstudio as a string, which is a JSON. convert the JSON string as a JSON object
    //...choices, messages, choice[0], content, and then send that back to the client. Make a new JSON and send that back to the client.
    // Make the content by reading in the file
    if (data_from_user == NULL) {
        free(data_from_user);
        json_object_put(msg);
        json_object_put(model);
        json_object_put(messages);
        json_object_put(root);
        free(buf);
        exit(1);
    }
    // Create role and content
    json_object *role_value = json_object_new_string("user");
    json_object *content_value = json_object_new_string(data_from_user);
    if (!role_value || !content_value) {
        fprintf(stderr, "Failed to have either a role or content string\n");
        json_object_put(content_value);
        json_object_put(role_value);
        json_object_put(msg);
        json_object_put(model);
        json_object_put(messages);
        json_object_put(root);
        exit(1);
    }
    json_object_object_add(msg, "role", role_value);
    json_object_object_add(msg, "content", content_value);
    json_object_array_add(messages, msg);
    json_object_object_add(root, "messages", messages);

    // Make the JSON payload to send to the client
    const char *json_payload = json_object_to_json_string(root);
    if (!json_payload) {
        fprintf(stderr, "Error: failed to serialize JSON.\n");
        exit(1);
    }
    char *curl_data = (char *)json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    if (!curl_data) {
        fprintf(stderr, "Error: failed to serialize JSON.\n");
        exit(1);
    }
    return 0;
}

int main (int argc, char* argv[])
{
    if (argc < 2) {                // argv[0]  argv[1]        argv[2]
        fprintf(stderr, "Usage: valgrind <program_name> <port> --leak-check=full");
    }
    const char* port = argv[1];
    int port_num = atoi(port);

    // Get the time stamp for the beginning of the program
    struct timeval stop, start;
    gettimeofday(&start, NULL);

    // Calls handle_shutdown() if CTRL-C is hit
    if (catch_signal(SIGINT, handle_shutdown) == DOES_NOT_EXIST) {
        fprintf(stderr, "Can't see the interrupt handler");
        exit(1);
    }

    // Listen and bind to a port and will close after one request
    int listener_d = open_listener_socket();
    bind_to_port(listener_d, port_num);
    if (listen(listener_d, ONE_RUNNING_PROCESS) == DOES_NOT_EXIST) {
        fprintf(stderr, "Can't listen");
        close(listener_d);
        exit(1);
    }
    puts("Waiting for connection");
    struct sockaddr_storage client_addr;
    unsigned int address_size = sizeof(client_addr);

    // Loop through the request of the client
    while (1) {
        int connect_d = accept(listener_d, (struct sockaddr *) &client_addr, &address_size);
        if (connect_d == IS_ERROR) {
            fprintf(stderr, "Can't open secondary socket using the accept method\n");
            close(connect_d);
            exit(1);
        }
        // Fork the child process
        int child_pid = fork();
        if (child_pid == IS_ERROR) {
            fprintf(stderr, "Could not fork the child\n");
            close(connect_d);
            exit(1);
        }
        if (child_pid > 0) {
            fprintf(stderr, "Parent is closing child socket. Child PID: %d\n\n", child_pid);
            //Parent is closing the CLIENT SOCKET
            close(connect_d);
        } else {
            // This is the child process, so close the listener.
            close(listener_d);
            char *buf = malloc(ONE_HUNDRED_THOUSAND_BYTES);
            memset(buf, '\0', ONE_HUNDRED_THOUSAND_BYTES);

            // Peek at the request without consuming it
            int chars_peeked = recv(connect_d, buf, ONE_HUNDRED_THOUSAND_BYTES, MSG_PEEK);
            if (chars_peeked == ERROR) {
                fprintf(stderr, "Error. Received 0 bytes. ");
                free(buf);
                close(connect_d);
                exit(1);
            }
            int is_options_request = check_if_options_request(buf);
            if (is_options_request == TRUE) {
                // Actually consume the request now
                recv(connect_d, buf, ONE_HUNDRED_THOUSAND_BYTES, 0);
                printf("Received an OPTIONS request\n");
                send_200_ok_response_options_request(connect_d);
                free(buf);
                close(connect_d);
                exit(0);
            }
            int is_post_request = check_if_post_request(buf);
            if (is_post_request == TRUE) {

                // Determine the API call between /api/upload, /api/transcribe, /api/summarize
                int returned_api_call = determine_api_call(buf);
                if (returned_api_call == API_UPLOAD) {
                    printf("Running upload\n");
                    int upload_response = execute_upload(connect_d, buf);
                    printf("Done running upload\n");
                    exit(0);
                } else if (returned_api_call == API_TRANSCRIBE) {
                    recv(connect_d, buf, ONE_HUNDRED_THOUSAND_BYTES, 0);
                    printf("Running transcribe\n");
                    // Make sure to load your model
                    int transcribe_response = execute_transcribe(connect_d, buf);
                    if (buf) {
                        free(buf);
                    }
                    exit(0);
                } else if (returned_api_call == API_SUMMARIZE) {

                    recv(connect_d, buf, ONE_HUNDRED_THOUSAND_BYTES, 0);
                    printf("Running summarize\n");
                    char *data_from_user = get_data_from_curl(buf);
                    // Check to see that the key matches with what the function would like
                    char *client_input_key = get_key(data_from_user);
                    char *desired_key_for_function = "data";
                    int is_key_matching = compare_keys(desired_key_for_function, client_input_key);
                    if (is_key_matching == FALSE) {
                        printf("The keys don't match\n");
                        free(client_input_key);
                        free(data_from_user);
                        free(buf);
                        send_400_bad_response(connect_d);
                        exit(1);
                    }
                    //... and the server will get back the response from LM studio, with the right call back request, and then
                    //.. build up that response from LMstudio as a string, which is a JSON. convert the JSON string as a JSON object
                    //...choices, messages, choice[0], content, and then send that back to the client. Make a new JSON and send that back to the client.
                    // Make the content by reading in the file
                    if (data_from_user == NULL) {
                        free(data_from_user);
                        free(buf);
                        exit(1);
                    }
                    // Create the role key and value obj
                    json_object *role_and_content_key_value_object = json_object_new_object();
                    json_object *user_value_str = json_object_new_string("user");
                    json_object_object_add(role_and_content_key_value_object, "role", user_value_str);
                    json_object *content_value_str = json_object_new_string(data_from_user);
                    json_object_object_add(role_and_content_key_value_object, "content", content_value_str);

                    // Add the messages array to root
                    char *LLM_model = "qwen3.5-9b";
                    json_object *root = json_object_new_object();
                    json_object *model_value_str = json_object_new_string(LLM_model);
                    json_object_object_add(root, "model", model_value_str);

                    // Put the role and content key value object into the message array
                    json_object *messages_array = json_object_new_array();
                    if (!messages_array) {
                        fprintf(stderr, "Failed to create message array.\n");
                        exit(0);
                    }
                    json_object_array_put_idx(messages_array, 0, role_and_content_key_value_object);

                    // Make the JSON payload to send to the client
                    json_object_object_add(root, "messages", messages_array);
                    const char *json_payload = json_object_to_json_string(root);
                    if (!json_payload) {
                        fprintf(stderr, "Error: failed to serialize JSON.\n");
                        exit(1);
                    }
                    char *curl_data = (char *)json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
                    if (!curl_data) {
                        fprintf(stderr, "Error: failed to serialize JSON.\n");
                        exit(1);
                    }

                    // NOTE: There is 98,907 Bytes of memory here that CAN NOT be freed
                    // after many attempts of looking through the internet.
                    CURL *curl_handle = curl_easy_init();
                    if (curl_handle) {
                        CURLcode result;
                        struct curl_slist *slist2 = NULL;
                        struct curl_slist *temp = NULL;
                        slist2 = curl_slist_append(slist2, "Content-Type: application/json");
                        if (!slist2) {
                            curl_slist_free_all(temp);
                            curl_slist_free_all(slist2);
                            exit(1);
                        }
                        slist2 = curl_slist_append(slist2, "Accept: application/json");
                        if (!slist2) {
                            curl_slist_free_all(slist2);
                            exit(1);
                        }
                        // Set Custom Headers
                        // For the endpoint I may even use http://10.171.168.174:8000
                        char *your_end_point = "http://127.0.0.1:8000/v1/chat/completions";
                        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, slist2);
                        curl_easy_setopt(curl_handle, CURLOPT_URL, your_end_point);
                        // Pass in a pointer to the data - libcurl does not copy
                        char *response = (char *) malloc(EIGHT_THOUSAND_BYTES * sizeof(char));
                        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_response_data_to_buf);
                        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);
                        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, curl_data);
                        result = curl_easy_perform(curl_handle);
                        if (result != CURLE_OK) {
                            printf("Curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
                            send_400_bad_response(connect_d);
                            exit(1);
                        }
                        json_object *j_data1 = json_tokener_parse(response);
                        json_object *objname;
                        // The key is called choices
                        char object_key[] = "choices";
                        json_object *object;
                        // Get the object
                        if (!json_object_object_get_ex(j_data1, object_key, &objname)) {
                            fprintf(stderr, "Unable to find object '%s'\n", object_key);
                            send_400_bad_response(connect_d);
                            exit(1);
                        }
                        // Get the value from the object
                        json_object_object_get_ex(j_data1, object_key, &object);
                        const char *value = json_object_get_string(object);
                        char *copied_value = strdup(value);
                        json_object *j_data2 = json_tokener_parse(copied_value);
                        json_object *medi_array_obj1 = json_object_array_get_idx(j_data2, IDX_ZERO);
                        json_object *medi_array_obj_name1 = json_object_object_get(medi_array_obj1, "message");
                        json_object *j_obj_content = json_object_object_get(medi_array_obj_name1,"content");

                        // Extract the string to send back to the client
                        char *key = "data";
                        const char *response_str = json_object_get_string(j_obj_content);
                        create_send_key_value_to_client(connect_d, key,response_str);
                        if (j_data2) {
                            json_object_put(j_data2);
                        }
                        free(copied_value);
                        json_object_put(j_data1);
                        free(response);
                        if (connect_d) {
                            close(connect_d);
                        }
                    }
                    // NOTE: We have to free the memory of the json objects in a specific order
                    json_object_put(root);
                    free(client_input_key);
                    free(data_from_user);
                    free(buf);
                    curl_easy_cleanup(curl_handle);
                    curl_global_cleanup();
                    close(connect_d);
                    exit(1);
                }
                if (buf) {
                    free(buf);
                }
            } else {
                recv(connect_d, buf, EIGHT_THOUSAND_BYTES, 0);
                send_400_bad_response(connect_d);
                free(buf);
                close(connect_d);
                exit(0);
            }
            send_400_bad_response(connect_d);
            close(connect_d);
            exit(0);
        }
    }
    return 0;
}