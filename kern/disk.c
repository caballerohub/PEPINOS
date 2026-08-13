#include "types.h"
#include "kmalloc.h"
#include "io.h"
#include "lib.h"

/* Configura puertos IDE con direccionamiento LBA de 28 bits */
int bl_common(int drive, int numblock, int count)
{
    outb(0x1F1, 0x00);                             /* Reset de características */
    outb(0x1F2, count);                            /* Cantidad de sectores */
    outb(0x1F3, (unsigned char) numblock);         /* LBA bits 0-7 */
    outb(0x1F4, (unsigned char) (numblock >> 8));  /* LBA bits 8-15 */
    outb(0x1F5, (unsigned char) (numblock >> 16)); /* LBA bits 16-23 */

    /* Modo LBA, selección de disco y LBA bits 24-27 */
    outb(0x1F6, 0xE0 | (drive << 4) | ((numblock >> 24) & 0x0F));

    return 0;
}

/* Lee sectores de 512 bytes desde el disco hacia un buffer */
int bl_read(int drive, int numblock, int count, char *buf)
{
    u16 tmpword;
    int idx, s;

    for (s = 0; s < count; s++) {
        bl_common(drive, numblock + s, 1);
        outb(0x1F7, 0x20); /* Comando READ SECTORS */

        /* Espera a que el disco esté listo (DRQ = 1) */
        while (!(inb(0x1F7) & 0x08));

        /* Lee 256 palabras (512 bytes) del puerto de datos */
        for (idx = 0; idx < 256; idx++) {
            tmpword = inw(0x1F0);
            buf[(s * 256 + idx) * 2] = (unsigned char) tmpword;        /* Byte bajo */
            buf[(s * 256 + idx) * 2 + 1] = (unsigned char) (tmpword >> 8);/* Byte alto */
        }
    }

    return count;
}

/* Escribe sectores de 512 bytes desde un buffer hacia el disco */
int bl_write(int drive, int numblock, int count, char *buf)
{
    u16 tmpword;
    int idx, s;

    for (s = 0; s < count; s++) {
        bl_common(drive, numblock + s, 1);
        outb(0x1F7, 0x30); /* Comando WRITE SECTORS */

        /* Espera a que el disco esté listo (DRQ = 1) */
        while (!(inb(0x1F7) & 0x08));

        /* Envía 256 palabras (512 bytes) al puerto de datos */
        for (idx = 0; idx < 256; idx++) {
            tmpword = (buf[(s * 256 + idx) * 2 + 1] << 8) | buf[(s * 256 + idx) * 2];
            outw(0x1F0, tmpword);
        }
    }

    return count;
}

/* Lee bytes en un offset arbitrario sin requerir alineación a 512 bytes */
int disk_read(int drive, int offset, char *buf, int count)
{
    char *bl_buffer;
    int bl_begin, bl_end, blocks;

    if (count <= 0)
        return 0;

    /* Calcula bloques necesarios */
    bl_begin = offset / 512;
    bl_end = (offset + count - 1) / 512;
    blocks = bl_end - bl_begin + 1;

    /* Lee bloques a un buffer temporal */
    bl_buffer = (char *) kmalloc(blocks * 512);
    bl_read(drive, bl_begin, blocks, bl_buffer);

    /* Copia solo el rango de bytes solicitado */
    memcpy(buf, (char *) (bl_buffer + (offset % 512)), count);

    kfree(bl_buffer);

    return count;
}
