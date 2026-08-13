#include "syscalls.h"

#define __MALLOC__
#include "malloc.h"

/* Reserva un bloque de memoria dinámica en el heap */
void* malloc(unsigned long size)
{
    unsigned long realsize;        /* Tamaño total incluyendo el encabezado */
    unsigned long i;
    struct malloc_header *bl, *newbl;

    realsize = sizeof(struct malloc_header) + size;
    if ((i = realsize % MALLOC_MINSIZE)) {
        realsize = realsize - i + MALLOC_MINSIZE;
    }

    /* Busca un bloque libre recorriendo el heap desde el inicio */
    if (b_heap == 0) {    /* Inicialización del heap */
        if ((b_heap = sbrk(realsize)) == (char*) -1) 
            return (char*) -1;

        e_heap = b_heap + realsize;

        bl = (struct malloc_header *) b_heap;
        bl->size = realsize;
        bl->used = 0;
    }
    else {
        bl = (struct malloc_header *) b_heap;

        while (bl->used || bl->size < realsize) {
            bl = (struct malloc_header *) ((char *) bl + bl->size);

            if (bl == (struct malloc_header *) e_heap) {
                if ((e_heap = sbrk(realsize)) < 0) {
                    return (char*) -1;
                }
                else {
                    bl = (struct malloc_header *) e_heap;
                    bl->size = realsize;
                    bl->used = 0;
                    e_heap += realsize;
                }
            } else if (bl > (struct malloc_header *) e_heap) {
                return (char *) -1;    /* Error de consistencia en el heap */ 
            }
        }
    }

    /* Asigna el bloque encontrado y realiza fragmentación si es posible */
    if (bl->size - realsize < MALLOC_MINSIZE) {
        bl->used = 1;
    } else {
        newbl = (struct malloc_header *) ((char *) bl + realsize);
        newbl->size = bl->size - realsize;
        newbl->used = 0;

        bl->size = realsize;
        bl->used = 1;
    }
    
    /* Retorna el puntero a la zona de datos */
    return (char *) bl + sizeof(struct malloc_header);
}
    
/* Libera el bloque de memoria previamente asignado */
void free(char *v_addr)
{
    struct malloc_header *bl, *nextbl;

    /* Obtiene la dirección del encabezado del bloque */
    bl = (struct malloc_header *) (v_addr - sizeof(struct malloc_header));

    /* Fusiona el bloque actual con el siguiente si este también está libre */
    while ((nextbl = (struct malloc_header *) ((char *) bl + bl->size))
             && nextbl < (struct malloc_header *) e_heap
             && nextbl->used == 0) 
        bl->size += nextbl->size;

    /* Marca el bloque como libre */
    bl->used = 0;
}
