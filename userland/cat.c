#include "libc/libc.h"
#include "libc/syscalls.h"
#include "libc/malloc.h"

#define SIZE        512    /* Tamaño del búfer de lectura (512 bytes) */

int main(int argc, char **argv)
{
    char *buf, out[SIZE];
    int fd, n, i;

    /* Reserva memoria en el heap para el búfer de lectura */
    if (-1 == (int) (buf = (char*) malloc(SIZE))) {
        console_write("error: malloc() failed\n");
        exit(1);
    }

    /* Recorre cada archivo pasado como argumento */
    for (i = 1; i < argc; i++) {

        /* Abre el archivo actual */
        if (-1 == (fd = open(argv[i]))) {
            sprintf(out, "error: open() %s failed\n", argv[i]);
            console_write(out);
            free(buf);
        }

        else {
            /* Lectura por bloques */
            do {
                /* Lee hasta (SIZE - 1) bytes del archivo */
                if (-1 == (n = read(fd, buf, SIZE - 1))) {
                    console_write("error: read() failed\n");
                    free(buf);
                    break;
                }

                /* Agrega el delimitador nulo al final del bloque */
                buf[n] = 0;

                /* Imprime el bloque leído en la consola */
                console_write(buf);

            } while (n); /* Lee hasta alcanzar el final del archivo (EOF) */

            /* Cierra el descriptor de archivo */
            close(fd);
        }
    }

    /* Libera el búfer de lectura */
    free(buf);

    /* Finaliza el programa */
    exit(0);

    return 0;
}
