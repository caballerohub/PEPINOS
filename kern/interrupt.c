#include "types.h"
#include "list.h"
#include "screen.h"
#include "io.h"
#include "kbd.h"
#include "lib.h"
#include "process.h"
#include "schedule.h"
#include "mm.h"
#include "kmalloc.h"
#include "syscalls.h"
#include "console.h"

/*
 * isr_default_int
 * RUTINA DE SERVICIO DE INTERRUPCIÓN (ISR) POR DEFECTO
 * Se ejecuta cuando ocurre una interrupción no mapeada explícitamente a otro manejador.
 */
void isr_default_int(void)
{
	printk("interrupt\n");
}

/*
 * isr_GP_exc
 * MANEJADOR DE EXCEPCIÓN #GP (General Protection Fault - Vector 13)
 * Ocurre cuando se viola una regla de protección del procesador (ej. acceso no autorizado de Ring 3 a Ring 0).
 */
void isr_GP_exc(void)
{
	printk("#GP\n");
	asm("hlt"); /* Detiene la CPU para prevenir corrupción de datos en memoria */
}

/*
 * isr_PF_exc
 * MANEJADOR DE EXCEPCIÓN #PF (Page Fault / Fallo de Página - Vector 14)
 * Ocurre al acceder a una dirección virtual no mapeada o sin permisos en la tabla de páginas.
 * Implementa la asignación dinámica de páginas bajo demanda (Demand Paging).
 */
void isr_PF_exc(void)
{
	u32 faulting_addr, code;
	u32 eip;
	struct page *pg;

	/* Extrae valores de la pila del kernel mediante EBP: EIP (dirección de instrucción), CR2 (dirección que falló) y código de error */
	asm(" 	movl 60(%%ebp), %%eax	\n \
		mov %%eax, %0		\n \
		mov %%cr2, %%eax	\n \
		mov %%eax, %1		\n \
		movl 56(%%ebp), %%eax	\n \
		mov %%eax, %2"
		: "=m"(eip), "=m"(faulting_addr), "=m"(code));

	/* Verifica si el acceso fue dentro del espacio de memoria válido para datos/pila de usuario */
	if (faulting_addr >= USER_OFFSET && faulting_addr < USER_STACK) {
		/* Reserva estructura para gestionar la nueva página */
		pg = (struct page *) kmalloc(sizeof(struct page));
		pg->p_addr = get_page_frame();                     /* Asigna un marco de página física */
		pg->v_addr = (char *) (faulting_addr & 0xFFFFF000); /* Alinea la dirección a límite de página de 4KB */
		list_add(&pg->list, &current->pglist);              /* Registra la página en la lista del proceso actual */
		pd_add_page(pg->v_addr, pg->p_addr, PG_USER, current->pd); /* Mapea la dirección física a la virtual en el Page Directory */
	} 
	else {		
		/* Si el acceso está fuera del espacio asignado al usuario, genera un error de segmentación y termina el proceso */
		printk("Segmentation fault on eip: %p. cr2: %p code: %p\n", eip, faulting_addr, code);
		sys_exit(1);
	}
}

/*
 * isr_clock_int
 * MANEJADOR DE INTERRUPCIÓN DEL RELOJ (Timer Tick - IRQ 0 / Vector 32)
 * Incrementa los contadores del sistema e invoca al planificador de tareas (Scheduler).
 */
void isr_clock_int(void)
{
	static int tic = 0;
	static int sec = 0;
	tic++;
	if (tic % 100 == 0) { /* Cada 100 tics/interrupciones se considera 1 segundo completo */
		sec++;
		tic = 0;
	}
	schedule(); /* Invoca la conmutación de procesos (Multitarea) */
}

/*
 * isr_kbd_int
 * MANEJADOR DE INTERRUPCIÓN DEL TECLADO PS/2 (IRQ 1 / Vector 33)
 * Lee los scan codes del controlador 8042, procesa teclas modificadoras y envía caracteres a la consola.
 */
void isr_kbd_int(void)
{
	uchar i;
	static int lshift_enable;
	static int rshift_enable;
	static int alt_enable;
	static int ctrl_enable;

	/* Espera activa hasta que el búfer de entrada del controlador 8042 (puerto 0x64) esté listo */
	do {
		i = inb(0x64);
	} while ((i & 0x01) == 0);

	/* Lee el Scan Code del puerto de datos 0x60 */
	i = inb(0x60);
	i--;

	/* Evalúa si la tecla fue presionada (Make Code: i < 0x80) o liberada (Break Code: i >= 0x80) */
	if (i < 0x80) {		/* Tecla presionada */
		switch (i) {
		case 0x29:
			lshift_enable = 1; /* Shift Izquierdo presionado */
			break;
		case 0x35:
			rshift_enable = 1; /* Shift Derecho presionado */
			break;
		case 0x1C:
			ctrl_enable = 1;   /* Control presionado */
			break;
		case 0x37:
			alt_enable = 1;    /* Alt presionado */
			break;
		default:
			/* Traduce el scan code a ASCII usando el kbdmap según el estado de la tecla Shift y lo envía a pantalla */
			putc_console(kbdmap[i * 4 + (lshift_enable || rshift_enable)]);
		}
	} else {		/* Tecla liberada */
		i -= 0x80;
		switch (i) {
		case 0x29:
			lshift_enable = 0; /* Shift Izquierdo liberado */
			break;
		case 0x35:
			rshift_enable = 0; /* Shift Derecho liberado */
			break;
		case 0x1C:
			ctrl_enable = 0;   /* Control liberado */
			break;
		case 0x37:
			alt_enable = 0;    /* Alt liberado */
			break;
		}
	}
}
