// 8/2 3PM - Make an array of character

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIQUE_LETTER_EXISTS 1
#define LETTER_A_97 97
#define TWENTY_SIX_ALPHA_LETTERS 26

struct Group
{
    int ascii_num_of_first_char;
    // Only for organizing
};

struct Letter_node
{
    int letter_label_char_int;                      // (97) A, B, (114) R,Z
    //struct Letter_node* head_node;                // Ri->Rog
    int num_of_unique_str_in_each_letter_group;     // A=1 B=2
    struct Letter_node* next;
};

struct Node
{
    char* data;
    struct Node* next;
};        //total_unique_groups++;

struct Letter_node* create_letter_node(void)
{
    struct Letter_node* new_letter = (struct Letter_node*)malloc(sizeof(struct Letter_node));
    new_letter->letter_label_char_int = 0;
    new_letter->num_of_unique_str_in_each_letter_group = 0;
    new_letter->next = NULL;
    return new_letter;
}


//struct Letter_node* add_data_to_letter_node(struct Letter_node* blank_letter_node, int letter_label_char_int, struct Letter_node* new_node)
//{
/*
    blank_letter_node->letter_label_char_int = letter_label_char_int;
    blank_letter_node->head_node = new_node;
    printf("38: blank_letter_node->: %s\n", blank_letter_node->head_node);
*/
    //if (blank_letter_node->hea == NULL) {
      //  printf("40: Hi!\n");
        //blank_letter_node->head_node = new_node;
        //printf("41: New head node with value: %s\n", new_node->data);
        //return blank_letter_node;
    //}

    /*
    struct Node* temp = blank_letter_node->head_node;
    while (temp->next != NULL)
    {
        printf("48: This is temp->data: %s\n", temp->data);
        temp = temp->next;
    }
    temp->next = new_node;
    //blank_letter_node->head_node = head_node;
    blank_letter_node->num_of_unique_str++;
    return blank_letter_node;
    */
//}

struct Node* create_node(char* data)
{
    if (data == NULL) {
        exit(1);
    }
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));
    node->data = NULL;
    node->next = NULL;
    node->data = data;
    return node;
}

void append_node(struct Node* head_node_of_ll, struct Node* new_node) {
    struct Node* temp = head_node_of_ll;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = new_node;
}

int* count_total_unique_groups(char** string_ptr, int num_of_strings, int* arr)
{
    // Count the number of unique characters
    for (int i = 0; i < num_of_strings; i++)
    {
        char* curr_str = string_ptr[i];
        int letter_label_char_int = *(curr_str + 0) - LETTER_A_97;
        int val_of_occur = arr[letter_label_char_int];
        arr[letter_label_char_int] = val_of_occur + 1;
    }
    return arr;
}

