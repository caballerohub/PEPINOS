#include "types.h"
#include "lib.h"
#include "mm.h"

#define __GDT__
#include "gdt.h"

/*
 * init_gdt_desc
 * Configura un descriptor de segmento individual dentro de la GDT.
 * Traduce parámetros de 32 bits al formato nativo estructurado del procesador x86.
 */
void init_gdt_desc(u32 base, u32 limite, u8 acces, u8 other,
		   struct gdtdesc *desc)
{
	/* Extrae los 16 bits inferiores del límite de memoria */
	desc->lim0_15 = (limite & 0xffff);

	/* Extrae los 16 bits inferiores de la dirección base */
	desc->base0_15 = (base & 0xffff);

	/* Extrae los bits 16 a 23 de la dirección base y los desplaza */
	desc->base16_23 = (base & 0xff0000) >> 16;

	/* Asigna el byte de acceso (presente, nivel de privilegio DPL, tipo de segmento) */
	desc->acces = acces;

	/* Extrae los 4 bits superiores del límite (bits 16 a 19) */
	desc->lim16_19 = (limite & 0xf0000) >> 16;

	/* Asigna los flags de granularidad y tamaño (4 bits inferiores) */
	desc->other = (other & 0xf);

	/* Extrae los 8 bits superiores de la dirección base (bits 24 a 31) */
	desc->base24_31 = (base & 0xff000000) >> 24;

	return;
}

/*
 * init_gdt
 * Inicializa los descriptores de la GDT y la TSS por defecto del sistema.
 */
void init_gdt(void)
{
	/* Configuración inicial de la Task State Segment (TSS) por defecto */
	default_tss.debug_flag = 0x00;
	default_tss.io_map = 0x00;
	default_tss.esp0 = 0x1FFF0; /* Puntero de pila del kernel en interrupciones de Ring 3 */
	default_tss.ss0 = 0x18;    /* Selector del segmento de pila del kernel */

	/* kgdt[0]: Descriptor Nulo (Exigido por la arquitectura x86) */
	init_gdt_desc(0x0, 0x0, 0x0, 0x0, &kgdt[0]);

	/* kgdt[1]: Código Kernel - Ring 0 (4GB, Ejecución/Lectura) */
	init_gdt_desc(0x0, 0xFFFFF, 0x9B, 0x0D, &kgdt[1]);

	/* kgdt[2]: Datos Kernel - Ring 0 (4GB, Lectura/Escritura) */
	init_gdt_desc(0x0, 0xFFFFF, 0x93, 0x0D, &kgdt[2]);

	/* kgdt[3]: Pila Kernel - Ring 0 */
	init_gdt_desc(0x0, 0x0, 0x97, 0x0D, &kgdt[3]);

	/* kgdt[4]: Código Usuario - Ring 3 (4GB, Ejecución/Lectura) */
	init_gdt_desc(0x0, 0xFFFFF, 0xFF, 0x0D, &kgdt[4]);

	/* kgdt[5]: Datos Usuario - Ring 3 (4GB, Lectura/Escritura) */
	init_gdt_desc(0x0, 0xFFFFF, 0xF3, 0x0D, &kgdt[5]);

	/* kgdt[6]: Pila Usuario - Ring 3 */
	init_gdt_desc(0x0, 0x0, 0xF7, 0x0D, &kgdt[6]);

	/* kgdt[7]: Descriptor del TSS (Task State Segment para cambio de contexto Ring 3 -> Ring 0) */
	init_gdt_desc((u32) & default_tss, 0x67, 0xE9, 0x00, &kgdt[7]);

	/* Inicialización de la estructura GDTR (Global Descriptor Table Register) */
	kgdtr.limite = GDTSIZE * sizeof(struct gdtdesc) - 1;
	kgdtr.base = GDTBASE;

	/* Copia la GDT temporal alojada en RAM a la dirección virtual/física final GDTBASE */
	memcpy((char *) kgdtr.base, (char *) kgdt, kgdtr.limite + 1);

	/* Carga el registro GDTR con la nueva tabla usando la instrucción 'lgdt' en ensamblador */
	asm("lgdtl (kgdtr)");

	/* Actualización de los registros de segmento con los nuevos selectores */
	asm("movw $0x10, %ax \n \
             movw %ax, %ds \n \
             movw %ax, %es \n \
             movw %ax, %fs \n \
             movw %ax, %gs \n \
             ljmp $0x08, $next \n \
             next:        \n");

	return;
}
