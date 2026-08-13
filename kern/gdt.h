/* 
 * ESTRUCTURA: gdtdesc
 * Define la estructura exacta de 64 bits (8 bytes) de un descriptor de segmento de la GDT 
 * requerida por la arquitectura x86 en Modo Protegido.
 */
struct gdtdesc {
	u16 lim0_15;       /* Bits 0 al 15 del límite del segmento (tamaño) */
	u16 base0_15;      /* Bits 0 al 15 de la dirección base en memoria */
	u8 base16_23;      /* Bits 16 al 23 de la dirección base */
	u8 acces;          /* Byte de acceso: contiene bits de Presente, DPL (Ring 0/3) y Tipo */
	u8 lim16_19:4;     /* Campo de bits: Bits 16 al 19 del límite (completa los 20 bits de límite) */
	u8 other:4;        /* Campo de bits: Flags de Granularidad (G), Tamaño por defecto (D/B), etc. */
	u8 base24_31;      /* Bits 24 al 31 de la dirección base (completa los 32 bits de base) */
} __attribute__ ((packed)); /* '__attribute__ ((packed))' evita que el compilador añada relleno (padding) */

/* 
 * ESTRUCTURA: gdtr
 * Representa el registro interno del procesador GDTR (Global Descriptor Table Register) 
 * que se carga mediante la instrucción de ensamblador 'lgdt'.
 */
struct gdtr {
	u16 limite;        /* Tamaño total de la GDT en bytes menos 1 */
	u32 base;          /* Dirección de memoria lineal donde comienza la GDT */
} __attribute__ ((packed));

/* 
 * ESTRUCTURA: tss
 * Task State Segment (TSS) de x86. Se utiliza para guardar/restaurar el estado del procesador.
 * En sistemas operativos modernos se usa principalmente para definir la pila del Kernel (esp0/ss0) 
 * durante las interrupciones desde espacio de usuario (Ring 3 -> Ring 0).
 */
struct tss {
	u16 previous_task, __previous_task_unused; /* Enlace a la tarea anterior en hardware multitasking */
	u32 esp0;                                  /* Puntero de pila de Ring 0 (Kernel) */
	u16 ss0, __ss0_unused;                     /* Selector del segmento de pila de Ring 0 */
	u32 esp1;                                  /* Puntero de pila de Ring 1 (sin uso habitual) */
	u16 ss1, __ss1_unused;                     /* Selector de pila de Ring 1 */
	u32 esp2;                                  /* Puntero de pila de Ring 2 (sin uso habitual) */
	u16 ss2, __ss2_unused;                     /* Selector de pila de Ring 2 */
	u32 cr3;                                   /* Registro de control 3 (Dirección del Page Directory) */
	u32 eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; /* Registros de propósito general y estado */
	u16 es, __es_unused;                       /* Registros de segmento de datos */
	u16 cs, __cs_unused;                       /* Registro de segmento de código */
	u16 ss, __ss_unused;                       /* Registro de segmento de pila */
	u16 ds, __ds_unused;                       /* Registro de segmento de datos */
	u16 fs, __fs_unused;                       /* Registro de segmento de datos adicional */
	u16 gs, __gs_unused;                       /* Registro de segmento de datos adicional */
	u16 ldt_selector, __ldt_sel_unused;        /* Selector de la Local Descriptor Table (si aplica) */
	u16 debug_flag, io_map;                    /* Flag de depuración y mapa de permisos I/O */
} __attribute__ ((packed));

/* 
 * MANEJO DE VARIABLES GLOBALES / EXTERNAS
 * Si el archivo que incluye esta cabecera define '__GDT__', reserva el espacio de memoria real 
 * para la tabla, el registro y el TSS. En caso contrario, los declara como externos ('extern').
 */
#ifdef __GDT__
	struct gdtdesc kgdt[GDTSIZE];	/* Arreglo que contiene los descriptores de la GDT del kernel */
	struct gdtr kgdtr;		/* Variable que almacena el puntero/límite de la GDT */
	struct tss default_tss;        /* Estructura TSS por defecto del sistema */
#else
	extern struct gdtdesc kgdt[];
	extern struct gdtr kgdtr;
	extern struct tss default_tss;
#endif

/* Prototipos de las funciones públicas del módulo GDT */
void init_gdt_desc(u32, u32, u8, u8, struct gdtdesc *);
void init_gdt(void);
