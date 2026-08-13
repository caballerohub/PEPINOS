#include "types.h"
#include "lib.h"
#include "mm.h"
#include "kmalloc.h"

/*
 * ksbrk
 * Incrementa el tamaño del heap del kernel asignando 'n' páginas de memoria física adicionales.
 * Asocia las nuevas páginas físicas al espacio virtual del kernel mediante pd0_add_page.
 */
void *ksbrk(int n)
{
	struct kmalloc_header *chunk;
	char *p_addr;
	int i;

	/* Verifica que la asignación no supere el límite máximo del heap virtual del kernel */
	if ((kern_heap + (n * PAGESIZE)) > (char *) KERN_HEAP_LIM) {
		printk
		    ("PANIC: ksbrk(): no virtual memory left for kernel heap !\n");
		return (char *) -1;
	}

	chunk = (struct kmalloc_header *) kern_heap;

	/* Asigna 'n' páginas físicas contiguas en el espacio virtual del kernel */
	for (i = 0; i < n; i++) {
		p_addr = get_page_frame(); /* Obtiene un marco de página física libre */
		if (p_addr < 0) {
			printk
			    ("PANIC: ksbrk(): no free page frame available !\n");
			return (char *) -1;
		}

		/* Asocia la página física al directorio de páginas del kernel (pd0) */
		pd0_add_page(kern_heap, p_addr, 0);

		/* Desplaza el puntero superior del heap */
		kern_heap += PAGESIZE;
	}

	/* Inicializa el encabezado del nuevo bloque de memoria marcado como libre */
	chunk->size = PAGESIZE * n;
	chunk->used = 0;

	return chunk;
}

/*
 * kmalloc
 * Reserva de forma dinámica 'size' bytes de memoria dentro del heap del kernel.
 * Emplea un algoritmo de ajuste First-Fit sobre bloques con encabezados de metadatos.
 */
void *kmalloc(unsigned long size)
{
	unsigned long realsize;	/* Tamaño total requerido (Datos + Encabezado de metadatos) */
	struct kmalloc_header *chunk, *other;

	/* Calcula el tamaño real ajustado al tamaño mínimo permitido para un bloque */
	if ((realsize =
	     sizeof(struct kmalloc_header) + size) < KMALLOC_MINSIZE)
		realsize = KMALLOC_MINSIZE;

	/* 
	 * Busca el primer bloque libre en el heap con espacio suficiente (First-Fit)
	 * iniciando la búsqueda desde la base del heap (KERN_HEAP)
	 */
	chunk = (struct kmalloc_header *) KERN_HEAP;
	while (chunk->used || chunk->size < realsize) {
		/* Detecta corrupción en la lista de bloques del heap */
		if (chunk->size == 0) {
			printk
			    ("PANIC: kmalloc(): corrupted chunk on %x with null size (heap %x) !\nSystem halted\n",
			     chunk, kern_heap);
			asm("hlt");
		}

		/* Avanza al siguiente bloque en la memoria del heap */
		chunk =
		    (struct kmalloc_header *) ((char *) chunk +
					       chunk->size);

		/* Si llega al final del heap actual, solicita más memoria al sistema con ksbrk */
		if (chunk == (struct kmalloc_header *) kern_heap) {
			if (ksbrk((realsize / PAGESIZE) + 1) < 0) {
				printk
				    ("PANIC: kmalloc(): no memory left for kernel !\nSystem halted\n");
				asm("hlt");
			}
		} else if (chunk > (struct kmalloc_header *) kern_heap) {
			printk
			    ("PANIC: kmalloc(): chunk on %x while heap limit is on %x !\nSystem halted\n",
			     chunk, kern_heap);
			asm("hlt");
		}
	}

	/* 
	 * Fragmenta el bloque encontrado si el residuo resultante es lo suficientemente grande.
	 * Evita fragmentación excesiva manteniendo bloques de tamaño mínimo.
	 */
	if (chunk->size - realsize < KMALLOC_MINSIZE)
		chunk->used = 1; /* Asigna todo el bloque sin dividir */
	else {
		/* Divide el bloque: crea un nuevo bloque libre con el espacio sobrante */
		other =
		    (struct kmalloc_header *) ((char *) chunk + realsize);
		other->size = chunk->size - realsize;
		other->used = 0;

		/* Asigna el bloque del tamaño exacto solicitado */
		chunk->size = realsize;
		chunk->used = 1;
	}

	kmalloc_used += realsize; /* Actualiza la métrica global de bytes asignados */

	/* Devuelve el puntero hacia la región útil de datos (saltando el encabezado) */
	return (char *) chunk + sizeof(struct kmalloc_header);
}

/*
 * kfree
 * Libera un bloque de memoria previamente asignado con kmalloc a partir de su dirección.
 * Une automáticamente el bloque recién liberado con bloques adyacentes libres (Coalescing).
 */
void kfree(void *v_addr)
{
	struct kmalloc_header *chunk, *other;

	/* Retrocede el puntero para acceder al encabezado del bloque */
	chunk =
	    (struct kmalloc_header *) (v_addr -
				       sizeof(struct kmalloc_header));
	chunk->used = 0; /* Marca el bloque como libre */

	kmalloc_used -= chunk->size; /* Actualiza el contador de memoria en uso */

	/* 
	 * Fusiona continuamente el bloque liberado con los bloques contiguos contiguos a la derecha 
	 * si estos también se encuentran desocupados.
	 */
	while ((other =
		(struct kmalloc_header *) ((char *) chunk + chunk->size))
	       && other < (struct kmalloc_header *) kern_heap
	       && other->used == 0)
		chunk->size += other->size;
}
