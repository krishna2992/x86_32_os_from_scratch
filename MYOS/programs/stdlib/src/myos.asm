[BITS 32]

section .asm
global print:function
global myos_getkey:function
global myos_putchar:function
global myos_malloc:function
global myos_free:function
global myos_process_load_start:function
global myos_system:function
global myos_process_get_arguments:function
global myos_exit:function

; void print(const char* message)
print:
    push ebp
    mov ebp, esp
    push dword[ebp+8]
    mov eax, 1
    int 0x80
    add esp, 4
    pop ebp
    ret

myos_getkey:
    push ebp
    mov ebp, esp
    mov eax, 2
    int 0x80
    pop ebp
    ret

; void putchar(char c)
myos_putchar:
    push ebp
    mov ebp, esp
    mov eax, 3
    push dword[ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret

; void* myos_malloc(size_t size)

myos_malloc:
    push ebp
    mov ebp, esp
    mov eax, 4 ;command for malloc
    push dword[ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret

myos_free:
    push ebp
    mov ebp, esp
    mov eax, 5 ;command for malloc
    push dword[ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret

; void myos_process_load_start(const char* filename)
myos_process_load_start:
    push ebp
    mov ebp, esp
    mov eax, 6 ;command for malloc
    push dword[ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret

; int myos_system(struct command_argument* arguments);
myos_system:
    push ebp
    mov ebp, esp
    mov eax, 7
    push dword[ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret

; void myos_process_get_arguments(struct process_arguments* arguments);

myos_process_get_arguments:
    push ebp
    mov ebp, esp
    mov eax, 8
    push dword[ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret

; void myos_exit()
myos_exit:
    push ebp
    mov ebp, esp
    mov eax, 9
    int 0x80
    add esp, 4
    pop ebp
    ret