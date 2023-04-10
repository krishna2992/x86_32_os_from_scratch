# x86_32_os_from_scratch

This is a basic and complete implementation of X86 32 bit operating system from Scratch.

This Project Contains Following Folders:

1. boot: This Contains bootloader code for our Project.Bootloader reads first 100 blocks from hard disk and loads it to memory location          as we specify and the os jumps to that location in protected mode.
2.  disk: This contains code for Hard Disk Reading.
3. fs : This contains code for fat16 filesystem
4. gdt: This folder contains code for Global Descriptor Table for our OS.
5. idt: Code for Loading Interrupt Descriptor Table is Written in this folder, as this is necessary for IO, WR operations
6. isr80h:This defines routines after int 0x80 interrupt is raised.This works as communication between User Level and Kernel Level
7. keyboard:This folder contains code for Keyboard driver.
8. loader: This is used to load user programs in mamin memory and then run them as a process.
9. memory/heap: Define a heap of memory for Furhur allocation of memory to user programs.
10. memory/paging: This the implementaion of Paging in our OS.
11. string :define different string functions for Kernel.
12. task: Defines process datastructure and its implementation.Can be further implemented to to use multithreading.

#BEFORE RUNNING THIS SET gcc target to i686 as follow

#run 
export TARGET=i686-elf

#then use make to create binaries
make all

#you can use gdb to debug your os
#run 

gdb
target remote | qemu-system-i386 -hda bin/os.bin -S -gdb stdio
add-symbol-file build/kernelfull.o 0x100000

#to set break point use

break function_name

eg. 
break kernel_main
