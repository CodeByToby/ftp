#include <ctype.h> // isspace
#include <string.h> // strlen

#include "string_helpers.h"

void trim(char * str) {
    int start = 0;
    int end = strlen(str) - 1;

    while(isspace((unsigned char) str[start]))
        start++;

    while((end >= start) && isspace((unsigned char) str[end]))
        end--;

    // Shift available content to the left
    int i;
    for(i = start; i <= end; ++i)
        str[i - start] = str[i];

    // Null terminate to cut off remainder of string
    str[i - start] = '\0';
} 

int strncmp_size(const char * s1, const char * s2) {
    return (strncmp(s1, s2, strlen(s1)) == 0 && 
            strlen(s1) == strlen(s2))? 1 : 0;
}