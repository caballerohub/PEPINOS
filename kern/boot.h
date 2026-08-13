#include "types.h"

/* 
 * ESTRUCTURA: multiboot_info
 * Contiene la información proporcionada por el cargador de arranque (Bootloader/GRUB)
 * al kernel según la especificación Multiboot v1.
 */
struct multiboot_info {
	u32 flags;        /* Muestra qué campos de la estructura son válidos (máscara de bits) */
	u32 low_mem;      /* Cantidad de memoria RAM baja en Kilobytes (debajo de 1MB) */
	u32 high_mem;     /* Cantidad de memoria RAM alta en Kilobytes (por encima de 1MB) */
	u32 boot_device;  /* Dispositivo de arranque utilizado (disco, partición, etc.) */
	u32 cmdline;      /* Dirección física de la cadena de parámetros de la línea de comandos */
	u32 mods_count;   /* Número de módulos cargados junto con el kernel */
	u32 mods_addr;    /* Dirección física donde inicia la lista de estructuras de módulos */
	
	/* 
	 * Estructura anidada para secciones ELF:
	 * Contiene la tabla de cabeceras de sección del archivo ELF del kernel
	 * cargado en memoria, útil para depuración y símbolos.
	 */
	struct {
		u32 num;   /* Número de entradas de cabecera de sección ELF */
		u32 size;  /* Tamaño de cada entrada de cabecera de sección ELF */
		u32 addr;  /* Dirección física de la tabla de cabeceras de sección */
		u32 shndx; /* Índice de la sección que contiene la tabla de nombres de cadenas */
	} elf_sec;
};
