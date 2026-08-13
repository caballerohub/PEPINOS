#include "types.h"
#include "lib.h"
#include "io.h"
#include "idt.h"
#include "mm.h"

/* Declaraciones externas de las rutinas de manejo de interrupciones definidas en ensamblador */
void _asm_default_int(void); /* Manejador genérico/por defecto para interrupciones no especificadas */
void _asm_exc_GP(void);      /* Manejador de Excepción 13: General Protection Fault (#GP) */
void _asm_exc_PF(void);      /* Manejador de Excepción 14: Page Fault (#PF) */
void _asm_irq_0(void);       /* Manejador IRQ 0: Interruptor de reloj del sistema (Timer Tick) */
void _asm_irq_1(void);       /* Manejador IRQ 1: Teclado PS/2 */
void _asm_syscalls(void);    /* Manejador para llamadas al sistema (Syscalls) vía software */

/*
 * init_idt_desc
 * Configura una entrada individual (descriptor de puerta) dentro de la IDT.
 * Extrae y empaqueta la dirección del manejador (offset), el selector de segmento y el tipo de puerta.
 */
void init_idt_desc(u16 select, u32 offset, u16 type, struct idtdesc *desc)
{
	/* Extrae los 16 bits inferiores de la dirección del manejador de interrupción */
	desc->offset0_15 = (offset & 0xffff);

	/* Asigna el selector de segmento de código (ej. 0x08 para código del Kernel) */
	desc->select = select;

	/* Configura los atributos y tipo de puerta (INTGATE, TRAPGATE, TASKGATE y nivel DPL) */
	desc->type = type;

	/* Extrae los 16 bits superiores de la dirección del manejador de interrupción */
	desc->offset16_31 = (offset & 0xffff0000) >> 16;

	return;
}

/*
 * init_idt
 * Inicializa la Tabla de Descriptores de Interrupción (IDT) con sus manejadores
 * por defecto, excepciones del procesador, interrupciones de hardware e int 0x30.
 */
void init_idt(void)
{
	int i;

	/* 1. Inicialización de todas las entradas de la IDT con un manejador por defecto */
	for (i = 0; i < IDTSIZE; i++) 
		init_idt_desc(0x08, (u32) _asm_default_int, INTGATE, &kidt[i]);

	/* 2. Configuración de excepciones críticas del procesador x86 (Vectores 0 a 31) */
	init_idt_desc(0x08, (u32) _asm_exc_GP, INTGATE, &kidt[13]);	/* Vector 13: General Protection Fault (#GP) */
	init_idt_desc(0x08, (u32) _asm_exc_PF, INTGATE, &kidt[14]);     /* Vector 14: Page Fault / Fallo de Página (#PF) */

	/* 3. Configuración de Interrupciones de Hardware (IRQs mapeadas a los vectores 32-47) */
	init_idt_desc(0x08, (u32) _asm_irq_0, INTGATE, &kidt[32]);	/* Vector 32 (IRQ 0): Reloj / Temporizador (PIT 8254) */
	init_idt_desc(0x08, (u32) _asm_irq_1, INTGATE, &kidt[33]);	/* Vector 33 (IRQ 1): Teclado */

	/* 4. Configuración de Interrupción por Software para llamadas al sistema (int 0x30 = Vector 48) */
	init_idt_desc(0x08, (u32) _asm_syscalls, TRAPGATE, &kidt[48]);	/* TRAPGATE permite invocación desde Ring 3 sin deshabilitar interrupciones */

	/* 5. Configuración del registro IDTR (Interrupt Descriptor Table Register) */
	kidtr.limite = IDTSIZE * 8; /* Tamaño total de la IDT en bytes (número de entradas * 8 bytes por descriptor) */
	kidtr.base = IDTBASE;       /* Dirección base en memoria donde residirá la IDT definitiva */

	/* Copia la tabla inicializada desde el buffer en RAM 'kidt' a la ubicación 'IDTBASE' */
	memcpy((char *) kidtr.base, (char *) kidt, kidtr.limite);

	/* Carga el registro IDTR del procesador ejecutando la instrucción 'lidt' en ensamblador */
	asm("lidtl (kidtr)");
}
