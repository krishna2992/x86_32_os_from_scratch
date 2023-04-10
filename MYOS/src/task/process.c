#include "process.h"
#include "memory/memory.h"
#include "task.h"
#include "status.h"
#include "fs/file.h"
#include "string/string.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "loader/formats/elfloader.h"
#include "loader/formats/elf.h"
#include "memory/paging/paging.h"

//The current process
struct process* current_process = 0;

static struct process* processes[MYOS_MAX_PROCESS] = {};

static void process_init(struct process* process)
{
    memset(process, 0x00, sizeof(process));   
}

struct process* process_current()
{
    return current_process;
}

struct process* process_get(int process_id)
{
    if(process_id < 0 || process_id >= MYOS_MAX_PROCESS)
    {
        return NULL;
    }
    return processes[process_id];
}

int process_switch(struct process* process)
{
    current_process = process;
    return 0;
}

static int process_find_free_allocation_index(struct process* process)
{
    int res = -ENOMEM;
    for (int i = 0; i < MYOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == 0)
        {
            res = i;
            break;
        }
    }
    return res;
}

void* process_malloc(struct process* process, size_t size)
{
    void* ptr = kzalloc(size);
    if(!ptr)
    {
        return 0;
    }
    int index = process_find_free_allocation_index(process);
    if(index < 0)
    {
        goto out_err;
    }
    int res = paging_map_to(process->task->page_directory, ptr, ptr, paging_align_address(ptr+size), PAGING_IS_WRITEABLE|PAGING_IS_PRESENT|PAGING_ACCESS_FROM_ALL);
    if(res < 0)
    {
        goto out_err;
    }
    process->allocations[index].ptr = ptr;
    process->allocations[index].size = size;
    return ptr;
out_err:
    if(ptr)
    {
        kfree(ptr);
    }
    return 0;

}

static bool process_is_pointer(struct process* process, void* ptr)
{
    for (int i = 0; i < MYOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == ptr)
        {
            return true;
            break;
        }
    }
    return false;
    
}

static void process_allocation_free(struct process* process, void* ptr)
{
    for (int i = 0; i < MYOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == ptr)
        {
            process->allocations[i].ptr = 0x00;
            process->allocations[i].size = 0x00;
            break;
        }
    }
}

static struct process_allocation* process_get_allocation_by_addr(struct process* process, void* addr)
{
    for (int i = 0; i < MYOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == addr)
        {
            return &process->allocations[i];
        }
    }
    return 0;
    
}

static int process_terminate_allocations(struct process* process)
{
    for (int i = 0; i < MYOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        process_free(process, process->allocations[i].ptr);
    }
    return 0;
}

int process_free_binary_data(struct process* process)
{
    kfree(process->ptr);
    return 0;
}

int process_free_elf_data(struct process* process)
{
    elf_close(process->elf_file);
    return 0;
}

int process_free_program_data(struct process* process)
{
    int res = 0;
    switch(process->filetype)
    {
        case PROCESS_FILETYPE_BINARY:
            res = process_free_binary_data(process);
        break;

        case PROCESS_FILETYPE_ELF:
            res = process_free_elf_data(process);
        break;

        default:
            res = -EINVARG;
    }
    return res;
}

void process_switch_to_any()
{
    for (int i = 0; i < MYOS_MAX_PROCESS; i++)
    {
        if(processes[i])
        {
            process_switch(processes[i]);
            return;
        }
    }
    panic("No process to switch to\n");
}

static void process_unlink(struct process* process)
{
    processes[process->id]  = 0x00;
    if(current_process == process)
    {
        process_switch_to_any();
    }
}

int process_terminate(struct process* process)
{
    int res = 0;
    res = process_terminate_allocations(process);
    if(res < 0)
    {
        goto out;
    }

    res = process_free_program_data(process);
    if(res < 0)
    {
        goto out;
    }
    kfree(process->stack);

    task_free(process->task);
    process_unlink(process);
out:
    return res;
}

