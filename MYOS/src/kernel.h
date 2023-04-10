#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 20

#define ERROR(value)    ((void*)value)
#define ERROR_I(value)  ((int)value)
#define ISERROR(value)  ((int)value < 0)

uint16_t terminal_make_char(char c, char color);
void terminal_initialize();
void print(const char* str);
void terminal_write_char(char c, char color);
void panic(const char* msg);
void kernel_page();
void kernel_registers();
void kernel_main();
#endif
