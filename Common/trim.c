#include <ctype.h> // isspace
#include <string.h> // strlen

#include "trim.h"

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