//Eric Makosch Assignment 2 CS283
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int count_words(char *, int, int);
//add additional prototypes here

int setup_buff(char *buff, char *user_str, int len) {
    char *src = user_str;
    char *dest = buff;
    int count = 0;
    int space_added = 0; //count for consequetive spaces

    while (*src != '\0') {
        //check if char is whitespace
        if (*src != ' ' && *src != '\t') {
            *dest = *src;
            dest++;
            count++;
            space_added = 0;
        } else if (!space_added) {
            //one space to replace consequetive
            *dest = ' ';
            dest++;
            count++;
            space_added = 1;
        }

        //check buffer
        if (count >= len) {
            return -1;
        }

        src++;
    }

    while (count < len) {
        *dest = '.';
        dest++;
        count++;
    }

    return count; //return processed string length
}

void print_buff(char *buff, int len) {
    printf("Buffer:  ");
    for (int i = 0; i < len; i++) {
        putchar(*(buff + i));
    }
    putchar('\n');
}

void usage(char *exename) {
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int len, int str_len) {
    int word_count = 0;
    int in_word = 0;

    //loop through buffer
    for (int i = 0; i < str_len; i++) {
        //check if whitespace
        if (*(buff + i) != ' ' && *(buff + i) != '\t') {
            if (!in_word) {
                in_word = 1;
                word_count++;
            }
        } else {
            in_word = 0;
        }
    }

    return word_count;
}

//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS
int reverse_string(char *buff, int len) {
    if (len <= 0) {
        return -1;
    }

    //start and end of string pointers
    char *start = buff;
    char *end = buff + len - 1;

    while (end > start && *end == '.') {
        end--;
    }

    while (start < end) {
        //swaps chars
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }

    return 0;
}


int main(int argc, char *argv[]) {

    char *buff;             //placeholder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int rc;                 //used for return codes
    int user_str_len;       //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if argv[1] does not exist?
    //Checks if there is either a dash to make sure the user
    // is using a flag or if the user provided enough CL args
    //if argv[1] doesn't exist, the first condition will make for a safe exit

    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1); //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    // The if statement checks to make sure there are enough arguments
    //provided through the command line

    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a
    //          return code of 99
    buff = (char *)malloc(BUFFER_SZ);
    if (!buff) {
        printf("Error: Memory allocation failed.\n");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ); //see todos
    if (user_str_len < 0) {
        printf("Error setting up buffer, error = %d\n", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt) {
    case 'c':
        rc = count_words(buff, BUFFER_SZ, user_str_len); // Word count implementation
        if (rc < 0) {
            printf("Error counting words, rc = %d\n", rc);
            free(buff);
            exit(2);
        }
        printf("Word Count: %d\n", rc);
        break;

    case 'w':
    printf("Word Print\n----------\n");
    int word_start = 0;
    int word_index = 0;

    for (int i = 0; i <= user_str_len; i++) {
        if (i == user_str_len || *(buff + i) == ' ' || *(buff + i) == '.') {
            if (word_start != i) {
                // Check if the current substring is not just dots
                int is_valid_word = 0;
                for (int j = word_start; j < i; j++) {
                    if (*(buff + j) != '.') {
                        is_valid_word = 1;
                        break;
                    }
                }

                if (is_valid_word) {
                    word_index++;
                    printf("%d. ", word_index);
                    for (int j = word_start; j < i; j++) {
                        if (*(buff + j) != '.') {
                            putchar(*(buff + j));
                        }
                    }

                    // Calculate word length excluding dots
                    int word_len = 0;
                    for (int j = word_start; j < i; j++) {
                        if (*(buff + j) != '.') {
                            word_len++;
                        }
                    }
                    printf(" (%d)\n", word_len);
                }
            }
            word_start = i + 1;
        }
    }
    break;


    case 'r':
        rc = reverse_string(buff, BUFFER_SZ);
        if (rc < 0) {
            printf("Error reversing string, rc = %d\n", rc);
            free(buff);
            exit(2);
        }
        printf("Reversed String: ");
        for (int i = 0; i < user_str_len; i++) {
            putchar(*(buff + i));
        }
        printf("\n");
        break;

    // Other cases here

    default:
        usage(argv[0]);
        free(buff); // Free allocated memory before exiting
        exit(1);
    }


    //TODO:  #6 Don't forget to free your buffer before exiting
    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that
//          the buff variable will have exactly 50 bytes?
//
// THis ensures that the program does not go outside of the buffer range
//and doesn't have the possibility of accessing memory beyond the buffer,
//making it safer