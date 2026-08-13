/*
 * schedule
 * Planificador principal del sistema operativo.
 * Selecciona el siguiente proceso a ejecutar mediante Round-Robin y conmuta de tarea.
 */
void schedule(void);

/*
 * switch_to_task
 * Prepara los registros del procesador y realiza el salto/conmutación 
 * de contexto hacia la nueva tarea especificada por su PID y modo de ejecución.
 */
void switch_to_task(int pid, int mode);
