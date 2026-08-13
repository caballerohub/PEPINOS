#include "io.h"

/*
 * init_pic
 * Reprograma y reasigna los vectores del PIC 8259 (Programmable Interrupt Controller).
 * Por defecto en PC x86, las IRQs 0-7 coinciden con las excepciones de la CPU (vectores 8-15).
 * Esta función remapea el PIC Maestro a los vectores 32-39 y el Esclavo a los vectores 96-103
 * para evitar conflictos con las interrupciones del procesador.
 */
void init_pic(void)
{
	/* 1. Inicialización de ICW1 (Initialization Command Word 1) */
	/* Modos de inicio: Modo en cascada, activación por flanco e indica que vendrá ICW4 */
	outb(0x20, 0x11); /* Envía ICW1 al PIC Maestro (puerto de comandos 0x20) */
	outb(0xA0, 0x11); /* Envía ICW1 al PIC Esclavo (puerto de comandos 0xA0) */

	/* 2. Inicialización de ICW2 (Initialization Command Word 2) */
	/* Define el vector base offset dentro de la IDT para cada PIC */
	outb(0x21, 0x20);	/* PIC Maestro: Vectores IRQ 0-7 mapeados a partir de 32 (0x20 a 0x27) */
	outb(0xA1, 0x70);	/* PIC Esclavo: Vectores IRQ 8-15 mapeados a partir de 96 (0x70 a 0x77) */

	/* 3. Inicialización de ICW3 (Initialization Command Word 3) */
	/* Establece la relación cascada entre PIC Maestro y Esclavo */
	outb(0x21, 0x04); /* PIC Maestro: Indica que el PIC Esclavo está conectado a la línea IRQ 2 (bit 2 = 0000 0100) */
	outb(0xA1, 0x02); /* PIC Esclavo: Le indica su ID de conexión a la línea cascade del Maestro (IRQ 2) */

	/* 4. Inicialización de ICW4 (Initialization Command Word 4) */
	/* Configuración adicional de entorno */
	outb(0x21, 0x01); /* PIC Maestro: Modo 8086/88 habilitado */
	outb(0xA1, 0x01); /* PIC Esclavo: Modo 8086/88 habilitado */

	/* 5. Enmascaramiento de Interrupciones (OCW1 - Operation Control Word 1) */
	/* Habilita o deshabilita líneas IRQ individuales mediante la máscara IMR */
	outb(0x21, 0x0);  /* PIC Maestro: Máscara 0x00 -> Habilita todas las interrupciones (IRQ 0 - IRQ 7) */
	outb(0xA1, 0x0);  /* PIC Esclavo: Máscara 0x00 -> Habilita todas las interrupciones (IRQ 8 - IRQ 15) */
}
