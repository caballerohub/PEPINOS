; ==============================================================================
; Rutina : do_switch (o switch_to_task)
; Descripción: Realiza la conmutación de contexto entre la tarea actual y la
;              siguiente, actualizando registros, espacio de memoria (CR3)
;              y rearmando el controlador de interrupciones (PIC).
; ==============================================================================

global do_switch                    ; Exporta el símbolo para ser llamado desde C (schedule.c)

do_switch:
    ; --- 1. Recuperación del puntero al contexto ---
    mov esi, [esp]                  ; Lee la dirección del PCB de la tarea destino (*current)
    pop eax                         ; Limpia el argumento/dirección de retorno de la pila

    ; --- 2. Apilado del nuevo contexto (Preparación para iret) ---
    push dword [esi+4]              ; EAX objetivo
    push dword [esi+8]              ; ECX objetivo
    push dword [esi+12]             ; EDX objetivo
    push dword [esi+16]             ; EBX objetivo
    push dword [esi+24]             ; EBP objetivo (Frame pointer)
    push dword [esi+28]             ; ESI objetivo
    push dword [esi+32]             ; EDI objetivo
    push dword [esi+48]             ; DS  (Selector de segmento de datos kernel/user)
    push dword [esi+50]             ; ES
    push dword [esi+52]             ; FS
    push dword [esi+54]             ; GS

    ; --- 3. Notificación al Controlador de Interrupciones (PIC) ---
    mov al, 0x20                    ; Comando EOI (End of Interrupt)
    out 0x20, al                    ; Envía EOI al puerto 0x20 (PIC Maestro) para reactivar IRQs

    ; --- 4. Cambio de Espacio de Memoria Virtual (Paginación) ---
    mov eax, [esi+56]               ; Lee la dirección del Page Directory de la nueva tarea
    mov cr3, eax                    ; Carga CR3: inhabilita/actualiza la TLB y cambia la memoria virtual

    ; --- 5. Restauración de Registres de la Nueva Tarea ---
    pop gs                          ; Restaura el segmento GS
    pop fs                          ; Restaura el segmento FS
    pop es                          ; Restaura el segmento ES
    pop ds                          ; Restaura el segmento DS
    pop edi                         ; Restaura EDI
    pop esi                         ; Restaura ESI
    pop ebp                         ; Restaura EBP
    pop ebx                         ; Restaura EBX
    pop edx                         ; Restaura EDX
    pop ecx                         ; Restaura ECX
    pop eax                         ; Restaura EAX

    ; --- 6. Salto a Userland / Nueva Tarea ---
    iret                            ; Restaure EIP, CS, EFLAGS (y ESP/SS si pasa de Ring 0 a Ring 3)
