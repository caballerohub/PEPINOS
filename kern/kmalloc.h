#include "types.h"

/* Tamaño mínimo que debe tener un bloque de memoria administrado por kmalloc (en bytes) */
#define KMALLOC_MINSIZE		16

/*
 * Estructura de cabecera (Header) para cada bloque del Heap administrado por kmalloc.
 * Utiliza campos de bits (Bit-fields) para optimizar el espacio en memoria (32 bits = 4 bytes en total).
 */
struct kmalloc_header {
	unsigned long size:31;	/* Tamaño total del bloque de memoria (incluye esta cabecera) en bits 0-30 */
	unsigned long used:1;	/* Estado del bloque en el bit 31: 1 = Ocupado / Asignado, 0 = Libre */
} __attribute__ ((packed)); /* Evita el relleno (padding) de la estructura por el compilador */

/* Prototipos de funciones públicas del asignador de memoria dinámica del kernel */

/* Expande el Heap del kernel en 'n' páginas físicas de memoria */
void *ksbrk(int);

/* Reserva un bloque dinámico de memoria de 'unsigned long' bytes */
void *kmalloc(unsigned long);

/* Libera un bloque de memoria reservado previamente */
void kfree(void *);
