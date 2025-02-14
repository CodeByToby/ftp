#include <stdlib.h>

#include "exits.h"

// Exit if 'val' equals 'check_val'
//
static void errexit_if_equals(const int val, const int check_val, const char * errlabel)
{
    if (val == check_val) { 
        perror(errlabel); 
        exit (EXIT_FAILURE); 
    }
}

// Exit if 'val' is smallet than 'check_val'
//
static void errexit_if_smaller(const int val, const int check_val, const char * errlabel)
{
    if (val < check_val) { 
        perror(errlabel); 
        exit (EXIT_FAILURE); 
    }
}