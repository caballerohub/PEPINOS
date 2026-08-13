#include "types.h"
#include "lib.h"
#include "io.h"
#include "process.h"

#define __MM__
#include "mm.h"

#include "kmalloc.h"

/*
 * get_page_frame
 * Recorre el bitmap de memoria física buscando una página libre (bit 0),
 * la marca como utilizada (bit 1) y devuelve su dirección física correspondiente.
 */
char* get_page_frame(void)
{
	int byte, bit;
	int page = -1;

	/* Recorre los bytes del bitmap de la RAM física */
	for (byte = 0; byte < RAM_MAXPAGE / 8; byte++)
		if (mem_bitmap[byte] != 0xFF) /* Si el byte no está completamente ocupado */
			for (bit = 0; bit < 8; bit++)
				if (!(mem_bitmap[byte] & (1 << bit))) { /* Busca el primer bit en 0 */
					page = 8 * byte + bit;          /* Calcula el número de página */
					set_page_frame_used(page);      /* Marca la página como ocupada en el bitmap */
					return (char *) (page * PAGESIZE); /* Devuelve la dirección física */
				}
	return (char *) -1; /* Devuelve error (-1) si no hay páginas físicas libres */
}

/* 
 * get_page_from_heap
 * Asigna una página virtual libre del heap del kernel y la asocia
 * con un marco de página física libre actualizando el Page Directory del kernel (pd0).
 */
struct page* get_page_from_heap(void)
{
	struct page *pg;
	struct vm_area *area;
	char *v_addr, *p_addr;

	/* 1. Solicita un marco de página física libre */
	p_addr = get_page_frame();
	if (p_addr < 0) {
		printk ("PANIC: get_page_from_heap(): no page frame available. System halted !\n");
		asm("hlt");
	}

	/* 2. Verifica si hay direcciones virtuales disponibles en el heap del kernel */
	if (list_empty(&kern_free_vm)) {
		printk ("PANIC: get_page_from_heap(): not memory left in page heap. System halted !\n");
		asm("hlt");
	}

	/* 3. Obtiene el primer rango de direcciones virtuales libres */
	area = list_first_entry(&kern_free_vm, struct vm_area, list);
	v_addr = area->vm_start;

	/* 4. Actualiza el rango virtual libre restando el tamaño de una página */
	area->vm_start += PAGESIZE;
	if (area->vm_start == area->vm_end) {
		list_del(&area->list);
		kfree(area);
	}

	/* 5. Asocia la dirección virtual con la física en el directorio del kernel */
	pd0_add_page(v_addr, p_addr, 0);

	/* 6. Crea y retorna la estructura contenedora 'page' */
	pg = (struct page*) kmalloc(sizeof(struct page));
	pg->v_addr = v_addr;
	pg->p_addr = p_addr;
	pg->list.next = 0;
	pg->list.prev = 0;

	return pg;
}

/*
 * release_page_from_heap
 * Libera una página virtual asignada al kernel, desmapea su dirección física 
 * y reincorpora el espacio libre a la lista enlazada de regiones virtuales (kern_free_vm).
 */
int release_page_from_heap(char *v_addr)
{
	struct vm_area *next_area, *prev_area, *new_area;
	char *p_addr;

	/* 1. Recupera la dirección física asociada a la dirección virtual y la libera */
	p_addr = get_p_addr(v_addr);
	if (p_addr) {
		release_page_frame(p_addr);
	}
	else {
		printk("WARNING: release_page_from_heap(): no page frame associated with v_addr %x\n", v_addr);
		return 1;
	}

	/* 2. Elimina la página del directorio de páginas */
	pd_remove_page(v_addr);

	/* 3. Reorganiza y fusiona los bloques de memoria virtual libre en la lista enlazada */
	list_for_each_entry(next_area, &kern_free_vm, list) {
		if (next_area->vm_start > v_addr)
			break;
	}

	prev_area = list_entry(next_area->list.prev, struct vm_area, list);
	
	if (prev_area->vm_end == v_addr) {
		prev_area->vm_end += PAGESIZE;
		if (prev_area->vm_end == next_area->vm_start) {
			prev_area->vm_end = next_area->vm_end;
			list_del(&next_area->list);
			kfree(next_area);
		}
	}
	else if (next_area->vm_start == v_addr + PAGESIZE) {
		next_area->vm_start = v_addr;
	}
	else if (next_area->vm_start > v_addr + PAGESIZE) {
		new_area = (struct vm_area*) kmalloc(sizeof(struct vm_area));
		new_area->vm_start = v_addr;
		new_area->vm_end = v_addr + PAGESIZE;
		list_add(&new_area->list, &prev_area->list);
	}
	else {
		printk ("PANIC: release_page_from_heap(): corrupted linked list. System halted !\n");
		asm("hlt");
	}

	return 0;
}

