/*
Assignment:
crcxor - CRC + XOR Algorithm Implementation
Author: Martin Hall
Language: C, C++, or Rust (only)
To Compile:
gcc -O2 -std=c11 -o crcxor crcxor.c
g++ -O2 -std=c++17 -o crcxor crcxor.cpp
rustc -O crcxor.rs -o crcxor
To Execute (on Eustis):
./crcxor <message_file> <crc_algorithm>
where:
<message_file> is the path to the input text file
<crc_algorithm> is 3, 4, or 8 (for CRC-3, CRC-4, or CRC-8)
Notes:
- Implements CRC-3, CRC-4, and CRC-8 algorithms
- Implements the XOR (LRC) checksum in addition to CRC
- Processes plain text messages and computes CRC values
- Outputs all intermediate steps and final CRC values
- Tested on Eustis.
Class: CIS3360 - Security in Computing - Spring 2026
Instructor: Jie Lin, Ph.D.
Due Date: See Webcourses
*/

//library inclusions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define _GNU_SOURCE

//given table
const char ascii_table[16][8] = {
    {'\0', '\0', ' ', '0', '@', 'P', '`', 'p'}, // Row 0
    {'\0', '\0', '!', '1', 'A', 'Q', 'a', 'q'}, // Row 1
    {'\0', '\0', '"', '2', 'B', 'R', 'b', 'r'}, // Row 2
    {'\0', '\0', '#', '3', 'C', 'S', 'c', 's'}, // Row 3
    {'\0', '\0', '$', '4', 'D', 'T', 'd', 't'}, // Row 4
    {'\0', '\0', '%', '5', 'E', 'U', 'e', 'u'}, // Row 5
    {'\0', '\0', '&', '6', 'F', 'V', 'f', 'v'}, // Row 6
    {'\0', '\0', '\'', '7', 'G', 'W', 'g', 'w'}, // Row 7
    {'\0', '\0', '(', '8', 'H', 'X', 'h', 'x'}, // Row 8
    {'\0', '\0', ')', '9', 'I', 'Y', 'i', 'y'}, // Row 9
    {'\0', '\0', '*', ':', 'J', 'Z', 'j', 'z'}, // Row 10
    {'\0', '\0', '+', ';', 'K', '[', 'k', '{'}, // Row 11
    {'\0', '\0', ',', '<', 'L', '\\', 'l', '|'}, // Row 12
    {'\0', '\0', '-', '=', 'M', ']', 'm', '}'}, // Row 13
    {'\0', '\0', '.', '>', 'N', '^', 'n', '~'}, // Row 14
    {'\0', '\0', '/', '?', 'O', '_', 'o', '\0'}  // Row 15
};

//function headers
int getValue(char c);
int* decimalToBinary(int n);
int* binaryToHex(int *values);

