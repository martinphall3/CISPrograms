/*
Assignment:
hillplayfair - Hill cipher followed by Playfair cipher
Author: Martin Hall
Language: C
To Compile:
gcc -O2 -std=c11 -o hillplayfair hillplayfair.c
To Execute (on Eustis):
./hillplayfair encrypt key.txt plain.txt keyword.txt
where:
key.txt = key matrix file
plain.txt = plaintext file
keyword.txt = Playfair keyword file
Notes:
- Input is text; process A-Z only (case-insensitive).
- Tested on Eustis.
Class: CIS3360 - Security in Computing - Spring 2026
Instructor: Dr. Jie Lin
Due Date: February 16th 2026
*/

//library inclusions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define _GNU_SOURCE

//declare function headers
int** matrix_multiply(int **arr_a, int **arr_b, int row_a, int row_b, int col_a, int col_b);
void w_printf(char *str);
char *grow_string(char *str, int size);

int main(int argc, char *argv[]) {
    
    /*printf("argc = %d\n", argc);
    for (int e = 0; e < argc; e++) {
        printf("argv[%d] = %s\n", e, argv[e]);
    }
    printf("\n");
    if (argc > 1) {
        int x = atoi(argv[1]); // convert string to int (simple)
        printf("Converted argv[1] to int: %d\n", x);
    } else {
        printf("No extra arguments provided.\n");
        printf("Try: ./00_args 123\n");
    }
    */ //starting point

    if (argc != 5) {
        printf("Incorrect number of arguments.");
        return 0;
    }

    //declaring loop variables
    int i = 0;
    int j = 0;

    //checking to see if the proper string was passed in
    if (strcmp(argv[1],"encrypt") != 0) {
        printf("Incorrect argument.");
        return 0;
    }

    //opening files
    FILE *key_file = fopen(argv[2], "r");
    FILE *plain_file = fopen(argv[3], "r");
    FILE *keyword_file = fopen(argv[4], "r");

    if(key_file == NULL || plain_file == NULL || keyword_file == NULL) {
        perror("fopen");
        return 0;
    }

    //printing required output to console
    printf("\nMode:\nEncryption Mode\n\n");

    //declaring local variables for plaintext filtering
    int c;
    char *o_plaintext = malloc(2);
    char *p_plaintext = malloc(2);
    if (p_plaintext == NULL || o_plaintext == NULL) { //safety check
        perror("malloc");
    }

    while ((c = fgetc(plain_file)) != EOF) { //only stop at the end of the file and not at spaces
        o_plaintext = grow_string(o_plaintext, i + 1);
            
        o_plaintext[i] = (char)c; //adding character to new string
        i++; //moving position in string

        if (isalpha((unsigned char)c)) { //only taking valid letters for processed string
            c = toupper((unsigned char)c); //and making them uppercase
            p_plaintext = grow_string(p_plaintext, j + 1); //reallocing more space for the string
            
            p_plaintext[j] = (char)c; //adding character to new string
            j++; //moving position in string
        }
    }

    o_plaintext[i] = '\0';
    p_plaintext[j] = '\0';
    printf("Original Plaintext:\n");
    printf("%s", o_plaintext);
    printf("\n\n");

    printf("Preprocessed Plaintext:\n");
    w_printf(p_plaintext);
    printf("\n\n");

    //declaring local variables for hill cipher
    char *p_ciphertext = malloc(strlen(p_plaintext) + 1);
    if (p_ciphertext == NULL) {
        perror("malloc");
        return 0;
    }
    strcpy(p_ciphertext, p_plaintext);
    int dim; fscanf(key_file, "%d", &dim);

    //printing dimension to console
    printf("Hill Cipher Key Dimension:\n");
    printf("%d\n\n", dim);

    int s_len = strlen(p_plaintext);
    int pad;
    if (s_len % dim != 0) { //checking if the dimensions match
        pad = dim - s_len % dim; //how much padding is needed
    } else {
        pad = 0; //no padding if dimensions match
    }
    int k = 0;
    int l = 0;
    int m = 0;
    int **hill_arr = (int**)malloc(dim * sizeof(int*)); //declaring array
    for (k = 0; k < dim; k++) {
        hill_arr[k] = (int*)malloc(dim * sizeof(int)); //allocating space in the array
    }

    //assigning values to the array and printing them
    printf("Hill Cipher Key Matrix:\n");
    for (k = 0; k < dim; k++) {
        for (l = 0; l < dim; l++) {
            fscanf(key_file, "%d", &hill_arr[k][l]);
            printf("%4d", hill_arr[k][l]);
        }
        printf("\n");
    }

    printf("\n");
    

    //padding if necessary
    for (k = 0; k < pad; k++) {
        p_ciphertext = grow_string(p_ciphertext, s_len + k + 1);

        p_ciphertext[s_len + k] = 'X'; //padding with X
    }
    p_ciphertext[s_len + pad] = '\0'; //adding the null terminator

    printf("Padded Hill Cipher Plaintext:\n");
    w_printf(p_ciphertext);
    printf("\n\n");

    //making a working copy
    char *c_ciphertext = malloc(2);
    if (c_ciphertext == NULL) {
        perror("malloc");
        return 0;
    }

    //checking new length
    int pad_len = strlen(p_ciphertext);
    
    for (m = 0; m < pad_len; m = m + dim) {
        //get dim elements of p_ciphertext
        int n = 0;
        int **mat = (malloc(dim * sizeof(int *))); //allocating space for the matrix
        for (n = 0; n < dim; n++) {
            char letter = p_ciphertext[m + n];
            mat[n] = calloc(1, sizeof(int)); //initializing to 0
            mat[n][0] = (int)letter - 65; //storing chars as ints in alphabet cipher
        }
        
        //matrix multiply with the key
        int **pmat = matrix_multiply(hill_arr, mat, dim, dim, dim, 1);

        for (n = 0; n < dim; n++) {
            c_ciphertext = grow_string(c_ciphertext, m + n + 1);
            c_ciphertext[m + n] = (char)(((pmat[n][0] % 26) + 26) % 26 + 65); //convert the results back into char, mod 26 to make sure the numbers are in range
        }

        for (n = 0; n < dim; n++) { //freeing the temp array
            free(mat[n]);
        }
        free(mat);

        for (n = 0; n < dim; n++) { //and the product array
            free(pmat[n]);
        } free(pmat);

    }

    c_ciphertext[pad_len] = '\0';

    printf("Ciphertext after Hill Cipher:\n");
    w_printf(c_ciphertext);
    printf("\n\n");

    //grabbing and padding the keyword
    char *keyword = malloc(2);
    int o = 0;
    
    while ((c = fgetc(keyword_file)) != EOF) { //only stop at the end of the file and not at spaces
        if(isalpha((unsigned char)c)) {
            c = toupper((unsigned char)c);
            keyword = grow_string(keyword, o + 1);
            keyword[o] = (char)c; //adding character to new string
            o++; //moving position in string
        }
            
    }
    keyword[o] = '\0';

    int used[26] = { 0 };
    used[9] = 1; //skipping J
    int q = 0;
    int keyword_len = strlen(keyword);
    int new_len = 0;
    
    //processing keyword
    for (q = 0; q < keyword_len; q++) {
        c = keyword[q];
        if (c == 'J')
            c = 'I'; //replacing J with I
        
        int index = c - 65;
        if (used[index] == 0) { //check if a letter has been repeated
            used[index] = 1;
            keyword[new_len++] = c;
        }
    }
    keyword[new_len] = '\0';
    keyword_len = new_len;

    //initializing playfair array
    char **play_array = malloc(5 * sizeof(char *));
    int x = 0;
    int y = 0;
    int count = 0;
    int let = 0;
    for (x = 0; x < 5; x++) {
        play_array[x] = malloc(5 * sizeof(char));
    }

    for (x = 0; x < 5; x++) {
        for (y = 0; y < 5; y++) {
            //if within strlen, add from keyword
            if (count < keyword_len) {
                play_array[x][y] = keyword[count];
                count++;
            } else {
                while (let < 26) {
                    if (used[let] == 0) {
                        play_array[x][y] = (char)(let + 65);
                        used[let] += 1; //recording usage
                        break;
                    }
                    let++;
                }
            }
        }
    }

    printf("Playfair Keyword:\n");
    printf("%s", keyword);
    printf("\n\n");

    printf("Playfair Table:\n");
    for (x = 0; x < 5; x++) {
        for (y = 0; y < 5; y++) {
            printf("%c ", play_array[x][y]);
        }
        printf("\n");
    }
    printf("\n");

    //processing text for cipher
    int c_len = strlen(c_ciphertext);

    char *pc_ciphertext = malloc(2);
    int a = 0;
    int b = 0;
    
    while (a < c_len) {
        c = c_ciphertext[a];
        if (c == 'J')
            c = 'I'; //replacing J with I
        a++;
        
        char next_c = '\0';

        if (a < c_len) {
            next_c = c_ciphertext[a];
            if (next_c == 'J')
            next_c = 'I'; //replacing J with I
        }
        
        pc_ciphertext = grow_string(pc_ciphertext, b + 1);
        pc_ciphertext[b] = c;
        b++;

        if (next_c == c) { //checking for repeated letters
            pc_ciphertext = grow_string(pc_ciphertext, b + 1);
            if (next_c == 'X') { //padding accordingly
                pc_ciphertext[b] = 'Z'; 
            } else {
                pc_ciphertext[b] = 'X';
            }
            b++;
        } else if (next_c != '\0') {
            pc_ciphertext = grow_string(pc_ciphertext, b + 1);
            pc_ciphertext[b] = next_c;
            a++;
            b++; //moving both ahead
        } else { //next_c == '\0'
            pc_ciphertext = grow_string(pc_ciphertext, b + 1);
            if (c == 'Z') { //checking last character to pad appropriately
                pc_ciphertext[b] = 'X'; 
            } else {
                pc_ciphertext[b] = 'Z';
            }
            b++;
        }
    } 

    pc_ciphertext[b] = '\0';

    //declaring local variables for playfair cipher
    int pc_len = strlen(pc_ciphertext);
    int g = 0;
    int row = 0;
    int col = 0;
    int out = 0;
    char *dc_ciphertext = malloc(1); //final string

    for (g = 0; g < pc_len; g += 2) {
        c = pc_ciphertext[g];
        char next_c = pc_ciphertext[g+1];
        int rindex1 = 0;
        int rindex2 = 0;
        int cindex1 = 0;
        int cindex2 = 0;

        //finding the chars in the array
        for (row = 0; row < 5; row++) {
            for (col = 0; col < 5; col++) {
                if (c == play_array[row][col]) {
                    rindex1 = row;
                    cindex1 = col;
                }
                if (next_c == play_array[row][col]) {
                    rindex2 = row;
                    cindex2 = col;
                }
            }
        }
        
        //applying the appropriate cipher rule
        if (rindex1 == rindex2) { //same row
            cindex1 = (cindex1 + 1) % 5;
            cindex2 = (cindex2 + 1) % 5;
        } else if (cindex1 == cindex2) { //similar to the row rule above
            rindex1 = (rindex1 + 1) % 5;
            rindex2 = (rindex2 + 1) % 5;
        } else { //rectangle rule
            int temp = cindex1;
            cindex1 = cindex2;
            cindex2 = temp;
        }
        dc_ciphertext = grow_string(dc_ciphertext, out + 2); //space for both chars
        dc_ciphertext[out++] = play_array[rindex1][cindex1];
        dc_ciphertext[out++] = play_array[rindex2][cindex2];
    }

    dc_ciphertext[out] = '\0';

    //printing the final result
    printf("Ciphertext after Playfair:\n");
    w_printf(dc_ciphertext);
    printf("\n");

    //closing files
    fclose(key_file);
    fclose(plain_file);
    fclose(keyword_file);

    //FREEING EVERYTHING
    for (k = 0; k < dim; k++) {
        free(hill_arr[k]);
    } free(hill_arr);

    for (k = 0; k < 5; k++) {
        free(play_array[k]);
    } free(play_array);

    free(keyword);
    free(pc_ciphertext);
    free(dc_ciphertext);
    free(o_plaintext);
    free(p_plaintext);
    free(p_ciphertext);
    free(c_ciphertext);

    return 0;
}