/* 
 * init_mm
 * Inicializa el subsistema de gestión de memoria: borra el bitmap,
 * configura las tablas de páginas iniciales del kernel (pd0),
 * activa la paginación en los registros de control (CR0, CR3, CR4) e inicializa el heap.
 */
void init_mm(u32 high_mem)
{
	int pg, pg_limit;
	unsigned long i;
	struct vm_area *p;

	/* Determina la cantidad total de páginas según la memoria RAM reportada */
	pg_limit = (high_mem * 1024) / PAGESIZE;

	/* 1. Inicializa el bitmap marcando libre la RAM existente y ocupada la inexistente */
	for (pg = 0; pg < pg_limit / 8; pg++)
		mem_bitmap[pg] = 0;

	for (pg = pg_limit / 8; pg < RAM_MAXPAGE / 8; pg++)
		mem_bitmap[pg] = 0xFF;

	/* 2. Marca como ocupadas en el bitmap las páginas reservadas por el código del Kernel */
	for (pg = PAGE(0x0); pg < PAGE((u32) pg1_end); pg++) {
		set_page_frame_used(pg);
	}

	/* 3. Mapea de forma directa (Identity Mapping) los primeros 8MB de memoria (páginas de 4MB) */
	pd0[0] = ((u32) pg0 | (PG_PRESENT | PG_WRITE | PG_4MB));
	pd0[1] = ((u32) pg1 | (PG_PRESENT | PG_WRITE | PG_4MB));
	for (i = 2; i < 1023; i++)
		pd0[i] =
		    ((u32) pg1 + PAGESIZE * i) | (PG_PRESENT | PG_WRITE);

	/* 4. Mapeo recursivo del directorio de páginas (Page Table Mirroring Trick) */
	pd0[1023] = ((u32) pd0 | (PG_PRESENT | PG_WRITE));

	/* 5. Activa el modo Paginación cargando CR3 (pd0), habilitando PSE en CR4 y el bit de paginación en CR0 */
	asm("	mov %0, %%eax \n \
		mov %%eax, %%cr3 \n \
		mov %%cr4, %%eax \n \
		or %2, %%eax \n \
		mov %%eax, %%cr4 \n \
		mov %%cr0, %%eax \n \
		or %1, %%eax \n \
		mov %%eax, %%cr0"::"m"(pd0), "i"(PAGING_FLAG), "i"(PSE_FLAG));

	/* 6. Inicializa el heap primario del kernel para 'kmalloc' */
	kern_heap = (char *) KERN_HEAP;
	ksbrk(1);

	/* 7. Inicializa la lista de rango de memoria virtual libre del kernel */
	p = (struct vm_area*) kmalloc(sizeof(struct vm_area));
	p->vm_start = (char*) KERN_PG_HEAP;
	p->vm_end = (char*) KERN_PG_HEAP_LIM;
	INIT_LIST_HEAD(&kern_free_vm);
	list_add(&p->list, &kern_free_vm);

	return;
}

/*
 * pd_create
 * Crea y configura un nuevo Page Directory para un nuevo proceso o tarea de usuario,
 * compartiendo las direcciones del espacio de kernel (primeras 256 entradas).
 */
struct page_directory *pd_create(void)
{
	struct page_directory *pd;
	u32 *pdir;
	int i;

	/* Reserva e inicializa una página física para alojar el nuevo Page Directory */
	pd = (struct page_directory *) kmalloc(sizeof(struct page_directory));
	pd->base = get_page_from_heap();

	/* Copia el espacio del kernel (primeros 1GB: v_addr < USER_OFFSET) de pd0 al nuevo directorio */
	pdir = (u32 *) pd->base->v_addr;
	for (i = 0; i < 256; i++)
		pdir[i] = pd0[i];

	/* Limpia las entradas correspondientes al espacio de usuario */
	for (i = 256; i < 1023; i++)
		pdir[i] = 0;

	/* Aplica el truco de mapeo recursivo al nuevo Page Directory */
	pdir[1023] = ((u32) pd->base->p_addr | (PG_PRESENT | PG_WRITE));

	/* Inicializa la lista de tablas de páginas asociadas al usuario */
	INIT_LIST_HEAD(&pd->pt);

	return pd;
}

/*
 * pd_destroy
 * Libera las tablas de páginas y las páginas de memoria de un Page Directory al destruir un proceso.
 */
