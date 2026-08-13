#include "libc/libc.h"
#include "libc/syscalls.h"
#include "libc/malloc.h"

int main(void)
{
	char **av;

	/*
	 * Reservación dinámica de memoria para el vector de argumentos (argv).
	 * Se asigna espacio para 3 elementos (punteros char*):
	 *   av[0] = Nombre de la aplicación a ejecutar ("task2")
	 *   av[1] = NULL (Fin del vector de argumentos)
	 *   av[2] = Margen de reserva en el heap
	 */
	av = (char**) malloc(sizeof(char*) * 3);

	/* Asigna memoria para almacenar la cadena con el nombre de la tarea objetivo */
	av[0] = (char*) malloc(4);
	strcpy(av[0], "task2");     /* Copia la cadena "task2" */
	av[1] = (char*) 0;           /* Delimita el final de la lista con NULL */

	/*
	 * Invocación de la Syscall 9 (exec):
	 * Inicia la ejecución del binario "task2" que se encuentra almacenado
	 * en el sistema de archivos EXT2.
	 */
	exec(av[0], av);

	/*
	 * CÓDIGO INALCANZABLE EN CONDICIONES NORMALES:
	 * Si la llamada a exec() es exitosa, reemplaza la imagen en memoria de este
	 * proceso por la de "task2", por lo que las líneas siguientes no se ejecutan.
	 * 
	 * Si exec() falla (ej. si el ejecutable "task2" no existe), el flujo continúa
	 * e intenta escribir en la dirección 0x00000000. Esto provocará de forma intencional
	 * una excepción de protección/Fallo de Página (Page Fault / Segment Violation).
	 */
	*((int*)0) = 0xdeadbeef;

	exit(0);

	return 0;
}
