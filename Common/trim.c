#include <string.h> // strspn()

#include "trim.h"

void ltrim(char * str) {
    const char spaces[7] = " \t\n\v\f\r";
    const int initialSpacesLen = strspn(str, spaces);

    // Shift available content to left
    int i = 0;
    while(str[i + initialSpacesLen] != '\0') {
        str[i] = str[i + initialSpacesLen];
        i++;
    }

    // Null terminate to cut off remainder of string
    str[i] = '\0';
}