int pd_destroy(struct page_directory *pd)
{
	struct page *pg;
	struct list_head *p, *n;

	/* Recorre y libera todas las tablas de páginas mapeadas en el espacio del usuario */
	list_for_each_safe(p, n, &pd->pt) {
		pg = list_entry(p, struct page, list);
		release_page_from_heap(pg->v_addr);
		list_del(p);
		kfree(pg);
	}

	/* Libera la página física contenedora del propio directorio de páginas */
	release_page_from_heap(pd->base->v_addr);
	kfree(pd);

	return 0;
}

/* 
 * pd0_add_page
 * Añade o modifica una entrada de página en el espacio de direccionamiento del kernel (pd0).
 */
int pd0_add_page(char *v_addr, char *p_addr, int flags)
{
	u32 *pde;
	u32 *pte;

	if (v_addr > (char *) USER_OFFSET) {
		printk("ERROR: pd0_add_page(): %p is not in kernel space !\n", v_addr);
		return 0;
	}

	/* Obtiene la entrada en el Page Directory usando direcciones virtuales calculadas mediante paginación recursiva */
	pde = (u32 *) (0xFFFFF000 | (((u32) v_addr & 0xFFC00000) >> 20));
	if ((*pde & PG_PRESENT) == 0) {
		printk("PANIC: pd0_add_page(): kernel page table not found for v_addr %p. System halted !\n", v_addr);
		asm("hlt");
	}

	/* Asigna la dirección física y banderas a la entrada correspondiente en la Page Table */
	pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
	*pte = ((u32) p_addr) | (PG_PRESENT | PG_WRITE | flags);

	return 0;
}

/* 
 * pd_add_page
 * Asocia una página física a una dirección virtual en un Page Directory específico.
 * Crea la tabla de páginas correspondiente si no existía.
 */
int pd_add_page(char *v_addr, char *p_addr, int flags, struct page_directory *pd)
{
	u32 *pde;		/* Dirección virtual de la entrada del Page Directory */
	u32 *pte;		/* Dirección virtual de la entrada de la Page Table */
	u32 *pt;		/* Dirección virtual de la tabla de páginas */
	struct page *pg;
	int i;

	/* Mapea la dirección mediante el esquema de direccionamiento recursivo */
	pde = (u32 *) (0xFFFFF000 | (((u32) v_addr & 0xFFC00000) >> 20));

	/* Si la Page Table no está presente, la crea dinámicamente */
	if ((*pde & PG_PRESENT) == 0) {

		/* Reserva una nueva página para alojar la tabla */
		pg = get_page_from_heap();

		/* Inicializa la tabla recién creada limpiando sus entradas */
		pt = (u32 *) pg->v_addr;
		for (i = 1; i < 1024; i++)
			pt[i] = 0;

		/* Escribe la dirección de la nueva tabla de páginas en el Page Directory */
		*pde = (u32) pg->p_addr | (PG_PRESENT | PG_WRITE | flags);

		/* Asocia la nueva tabla al Page Directory de la tarea */
		if (pd) 
			list_add(&pg->list, &pd->pt);
	}

	/* Asigna la dirección física de destino en la entrada de la Page Table correspondiente */
	pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
	*pte = ((u32) p_addr) | (PG_PRESENT | PG_WRITE | flags);

	return 0;
}

/*
 * pd_remove_page
 * Elimina una entrada de página de las tablas virtuales e invalida la entrada TLB de la CPU.
 */
int pd_remove_page(char *v_addr)
{
	u32 *pte;

	if (get_p_addr(v_addr)) {
		/* Desmarca el bit PG_PRESENT en la entrada de la tabla de páginas */
		pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
		*pte = (*pte & (~PG_PRESENT));
		/* Invalida el caché de la TLB (Translation Lookaside Buffer) para la dirección especificada */
		asm("invlpg %0"::"m"(v_addr));
	}

	return 0;
}

/*
 * get_p_addr
 * Traduce una dirección virtual a su dirección física real navegando por el directorio y tablas de páginas.
 */
char *get_p_addr(char *v_addr)
{
	u32 *pde;		/* Dirección virtual del PDE */
	u32 *pte;		/* Dirección virtual del PTE */

	pde = (u32 *) (0xFFFFF000 | (((u32) v_addr & 0xFFC00000) >> 20));
	if ((*pde & PG_PRESENT)) {
		pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
		if ((*pte & PG_PRESENT))
			/* Calcula la dirección física combinando la base del marco y el desplazamiento relativo */
			return (char *) ((*pte & 0xFFFFF000) + (VADDR_PG_OFFSET((u32) v_addr)));
	}

	return 0;
}
