#include "types.h"
#include "io.h"
#include "process.h"
#include "schedule.h"
#include "syscalls.h"

/*
 * Llamada al sistema sys_sigreturn: Restaura el estado original de la CPU
 * tras la ejecución de una rutina de tratamiento de señales en espacio de usuario.
 */
void sys_sigreturn(void)
{
    u32 *esp;                           /* Puntero para navegar por la pila de usuario */

    cli;                                /* Sección Crítica: Deshabilita interrupciones */

    /* 
     * Obtiene el Frame Pointer (EBP) de la función actual y lee la dirección de la pila.
     * Se desplaza 17 posiciones (68 bytes) para ubicarse exactamente en el inicio del
     * marco de registros guardado previamente en handle_signal() dentro de signal.c.
     */
    asm("mov (%%ebp), %%eax; mov %%eax, %0": "=m"(esp):);
    esp += 17;

    /* --- Restauración de la Pila de Kernel y Registros de Segmento / Control --- */
    current->kstack.esp0 = esp[17];     /* Restaura el tope original de la pila de Kernel */
    current->regs.ss     = esp[16];     /* Restaura el selector de segmento de pila (SS) */
    current->regs.esp    = esp[15];     /* Restaura el puntero de pila del usuario (ESP) */
    current->regs.eflags = esp[14];     /* Restaura el registro de banderas de la CPU */
    current->regs.cs     = esp[13];     /* Restaura el selector de segmento de código (CS) */
    current->regs.eip    = esp[12];     /* Restaura el contador de programa (dirección a reanudar) */

    /* --- Restauración de Registros de Propósito General --- */
    current->regs.eax    = esp[11];     /* Restaura EAX (preserva el valor previo a la interrupción) */
    current->regs.ecx    = esp[10];     /* Restaura ECX */
    current->regs.edx    = esp[9];      /* Restaura EDX */
    current->regs.ebx    = esp[8];      /* Restaura EBX */
    current->regs.ebp    = esp[7];      /* Restaura EBP (Base Pointer original) */
    current->regs.esi    = esp[6];      /* Restaura ESI */
    current->regs.edi    = esp[5];      /* Restaura EDI */

    /* --- Restauración de Registros de Segmento de Datos --- */
    current->regs.ds     = esp[4];      /* Restaura el segmento de datos DS */
    current->regs.es     = esp[3];      /* Restaura el segmento extra ES */
    current->regs.fs     = esp[2];      /* Restaura el segmento FS */
    current->regs.gs     = esp[1];      /* Restaura el segmento GS */

    /* 
     * Reorganiza la tarea actual y transfiere el control para forzar la vuelta a Ring 3
     * cargando el contexto recién restaurado en los registros del CPU.
     */
    switch_to_task(0, KERNELMODE);
}
