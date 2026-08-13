#include "types.h"
#include "lib.h"
#include "process.h"
#include "console.h"

/*
 * Llamada al sistema sys_console_read: Lee caracteres ingresados en la consola.
 * Argumento: u_buf - Búfer en espacio de usuario donde se copiará la cadena leída.
 * Retorno: Longitud de la cadena leída (int) o -1 en caso de error.
 */
int sys_console_read(char *u_buf)
{
    /* 1. Verificación de terminal asignado al proceso actual */
    if (!current->console->term) {
        printk("DEBUG: sys_console_read(): process without term\n");
        return -1;                  /* Retorna error -1 si el proceso no tiene terminal activo */
    }

    /* 2. Espera activa si el terminal ya está siendo leído por otro proceso */
    while (current_term->pread);

    /* 3. Bloquea el terminal marcando al proceso actual como el lector activo */
    current->console->term->pread = current;
    current->console->inlock = 1;      /* Asigna el Cerrojo de entrada (Input Lock) */

    /* 4. Espera activa (Bloqueo del proceso) hasta que el manejador de teclado llene el búfer */
    while (current->console->inlock == 1);

    /* 5. Copia la cadena desde el búfer de consola en kernel (inb) al búfer de usuario (u_buf) */
    strcpy(u_buf, current->console->inb);

    /* 6. Devuelve el número de caracteres leídos efectivamente */
    return strlen(u_buf);
}
