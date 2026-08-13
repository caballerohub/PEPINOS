#include "types.h"
#include "io.h"
#include "lib.h"
#include "mm.h"
#include "kmalloc.h"
#include "ext2.h"
#include "elf.h"
#include "file.h"
#include "list.h"
#include "signal.h"

#define __PLIST__
#include "process.h"

/*
 * load_task
 * Carga un ejecutable ELF en memoria, crea su espacio de direcciones (Page Directory),
 * prepara la pila de usuario con los argumentos de entrada (argc/argv), asigna la pila de kernel,
 * configura el bloque de control del proceso (PCB) y lo registra en la tabla de procesos.
 */
int load_task(struct disk *hd, struct ext2_inode *inode, int argc, char **argv)
{
	struct page *kstack;
	struct process *previous;
	struct vconsole *console;

	char **param, **uparam;
	char *file;
	u32 stackp;
	u32 e_entry; 

	int pid;
	int i;

	/* 
	 * Busca un PID libre recorriendo la tabla p_list[].
	 * Asume que el límite MAXPID no será alcanzado.
	 */
	pid = 1;
	while (p_list[pid].state != 0 && pid++ < MAXPID);
	if (p_list[pid].state != 0) {
		printk("PANIC: not enough slot for processes\n");
		return 0;
	}

	n_proc++; /* Incrementa el contador global de procesos */
	p_list[pid].pid = pid;

	/* Copia los argumentos recibidos para el nuevo programa a memoria del Kernel */
	if (argc) {
		param = (char**) kmalloc(sizeof(char*) * (argc+1));
		for (i=0 ; i<argc ; i++) {
			param[i] = (char*) kmalloc(strlen(argv[i]) + 1);
			strcpy(param[i], argv[i]);
		}
		param[i] = 0;
	}

	/* Crea un nuevo directorio de páginas asignado al proceso */
	p_list[pid].pd = pd_create();

	/* Inicializa la lista enlazada para rastrear los marcos de página usados */
	INIT_LIST_HEAD(&p_list[pid].pglist);

	/* 
	 * Cambia el espacio de direccionamiento al del nuevo proceso.
	 * Se actualiza la variable 'current' para asegurar que los fallos de página (Page Faults)
	 * vinculen adecuadamente las páginas asignadas al proceso correcto.
	 */
	previous = current;
	current = &p_list[pid];
	asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(p_list[pid].pd->base->p_addr));

	/* 
	 * Lee el archivo del disco y carga el ejecutable formato ELF en memoria.
	 * En caso de falla, libera los recursos que fueron reservados previamente.
	 */
	file = ext2_read_file(hd, inode);
	e_entry = (u32) load_elf(file, &p_list[pid]);
	kfree(file);

	if (e_entry == 0) {    /* Falla en la carga del formato ELF */
		if (argc) {
			for (i=0 ; i<argc ; i++) 
				kfree(param[i]);
			kfree(param);
		}
		/* Restaura el contexto y el espacio de memoria del proceso anterior */
		current = previous;
		asm("mov %0, %%eax ;mov %%eax, %%cr3"::"m" (current->regs.cr3));
		pd_destroy(p_list[pid].pd);
		
		/* Resetea el estado del proceso fallido y ajusta contadores */
		p_list[pid].state = 0;
		n_proc--; 
		return 0;
	}

	/* 
	 * Configura el puntero base de la pila de usuario.
	 * Las páginas físicas asociadas a la pila se asignan dinámicamente mediante #PF.
	 */
	stackp = USER_STACK - 16;

	/* Copia las cadenas de texto de los argumentos e invoca la firma de main() en la pila de usuario */
	if (argc) {
		uparam = (char**) kmalloc(sizeof(char*) * argc);

		/* Apila cada cadena de texto del vector de argumentos */
		for (i=0 ; i<argc ; i++) {
			stackp -= (strlen(param[i]) + 1);
			strcpy((char*) stackp, param[i]);
			uparam[i] = (char*) stackp;
		}

		stackp &= 0xFFFFFFF0;		/* Alinea la pila a 16 bytes (ABI System V x86) */

		/* Construye los argumentos requeridos por main(int argc, char **argv) */
		stackp -= sizeof(char*);
		*((char**) stackp) = 0;

		/* Apila la lista de punteros argv[n] hasta argv[0] */
		for (i=argc-1 ; i>=0 ; i--) {
			stackp -= sizeof(char*);
			*((char**) stackp) = uparam[i]; 
		}

		stackp -= sizeof(char*);	/* Puntero base a argv */
		*((char**) stackp) = (char*) (stackp + 4); 

		stackp -= sizeof(char*);	/* Argumento argc */
		*((int*) stackp) = argc; 

		stackp -= sizeof(char*);

		/* Libera la memoria temporal reservada en el Kernel para los parámetros */
		for (i=0 ; i<argc ; i++) 
			kfree(param[i]);

		kfree(param);
		kfree(uparam);
	}

	/* Reserva e inicializa la página dedicada a la pila de kernel del proceso (SS0:ESP0) */
	kstack = get_page_from_heap();

	/* Inicializa y asigna una consola virtual (vconsole) al proceso */
	console = (struct vconsole *) kmalloc(sizeof(struct vconsole));
	console->term = current_term;
	console->inlock = 0;
	console->keypos = 0;
	console->mode = 1;	/* Modo de búfer de entrada habilitado */

	/* Configura los registros de la CPU para ejecutar el proceso en Ring 3 (Usuario) */
	p_list[pid].regs.ss = 0x33;      /* Selector de Segmento de Datos de Usuario */
	p_list[pid].regs.esp = stackp;   /* Puntero a la Pila de Usuario creada */
	p_list[pid].regs.eflags = 0x0;   /* EFLAGS iniciales */
	p_list[pid].regs.cs = 0x23;      /* Selector de Segmento de Código de Usuario */
	p_list[pid].regs.eip = e_entry;  /* Punto de entrada ejecutable (Entry Point ELF) */
	p_list[pid].regs.ds = 0x2B;      /* Selectores de Segmento de Datos extra */
	p_list[pid].regs.es = 0x2B;
	p_list[pid].regs.fs = 0x2B;
	p_list[pid].regs.gs = 0x2B;
	p_list[pid].regs.cr3 = (u32) p_list[pid].pd->base->p_addr; /* Dirección física del Page Directory */

	/* Define la pila del Kernel usada ante interrupciones desde espacio de usuario (TSS) */
	p_list[pid].kstack.ss0 = 0x18;   /* Selector de Segmento de Pila del Kernel */
	p_list[pid].kstack.esp0 = (u32) kstack->v_addr + PAGESIZE - 16; /* Tope de la página de pila */

	/* Limpia los registros de propósito general */
	p_list[pid].regs.eax = 0;
	p_list[pid].regs.ecx = 0;
	p_list[pid].regs.edx = 0;
	p_list[pid].regs.ebx = 0;

	p_list[pid].regs.ebp = 0;
	p_list[pid].regs.esi = 0;
	p_list[pid].regs.edi = 0;

	/* Configura el Heap inicial del proceso justo después de la sección .bss cargada */
	p_list[pid].b_heap = (char*) ((u32) p_list[pid].e_bss & 0xFFFFF000) + PAGESIZE;
	p_list[pid].e_heap = p_list[pid].b_heap;

	p_list[pid].pwd = previous->pwd; /* Hereda el directorio de trabajo (pwd) */

	p_list[pid].fd = 0;	/* Inicializa la tabla de descriptores de archivo (ninguno abierto) */

	/* Enlaza la relación jerárquica padre/hijo con el proceso creador */
	if (previous->state != 0) 
		p_list[pid].parent = previous;
	else 
		p_list[pid].parent = &p_list[0];

	INIT_LIST_HEAD(&p_list[pid].child);

	/* Añade el proceso a la lista de procesos hermanos del padre */
	if (previous->state != 0) 
		list_add(&p_list[pid].sibling, &previous->child);
	else 
		list_add(&p_list[pid].sibling, &p_list[0].child);

	p_list[pid].console = console;

	/* Asigna manejadores de señales por defecto (SIG_DFL) */
	p_list[pid].signal = 0;
	for(i=0 ; i<32 ; i++)
		p_list[pid].sigfn[i] = (char*) SIG_DFL;

	p_list[pid].status = 0;

	p_list[pid].state = 1; /* Establece el estado del proceso a Listo/Ejecutable (Ready) */

	/* Restaura el directorio de páginas y el contexto del proceso ejecutor original */
	current = previous;
	asm("mov %0, %%eax ;mov %%eax, %%cr3":: "m"(current->regs.cr3));

	return pid; /* Retorna el PID asignado al nuevo proceso */
}
