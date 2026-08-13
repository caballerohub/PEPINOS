; Símbolos globales para el linker
global _start, start

; Función externa definida en C
extern kmain

; Constantes Multiboot
%define MULTIBOOT_HEADER_MAGIC  0x1BADB002 ; Número mágico para GRUB
%define MULTIBOOT_HEADER_FLAGS  0x00000003 ; Flags de configuración
%define CHECKSUM -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) ; Checksum (debe dar 0)

;-- Punto de entrada --
_start:
    jmp start ; Salta a la inicialización


;-- Cabecera Multiboot (alineada a 4 bytes) --
align 4

multiboot_header:
    dd MULTIBOOT_HEADER_MAGIC ; Número mágico
    dd MULTIBOOT_HEADER_FLAGS ; Flags
    dd CHECKSUM               ; Checksum
;-- Fin cabecera --

;-- Inicialización --
start:
    push ebx   ; Pasa la info de multiboot como argumento
    call kmain ; Llama a la función principal kmain

    cli        ; Desactiva interrupciones
    hlt        ; Detiene la CPU
