#include "types.h"
#include "gdt.h"
#include "process.h"
#include "lib.h"
#include "mm.h"
#include "signal.h"

/* 
 * switch_to_task
 * Prepara la conmutación de procesos antes de invocar la rutina do_switch() en ensamblador.
 * Carga el TSS del proceso objetivo, configura la pila del kernel adecuada y empuja los registros
 * necesarios (SS, ESP, EFLAGS, CS, EIP, y 'current') para efectuar el retorno por 'iret'.
 */
void switch_to_task(int n, int mode)
{
	u32 kesp, eflags;
	u16 kss, ss, cs;
	int sig;

	/* 1. Actualiza el puntero del proceso actual */
	current = &p_list[n];

	/* 2. Procesa y despacha las señales pendientes del proceso */
	if ((sig = dequeue_signal(current->signal))) 
		handle_signal(sig);

	/* 3. Actualiza el TSS por defecto con la pila de kernel (Ring 0) del nuevo proceso */
	default_tss.ss0 = current->kstack.ss0;
	default_tss.esp0 = current->kstack.esp0;

	/* 4. Prepara valores de registros y habilita interrupciones en eflags (bit IF - 0x200) */
	ss = current->regs.ss;
	cs = current->regs.cs;
	eflags = (current->regs.eflags | 0x200) & 0xFFFFBFFF;

	/* 5. Determina los valores de SS y ESP según el modo de origen del proceso objetivo */
	if (mode == USERMODE) {
		kss = current->kstack.ss0;
		kesp = current->kstack.esp0;
	} else {			/* KERNELMODE */
		kss = current->regs.ss;
		kesp = current->regs.esp;
	}

	/* 
	 * 6. Empuja el marco de retorno iret a la pila e invoca do_switch().
	 * Si la tarea viene de modo usuario, se empujan SS y ESP adicionales en la pila.
	 */
	asm("	mov %0, %%ss; \
		mov %1, %%esp; \
		cmp %[KMODE], %[mode]; \
		je next; \
		push %2; \
		push %3; \
		next: \
		push %4; \
		push %5; \
		push %6; \
		push %7; \
		ljmp $0x08, $do_switch" 
		:: \
		"m"(kss), \
		"m"(kesp), \
		"m"(ss), \
		"m"(current->regs.esp), \
		"m"(eflags), \
		"m"(cs), \
		"m"(current->regs.eip), \
		"m"(current), \
		[KMODE] "i"(KERNELMODE), \
		[mode] "g"(mode)
	    );
}

/*
 * schedule
 * Planificador del sistema operativo (Scheduler).
 * Guarda el contexto completo del proceso que está cediendo la CPU,
 * selecciona el siguiente proceso listo (Ready) usando un algoritmo Round-Robin,
 * y efectúa la conmutación de contexto.
 */
void schedule(void)
{
	struct process *p;
	u32 *stack_ptr;
	int i, newpid;

	/* Extrae el puntero al marco de registros guardado en la pila durante la interrupción */
	asm("mov (%%ebp), %%eax; mov %%eax, %0": "=m"(stack_ptr):);

	/* Caso 1: Si no hay procesos registrados, retorna de inmediato */
	if (!n_proc) {
		return;
	}

	/* Caso 2: Si solo existe un proceso activo distinto al proceso IDLE (PID 0), continúa su ejecución */
	else if (n_proc == 1 && current->pid != 0) {
		return;
	}

	/* Caso 3: Guarda el contexto del proceso saliente para alternar tareas */
	else {
		/* Guarda los registros de la CPU en la estructura PCB del proceso actual */
		current->regs.eflags = stack_ptr[16];
		current->regs.cs = stack_ptr[15];
		current->regs.eip = stack_ptr[14];
		current->regs.eax = stack_ptr[13];
		current->regs.ecx = stack_ptr[12];
		current->regs.edx = stack_ptr[11];
		current->regs.ebx = stack_ptr[10];
		current->regs.ebp = stack_ptr[8];
		current->regs.esi = stack_ptr[7];
		current->regs.edi = stack_ptr[6];
		current->regs.ds = stack_ptr[5];
		current->regs.es = stack_ptr[4];
		current->regs.fs = stack_ptr[3];
		current->regs.gs = stack_ptr[2];

		/* 
		 * Guarda el puntero de pila (SS:ESP).
		 * Si la interrupción ocurrió en modo usuario (CS != 0x08), la CPU apiló SS/ESP automáticamente.
		 * Si ocurrió durante una syscall en Ring 0, se ajusta la dirección de la pila del kernel.
		 */
		if (current->regs.cs != 0x08) {	/* Modo usuario */
			current->regs.esp = stack_ptr[17];
			current->regs.ss = stack_ptr[18];
		} else {	/* Modo kernel (durante System Call) */
			current->regs.esp = stack_ptr[9] + 12;	/* Apunta a la dirección previa a la llamada */
			current->regs.ss = default_tss.ss0;
		}

		/* Guarda los punteros de la pila de kernel del proceso en el TSS */
		current->kstack.ss0 = default_tss.ss0;
		current->kstack.esp0 = default_tss.esp0;
	}

	/* Algoritmo Round-Robin: busca el siguiente PID ejecutable (state == 1) */
	newpid = 0;
	/* Busca desde el PID actual + 1 hacia el final del vector de procesos */
	for (i = current->pid + 1; i < MAXPID && newpid == 0; i++) {
		if (p_list[i].state == 1)
			newpid = i;
	}

	/* Si no encontró ninguno superior, reinicia la búsqueda desde PID 1 hasta el proceso actual */
	if (!newpid) {
		for (i = 1; i < current->pid && newpid == 0; i++) {
			if (p_list[i].state == 1)
				newpid = i;
		}
	}

	p = &p_list[newpid];

	/* Ejecuta el cambio de proceso evaluando el segmento de código (CS) de destino */
	if (p->regs.cs != 0x08)
		switch_to_task(p->pid, USERMODE);
	else
		switch_to_task(p->pid, KERNELMODE);
}