int main(int argc, char *argv[]) {

    /*printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
    printf("argv[%d] = %s\n", i, argv[i]);
    }
    printf("\n");
    if (argc > 1)
    {
        int x = atoi(argv[1]); // convert string to int (simple)
        printf("Converted argv[1] to int: %d\n", x);
    } else {
        printf("No extra arguments provided.\n");
        printf("Try: ./00_args 123\n");
    } */

    if (argc != 3) {
        printf("Incorrect number of arguments.");
        return 0;
    }

    
    FILE *msg_file = fopen(argv[1], "r");
    int crcmode = atoi(argv[2]);
    if (crcmode != 3 && crcmode != 4 && crcmode != 8) {
        printf("Invalid CRC mode.");
        return 0;
    }

    if(msg_file == NULL) {
        perror("fopen");
        return 0;
    }

    //declaring local variables for text filtering
    int i = 0; int j = 0; int c = 0;
    char *o_text = malloc(1);
    char *p_text = malloc(1);
    char *temp;

    if (p_text == NULL || o_text == NULL) { //safety check
        perror("malloc");
        return 0;
    }

    while ((c = fgetc(msg_file)) != EOF) { //only stop at the end of the file and not at spaces
        temp = realloc(o_text, i + 1);
        if (temp != NULL) o_text = temp;
            
        o_text[i] = (char)c; //adding character to new string
        i++; //moving position in string

        if (isalpha((unsigned char)c)) { //only taking valid letters for processed string
            temp = realloc(p_text, j + 1); //reallocing more space for the string
            if (temp != NULL) {
                p_text = temp;
            } else {
                perror("realloc");
                return 0;
            }
            p_text[j] = (char)c; //adding character to new string
            j++; //moving position in string
        }
    }
    //terminating the strings
    o_text[i] = '\0';
    p_text[j] = '\0';

    printf("The original message:\n");
    printf("%s\n\n", o_text);

    printf("The preprocessed message (invisible characters removed):\n");
    printf("%s\n\n", p_text);

    //declaring variables for next section
    int len = (int)strlen(p_text);
    int *decimal_text = malloc(sizeof(int) * len);

    if (decimal_text == NULL) {
        perror("malloc");
        return 0;
    }

    printf("The decimal representation of the preprocessed message:\n");
    for (i = 0; i < len; i++) {
        decimal_text[i] = getValue(p_text[i]); //converting char to int using the table
        printf("%d ", decimal_text[i]);
    }
    printf("\n\n");

    //malloc based on size of array to represent each type of base
    int b_len = len * 8;
    int d_len = b_len + crcmode;
    int h_len = len * 2; //don't need to pad the hex string
    int *binary_string = malloc(sizeof(int) * b_len);
    int *hex_string = malloc(sizeof(int) * h_len);
    //creating a working copy of binary string
    int *dividend = malloc(sizeof(int) * d_len);

    if (binary_string == NULL || hex_string == NULL || dividend == NULL) {
        perror("malloc");
        return 0;
    }
    
    int k;
    for (i = 0; i < len; i++) {
        int *temp = decimalToBinary(decimal_text[i]);
        int *tmp = binaryToHex(temp);
        for (j = 0; j < 8; j++) {
            binary_string[i*8+j] = temp[j];
            if (i+1 == len && j+1 == 8) { //if at the end of the string, pad
                memcpy(dividend, binary_string, sizeof(int) * b_len);
                for (k = 1; k <= crcmode; k++) {
                    dividend[i*8+j+k] = 0;
                }
            }
        }
        for (j = 0; j < 2; j++) {
            hex_string[i*2+j] = tmp[j];
        }

        //freeing memory
        free(temp);
        free(tmp);
    }

    printf("The hex representation of the preprocessed message:\n");
    for (i = 0; i < h_len; i++) {
        if (i > 0 && i % 2 == 0) {
            printf(" ");
        }
        printf("%X", hex_string[i]); //displays the numbers as hexdecimal values
    }
    printf("\n\n");

    printf("The binary representation of the preprocessed message:\n"); //printing the binary string excluding the padding
    for (i = 0; i < (len*8); i++) {
        if (i > 0 && i % 8 == 0) {
            printf(" ");
        }
        printf("%d", binary_string[i]);
    }
    printf("\n\n");



    printf("The binary representation of the original message prepared for CRC computation\n(padded with %d zeros):\n", crcmode); //includes the padding zeroes when printing
    for (i = 0; i < d_len; i++) {
        if (i > 0 && i % 8 == 0) {
            printf(" ");
        }
        printf("%d", dividend[i]);
    }
    printf("\n\n");

    
    //printf("%d", b_len);
    

    //determining the CRC polynomial and creating a container for the final CRC value
    int *crcpoly = malloc(sizeof(int) * (crcmode+1));

    if (crcpoly == NULL) {
        perror("malloc");
        return 0;
    }

    switch (crcmode) {
        case 3:
        crcpoly[0] = 1; crcpoly[1] = 1; crcpoly[2] = 0; crcpoly[3] = 1;
        break;
        case 4:
        crcpoly[0] = 1; crcpoly[1] = 0; crcpoly[2] = 1; crcpoly[3] = 1; crcpoly[4] = 0;
        break;
        case 8:
        crcpoly[0] = 1; crcpoly[1] = 0; crcpoly[2] = 0; crcpoly[3] = 1; crcpoly[4] = 1; crcpoly[5] = 0; crcpoly[6] = 1; crcpoly[7] = 0; crcpoly[8] = 1;
        break;
        default:
        printf("Invalid CRC mode.");
        return 0;
    }
    int *crcval = calloc(crcmode, sizeof(int));

    if (crcval == NULL) {
        perror("calloc");
        return 0;
    }

    /* for (i = 0; i <= crcmode; i++) {
        printf("%d", crcpoly[i]);
    } */

    i = 0;

    //continous XOR operation (long division)
    while(i < b_len) {
        if (dividend[i] == 1) {
            for (j = 0; j <= crcmode; j++)
            dividend[i+j] = dividend[i+j] ^ crcpoly[j];
        }
        i++;
    }

    /*for (i = 0; i < b_len; i++) {
        if (i > 0 && i % 8 == 0) {
            printf(" ");
        }
        printf("%d", dividend[i]);
    }

    printf("\n\n");*/

    //assigning crc val
    for (i = 0; i < crcmode; i++) {
        crcval[crcmode-1-i] = dividend[d_len-1-i];
    }

    //printing crc val
    printf("The crc value for the chosen crc algorithm in binary:\n");
    for (i = 0; i < crcmode; i++)
    printf("%d", crcval[i]);
    printf("\n\n");

    //declaring hex variables
    int *hex_crc;
    int hex_size;
    int bin_size;

    if (crcmode == 3) {
        //special case for 3, which needs a 0 added to the start
        int *temp = realloc(crcval, sizeof(int) * 4);

        if (temp == NULL) {
            perror("realloc");
            return 0;
        }

        crcval = temp;

        for (i = 3; i > 0; i--) {
            crcval[i] = crcval[i-1];
        }
        crcval[0] = 0;

        /*for (i = 0; i < 4; i++)
        printf("%d", crcval[i]);*/

        hex_crc = malloc(sizeof(int));
        hex_size = 1;
        bin_size = 4;
    } else if (crcmode == 4) {
        hex_crc = malloc(sizeof(int));
        hex_size = 1;
        bin_size = 4;
    } else {
        hex_crc = malloc(sizeof(int) * 2);
        hex_size = 2;
        bin_size = 8;
    }

    //calcuating hex value(s)
    for (i = 0; i < hex_size; i++) {
        hex_crc[i] = crcval[i*4] * 8 + crcval[i*4+1] * 4 + crcval[i*4+2] * 2 + crcval[i*4+3] * 1;
    }

    printf("The crc value for the chosen crc algorithm in hex:\n");
    for (i = 0; i < hex_size; i++)
    printf("%X", hex_crc[i]);
    printf("\n\n");

    //adding the hex value to the hex string
    int *tmp = realloc(hex_string, sizeof(int) * (h_len + hex_size));

    //safety check
    if (tmp == NULL) {
        perror("realloc");
        return 0;
    }
    hex_string = tmp;

    for (i = 0; i < hex_size; i++) {
        hex_string[h_len+i] = hex_crc[i];
    }
    h_len += hex_size; //updating size of hex value list

    printf("The input in hex for this XOR checksum computation:\n");
    for (i = 0; i < h_len; i++) {
        printf("%X", hex_string[i]);
    }
    printf("\n\n");

    //adding the hex value to the hex string
    tmp = realloc(binary_string, sizeof(int) * (b_len + bin_size));

    //safety check
    if (tmp == NULL) {
        perror("realloc");
        return 0;
    }
    binary_string = tmp;

    for (i = 0; i < bin_size; i++) {
        binary_string[b_len+i] = crcval[i];
    }
    b_len += bin_size; //updating size of hex value list

    printf("The input in binary for this XOR checksum computation:\n");
    for (i = 0; i < b_len; i++) {
        if (i > 0 && i % 4 == 0) {
            printf(" ");
        }
        printf("%d", binary_string[i]);
    }
    printf("\n\n");

    //calculating the XOR checksum
    int *checksum = malloc(sizeof(int) * 4);
    
    if (checksum == NULL) {
        perror("malloc");
        return 0;
    }
    //initializing the list
    for (i = 0; i < 4; i++) {
        checksum[i] = binary_string[i];
    }

    //XOR operations
    for (i = 4; i < b_len; i += 4) {
        for (j = 0; j < 4; j++) {
            checksum[j] = checksum[j] ^ binary_string[i+j];
        }
    }

    printf("The xor checksum value for the chosen crc algorithm in binary:\n");
    for (i = 0; i < 4; i++) {
        printf("%d", checksum[i]);
    }
    printf("\n\n");

    //adding the checksum to the hex string
    tmp = realloc(hex_string, sizeof(int) * (h_len + 1));

    //safety check
    if (tmp == NULL) {
        perror("realloc");
        return 0;
    }

    hex_string = tmp;

    int new_hex = 8*checksum[0] + 4*checksum[1] + 2*checksum[2] + 1*checksum[3];
    hex_string[h_len] = new_hex;
    h_len += 1; //updating size of the hex string


    printf("The xor checksum value for the chosen crc algorithm in hex:\n");
    printf("%X", new_hex);
    printf("\n\n");

    //printing the finalized hex string
    printf("The final message is going to be transmitted in hex:\n");
    for (i = 0; i < h_len; i++) {
        printf("%X", hex_string[i]);
    }
    printf("\n\n");

    //freeing everything
    free(o_text);
    free(p_text);
    free(binary_string);
    free(hex_string);
    free(crcpoly);
    free(dividend);
    free(hex_crc);
    free(crcval);
    free(decimal_text);
    free(checksum);

    //closing file
    fclose(msg_file);

    return 0;

}

