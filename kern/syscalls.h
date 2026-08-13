#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "types.h"

/* --- Definición de Números de Syscalls (Índices para sys_call_table) --- */
#define SYS_EXIT          1     /* Número de identificador para terminar el proceso */
#define SYS_FORK          2     /* Número de identificador para duplicar el proceso actual */
#define SYS_READ          3     /* Número de identificador para lectura de datos */
#define SYS_WRITE         4     /* Número de identificador para escritura de datos */
#define SYS_OPEN          5     /* Número de identificador para abrir archivos */
#define SYS_CLOSE         6     /* Número de identificador para cerrar descriptores de archivo */
#define SYS_WAIT          7     /* Número de identificador para esperar finalización de un hijo */
#define SYS_EXECVE       11     /* Número de identificador para ejecutar un programa ELF */
#define SYS_SBRK         45     /* Número de identificador para modificar el tamaño del Heap */
#define SYS_SIGRETURN    48     /* Número de identificador para retornar tras procesar una señal */

/* --- Declaraciones/Prototipos de las Syscalls en Espacio de Kernel --- */

/* 
 * Finaliza la ejecución del proceso actual y libera sus recursos.
 * Argumento: exit_code - Código de salida devuelto al proceso padre.
 */
void sys_exit(int exit_code);

/* 
 * Abre un archivo en el sistema de archivos EXT2 según la ruta especificada.
 * Argumento: path - Puntero a la cadena con la ruta del archivo.
 * Retorno: Descriptor de archivo asignado (int) o código negativo en caso de error.
 */
int sys_open(char *path);

/* 
 * Modifica dinámicamente el límite del segmento de datos (Heap) del usuario.
 * Argumento: increment - Cantidad de bytes a expandir (positivo) o contraer (negativo).
 * Retorno: Puntero (char*) a la dirección anterior del límite del Heap.
 */
char* sys_sbrk(int increment);

/* 
 * Reemplaza la imagen del proceso actual por un nuevo programa ejecutable en formato ELF.
 * Argumentos: path - Ruta del ejecutable. argv - Arreglo de argumentos.
 * Retorno: Devuelve un entero negativo solo si falla el reemplazo.
 */
int sys_exec(char *path, char **argv);

/* 
 * Lee caracteres del dispositivo de entrada estándar (teclado/consola).
 * Argumento: buf - Búfer en espacio de usuario donde se almacenarán los datos leídos.
 * Retorno: Número de bytes leídos efectivamente.
 */
int sys_console_read(char *buf);

/* 
 * Suspende la ejecución del proceso padre hasta que uno de sus procesos hijos termine.
 * Argumento: status - Puntero donde se almacenará el estado de salida del proceso hijo.
 * Retorno: PID del proceso hijo finalizado.
 */
int sys_wait(int *status);

/* 
 * Restaura el marco de la pila original tras la ejecución de un manejador de señales en Ring 3.
 */
void sys_sigreturn(void);

#endif /* _SYSCALLS_H_ */
