#include "screen.h"
#include "console.h"
#include "process.h"

/* Copia un carácter al buffer del proceso actual */
void putc_console(char c)
{
    struct process *p;

    /* Sin consola activa, escribe directo en memoria de video */
    if (!current_term || !(p = current_term->pread) || p->state<1) {
        putcar(c);
        return;
    }

    if (p->console->mode == 0) {    /* Modo sin buffer */
        putcar(c);
        p->console->inb[0] = c;
        p->console->term->pread = 0;
        p->console->inlock = 0;
    } else {            /* Modos con buffer */
        if (c == 8) {        /* Retroceso (backspace) */
            if (p->console->keypos) {
                p->console->inb[p->console->keypos--] = 0;
                if (p->console->mode == 1)
                    putcar(c);
            }
        }
        else if (c == 10) {    /* Salto de línea */
            if (p->console->mode == 1)
                putcar(c);
            p->console->inb[p->console->keypos++] = c;
            p->console->inb[p->console->keypos] = 0; 
            p->console->term->pread = 0;
            p->console->inlock = 0;
            p->console->keypos = 0;
        }
        else {                /* Carácter normal */
            if (p->console->mode == 1)
                putcar(c);
            p->console->inb[p->console->keypos++] = c; 
        }

    } 
}
