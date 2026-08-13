#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "types.h"

/* --- Definición de constantes numéricas para cada Señal --- */
#define SIGHUP		 1      /* Hangup: Cierre de terminal o proceso colgado */
#define SIGINT		 2      /* Interrupt: Interrupción enviada desde el teclado (Ctrl+C) */
#define SIGQUIT		 3      /* Quit: Solicitud de salida con volcado de memoria (Ctrl+\) */
#define SIGBUS		 7      /* Bus Error: Error de acceso a memoria física no alineada o inexistente */
#define SIGKILL		 9      /* Kill: Finalización forzada e incondicional (no se puede capturar) */
#define SIGUSR1		10      /* User Signal 1: Señal definida por el usuario para propósitos generales */
#define SIGSEGV		11      /* Segmentation Fault: Acceso no válido a memoria virtual (paginación) */
#define SIGUSR2		12      /* User Signal 2: Segunda señal personalizada para el usuario */
#define SIGPIPE		13      /* Broken Pipe: Escritura en una tubería sin un proceso lector */
#define SIGALRM		14      /* Alarm Clock: Temporizador de tiempo transcurrido (alerta programada) */
#define SIGTERM		15      /* Termination: Solicitud amable de terminación de proceso */
#define SIGCHLD		17      /* Child Status: Enviada al padre cuando un proceso hijo termina o se detiene */
#define SIGCONT		18      /* Continue: Reanuda la ejecución de un proceso detenido */
#define SIGSTOP		19      /* Stop: Detención temporal de la ejecución (no se puede ignorar) */

/* --- Comportamientos por defecto de las señales --- */
#define SIG_DFL		0	    /* Acción por defecto (Default action) */
#define SIG_IGN		1	    /* Ignorar señal (Ignore signal) */

/* --- Macros para la manipulación de la máscara de bits de señales (Bitmask) --- */

/* Activa el bit correspondiente a 'sig' dentro del mapa de bits '*mask' */
#define set_signal(mask, sig)	*(mask) |= ((u32) 1 << (sig - 1))

/* Desactiva (limpia) el bit correspondiente a 'sig' dentro del mapa de bits '*mask' */
#define clear_signal(mask, sig)	*(mask) &= ~((u32) 1 << (sig - 1))

/* Evalúa si el bit de 'sig' está activo en la máscara 'mask' (retorna distinto de 0 si está activo) */
#define is_signal(mask, sig)	(mask & ((u32) 1 << (sig - 1)))

/* --- Declaración de Funciones Exportadas de signal.c --- */

/* Extrae y devuelve el número de la siguiente señal a procesar dentro de la máscara */
int dequeue_signal(int mask);

/* Ejecuta la acción correspondiente para procesar la señal recibida en el contexto actual */
int handle_signal(int sig);

#endif /* _SIGNAL_H_ */
