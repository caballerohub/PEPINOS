#include "elf.h"
#include "ext2.h"
#include "mm.h"
#include "lib.h"
#include "kmalloc.h"

/* 
 * Comprueba si el búfer de memoria recibido contiene la firma mágica de un ejecutable ELF.
 * Argumento: file - Puntero al búfer de memoria donde se cargó el archivo.
 * Retorno: 1 si es un formato ELF válido, 0 en caso contrario.
 */
int is_elf(char *file)
{
    Elf32_Ehdr *hdr;

    hdr = (Elf32_Ehdr *) file;         /* Mapea la cabecera ELF base sobre el búfer */

    /* Imprime los primeros 4 bytes leídos para depuración (Magic Number) */
    printk("DEBUG: ELF header bytes: 0x%x '%c' '%c' '%c'\n", 
           (unsigned char)file[0], file[1], file[2], file[3]);

    /* Verifica la firma mágica Estándar ELF: 0x7F 'E' 'L' 'F' */
    if (hdr->e_ident[0] == 0x7f && hdr->e_ident[1] == 'E'
        && hdr->e_ident[2] == 'L' && hdr->e_ident[3] == 'F')
        return 1;
    else
        return 0;
}

/*
 * Carga las secciones de un ejecutable ELF en sus direcciones de memoria virtual correspondientes.
 * Argumentos: 
 *   - file: Puntero al contenido completo del archivo ELF en memoria.
 *   - proc: Puntero a la estructura de proceso (PCB) donde se registrarán los límites de memoria.
 * Retorno: Dirección virtual del punto de entrada del programa (e_entry) o 0 en caso de error.
 */
u32 load_elf(char *file, struct process *proc)
{
	char *p;
	u32 v_begin, v_end;
	Elf32_Ehdr *hdr;
	Elf32_Phdr *p_entry;
	int i, pe;

	hdr = (Elf32_Ehdr *) file;
	/* Apunta a la Tabla de Cabeceras de Programa (Program Header Table) usando su offset */
	p_entry = (Elf32_Phdr *) (file + hdr->e_phoff);	

	/* 1. Valida que el archivo posea la estructura y número mágico ELF */
	if (!is_elf(file)) {
		printk("INFO: load_elf(): file not in ELF format !\n");
		return 0;
	}

	/* 2. Recorre cada entrada de la Tabla de Cabeceras de Programa (Program Headers) */
	for (pe = 0; pe < hdr->e_phnum; pe++, p_entry++) {	

		/* Procesa únicamente los segmentos cargables en memoria (PT_LOAD) */
		if (p_entry->p_type == PT_LOAD) {
			v_begin = p_entry->p_vaddr;                  /* Dirección virtual inicial deseada */
			v_end = p_entry->p_vaddr + p_entry->p_memsz;/* Dirección virtual final deseada */

			/* Comprueba que la dirección no invada el espacio reservado del Kernel */
			if (v_begin < USER_OFFSET) {
				printk ("INFO: load_elf(): can't load executable below %p\n", USER_OFFSET);
				return 0;
			}

			/* Comprueba que la dirección no sobrepase el tope de la Pila de Usuario */
			if (v_end > USER_STACK) {
				printk ("INFO: load_elf(): can't load executable above %p\n", USER_STACK);
				return 0;
			}

			/* Mapea la sección ejecutable y de datos de solo lectura (.text + .rodata) */
			if (p_entry->p_flags == PF_X + PF_R) {	
				proc->b_exec = (char*) v_begin;         /* Inicio de la zona de código ejecutable */
				proc->e_exec = (char*) v_end;           /* Fin de la zona de código ejecutable */
			}

			/* Mapea la sección de datos globales/estáticos no inicializados (.bss) */
			if (p_entry->p_flags == PF_W + PF_R) {	
				proc->b_bss = (char*) v_begin;          /* Inicio de la zona BSS / Datos */
				proc->e_bss = (char*) v_end;            /* Fin de la zona BSS / Datos */
			}

			/* Copia el contenido del segmento desde el archivo en disco hacia la RAM del proceso */
			memcpy((char *) v_begin, (char *) (file + p_entry->p_offset), p_entry->p_filesz);

			/* 
			 * Inicialización del segmento BSS:
			 * Si el tamaño en memoria (p_memsz) es mayor al tamaño en disco (p_filesz),
			 * el espacio sobrante se debe rellenar explícitamente con ceros (0).
			 */
			if (p_entry->p_memsz > p_entry->p_filesz)
				for (i = p_entry->p_filesz, p = (char *) p_entry->p_vaddr; i < p_entry->p_memsz; i++)
					p[i] = 0;
		}
	}

	/* Retorna la dirección virtual del punto de entrada (EIP inicial, por ejemplo 'main' o '_start') */
	return hdr->e_entry;
}
