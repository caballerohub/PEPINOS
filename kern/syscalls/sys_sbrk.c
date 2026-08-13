#include "process.h"

/*
 * Llamada al sistema sys_sbrk: Incrementa o decrementa la memoria del Heap del usuario.
 * Argumento: size - Tamaño en bytes a expandir (si es positivo) o reducir (si es negativo).
 * Retorno: Puntero al límite anterior del Heap (dirección base del nuevo bloque).
 */
char* sys_sbrk(int size)
{
    char *ret;                  /* Variable para almacenar la dirección virtual de retorno */

    ret = current->e_heap;      /* Guarda la dirección actual del final del Heap (dirección base) */

    current->e_heap += size;    /* Desplaza el límite del Heap sumando la cantidad de bytes solicitada */

    return ret;                 /* Retorna la dirección base antes del incremento */
}
