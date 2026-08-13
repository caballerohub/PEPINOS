#include "types.h"
#include "list.h"
#include "lib.h"
#include "io.h"
#include "process.h"
#include "kmalloc.h"
#include "mm.h"
#include "schedule.h"
#include "file.h"
#include "console.h"
#include "signal.h"
#include "syscalls.h"

/*
 * Llamada al sistema sys_open: Abre un archivo especificando su ruta.
 * Argumento: path - Cadena de texto con la ruta del archivo (ej. "/bin/shell").
 * Retorno: Número del descriptor de archivo asignado (fd >= 0) o -1 en caso de error.
 */
int sys_open(char *path)
{
    u32 fd;                             /* Almacenará el número identificador del descriptor asignado */
    struct file *fp;                    /* Puntero a la estructura de archivo en el VFS */
    struct open_file *of;               /* Puntero auxiliar para recorrer los descriptores del proceso */

    /* 1. Traduce la ruta textual a un nodo/estructura de archivo en el sistema de archivos */
    if (!(fp = path_to_file(path))) {
        //// printk("DEBUG: sys_open(): can't open %s\n", path);
        return -1;                      /* Si no existe la ruta, retorna error (-1) */
    }

    //// printk("DEBUG: sys_open(): process[%d] opening file %s\n", current->pid, fp->name);
    
    /* 2. Incrementa el contador global de procesos que mantienen abierto este archivo */
    fp->opened++;

    /* 3. Si el inodo del archivo no está cargado en memoria RAM, lo lee desde el disco EXT2 */
    if (!fp->inode)
        fp->inode = ext2_read_inode(fp->disk, fp->inum);

    /* 4. Carga el contenido del archivo desde el disco hacia el búfer de mapeo en memoria (mmap) */
    fp->mmap = ext2_read_file(fp->disk, fp->inode);

    /* 
     * --- 5. Búsqueda y Asignación de un Descriptor Libres (FD) --- 
     */
    fd = 0;                             /* Inicializa el índice del descriptor en 0 */

    /* CASO A: Es el primer archivo que abre el proceso actual */
    if (current->fd == 0) {
        current->fd = (struct open_file *) kmalloc(sizeof(struct open_file)); /* Asigna nodo inicial */
        current->fd->file = fp;         /* Vincula con el archivo abierto */
        current->fd->ptr = 0;           /* Inicializa el puntero de lectura/escritura (offset = 0) */
        current->fd->next = 0;          /* Sin siguiente descriptor por ahora */
    } 
    /* CASO B: El proceso ya posee al menos un descriptor en uso */
    else {
        of = current->fd;               /* Comienza en la cabeza de la lista de descriptores */
        while (of->file && of->next) {  /* Avanza hasta el final de la lista o un nodo reutilizable */
            of = of->next;              /* Pasa al siguiente nodo */
            fd++;                       /* Incrementa el índice numérico del descriptor */
        }

        /* Reutilización de un descriptor previamente cerrado/liberado */
        if (of->file == 0) {    
            of->file = fp;              /* Reasigna el puntero de archivo */
            of->ptr = 0;                /* Reinicia el offset de lectura */
        } 
        /* Creación de un nuevo nodo descriptor al final de la lista */
        else {        
            of->next = (struct open_file *) kmalloc(sizeof(struct open_file)); /* Crea el nuevo nodo */
            of->next->file = fp;        /* Asigna la estructura de archivo */
            of->next->ptr = 0;          /* Inicializa el offset a 0 */
            of->next->next = 0;         /* Fin de la lista */
            fd++;                       /* Incrementa el contador para devolver el nuevo FD */
        }
    }

    return fd;                          /* Retorna el número del descriptor de archivo asignado */
}