/*Name: matrix_multiply
Preconditions: two 2D arrays
Postconditions: one 2D array that is the product of both arrays
Actions: uses the row-column method of matrix multiplication to compute a product matrix
*/

int** matrix_multiply(int **arr_a, int **arr_b, int row_a, int row_b, int col_a, int col_b) {
    //declaring local variables for loop
    int i, j;
    
    //dimension check
    if (col_a != row_b) {
        printf("Incompatible dimensions.");
        return NULL;
    }
    
    //declaring product array
    int **arr_c = (int**)malloc(row_a * sizeof(int*));

    //allocating space for each row
    for (i = 0; i < row_a; i++) {
        arr_c[i] = (int*)malloc(col_b * sizeof(int));
    }
    
    //filling the array with product values
    for (i = 0; i < row_a; i++) {
        for (j = 0; j < col_b; j++) {
            arr_c[i][j] = 0;
            for (int k = 0; k < col_a; k++) {
                arr_c[i][j] += arr_a[i][k] * arr_b[k][j];
            }
        }
    }
    
    //returning the resulting array
    return arr_c;
}

/*Name: w_printf 
Preconditions: the string to be printed
Postconditions: none
Actions: prints the string passed to the function with 80 characters per line
*/

void w_printf(char *str) {
    int i;
    int len = strlen(str);
    for (i = 0; i < len; i++) {
        if (i % 80 == 0 && i != 0) {
            printf("\n");
        }
        printf("%c", str[i]);
    }

}

/*Name: grow_string
Preconditions: a string and an int that represents the new size
Postconditions: a pointer to the longer string
Actions: takes the input string and allocates an extra character for it
*/

char *grow_string(char *str, int new_size) {
    char *temp = realloc(str, new_size + 1); //increasing string size by +1 for the null terminator
        if (temp == NULL) { //safety check
            perror("realloc");
            return 0;
        }
        
        return temp;
}