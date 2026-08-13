#include "libc/libc.h"
#include "libc/syscalls.h"
#include "libc/malloc.h"

#define SIZE        512

int main(int argc, char **argv)
{
    char out[SIZE];
    int signal, pid;

    /* Valida la cantidad de argumentos requeridos */
    if (argc < 3) {
        sprintf(out, "usage: %s <signal> <pid>\n", argv[0]);
        console_write(out);
        exit(1);
    }

    /* Convierte los argumentos de texto a enteros */
    signal = atoi(argv[1]);
    pid = atoi(argv[2]);

    /* Envía la señal al proceso mediante la syscall kill */
    kill(pid, signal);

    /* Finaliza el programa */
    exit(0);

    return 0;
}
