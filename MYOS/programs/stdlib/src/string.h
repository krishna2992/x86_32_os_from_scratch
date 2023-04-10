#ifndef MYOS_STRING_H
#define MYOS_STRING_H

#include <stdbool.h>

int strlen(const char* ptr);
int strnlen(const char* ptr, int max);
char tolower(char c);
int istrncmp(const char* s1, const char* s2, int n);
int strncmp(const char* str1, const char* str2, int n);
char* strcpy(char* dest, char* src);
char* strncpy( char* dest, char* src, int count);
bool isdigit(char c);
int tonumericdigit(char c);
char* strtok(char* str, const char* delimiters);

#endif