void process_get_arguments(struct process* process, int* argc, char*** argv)
{
    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

int process_count_command_argumentd(struct command_argument* root_arguments)
{
    int i = 0;
    struct command_argument* current = root_arguments;
    while(current)
    {
        i++;
        current = current->next;
    }
    return i;
}

int process_inject_arguments(struct process* process, struct command_argument* root_argument)
{
    int res = 0;
    struct command_argument* current = root_argument;
    int i = 0;
    int argc = process_count_command_argumentd(root_argument );
    if(argc == 0)
    {
        res = -EIO;
        goto out;
    }
    
    char** argv = process_malloc(process, sizeof(const char*) * argc);
    if(!argv)
    {
        res = -ENOMEM;
        goto out;
    }
    while(current)
    {
        char* arg_str = process_malloc(process, sizeof(current->argument));
        if(!arg_str)
        {
            res = -ENOMEM;
            goto out;
        }
        strncpy(arg_str, current->argument, sizeof(current->argument));
        argv[i++] = arg_str;
        current = current->next;

    }

    process->arguments.argc = argc;
    process->arguments.argv = argv;

out:
    return res;
}

void process_free(struct process* process, void* ptr)
{
    struct process_allocation* allocation = process_get_allocation_by_addr(process, ptr);
    if(!allocation)
    {
        return;
    }
    int res = paging_map_to(process->task->page_directory, allocation->ptr, allocation->ptr, paging_align_address(allocation->ptr+allocation->size), 0x00);
    if(res < 0)
    {
        return;
    }
    process_allocation_free(process, ptr);
    kfree(ptr);

}

static int process_load_binary(const char* filename,struct process* process)
{
    void* program_data_ptr = 0;
    int res = 0;
    int fd = fopen(filename, "r");
    if(!fd)
    {
        res = -EIO;
        goto out;
    }
    struct file_stat stat;
    res = fstat(fd, &stat);
    if(res != MYOS_ALL_OK)
    {
        goto out;
    }

    program_data_ptr = kzalloc(stat.filesize);
    if(!program_data_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    if(fread(program_data_ptr, stat.filesize, 1, fd) != 1)
    {
        res = -EIO;
        goto out;
    }
    process->filetype = PROCESS_FILETYPE_BINARY;
    process->ptr = program_data_ptr;
    process->size = stat.filesize;
out:
    if(res < 0)
    {
        if(program_data_ptr)
        {
            kfree(program_data_ptr);
        }
    }
    fclose(fd);
    return res;
}

static int process_load_elf(const char* filename, struct process* process)
{
    int res = 0;
    struct elf_file* elf_file = 0;
    res = elf_load(filename, &elf_file);
    if(res < 0)
    {
        goto out;
    }
    process->filetype = PROCESS_FILETYPE_ELF;
    process->elf_file = elf_file;

out:
    return res;
}

static int process_load_data(const char* filename, struct process* process)
{
    int res = 0;
    res = process_load_elf(filename, process);
    if (res == -EINFORMAT)
    {
        res = process_load_binary(filename, process);
    }

    return res;
}

static int process_map_binary(struct process* process)
{
    int res = 0;
    paging_map_to(process->task->page_directory, (void*)MYOS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr+process->size), PAGING_IS_PRESENT|PAGING_ACCESS_FROM_ALL|PAGING_IS_WRITEABLE);
    return res;
}


static int process_map_elf(struct process* process)
{
    int res = 0;
    struct elf_file* elf_file = process->elf_file;
    struct elf_header* header = elf_header(elf_file);
    struct elf32_phdr* phdrs = elf_pheader(header);

    for(int i=0; i < header->e_phnum; i++)
    {
        struct elf32_phdr* phdr = &phdrs[i];
        void* phdr_phys_address = elf_phdr_phys_address(elf_file, phdr);
        int flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL;
        if(phdr->p_flags & PF_W)
        {
            flags |= PAGING_IS_WRITEABLE;
        }
        res = paging_map_to(process->task->page_directory, paging_align_to_lower_page((void*)phdr->p_vaddr), paging_align_to_lower_page(phdr_phys_address), paging_align_address(phdr_phys_address+phdr->p_memsz), flags);
        if(ISERROR(res))
        {
            break;
        }
    }
    
    return res;
} 

int process_map_memory(struct process* process)
{
    int res = 0;

    switch(process->filetype)
    {
        case PROCESS_FILETYPE_ELF:
            res = process_map_elf(process);
        break;

        case PROCESS_FILETYPE_BINARY:
            res = process_map_binary(process);
        break;

        default:
            panic("process_map_memory: Invalid filetype\n");
    }

    if (res < 0)
    {
        goto out;
    }

    // Finally map the stack
    paging_map_to(process->task->page_directory, (void*)MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address(process->stack+MYOS_USER_PROGRAM_STACK_SIZE), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
out:
    return res;
}

int process_get_slot()
{
    for(int i = 0 ; i < MYOS_MAX_PROCESS; i++)
    {
        if(processes[i] ==  0)
            return i;
    }
    return -EISTKN;
}

int process_load_switch(char* filename, struct process** process)
{
    int res = process_load(filename, process);
    if(res == 0)
    {
        process_switch(*process);
    }
    return res;
}

int process_load_for_slot( char* filename, struct process** process, int process_slot)
{
    int res = 0;
    struct task* task = 0;
    struct process* _process;
    void* program_stack_pointer = 0;
    

    if(process_get(process_slot) != 0)
    {
        res = -EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process));

    if(!_process)
    {
        res = -ENOMEM;
        goto out;
    }
    process_init(_process);
    res = process_load_data(filename, _process);
    if(res < 0)
    {
        goto out;
    }
    program_stack_pointer = kzalloc(MYOS_USER_PROGRAM_STACK_SIZE);
    if(!program_stack_pointer)
    {
        res = -ENOMEM;
        goto out;
    }
    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->stack = program_stack_pointer;
    _process->id = process_slot;

    //create task
    task = task_new(_process);
    if(ERROR_I(task) == 0 )
    {
        res = ERROR_I(task);
        goto out;
    }

    _process->task = task;

    res = process_map_memory(_process);
    if(res < 0)
    {
        goto out;
    }
    *process = _process;

    //ADD the process to array.
    processes[process_slot] = _process;

out:
    if(ISERROR(res))
    {
        if(_process && _process->task)
        {
            task_free(_process->task);
        }
        //Free the process data
    }
    return res;
}

int process_load( char* filename, struct process** process)
{
    int res = 0;
    int process_slot = process_get_slot();
    if(process_slot < 0)
    {
        res = -EISTKN;
        goto out;
    }
    res = process_load_for_slot(filename, process, process_slot);
out:
    return res;
}