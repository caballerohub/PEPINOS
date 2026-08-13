#include "types.h"
#include "lib.h"
#include "process.h"
#include "syscalls.h"

/* Declaración de las syscalls implementadas */
extern int sys_exit(int code);
extern int sys_fork(void);
extern int sys_read(int fd, char *buf, u32 count);
extern int sys_write(int fd, const char *buf, u32 count);
extern int sys_open(const char *filename, int flags);
extern int sys_close(int fd);
extern int sys_wait(int *status);
extern int sys_execve(const char *filename, char **argv, char **envp);
extern int sys_sbrk(int increment);
extern int sys_sigreturn(void);

/* Tabla de punteros para enrutar las syscalls (índice = número de syscall en EAX) */
void *sys_call_table[] = {
    [SYS_EXIT]      = sys_exit,        /* 1: Termina proceso */
    [SYS_FORK]      = sys_fork,        /* 2: Crea proceso hijo */
    [SYS_READ]      = sys_read,        /* 3: Lee de archivo/dispositivo */
    [SYS_WRITE]     = sys_write,       /* 4: Escribe en archivo/pantalla */
    [SYS_OPEN]      = sys_open,        /* 5: Abre archivo */
    [SYS_CLOSE]     = sys_close,       /* 6: Cierra archivo */
    [SYS_WAIT]      = sys_wait,        /* 7: Espera a proceso hijo */
    [SYS_EXECVE]    = sys_execve,      /* 11: Ejecuta programa ELF */
    [SYS_SBRK]      = sys_sbrk,        /* 45: Modifica el heap */
    [SYS_SIGRETURN] = sys_sigreturn    /* 48: Retorno de manejador de señal */
};

/* Total de syscalls en la tabla */
#define NUM_SYSCALLS (sizeof(sys_call_table) / sizeof(void *))

/* Manejador de la interrupción int 0x80 */
void do_syscall(void)
{
    u32 sys_num;                       /* Número de syscall */
    int ret;                           /* Valor de retorno */
    int (*fn)(u32, u32, u32, u32, u32); /* Puntero a la función de la syscall */

    /* Obtiene el número de syscall desde EAX */
    sys_num = current->regs.eax;

    /* Valida el rango de la syscall y que la función exista */
    if (sys_num >= NUM_SYSCALLS || !sys_call_table[sys_num]) {
        printk("ERROR: Syscall invalida %d por proceso %d\n", sys_num, current->pid);
        current->regs.eax = -1;       /* Devuelve error -1 */
        return;
    }

    /* Obtiene la función de la tabla */
    fn = sys_call_table[sys_num];

    /* Llama a la syscall pasando los argumentos guardados en los registros */
    ret = fn(current->regs.ebx, 
             current->regs.ecx, 
             current->regs.edx, 
             current->regs.esi, 
             current->regs.edi);

    /* Guarda el resultado en EAX para devolverlo a usuario */
    current->regs.eax = ret;
}
