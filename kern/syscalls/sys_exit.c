#include "types.h"
#include "list.h"
#include "lib.h"
#include "io.h"
#include "process.h"
#include "kmalloc.h"
#include "mm.h"
#include "schedule.h"
#include "file.h"
#include "signal.h"

/*
 * Llamada al sistema sys_exit: Termina el proceso actual y destruye sus recursos.
 * Argumento: status - Código de salida reportado al proceso padre.
 */
void sys_exit(int status) 
{
    u16 kss;                            /* Guardará el selector de segmento de pila del kernel para el cambio */
    u32 kesp;                           /* Guardará el puntero de pila del kernel para el cambio temporal */
    struct list_head *p, *n;            /* Punteros auxiliares para recorrer listas en forma segura */
    struct page *pg;                    /* Puntero a estructura de página de memoria */
    struct open_file *fd, *fdn;         /* Punteros para recorrer los descriptores de archivos abiertos */
    struct process *proc;               /* Puntero auxiliar a estructuras de proceso */

    printk("DEBUG: sys_exit(): process[%d] exit\n", current->pid);     /* Imprime mensaje de depuración */

    n_proc--;                           /* Decrementa el contador global de procesos activos en el sistema */
    current->state = -1;                /* Establece el estado del proceso en -1 (ZOMBIE / Terminado) */
    current->status = status;           /* Guarda el código de retorno pasado como argumento */

    /* 
     * --- 1. Liberación de Marcos de Memoria Física ---
     * Recorre la lista 'pglist' (páginas asociadas al código, datos y pila de usuario)
     * liberando los Marcos Físicos (frames) y la memoria del kernel usada para rastrearlos.
     */
    list_for_each_safe(p, n, &current->pglist) {
        pg = list_entry(p, struct page, list);
        release_page_frame(pg->p_addr); /* Liberación del frame físico en el bitmap del Kernel */
        list_del(p);                    /* Elimina el nodo de la lista */
        kfree(pg);                      /* Libera la estructura struct page del Heap del kernel */
    }

    /* 
     * --- 2. Liberación de Descriptores de Archivos ---
     * Cierra y libera todos los descriptores de archivo abiertos por este proceso.
     */
    fd = current->fd;
    while (fd) {
        /* Decrementa el contador de referencias de apertura del archivo */
        fd->file->opened--;
        if (fd->file->opened == 0) {
            kfree(fd->file->mmap);      /* Si nadie más lo usa, libera el búfer de mapeo */
            fd->file->mmap = 0;
        }

        fdn = fd->next;                 /* Guarda el puntero al siguiente archivo abierto */
        kfree(fd);                      /* Libera la estructura del descriptor actual */
        fd = fdn;
    }

    /* 
     * --- 3. Notificación al Proceso Padre ---
     * Envía la señal SIGCHLD al padre si sigue vivo; de lo contrario emite una advertencia.
     */
    if (current->parent->state > 0) 
        set_signal(&current->parent->signal, SIGCHLD);
    else
        printk("WARNING: sys_exit(): process %d without valid parent\n", current->pid);
    
    /* 
     * --- 4. Reasignación de Procesos Hijos (Orfandad) ---
     * Si el proceso que muere tenía procesos hijos, los reasigna al proceso 0 (Init/Idle).
     */
    list_for_each_safe(p, n, &current->child) {
        proc = list_entry(p, struct process, sibling);
        proc->parent = &p_list[0];      /* Cambia el padre apuntando a p_list[0] (Init/Kernel) */
        list_del(p);                    /* Desvincula del proceso actual */
        list_add(p, &p_list[0].child); /* Agrega el hijo a la lista del proceso Init */
    }

    /* --- 5. Liberación de la Consola Asignada --- */
    kfree(current->console);
    current->console = 0;

    /* 
     * --- 6. Liberación de la Pila de Kernel (kstack) ---
     * Dado que sys_exit está ejecutándose sobre la propia pila de kernel del proceso que va a morir,
     * conmuta temporalmente la pila (SS:ESP) a la del proceso 0 antes de destruirla.
     */
    kss = p_list[0].regs.ss;
    kesp = p_list[0].regs.esp;
    asm("mov %0, %%ss; mov %1, %%esp;"::"m"(kss), "m"(kesp));
    release_page_from_heap((char *) ((u32) current->kstack.esp0 & 0xFFFFF000)); /* Libera la página de pila del Heap */

    /* 
     * --- 7. Destrucción de Tablas de Páginas ---
     * Vuelve al directorio de páginas base del kernel (pd0) mediante CR3 y destruye
     * el directorio de páginas (Page Directory) del proceso finalizado.
     */
    asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(pd0));
    pd_destroy(current->pd);

    /* --- 8. Cambio Final de Tarea --- */
    switch_to_task(0, KERNELMODE);      /* Conmuta el procesador a la tarea Kernel (Process 0) */
}
