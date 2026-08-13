#include <stdarg.h>
#include "libc.h"

/* Syscall 1: Escribe una cadena de texto en la consola/pantalla */
void console_write(char *msg)
{
    int sys_num = 1;

    asm volatile ("           \
        mov %0, %%eax    \n \
        mov %1, %%ebx    \n \
        int $0x30" 
        :: "g" (sys_num), "g" (msg)
    );
}

/* Syscall 2: Finaliza la ejecución del proceso actual devolviendo un código de estado */
void exit(int status)
{
    int sys_num = 2;

    asm volatile ("           \
        mov %0, %%eax    \n \
        mov %1, %%ebx    \n \
        int $0x30" 
        :: "g" (sys_num), "g" (status)
    );
}

/* Syscall 3: Abre un archivo del sistema y retorna su File Descriptor (FD) */
int open(char *file)
{
    int fd;
    int sys_num = 3;

    asm volatile("               \
        mov %1, %%eax        \n \
        mov %2, %%ebx        \n \
        int $0x30        \n \
        mov %%eax, %0"    
        : "=g" (fd) 
        : "g" (sys_num), "m" (file)
    );
    
    return fd;
}

/*
 * Syscall 4: Lee 'size' bytes desde un archivo (fd) hacia un búfer en memoria.
 * Pasa argumentos en EAX (ID), EBX (FD), ECX (búfer) y EDX (tamaño).
 */
int read(int fd, char *buf, int size)
{
    int count;
    int sys_num = 4;

    asm volatile ("               \
        mov %1, %%eax        \n \
        mov %2, %%ebx        \n \
        mov %3, %%ecx        \n \
        mov %4, %%edx        \n \
        int $0x30        \n \
        mov %%eax, %0" 
        : "=g" (count) 
        : "g" (sys_num), "g" (fd), "g" (buf), "g" (size)
    );
    
    return count;
}

/* Syscall 5: Cierra un descriptor de archivo previamente abierto */
void close(int fd)
{
    int sys_num = 5;

    asm volatile ("           \
        mov %0, %%eax    \n \
        mov %1, %%ebx    \n \
        int $0x30" 
        :: "g" (sys_num), "g" (fd)
    );
}

/* Syscall 6: Lee la entrada de teclado enviada a la consola hacia un búfer */
int console_read(char *buf)
{
    int count;
    int sys_num = 6;

    asm volatile ("           \
        mov %1, %%eax    \n \
        mov %2, %%ebx    \n \
        int $0x30    \n \
        mov %%eax, %0" 
        : "=g" (count) 
        : "g" (sys_num), "g" (buf)
    );

    return count;
}

/* Syscall 7: Cambia el directorio de trabajo actual (PWD) del proceso */
void chdir(char *path)
{
    int sys_num = 7;

    asm volatile ("           \
        mov %0, %%eax    \n \
        mov %1, %%ebx    \n \
        int $0x30" 
        :: "g" (sys_num), "g" (path)
    );
}

/* Syscall 8: Obtiene la ruta del directorio de trabajo actual */
void pwd(char *buf)
{
    int sys_num = 8;

    asm volatile ("           \
        mov %0, %%eax    \n \
        mov %1, %%ebx    \n \
        int $0x30" 
        :: "g" (sys_num), "g" (buf)
    );
}

/* Syscall 9: Reemplaza la imagen del proceso actual por un nuevo ejecutable ELF */
int exec(char *path, char *argv[])
{
    int ret;
    int sys_num = 9;

    asm volatile ("           \
        mov %1, %%eax    \n \
        mov %2, %%ebx    \n \
        mov %3, %%ecx    \n \
        int $0x30    \n \
        mov %%eax, %0"    
        : "=g" (ret) 
        : "g" (sys_num), "g" (path), "g" (argv)
    );

    return ret;
}

/* Syscall 10: Incrementa o decrementa dinámicamente el tamaño del Heap (Memoria de usuario) */
void* sbrk(int n)
{
    char *addr;
    int sys_num = 10;

    asm volatile("               \
        mov %1, %%eax        \n \
        mov %2, %%ebx        \n \
        int $0x30        \n \
        mov %%eax, %0"    
        : "=g" (addr) 
        : "g" (sys_num), "m" (n)
    );
    
    return addr;
}

/* Syscall 11: Detiene la ejecución del proceso padre hasta que finalice un hijo */
int wait(int *status)
{
    int pid;
    int sys_num = 11;

    asm volatile("               \
        mov %1, %%eax        \n \
        mov %2, %%ebx        \n \
        int $0x30        \n \
        mov %%eax, %0"    
        : "=g" (pid) 
        : "g" (sys_num), "m" (status)
    );

    return pid;
}

/* Syscall 12: Envía una señal a un proceso objetivo especificando su PID */
int kill(int pid, int sig)
{
    int ret;
    int sys_num = 12;

    asm volatile("               \
        mov %1, %%eax        \n \
        mov %2, %%ebx        \n \
        mov %3, %%ecx        \n \
        int $0x30        \n \
        mov %%eax, %0"    
        : "=g" (ret) 
        : "g" (sys_num), "g" (pid), "g" (sig)
    );
    
    return ret;
}

/* Syscall 13: Registra un manejador/handler para una señal específica */
int sigaction(int sig, void *fn)
{
    int ret;
    int sys_num = 13;

    asm volatile("               \
        mov %1, %%eax        \n \
        mov %2, %%ebx        \n \
        mov %3, %%ecx        \n \
        int $0x30        \n \
        mov %%eax, %0"    
        : "=g" (ret) 
        : "g" (sys_num), "g" (sig), "g" (fn)
    );
    
    return ret;
}
