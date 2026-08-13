#include <stdarg.h>
#include "syscalls.h"

/* Copia n bytes de memoria de src a dst */
void *memcpy(char *dst, char *src, int n)
{
    char *p = dst;
    while (n--)
        *dst++ = *src++;
    return p;
}

/* Calcula la longitud de una cadena terminada en nulo */
int strlen (char *s)
{
    int i = 0;
    while (*s++)
        i++;
    return i;
}

/* Copia la cadena src en dst */
int strcpy (char *dst, char *src)
{
    int i = 0;
    while ((dst[i] = src[i++]));
    
    return i;
}

/* Copia como máximo n caracteres de src a dst */
int strncpy (char *dst, char *src, int n)
{
    int i = 0;

    while (n-- && (dst[i] = src[i++]));

    return i;
}

/* Compara dos cadenas (devuelve 0 si coinciden, 1 si difieren) */
int strcmp (char *dst, char *src)
{
    int i = 0;

    while ((dst[i] == src[i])) 
        if (src[i++] == 0)
            return 0;
    
    return 1;
}

/* Compara como máximo n caracteres entre dos cadenas */
int strncmp (char *dst, char *src, int n)
{
    int i = 0;

    while ((dst[i] == src[i])) 
        if (--n == 0 || src[i++] == 0)
            return 0;
    
    return 1;
}

/* Convierte una cadena de texto a un entero */
int atoi(char *str)
{
    char *c;
    int n, neg;

    c = str;

    /* Evalúa el signo negativo */
    if (*c == '-') {
        neg = -1;
        c++;
    }
    else 
        neg = 1;

    n = 0;
    /* Convierte los caracteres numéricos */
    while (*c >= '0' && *c <= '9') 
            n = n * 10 + *c++ - '0';

    return neg * n;
}

/* Convierte un entero a una cadena en la base especificada */
void itoa(char *buf, unsigned long int n, int base)
{
    unsigned long int tmp;
    int i, j;

    tmp = n;
    i = 0;

    /* Extrae los dígitos en orden inverso */
    do {
        tmp = n % base;
        buf[i++] = (tmp < 10) ? (tmp + '0') : (tmp + 'a' - 10);
    } while (n /= base);
    buf[i--] = 0;

    /* Invierte la cadena resultante */
    for (j = 0; j < i; j++, i--) {
        tmp = buf[j];
        buf[j] = buf[i];
        buf[i] = tmp;
    }
}

/* Construye una cadena formateada procesando especificadores de formato */
int sprintf(char *str, const char *format, ...)
{
    va_list ap;

    char buf[16], *p, *b_str;
    unsigned char c;
    int i, j, size, buflen, neg, ival;
    unsigned int uival;

    b_str = str;

    va_start(ap, format);

    while ((c = *format++)) {
        size = 0;
        neg = 0;

        if (c == 0)
            break;
        else if (c == '%') {
            c = *format++;
            /* Obtiene el ancho del campo si existe */
            if (c >= '0' && c <= '9') {
                size = c - '0';
                c = *format++;
            }

            /* Especificador %d: Entero con signo */
            if (c == 'd') {
                ival = va_arg(ap, int);
                if (ival < 0) {
                    uival = 0 - ival;
                    neg++;
                } else {
                    uival = ival;
                }

                itoa(buf, uival, 10);

                buflen = strlen(buf);
                if (buflen < size)
                    for (i = size, j = buflen; i >= 0; i--, j--)
                        buf[i] = (j >= 0) ? buf[j] : '0';

                if (neg) {
                    sprintf(str, "-%s", buf);
                    str += buflen + 1;
                }
                else {
                    sprintf(str, buf);
                    str += buflen;
                }

            /* Especificador %u: Entero sin signo */
            } else if (c == 'u') {
                uival = va_arg(ap, int);
                itoa(buf, uival, 10);

                buflen = strlen(buf);
                if (buflen < size)
                    for (i = size, j = buflen; i >= 0; i--, j--)
                        buf[i] = (j >= 0) ? buf[j] : '0';

                sprintf(str, buf);
                str += buflen;

            /* Especificador %x / %X: Hexadecimal */
            } else if (c == 'x' || c == 'X') {
                uival = va_arg(ap, int);
                itoa(buf, uival, 16);

                buflen = strlen(buf);
                if (buflen < size)
                    for (i = size, j = buflen; i >= 0; i--, j--)
                        buf[i] = (j >= 0) ? buf[j] : '0';

                sprintf(str, "0x%s", buf);
                str += buflen + 2;

            /* Especificador %p: Dirección de memoria */
            } else if (c == 'p') {
                uival = va_arg(ap, int);
                itoa(buf, uival, 16);
                size = 8;

                buflen = strlen(buf);
                if (buflen < size)
                    for (i = size, j = buflen; i >= 0; i--, j--)
                        buf[i] = (j >= 0) ? buf[j] : '0';

                sprintf(str, "0x%s", buf);
                str += buflen + 2;

            /* Especificador %s: Cadena de caracteres */
            } else if (c == 's') {
                p = (char *) va_arg(ap, int);
                sprintf(str, p);
                str += strlen(p);

            /* Especificador %c: Carácter único */
            } else if (c == 'c') {
                *str++ = va_arg(ap, int);

            } else {
                /* Carácter no reconocido */
            }
        } else {
            *str++ = c;
        }
    }

    *str = 0;

    va_end(ap);
    return (str - b_str);
}
