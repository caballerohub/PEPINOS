#include "list.h"
#include "lib.h"
#include "io.h"
#include "process.h"
#include "signal.h"

/*
 * Llamada al sistema sys_wait: Suspende al padre hasta que muera un hijo y recolecta su estado.
 * Argumento: status - Puntero donde se almacenará el código de salida del hijo.
 * Retorno: El PID del proceso hijo que ha finalizado.
 */
int sys_wait(int* status)
{
    int pid;                            /* Almacenará el PID del hijo recolectado */
    struct list_head *p, *n;            /* Punteros auxiliares para recorrer la lista de hijos */
    struct process *children;           /* Puntero para inspeccionar cada proceso hijo */

    //// printk("DEBUG: sys_wait(): [%d] wait for children death\n", current->pid);

    /* 
     * Espera activa: Bloquea la ejecución del padre mientras no tenga activa
     * la señal SIGCHLD (notificación de muerte de un hijo).
     */
    while (0 == is_signal(current->signal, SIGCHLD))
        ;

    printk("DEBUG: sys_wait(): [%d] has a dead children\n", current->pid);

    /* Sección crítica: Deshabilita interrupciones para manipular listas de procesos de forma segura */
    cli;

    /* --- Búsqueda del hijo muerto (estado -1 / ZOMBIE) en la lista de hijos --- */
    list_for_each_safe(p, n, &current->child) {
        children = list_entry(p, struct process, sibling);
        if (children->state == -1) {     /* Si el hijo está en estado terminado */
            pid = children->pid;         /* 1. Recupera el PID del hijo */
            *status = children->status;  /* 2. Copia el estado de salida al puntero proporcionado */
            children->state = 0;         /* 3. Libera la entrada del descriptor del hijo */
            list_del(p);                 /* 4. Remueve al hijo de la lista de relaciones familiares */
            clear_signal(&current->signal, SIGCHLD); /* 5. Borra la señal SIGCHLD ya procesada */
            break;                       /* Sale del bucle al haber recolectado al hijo */
        }
    }

    /* Restaura las interrupciones */
    sti;

    return pid;                         /* Retorna el PID del hijo finalizado */
}
