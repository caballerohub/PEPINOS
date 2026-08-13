#define INBUFSIZE 4096

#ifndef __CONS__
#define __CONS__
struct terminal {
    struct process *pread;     /* Proceso que lee de la consola */
    struct process *pwrite;    /* Proceso que escribe en la consola */
};

struct vconsole {
    struct terminal *term;     /* Terminal */
    char inb[INBUFSIZE];       /* Buffer de entrada */
    int inlock;                /* Espera entrada (1) */
    int keypos;                /* Posición de lectura */
    int mode;                  /* Modo: sin buffer (0), con buffer (1), buffer completo (2) */
};
#endif

struct terminal *current_term;

void putc_console(char);