/*Name: getValue
Preconditions: a character in the ASCII table
Postconditions: an int representing the decimal equivalent
Actions: looks up the value in the ASCII table and calcuates its value
*/
int getValue(char c) {
    int i, j;
    for (i = 0; i < 16; i++) {
        for (j = 2; j < 8; j++) {
            if (c == ascii_table[i][j]) {
                return 16 * j + i;
            }
        }
    }

    printf("Value not found.\n");
    return -1;

}

/*Name: binaryToHex
Preconditions: an int array representing a binary value made up of 8 bits
Postconditions: a pointer to a int array representing the hex values that make up the binary number
Actions: takes segements of the binary value and adds the corresponding hex value to the list
*/
int *binaryToHex(int *values) {

    int *hexNum = calloc(2, sizeof(int)); //8 bits = 2 hex values

    if (hexNum == NULL) {
        perror("calloc");
        return 0;
    }

    for (int i = 0; i < 8; i += 4) {
        hexNum[i/4] = values[i] * 8 + values[i+1] * 4 + values[i+2] * 2 + values[i+3];
    }

    return hexNum;

}

/*Name: decimalToBinary
Preconditions: an int representing a decimal
Postconditions: a pointer to an int array representing the bits that make up a binary number
Actions: takes the decimal number and repeatedly uses subtraction to create the binary string
*/
int* decimalToBinary(int n)
{
    int *binaryNum = calloc(8, sizeof(int)); //1 char = 1 byte

    if (binaryNum == NULL) {
        perror("calloc");
        return 0;
    }

    // counter for binary array
    int i;
    int count = 0;

    for (i = 128; i > 0; i /= 2) {
        
        if (n == 0) {
            break;
        }

        if (n >= i) {
            binaryNum[count] = 1;
            n -= i;
        }

        count++;

    }

    return binaryNum;
    
}