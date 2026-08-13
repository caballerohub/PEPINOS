#include "libc/libc.h"
#include "libc/syscalls.h"
#include "libc/malloc.h"

/*
 * Función Manejadora de Señales (Signal Handler)
 * Esta función es invocada asíncronamente por el kernel cuando el proceso
 * recibe la señal registrada.
 */
void foo(void)
{
	console_write("signal trapped !\n");
}

int main(void)
{
	/*
	 * Syscall 13 (sigaction): Registra la función 'foo' como la rutina 
	 * para atender la señal número 1.
	 */
	sigaction(1, foo);

	/*
	 * Bucle Infinito: Mantiene el proceso activo en la tabla de procesos.
	 * Sirve para probar que el timer/scheduler puede interrumpir este proceso
	 * y que las señales pueden pausar su ejecución en segundo plano para 
	 * saltar a 'foo'.
	 */
	while (1);

	/* Código inalcanzable debido al bucle infinito previo */
	exit(0);

	return 0;
}
