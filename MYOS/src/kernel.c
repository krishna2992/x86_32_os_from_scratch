#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "string/string.h"
#include "keyboard/keyboard.h"
#include "task/task.h"
#include "task/process.h"
#include "fs/fat/fat16.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "kernel.h"
#include "gdt/gdt.h"
#include "disk/streamer.h"
#include "config.h"
#include "status.h"
#include "task/tss.h"
#include "isr80h/isr80h.h"

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

void terminal_putchar(int x, int y, char c, char color)
{
    video_mem[(y*VGA_WIDTH)+ x] = terminal_make_char(c, color);
}

void terminal_backspace()
{
    if(terminal_row == 0 && terminal_col == 0)
    {
        return;
    }
    if(terminal_col == 0)
    {
        terminal_row -= 1;
        terminal_col = VGA_WIDTH;
    }

    terminal_col -= 1;
    terminal_write_char(' ', 15);
    terminal_col -= 1;

}

void terminal_write_char(char c, char color)
{
    if( c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }

    if( c == 0x08 )
    {
        terminal_backspace();
        return;
    }
    terminal_putchar(terminal_col, terminal_row, c, color);
    terminal_col += 1;
    if(terminal_col  >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}

void terminal_initialize()
{
    video_mem = (uint16_t*)(0XB8000);
    
    for(int y = 0; y < VGA_HEIGHT; y++)
    {
        for(int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0);
        }
    }

}

uint16_t terminal_make_char(char c, char color)
{
    return (color << 8)|c;
}



void print(const char* str)
{
    size_t len = strlen(str);
    for(int i = 0; i < len ;i++)
    {
        terminal_write_char(str[i], 15);
    }
}

void panic(const char* msg)
{
    print(msg);
    while (1)   {}
}

static struct paging_4gb_chunk* kernel_chunk;

void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_chunk);
}

struct tss tss;
struct gdt gdt_real[MYOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[MYOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00,          .limit = 0x00, .type = 0x00},        //NULL Segment
    {.base = 0x00,          .limit = 0xffffffff,    .type = 0x9a},  //Kernel Code Segment
    {.base = 0x00,          .limit = 0xffffffff,    .type = 0x92},  //Kernel Data Segment
    {.base = 0x00,          .limit = 0xffffffff,    .type = 0xf8},  //User code segment
    {.base = 0x00,          .limit = 0xffffffff,    .type = 0xf2},  //user data segment
    {.base = (uint32_t)&tss,.limit = sizeof(tss),   .type = 0xE9}  //TSS segment
};

void kernel_main()
{
    terminal_initialize();
    memset(gdt_real , 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, MYOS_TOTAL_GDT_SEGMENTS);
    gdt_load(gdt_real, sizeof(gdt_real));

    kheap_init();
    fs_init();
    disk_search_and_init();
    idt_init();
    //setup tss
    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SELECTOR;

    tss_load(0x28);

    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(kernel_chunk);
    //Enable paging
    enable_paging();
    
    //Register all isr commands
    isr80h_register_commands();
    //Initialise all system keyboards
    keyboard_init();

    struct process* process = 0;
    int res = process_load_switch("0:/BLANK.ELF", &process);
    if(res != MYOS_ALL_OK)
    {
        panic("Failed to load blank.elf");
    }

    struct command_argument argument;
    argument.next = 0x00;
    strcpy(argument.argument, "Testing!");
    process_inject_arguments(process, &argument);

    struct process* process2 = 0;
    res = process_load_switch("0:/BLANK.ELF", &process2);
    if(res != MYOS_ALL_OK)
    {
        panic("Failed to load blank.elf");
    }
    struct command_argument argument2;
    argument2.next = 0x00;
    strcpy(argument2.argument, "ABC!");
    process_inject_arguments(process2, &argument2);
    
    //task_run_first_ever_task();
    while(1)    {}
}