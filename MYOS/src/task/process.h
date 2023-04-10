#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include "config.h"
#include "task.h"
#include <stdbool.h>

#define PROCESS_FILETYPE_ELF    0
#define PROCESS_FILETYPE_BINARY 1

typedef unsigned char PROCESS_FILETYPE;

struct process_allocation
{
    void* ptr;
    size_t size;
};

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

struct process
{
    uint16_t id;
    char filename[MYOS_MAX_PATH];
    struct task* task;

    struct process_allocation allocations[MYOS_MAX_PROGRAM_ALLOCATIONS];

    PROCESS_FILETYPE filetype;
    union 
    {
        //Physical Pointer to physical memory
        void* ptr;
        struct elf_file* elf_file;
    };
    
    //Physical pointer to stack memory
    void* stack;
    //The size of data pointed to by "ptr"
    uint32_t size;

    struct keyboard_buffer
    {
        char buf[MYOS_KEYBOARD_BUFFER_SIZE];
        int head;
        int tail;

    }keyboard;
    
    struct process_arguments arguments;
};

int process_switch(struct process* process);
int process_load_switch(char* filename, struct process** process);
int process_load( char* filename, struct process** process);
int process_load_for_slot( char* filename, struct process** process, int process_slot);
struct process* process_current();
struct process* process_get(int process_id);
void* process_malloc(struct process* process, size_t size);
void process_free(struct process* process, void* ptr);

void process_get_arguments(struct process* process, int* argc, char*** argv);
int process_inject_arguments(struct process* process, struct command_argument* root_argument);
int process_terminate(struct process* process);
#endif