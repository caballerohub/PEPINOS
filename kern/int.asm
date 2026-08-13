; Declaración de funciones C externas invocadas desde los manejadores en ensamblador
extern isr_default_int, isr_GP_exc, isr_PF_exc, isr_clock_int, isr_kbd_int, do_syscalls

; Declaración de etiquetas globales exportadas para ser vinculadas con la IDT
global _asm_default_int, _asm_exc_GP, _asm_exc_PF, _asm_irq_0, _asm_irq_1, _asm_syscalls

; 
; MACRO: SAVE_REGS
; Guarda el contexto completo de la CPU en la pila antes de ejecutar C.
; Cambia los segmentos de datos al selector del kernel (0x10) para ejecutar código C de forma segura.
;
%macro	SAVE_REGS 0
	pushad         ; Guarda registros de propósito general (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
	push ds        ; Guarda segmento de datos
	push es        ; Guarda segmento extra
	push fs        ; Guarda segmento auxiliar FS
	push gs        ; Guarda segmento auxiliar GS
	push ebx       ; Preserva temporalmente EBX en la pila
	mov bx,0x10    ; Carga el selector de datos de Kernel Ring 0 (0x10)
	mov ds,bx      ; Establece DS al segmento de datos del Kernel
	pop ebx        ; Restaura el valor original de EBX
%endmacro

;
; MACRO: RESTORE_REGS
; Restaura el contexto guardado previamente en la pila antes de retornar de la interrupción.
;
%macro	RESTORE_REGS 0
	pop gs         ; Restaura segmento GS del proceso/tarea previa
	pop fs         ; Restaura segmento FS
	pop es         ; Restaura segmento ES
	pop ds         ; Restaura segmento DS
	popad          ; Restaura registros de propósito general
%endmacro

;
; _asm_default_int
; Wrapper en ensamblador para interrupciones por defecto/desconocidas.
;
_asm_default_int:
	SAVE_REGS            ; Guarda registros
	call isr_default_int ; Llama al manejador C por defecto
	mov al,0x20          ; 0x20 = Comando End of Interrupt (EOI)
	out 0x20,al          ; Notifica al PIC Master que la interrupción finalizó
	RESTORE_REGS         ; Restaura registros
	iret                 ; Retorno de interrupción (restaura EFLAGS, CS, EIP)

;
; _asm_exc_GP
; Wrapper para Excepción General Protection Fault (#GP - Vector 13).
;
_asm_exc_GP:
	SAVE_REGS            ; Guarda contexto
	call isr_GP_exc      ; Llama al manejador C de la excepción #GP
	RESTORE_REGS         ; Restaura contexto
	add esp,4            ; Descarta el código de error (Error Code) empujado por la CPU antes de iret
	iret                 ; Retorna de la excepción

;
; _asm_exc_PF
; Wrapper para Excepción Page Fault (#PF - Vector 14).
;
_asm_exc_PF:
	SAVE_REGS            ; Guarda contexto
	call isr_PF_exc      ; Llama al manejador C de fallo de página
	RESTORE_REGS         ; Restaura contexto
	add esp,4            ; Descarta el código de error empujado por la CPU en la excepción #PF
	iret                 ; Retorna de la excepción

;
; _asm_irq_0
; Wrapper para IRQ 0 (Temporizador / Clock Interrupt).
;
_asm_irq_0:
	SAVE_REGS            ; Guarda contexto
	call isr_clock_int   ; Llama a la rutina de reloj y temporizador en C (schedule)
	mov al,0x20          ; Envia señal EOI (End of Interrupt)
	out 0x20,al          ; Escribe EOI al puerto de comandos del PIC Master (0x20)
	RESTORE_REGS         ; Restaura contexto del proceso
	iret                 ; Retorna al proceso en ejecución

;
; _asm_irq_1
; Wrapper para IRQ 1 (Interrupción de Teclado PS/2).
;
_asm_irq_1:
	SAVE_REGS            ; Guarda contexto
	call isr_kbd_int     ; Llama al manejador C de lectura de teclado
	mov al,0x20          ; Envía señal EOI
	out 0x20,al          ; Notifica la recepción de interrupción al PIC
	RESTORE_REGS         ; Restaura contexto
	iret                 ; Retorna de la interrupción

;
; _asm_syscalls
; Wrapper para llamadas al sistema vía interrupción por software (int 0x30).
;
_asm_syscalls:
	SAVE_REGS            ; Guarda el estado del proceso ejecutor
	push eax             ; Pasa el número de syscall (almacenado en EAX) como argumento para do_syscalls
	call do_syscalls     ; Ejecuta el despachador de llamadas al sistema en C
	pop eax              ; Limpia el argumento de la pila
	RESTORE_REGS         ; Restaura los registros del proceso con el resultado de la syscall en EAX
	iret                 ; Retorna al espacio de usuario (Ring 3)
