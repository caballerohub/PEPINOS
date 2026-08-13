#include "mm.h"

/* 
 * CONSTANTES DE TIPOS DE PUERTA (GATE DESCRIPTORS)
 * INTGATE (0x8E00): Puerta de Interrupción. Deshabilita automáticamente las interrupciones (IF = 0) al entrar. DPL = 0 (Ring 0).
 * TRAPGATE (0xEF00): Puerta de Trampa/Excepción. Mantiene las interrupciones habilitadas (IF no cambia). DPL = 3 (Invocable desde Ring 3).
 */
#define INTGATE  0x8E00		/* Utilizado para gestionar las interrupciones de hardware y excepciones */
#define TRAPGATE 0xEF00		/* Utilizado para llamadas al sistema (syscalls) accesibles desde modo usuario */

/* 
 * ESTRUCTURA: idtdesc
 * Define la estructura de 64 bits (8 bytes) de una entrada en la IDT según la especificación x86.
 */
struct idtdesc {
	u16 offset0_15;   /* Bits 0 al 15 de la dirección de memoria del manejador de la interrupción */
	u16 select;       /* Selector del segmento de código en la GDT (ej. 0x08 para Kernel Code) */
	u16 type;         /* Tipo de puerta, flags de presencia y nivel de privilegio DPL (INTGATE/TRAPGATE) */
	u16 offset16_31;  /* Bits 16 al 31 de la dirección de memoria del manejador */
} __attribute__ ((packed)); /* '__attribute__ ((packed))' evita que el compilador inserte bytes de relleno (padding) */

/* 
 * ESTRUCTURA: idtr
 * Representa la estructura del registro IDTR (Interrupt Descriptor Table Register) 
 * que lee la instrucción de ensamblador 'lidt'.
 */
struct idtr {
	u16 limite;       /* Tamaño total de la IDT en bytes (IDTSIZE * 8) */
	u32 base;         /* Dirección de memoria lineal donde está ubicada la IDT */
} __attribute__ ((packed));

/* Instanciación de variables globales del módulo de interrupciones */
struct idtr kidtr;            /* Variable global para cargar la estructura en el registro IDTR */
struct idtdesc kidt[IDTSIZE]; /* Arreglo global de entradas que componen la IDT del Kernel */

/* Prototipos de las funciones del módulo de gestión de interrupciones */
void init_idt_desc(u16, u32, u16, struct idtdesc *);
void init_idt(void);
void init_pic(void);
