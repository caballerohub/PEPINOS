#include "types.h"
#include "list.h"
#include "ext2.h"
#include "console.h"

/* Modos de ejecución del procesador */
#define KERNELMODE	0
#define USERMODE	1

/* Límite máximo de procesos simultáneos en el sistema */
#define MAXPID		32

#ifndef __PROCESS_STRUCT__
#define __PROCESS_STRUCT__

/*
 * Estrutura PCB (Process Control Block)
 * Representa un proceso dentro del sistema operativo, guardando su estado,
 * contexto de registros de CPU, memoria virtual, jerarquía de procesos y recursos.
 */
struct process {
	int pid; /* Identificador único del proceso */

	/* Registros de la CPU guardados durante una conmutación de contexto */
	struct {
		u32 eax, ecx, edx, ebx; /* Registros de propósito general */
		u32 esp, ebp, esi, edi; /* Punteros de pila e índices */
		u32 eip, eflags;        /* Puntero de instrucción y registro de estado/banderas */
		u32 cs:16, ss:16, ds:16, es:16, fs:16, gs:16; /* Selectores de segmento de 16 bits */
		u32 cr3;                /* Registro de control CR3 (Dirección física del Page Directory) */
	} regs __attribute__ ((packed));

	/* Pila de Kernel (TSS) utilizada al realizar interrupciones/llamadas al sistema desde Ring 3 */
	struct {
		u32 esp0; /* Puntero a la pila de kernel */
		u16 ss0;  /* Selector de segmento de pila de kernel */
	} kstack __attribute__ ((packed));

	/* 
	 * ¡ATENCIÓN! No modificar la disposición de los miembros superiores a este punto.
	 * La función en ensamblador `do_switch()` depende críticamente de estos offsets de memoria
	 * para guardar y restaurar el contexto de la CPU durante el cambio de contexto.
	 */

	struct page_directory *pd;	/* Puntero a la estructura del directorio de páginas */

	struct list_head pglist;	/* Lista enlazada de páginas físicas asignadas al proceso (código, datos, pila) */

	/* Límites de las secciones de memoria del ejecutable */
	char *b_exec; /* Inicio del segmento de código (.text) */
	char *e_exec; /* Fin del segmento de código */
	char *b_bss;  /* Inicio del segmento de datos no inicializados (.bss) */
	char *e_bss;  /* Fin del segmento .bss */
	char *b_heap; /* Dirección base del Heap de usuario */
	char *e_heap; /* Puntero actual al tope del Heap de usuario */

	struct file *pwd;		/* Directorio de trabajo actual del proceso */
	struct open_file *fd;		/* Lista enlazada de descriptores de archivos abiertos */

	/* Relaciones jerárquicas del proceso */
	struct process *parent;		/* Puntero al proceso padre */
	struct list_head child; 	/* Cabeza de la lista de procesos hijos */
	struct list_head sibling; 	/* Nodo de la lista para enlazar con procesos hermanos */

	struct vconsole *console;	/* Consola virtual asociada al proceso */

	/* Manejo de señales (IPC) */
	u32 signal;        /* Máscara de bits para señales pendientes */
	void* sigfn[32];   /* Tabla de punteros a funciones manejadoras de señales (Signal Handlers) */

	int status;	/* Código de salida devuelto al finalizar (Exit Status) */

	int state;	/* Estado del proceso: -1 = Zombie, 0 = No ejecutable, 1 = Listo/En ejecución, 2 = Suspendido/Sleep */

} __attribute__ ((packed));
#endif


#ifdef __PLIST__

struct process p_list[MAXPID + 1]; /* Tabla global de procesos del sistema */
struct process *current = 0;         /* Puntero al proceso actualmente en ejecución en la CPU */
int n_proc = 0;                    /* Contador de procesos activos */

#else

extern struct process p_list[];
extern struct process *current;
extern int n_proc;

#endif

/* Carga un archivo ejecutable desde el sistema de archivos ext2 y crea un nuevo proceso */
int load_task(struct disk *, struct ext2_inode *, int, char **);
