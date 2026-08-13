#include "types.h"
#include "list.h"
#include "io.h"
#include "lib.h"
#include "file.h"
#include "process.h"

/*
 * Llamada al sistema sys_exec: Carga y ejecuta un binario desde el sistema de archivos.
 * Argumentos: 
 *   - path: Ruta del archivo ejecutable (ej. "/bin/shell").
 *   - argv: Arreglo de punteros a cadenas de texto con los argumentos.
 * Retorno: PID del nuevo proceso/tarea cargada o -1 en caso de error.
 */
int sys_exec(char *path, char **argv)
{
    char **ap;                         /* Puntero auxiliar para recorrer el arreglo de argumentos */
    int argc, pid;                     /* argc: Contador de argumentos | pid: ID del proceso creado */
    struct file *fp;                   /* Estrutura de archivo que representa el ejecutable en disco */

    /* 1. Busca la estructura de archivo asociada a la ruta especificada */
    if (!(fp = path_to_file(path))) {
        printk("DEBUG: sys_exec(): %s: command not found\n", path); /* Notifica que no se encontró el archivo */
        return -1;                     /* Devuelve error -1 a espacio de usuario */
    }

    /* 2. Si el Inodo del archivo no ha sido cargado en memoria, lo lee desde el disco EXT2 */
    if (!fp->inode)
        fp->inode = ext2_read_inode(fp->disk, fp->inum);

    /* 3. Cuenta la cantidad de argumentos presentes en el arreglo argv */
    ap = argv;                         /* Inicializa el puntero auxiliar al inicio de argv */
    argc = 0;                          /* Inicializa el contador en 0 */
    while (*ap++)                      /* Recorre el arreglo hasta encontrar un puntero nulo (NULL) */
        argc++;                        /* Incrementa el número de argumentos */

    /* 4. Sección Crítica: Deshabilita interrupciones antes de modificar las estructuras de tareas */
    cli;                               /* Deshabilita interrupciones (Clear Interrupt Flag) */
    
    /* Carga la imagen ELF del programa en memoria y crea/reemplaza la tarea */
    pid = load_task(fp->disk, fp->inode, argc, argv);
    
    sti;                               /* Habilita interrupciones nuevamente (Set Interrupt Flag) */

    /*
     * Bloque de depuración comentado:
     * Permite inspeccionar en consola la relación Padre-Hijo dentro de la lista de procesos.
     *
    {
        struct process *proc;
        struct list_head *p;
        printk("DEBUG: proc[%d]: parent[%d]\n", current->pid, current->parent->pid);
        list_for_each(p, &current->child){
            proc = list_entry(p, struct process, sibling);
            printk("DEBUG: child[%d]\n", proc->pid);
        }
    }
     */

    return pid;                        /* Retorna el PID resultante de la operación */
}
