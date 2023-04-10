#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "myos.h"

int main(int argc, char const *argv[])
{
    print("MyOS v1.0.0\n");
    while (1)
    {
        print("\n> ");
        char buf[1024];
        myos_terminal_readline(buf, sizeof(buf), true);
        print("\n");
        myos_system_run(buf);
    }
    return 0;
}