int main(void)
{
    // Head node of all Letter nodes
    struct Letter_node* head_node_of_all_letter_nodes = create_letter_node();
    head_node_of_all_letter_nodes->letter_label_char_int = -1;

    int num_of_strings = 12;
    // 8 Unique string
    char* james_str = "james";
    char* jane_str = "jane";
    char* ian_str = "ian";
    char* i_str = "indigo";
    char* eric_str = "eric";
    char* den_str = "denise";
    char* dan_str = "dan";
    char* ri_str = "ri";
    char* rog_str = "rog";
    char* al_str = "al";
    char* shan_str = "shan";
    char* gab_str = "gabby";

    // List of Names
    char* list_of_names[] = {james_str, jane_str, ian_str, i_str, eric_str, den_str, dan_str, shan_str, gab_str, ri_str, rog_str, al_str};
    char** string_ptr = list_of_names;

    // Count the groups to make for the group linkedlist
    int arr[TWENTY_SIX_ALPHA_LETTERS] = {0};
    int* arr_result = count_total_unique_groups(string_ptr, num_of_strings, arr);

    for (int a = 0; a < TWENTY_SIX_ALPHA_LETTERS; a++)
    {
        int each_char = *(arr_result + a);
        if (each_char > 0) {
            printf("133: %i with letter: %c\n", *(arr_result + a), a + LETTER_A_97);
        }
    }

    //char** arr_of_lists[total_unique_groups];
    // Place only letters into nodes
    for (int i = 0; i < num_of_strings; i++) {
        char* curr_str = string_ptr[i];
        char first_char = *(curr_str + 0);
        //printf("140: >>>>>>>> First char only: %c of curr_str: %s\n", first_char, curr_str);

        // Make letter node for each letter /////////////////////////////
        /*
        struct Letter_node* head_node_for_new_letter = create_letter_node();
        head_node_for_new_letter->letter_label_char_int = first_char;
        printf("136: This is curr_str: %s first_char: %c str_as_int: %i for idx: %i\n", curr_str, first_char, first_char, i);
        head_node_for_new_letter = head_node_for_new_letter->next;
        */
        //new_letter->letter_label_char_int = first_char;
        //////////////////////////////////////////////////////////////
        ///
        // Assign the letter to the first index of the list_of_list_arr
        //*(list_of_list_of_arr + list_of_list_of_arr_idx) = new_letter;s
    }
    // Make a letter node and string
    for (int j = 0; j < num_of_strings; j++) {

    }
    //printf("135: total_unique_groups: %i\n", total_unique_groups);

    //printf("110: head_node_of_all_letter_nodes: %i\n head_node: %p\nnum_uniq_group: %i\n", head_node_of_all_letter_nodes->letter_label_char_int, head_node_of_all_letter_nodes->head_node, head_node_of_all_letter_nodes->num_of_unique_str_in_each_letter_group);
    //printf("119: This is char: %i with %i curr_str: %s first_char: %c\n", arr[letter_label_char_int], letter_label_char_int, curr_str, *(curr_str + 0));
    /*
    char** list_of_list_of_arr[total_unique_groups];
    int list_of_list_of_arr_idx = 0;



        /*
        printf("139: Letter_label_char_int: %c\n", new_letter->letter_label_char_int);
        printf("140: head_node: %s\n", new_letter->head_node);
        printf("141: num_of_unique_str_in_each_letter_group: %i\n", new_letter->num_of_unique_str_in_each_letter_group);
        */
    //}


    /*
    for (int j = 0; j < total_unique_groups; j++) {
        struct Letter_node* new_letter_node = create_letter_node();
        // Make each letter node
    }
*/
    //printf("130: Total unique groups: %i\n", total_unique_groups);
    /*for (int j = 0; j < TWENTY_SIX_ALPHA_LETTERS; j++)
    {
        //if (arr[j] = UNIQUE_LETTER_EXISTS) {
        //}
        printf("129: idx: %i containing: %i\n", j, arr[j]);
    }
    */

        //printf("118: This is the letter_label_char_int: %c\n", letter_label_char_int);
        /* Make letters for the Group Struct
                - Letter_label_char:    a,b,z
                - Head_node:            NULL
                - Num_unique_str:       0
        //struct Letter_node* new_letter_node = (struct Letter_node*)malloc(sizeof(struct Letter_node));
        new_letter_node->letter_label_char_int = letter_label_char_int;
        printf("126: %c\n", new_letter_node->letter_label_char_int);
        struct Node* new_node = create_node(curr_str);
        new_letter_node->head_node = new_node;
        new_letter_node->num_of_unique_str_in_each_letter_group = 0;
        printf("128: curr_str: %s\nletter_label_char_int: %c\n", curr_str, new_letter_node->letter_label_char_int);
        printf("129: letter_label_char_int: %s\n", new_letter_node->head_node->data);
        printf("130: letter_label_char_int: %i\n\n", new_letter_node->num_of_unique_str_in_each_letter_group);
        if (new_letter_node->head_node->data != NULL) {
            printf("132 Add to the head_node\n");
        } else
        {
            printf("135 Don't add to the head_node\n");
        }
        new_letter_node->letter_label_char_int = 0;
        ////// NOTEEEEEEEEEEEEEEEEEEEEEEEEEEE THE LETTER MUST BE RESET!! /////////////////
    }
*/
        /*
        struct Node* new_node = create_node(curr_str);

        //printf("117: %i curr_str: %s and head_node: %s\n", letter_label_char_int, new_node->data);

        if (new_letter_node->letter_label_char_int == letter_label_char_int) {
            printf("121: Add to existing linkedlist\n");
            //new_letter_node->letter_label_char_int = letter_label_char_int;
            //new_letter_node->head_node = new_node;
            //new_letter_node->num_of_unique_str_in_each_letter_group++;
        } else {
            printf("126: Start a new linkedList\n");
        }
        */

        //free(new_letter_node);
        /*
        // Creates a node for a new letter

        // Check the letter node
        printf("125: This is the letter char: %i\n", new_letter_node->letter_label_char_int);
        printf("126: This is the value of the head_node: %s\n", new_letter_node->head_node->data);
        printf("126: This is the num_of_unique_str: %i\n", new_letter_node->num_of_unique_str_in_each_letter_group);

    */
        /*
        printf("curr_result: %s letter_label_char_int: %i\n", new_node->data, letter_label_char_int);
        if (new_letter_node->letter_label_char_int == 0) {
            printf("There is nothing\n");
            new_letter_node->letter_label_char_int = letter_label_char_int;
            printf("122: %i\n", new_letter_node->letter_label_char_int);
        } else {
            printf("There is something which is: %i\n", new_letter_node->letter_label_char_int);
        }
        */

        //new_letter_node->head_node = new_node;

        //printf("124: status of head_node = %s new_node->data: %s\n", new_letter_node->head_node->data, new_node->data);

        /*
        // If the head_node is not NULL, add the head_node
        if (new_letter_node->head_node == NULL)
        {
            printf("128: The head_node_data is NULL\n");
            new_letter_node->head_node = new_node;
            new_letter_node->num_of_unique_str++;
        } else {
            printf("132: iterating\n");
            struct Node* temp = new_letter_node->head_node;
            while (temp->next != NULL) {
                printf("135: This is temp_data: %s\n", temp->data);
                temp = temp->next;
            }
            temp->next = new_node;
            printf("Adding 139: %s\n", temp->next->data);
            new_letter_node->num_of_unique_str++;
        }
        printf("141: %s\n", new_letter_node->head_node->data);
        */

        //new_letter_node = add_data_to_letter_node(new_letter_node, letter_label_char_int, new_node);
        //printf("93: DEBUG:\nnew_letter_node: %i\ndata_of_head_node: %s num_unique_str: %i\n", new_letter_node->letter_label_char_int, new_letter_node->head_node->data, new_letter_node->num_of_unique_str);
        // Assign this node into the Group struct
        //printf("57: first_letter: %c curr_result: %s\n", *(curr_str + 0), new_node->data);
        //printf("\n\ns");

        //free(curr_result);
        //free(new_letter_node);s
    //}

    //struct Node* hi_result = create_node(ri_str);
    //struct Node* hi_result2 = create_node(rog_str);
    //append_node(hi_result, hi_result2);

    /*
    ri->data = "Riley";
    ri->next = NULL;
    rog->data = "Roger";
    rog->next = NULL;
    al->data = "Alexa";
    al->next = NULL;
    ri->next = rog;
    */

    //free(hi_result);
    //free(new_letter_node);

    return 0;
}