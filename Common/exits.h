#ifndef EXITS_H
#define EXITS_H

static void errexit_if_equals(const int val, const int check_val, const char * errlabel);
static void errexit_if_smaller(const int val, const int check_val, const char * errlabel);

#endif