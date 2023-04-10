#include "memory/memory.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "task.h"
#include "status.h"
#include "process.h"
#include "kernel.h"
#include "config.h"
#include "idt/idt.h"
#include "kernel.h"
#include "loader/formats/elfloader.h"
#include "memory/heap/kheap.h"


//current task
struct task* current_task = 0;

struct task* task_tail = 0;
struct task* task_head = 0;


int task_init(struct task* task, struct process* process);

struct task* task_current()
{
    return current_task;
}


struct task* task_get_next()
{
    if(!current_task->next)
    {
        return task_head;
    }
    return current_task->next;
}
static void task_list_remove(struct task* task)
{
    if(task->previous)
    {
        task->previous->next = task->next;
    }
    if(task == task_head)
    {
        task_head = task->next;
    }
    if(task_tail == task)
    {
        task_tail = task->previous;
    }
    if(task == current_task)
    {
        current_task = task_get_next();
    }
}

int task_free(struct task* task)
{
    paging_free_4gb(task->page_directory);
    task_list_remove(task);
    kfree(task);
    return 0;
}

void task_next()
{
    struct task* next_task = task_get_next();
    if(!next_task)
    {
        panic("No next task\n");
    }
    task_switch(next_task);
    task_return(&next_task->regs);
}

int task_switch(struct task* task)
{
    current_task = task;
    paging_switch(task->page_directory);
    return 0;
}

void task_save_state(struct task* task, struct interrupt_frame* frame)
{
    task->regs.ip = frame->ip;
    task->regs.cs = frame->cs;
    task->regs.flags = frame->flags;
    task->regs.esp = frame->esp;
    task->regs.ss = frame->ss;
    task->regs.eax = frame->eax;
    task->regs.ebp = frame->ebp;
    task->regs.ebx = frame->ebx;
    task->regs.ecx = frame->ecx;
    task->regs.edi = frame->edi;
    task->regs.edx = frame->edx;
    task->regs.esi = frame->esi;
}

int copy_string_from_task(struct task* task, void* virt, void* phys, int max)
{
    if(max >= PAGING_PAGE_SIZE)
    {
        return -EINVARG;
    }
    int res = 0;
    char* tmp = kzalloc(max);
    if(!tmp)
    {
        res = -ENOMEM;
        goto out;
    }
    uint32_t* task_directory = task->page_directory->directory_entry;
    uint32_t old_entry = paging_get(task_directory, tmp);
    paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITEABLE|PAGING_ACCESS_FROM_ALL|PAGING_IS_PRESENT);
    paging_switch(task->page_directory);
    strncpy(tmp, virt, max);
    kernel_page();

    res = paging_set(task_directory, tmp, old_entry);
    if(res < 0)
    {
        res = -EIO;
        goto out_free;
    }
    strncpy(phys, tmp, max);
out_free:
    kfree(tmp);
out:
    return res;
}

void task_current_save_state(struct interrupt_frame* frame)
{
    if(!task_current())
    {
        panic("No current task to save\n");
    }
    struct task* task = task_current();
    task_save_state(task, frame);

}

int task_page()
{
    user_registers();
    task_switch(current_task);
    return 0;
}

int task_page_task(struct task* task)
{
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

void task_run_first_ever_task()
{
    if(!current_task)
    {
        panic("\ntask_reu_first_ever_task(): No current task exists");
    }
    task_switch(task_head);
    task_return(&task_head->regs);
}

int task_init(struct task* task, struct process* process)
{
    memset(task, 0, sizeof(task));
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT|PAGING_ACCESS_FROM_ALL);
    if(!task->page_directory)
    {
        return -EIO;
    }
    
    task->regs.ip = MYOS_PROGRAM_VIRTUAL_ADDRESS;
    if(process->filetype == PROCESS_FILETYPE_ELF)
    {
        task->regs.ip = elf_header(process->elf_file)->e_entry;
    }
    task->regs.ss = USER_DATA_SEGMENT;
    task->regs.cs = USER_CODE_SEGMENT;
    task->regs.esp = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    task->process = process;
    return 0;
}

struct task* task_new(struct process* process)
{
    int res = 0;
    struct task* task = kzalloc(sizeof(struct task));
    if(!task)
    {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if(res != MYOS_ALL_OK)
    {
        goto out;
    }
    if(task_head == 0)
    {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }

    task_tail->next = task;
    task->previous = task_tail;
    task_tail = task;

out:
    if(ISERROR(res))
    {
        task_free(task);
        return ERROR(res);
    }
    return task;
}

void* task_get_stack_item(struct task* task, int index)
{
    void* result = 0;
    uint32_t* sp_ptr = (uint32_t*)task->regs.esp;
    task_page_task(task);

    result = (void*) sp_ptr[index];

    kernel_page();
    return result;
}

void* task_virtual_address_to_physical(struct task* task, void* virt)
{    
    return paging_get_physical_address(task->page_directory->directory_entry, virt);
}