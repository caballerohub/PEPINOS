#include "types.h"
#include "list.h"

/* --- Configuración de Paginación y Memoria Física --- */
#define PAGESIZE         4096        /* Tamaño de página (4 KB) */
#define RAM_MAXSIZE      0x100000000 /* Límite de memoria RAM direccionable (4 GB) */
#define RAM_MAXPAGE      0x100000    /* Número máximo de páginas de 4KB (1,048,576 páginas) */

/* --- Tablas del Sistema (GDT e IDT) --- */
#define IDTSIZE          0xFF        /* Número máximo de descriptores en la IDT (255) */
#define GDTSIZE          0xFF        /* Número máximo de descriptores en la GDT (255) */

#define IDTBASE          0x00000000  /* Dirección física base donde reside la IDT */
#define GDTBASE          0x00000800  /* Dirección física base donde reside la GDT */

/* --- Mapa de Memoria Virtual del Kernel --- */
#define KERN_PDIR        0x00001000  /* Dirección del Page Directory del Kernel (pd0) */
#define KERN_STACK       0x0009FFF0  /* Dirección de inicio de la pila del Kernel */
#define KERN_BASE        0x00100000  /* Dirección base de carga del código del Kernel (1 MB) */
#define KERN_PG_HEAP     0x00800000  /* Inicio de memoria virtual asignada para páginas del Kernel (8 MB) */
#define KERN_PG_HEAP_LIM 0x10000000  /* Límite de memoria virtual asignada para páginas del Kernel (256 MB) */
#define KERN_HEAP        0x10000000  /* Dirección de inicio del Heap del Kernel para kmalloc (256 MB) */
#define KERN_HEAP_LIM    0x40000000  /* Límite superior del Heap del Kernel para kmalloc (1 GB) */

/* --- Mapa de Memoria Virtual de Procesos de Usuario --- */
#define USER_OFFSET      0x40000000  /* Offset base para procesos de usuario (1 GB) */
#define USER_STACK       0xE0000000  /* Dirección de la pila de usuario (3.5 GB) */

/* --- Macros para Descomposición de Direcciones Virtuales x86 --- */
#define VADDR_PD_OFFSET(addr)  (((addr) & 0xFFC00000) >> 22) /* Obtiene el índice en el Page Directory (Bits 31-22) */
#define VADDR_PT_OFFSET(addr)  (((addr) & 0x003FF000) >> 12) /* Obtiene el índice en la Page Table (Bits 21-12) */
#define VADDR_PG_OFFSET(addr)  ((addr) & 0x00000FFF)         /* Obtiene el desplazamiento dentro de la página (Bits 11-0) */
#define PAGE(addr)             ((addr) >> 12)                /* Convierte dirección en número de página */

/* --- Banderas de Registros de Control de CPU --- */
#define PAGING_FLAG      0x80000000  /* Bit 31 de CR0: Activa el modo de paginación */
#define PSE_FLAG         0x00000010  /* Bit 4 de CR4: Activa el tamaño de página extendido (Page Size Extension, 4MB) */

/* --- Banderas de Control de Atributos de Página/Directorio --- */
#define PG_PRESENT       0x00000001  /* Bit P: Indica que la página/tabla está presente en memoria */
#define PG_WRITE         0x00000002  /* Bit R/W: Lectura/Escritura (1) o Solo Lectura (0) */
#define PG_USER          0x00000004  /* Bit U/S: Acceso de usuario Ring 3 (1) o Kernel Ring 0 (0) */
#define PG_4MB           0x00000080  /* Bit PS: Tamaño de página de 4 Megabytes */


#ifndef __MM_STRUCT__
#define __MM_STRUCT__

/*
 * ESTRUCTURAS DE GESTIÓN DE MEMORIA
 */

/* Representa una asignación de página de memoria individual */
struct page {
	char *v_addr;           /* Dirección virtual asignada a la página */
	char *p_addr;           /* Dirección física correspondiente */
	struct list_head list;  /* Nodo para listas enlazadas (ej. lista de páginas de un proceso) */
};

/* Encapsula la estructura del directorio de páginas de una tarea */
struct page_directory {
	struct page *base;      /* Página física que contiene el Page Directory */
	struct list_head pt;    /* Lista enlazada de las Page Tables asociadas a este directorio */
};

/* Describe un área o bloque contiguo de memoria virtual asignada o libre */
struct vm_area {
	char *vm_start;         /* Dirección virtual de inicio */
	char *vm_end;           /* Dirección virtual de fin (límite excluido) */
	struct list_head list;  /* Nodo para la lista enlazada de regiones VM */
};

#endif


/* Puntero global a la dirección actual del Heap del Kernel */
char *kern_heap;

/* Cabecera de la lista enlazada que gestiona los bloques de memoria virtual libres del Kernel */
struct list_head kern_free_vm;


#ifdef __MM__
	u32 *pd0 = (u32 *) KERN_PDIR;        /* Puntero al directorio de páginas del Kernel */
	char *pg0 = (char *) 0;             /* Mapeo de los primeros 4MB de memoria */
	char *pg1 = (char *) 0x400000;      /* Mapeo de los siguientes 4MB de memoria (4MB - 8MB) */
	char *pg1_end = (char *) 0x800000;  /* Límite del mapeo base del Kernel (8MB) */
	u8 mem_bitmap[RAM_MAXPAGE / 8];     /* Bitmap de asignación de marcos de página física */

	u32 kmalloc_used = 0;               /* Contador de bytes asignados dinámicamente mediante kmalloc */
#else
	u32 *pd0;
	extern u8 mem_bitmap[];

	u32 kmalloc_used;
#endif


/* MACROS PARA CONTROL DEL BITMAP DE PÁGINAS FÍSICAS */

/* Marca un marco de página física como UTILIZADO dentro del bitmap */
#define set_page_frame_used(page)	mem_bitmap[((u32) page)/8] |= (1 << (((u32) page)%8))

/* Marca un marco de página física como LIBRE dentro del bitmap */
#define release_page_frame(p_addr)	mem_bitmap[((u32) p_addr/PAGESIZE)/8] &= ~(1 << (((u32) p_addr/PAGESIZE)%8))


/* PROTOTIPOS DE FUNCIONES DEL GESTOR DE MEMORIA */

/* Busca en el bitmap y asigna un marco de página física libre */
char *get_page_frame(void);

/* Reserva una página física del bitmap y la enlaza con una dirección del heap virtual del Kernel */
struct page *get_page_from_heap(void);

/* Libera una página virtual asignada al heap y devuelve su marco físico al bitmap */
int release_page_from_heap(char *);

/* Inicializa el sistema de memoria, bitmap, tablas iniciales y activa la paginación x86 */
void init_mm(u32);

/* Crea y destruye un Page Directory independiente para un nuevo proceso */
struct page_directory *pd_create(void);
int pd_destroy(struct page_directory *);

/* Agrega una página al espacio de direccionamiento global del Kernel (pd0) */
int pd0_add_page(char *, char *, int);

/* Mapea o elimina páginas dentro del directorio del proceso activo/especificado */
int pd_add_page(char *, char *, int, struct page_directory *);
int pd_remove_page(char *);

/* Devuelve la dirección física asociada a una dirección virtual resolviendo la paginación */
char *get_p_addr(char *);
