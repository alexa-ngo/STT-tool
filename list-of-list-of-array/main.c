// 8/1 8PM - Make an array of character

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Group
{
    int ascii_num_of_first_char;
    // Only for organizing
};

struct Letter_node
{
    int letter_label_char_int;          // (97) A, B, (114) R, Z
    struct Node* head_node;     // Ri->Rog
    int num_of_unique_str;      // A=1 B=2
};

struct Node
{
    char* data;
    struct Node* next;
};

struct Letter_node* create_letter_node(int letter_label_char_int, struct Node* head_node)
{
    struct Letter_node* new_letter = (struct Letter_node*)malloc(sizeof(struct Letter_node));
    new_letter->letter_label_char_int = 0;
    head_node->data = NULL;
    head_node->next = NULL;
    new_letter->num_of_unique_str = 0;
    return new_letter;
}

struct Node* create_node(char* data)
{
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));
    node->data = NULL;
    node->next = NULL;
    if (data == NULL) {
        exit(1);
    }
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

/*
int main() {
    char* strings[] = {"I love", "Embedded", "Systems"};

    // Create a char** pointer to the strings array
    char** string_ptr = strings;

    // Access and print the strings using char**
    for (int i = 0; i < 3; i++) {
        printf("%s\n", string_ptr[i]);
    }

    return 0;
}
*/

int main(void)
{
    char* ri_str = "ri";
    char* rog_str = "rog";
    char* al_str = "al";

    // List of Names
    char* list_of_names[] = {ri_str, rog_str, al_str};
    char** string_ptr = list_of_names;

    for (int i = 0; i < 3; i++) {
        char* curr_str = string_ptr[i];
        int ascii_num_of_first_char = *(curr_str + 0);
        struct Node* curr_result = create_node(curr_str);
        printf("87: %s\n", curr_result->data);

        // Make the letter node
        printf("ascii_num_of_first_char: %i\n", ascii_num_of_first_char);
        printf("curr_result: %s\n", curr_result->data);

        struct Letter_node* new_letter_node = create_letter_node(ascii_num_of_first_char, curr_result);
        printf("93: new_letter_node: %i\n", new_letter_node->letter_label_char_int);
        // Assign this node into the Group struct
        printf("57: first_letter: %c curr_result: %s\n", *(curr_str + 0), curr_result->data);
    }

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
    //free(hi_result2);

    return 0;
}