#include "libc/libc.h"
#include "libc/syscalls.h"
#include "libc/malloc.h"
#include "dir.h"

#define PROMPT    "minishell> "   /* Cadena base del prompt */

/* Manejador para la señal capturada */
void sigtrap(void)
{
    console_write("signal trapped\n");
}

int main(void)
{
    struct directory_entry *dir;
    char buf[512], out[512]; 
    char *beg_p, *end_p, **av;
    int fd, count, ac;
    int status, pid;
    int i, num;

    /* Captura la señal 10 con sigtrap */
    sigaction(10, &sigtrap);

    num = 0;
    /* Bucle principal del shell (REPL) */
    while(1) {
        num++;
        sprintf(out, "%d:%s", num, PROMPT); /* Construye el prompt numerado */
        console_write(out);

        /* Limpia el búfer de entrada */
        for(i = 0; i < 512; i++)
            buf[i] = 0;

        /* Lee la orden del usuario desde la consola */
        console_read(buf);

        beg_p = buf;
        /* Omite espacios e indentaciones iniciales */
        while (*beg_p == ' ' || *beg_p == '\t')
            beg_p++;

        /* Comando: exit */
        if (strncmp("exit", beg_p, 4) == 0) {
            exit(0);
        }

        /* Comando: ls */
        else if (strncmp("ls", beg_p, 2) == 0) {
            fd = open(".");                         /* Abre el directorio actual */
            count = read(fd, buf, sizeof(buf));     /* Lee las entradas del directorio */
            dir = (struct directory_entry*) buf;

            /* Recorre e imprime los nombres de las entradas */
            while(count > 0 && dir->inode) {
                memcpy(out, &dir->name, dir->name_len);
                out[dir->name_len] = 0;
                
                console_write(out);
                console_write("\n");

                count -= dir->rec_len;
                dir = (struct directory_entry*) ((char*) dir + dir->rec_len);
            }
            close(fd);                              /* Cierra el directorio */
        }

        /* Comando: cd */
        else if (strncmp("cd", beg_p, 2) == 0) {
            beg_p += 2;
            while (*beg_p == ' ' || *beg_p == '\t') 
                beg_p++;

            end_p = beg_p;
            while (*end_p && *end_p != '\n' && *end_p != ' ' && *end_p != '\t') 
                end_p++;
            *end_p = 0;                             /* Delimita la ruta introducida */

            chdir(beg_p);                           /* Cambia el directorio actual */
        }

        /* Comando: pwd */
        else if (strncmp("pwd", beg_p, 3) == 0) {
            pwd(buf);                               /* Obtiene la ruta de trabajo actual */
            console_write(buf);
            console_write("\n");
        }

        /* Comando: help */
        else if (strncmp("help", beg_p, 4) == 0) {
            console_write("usage:\n\tcd\n\texit\n\tls\n\tpwd\n");
        }

        /* Ejecución de comandos/programas externos */
        else {
            /* Cuenta la cantidad de argumentos */
            ac = 1;
            end_p = beg_p;
            while (*end_p && *end_p != '\n') {
                while (*end_p && *end_p != '\n' && *end_p != ' ' && *end_p != '\t') 
                    end_p++;
                ac++;
                while (*end_p == ' ' || *end_p == '\t') 
                    end_p++;
            }

            if (ac > 1) {
                /* Reserva espacio para la lista de argumentos */
                av = (char**) malloc(sizeof(char*) * ac);

                beg_p = end_p = beg_p;
                /* Extrae cada argumento y asigna memoria dinámicamente */
                for(i = 0; i < (ac - 1); i++) {
                    while (*end_p == ' ' || *end_p == '\t')
                        end_p++;
                    beg_p = end_p;
    
                    while (*end_p && *end_p != '\n' && *end_p != ' ' && *end_p != '\t')
                        end_p++;
    
                    av[i] = (char*) malloc(end_p - beg_p + 1);
                    strncpy(av[i], beg_p, end_p - beg_p);
                    av[i][end_p - beg_p] = 0;
                }
                av[i] = (char*) 0;                  /* Termina la lista argv con NULL */
    
                /* Ejecuta el comando en un nuevo proceso */
                pid = exec(av[0], av);

                if (pid > 0) {
                    sprintf(out, "shell: create child [%d]\n", pid);
                    console_write(out);
                    pid = wait(&status);            /* Espera a que el proceso hijo termine */
                }

                /* Libera la memoria asignada para los argumentos */
                for(i = 0; i < (ac - 1); i++) 
                    free(av[i]);
                free((char*) av);
            }
        }
    }

    exit(0);

    return 0;
}
