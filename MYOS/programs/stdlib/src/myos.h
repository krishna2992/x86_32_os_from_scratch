#ifndef MYOS_H
#define MYOS_H
#include <stddef.h>
#include <stdbool.h>

struct command_argument
{
    char argument[512];
    struct command_argument* next;
};

struct process_arguments
{
    int argc;
    char** argv;
};

void print(const char* message);
int myos_getkey();
void myos_putchar(char c);
void* myos_malloc(size_t size);
void myos_free(void* ptr);
int myos_getkey_block();
void myos_terminal_readline(char* out, int max, bool output_while_typing);
void myos_process_load_start(const char* filename);
struct command_argument* myos_parse_command(const char* command, int max);
int myos_system(struct command_argument* arguments);
void myos_process_get_arguments(struct process_arguments* arguments);
int myos_system_run(const char* commnd);
void myos_exit();
#endif