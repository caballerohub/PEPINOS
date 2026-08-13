#include "elf.h"
#include "ext2.h"
#include "mm.h"
#include "lib.h"
#include "kmalloc.h"

/* Verifica si el archivo tiene un formato ELF válido */
int is_elf(char *file)
{
    Elf32_Ehdr *hdr;

    hdr = (Elf32_Ehdr *) file;         /* Mapea la cabecera ELF */

    /* Imprime los primeros 4 bytes para depuración */
    printk("DEBUG: ELF header bytes: 0x%x '%c' '%c' '%c'\n", 
           (unsigned char)file[0], file[1], file[2], file[3]);

    /* Verifica el número mágico ELF (0x7F 'E' 'L' 'F') */
    if (hdr->e_ident[0] == 0x7f && hdr->e_ident[1] == 'E'
        && hdr->e_ident[2] == 'L' && hdr->e_ident[3] == 'F')
        return 1;
    else
        return 0;
}

/* Carga los segmentos de un ELF en memoria virtual y retorna el punto de entrada */
u32 load_elf(char *file, struct process *proc)
{
    char *p;
    u32 v_begin, v_end;
    Elf32_Ehdr *hdr;
    Elf32_Phdr *p_entry;
    int i, pe;

    hdr = (Elf32_Ehdr *) file;
    /* Apunta a la tabla de cabeceras de programa */
    p_entry = (Elf32_Phdr *) (file + hdr->e_phoff);    

    /* Valida el formato ELF */
    if (!is_elf(file)) {
        printk("INFO: load_elf(): file not in ELF format !\n");
        return 0;
    }

    /* Recorre las cabeceras de programa */
    for (pe = 0; pe < hdr->e_phnum; pe++, p_entry++) {    

        /* Procesa solo segmentos cargables (PT_LOAD) */
        if (p_entry->p_type == PT_LOAD) {
            v_begin = p_entry->p_vaddr;                  /* Dirección virtual inicial */
            v_end = p_entry->p_vaddr + p_entry->p_memsz;/* Dirección virtual final */

            /* Verifica que no invada el espacio del kernel */
            if (v_begin < USER_OFFSET) {
                printk ("INFO: load_elf(): can't load executable below %p\n", USER_OFFSET);
                return 0;
            }

            /* Verifica que no supere el tope de la pila de usuario */
            if (v_end > USER_STACK) {
                printk ("INFO: load_elf(): can't load executable above %p\n", USER_STACK);
                return 0;
            }

            /* Registra límites de código ejecutable (.text) */
            if (p_entry->p_flags == PF_X + PF_R) {    
                proc->b_exec = (char*) v_begin;         /* Inicio de código */
                proc->e_exec = (char*) v_end;           /* Fin de código */
            }

            /* Registra límites de datos y BSS (.data / .bss) */
            if (p_entry->p_flags == PF_W + PF_R) {    
                proc->b_bss = (char*) v_begin;          /* Inicio de datos/BSS */
                proc->e_bss = (char*) v_end;            /* Fin de datos/BSS */
            }

            /* Copia el segmento del archivo a la RAM */
            memcpy((char *) v_begin, (char *) (file + p_entry->p_offset), p_entry->p_filesz);

            /* Rellena con ceros el espacio sobrante (sección .bss) */
            if (p_entry->p_memsz > p_entry->p_filesz)
                for (i = p_entry->p_filesz, p = (char *) p_entry->p_vaddr; i < p_entry->p_memsz; i++)
                    p[i] = 0;
        }
    }

    /* Retorna la dirección virtual del punto de entrada */
    return hdr->e_entry;
}
