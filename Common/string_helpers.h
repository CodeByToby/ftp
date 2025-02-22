#ifndef STRING_HELPERS_H
#define STRING_HELPERS_H

/// @brief Removes leading and trailing spaces from string.
/// @param str string to be trimmed.
void trim(char * str);

/// @brief Compares two strings' contents and sizes
/// @param s1 validation string
/// @param s2 comparison string
/// @return 1 if strings are equal, 0 otherwise
int strncmp_size(const char * s1, const char * s2);

#endif