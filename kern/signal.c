#include "types.h"
#include "lib.h"
#include "process.h"
#include "signal.h"
#include "syscalls.h"

/* Obtiene el número de la primera señal activa en la máscara (1 a 32, o 0) */
int dequeue_signal(int mask) 
{
    int sig;

    if (mask) {
        sig = 1;
        while (!(mask & 1)) { /* Busca el primer bit en 1 */
            mask = mask >> 1;
            sig++;
        }
    }
    else
        sig = 0;

    return sig;
}

/* Maneja la ejecución de una señal para el proceso actual */
int handle_signal(int sig)
{
    u32 *esp;

    printk("DEBUG: handle_signal(): signal %d for process %d\n", sig, current->pid);

    /* Caso 1: Ignorar señal (SIG_IGN) */
    if (current->sigfn[sig] == (void*) SIG_IGN) {
        clear_signal(&current->signal, sig);
    }
    /* Caso 2: Acción por defecto (SIG_DFL) */
    else if (current->sigfn[sig] == (void*) SIG_DFL) {
        switch(sig) {
            case SIGHUP : case SIGINT : case SIGQUIT : 
                /* Carga el directorio de páginas (CR3) */
                asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(current->regs.cr3));
                sys_exit(1);
                break;
            case SIGCHLD : 
                break; /* Se ignora por defecto */
            default :
                clear_signal(&current->signal, sig);
        }
    }
    /* Caso 3: Manejador de usuario */
    else {
        /* Reserva 80 bytes (20 palabras) en la pila de usuario */
        esp = (u32*) current->regs.esp - 20;

        /* Asegura el espacio de memoria del proceso */
        asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(current->regs.cr3));

        /* Inyecta el código trampolín (mov $0x30, %eax; int $0x80) */
        esp[19] = 0x0030CD00;
        esp[18] = 0x00000EB8;

        /* Guarda el contexto original en la pila de usuario */
        esp[17] = current->kstack.esp0;
        esp[16] = current->regs.ss;
        esp[15] = current->regs.esp;
        esp[14] = current->regs.eflags;
        esp[13] = current->regs.cs;
        esp[12] = current->regs.eip;
        esp[11] = current->regs.eax;
        esp[10] = current->regs.ecx;
        esp[9] = current->regs.edx;
        esp[8] = current->regs.ebx;
        esp[7] = current->regs.ebp;
        esp[6] = current->regs.esi;
        esp[5] = current->regs.edi;
        esp[4] = current->regs.ds;
        esp[3] = current->regs.es;
        esp[2] = current->regs.fs;
        esp[1] = current->regs.gs;

        /* Dirección de retorno al trampolín */
        esp[0] = (u32) &esp[18];

        /* Redirige ESP a la nueva pila y EIP al manejador */
        current->regs.esp = (u32) esp;
        current->regs.eip = (u32) current->sigfn[sig];

        /* Restaura acción a SIG_DFL y limpia la señal */
        current->sigfn[sig] = (void*) SIG_DFL;
        if (sig != SIGCHLD)
            clear_signal(&current->signal, sig);
    }

    return 0;
}
