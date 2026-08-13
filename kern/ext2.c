#include "ext2.h"
#include "disk.h"
#include "kmalloc.h"
#include "lib.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

/*
 * Obtiene la información general de la partición y la estructura lógica del disco EXT2.
 * Devuelve un puntero a struct disk listo para operar.
 */
struct disk *ext2_get_disk_info(int device, struct partition *part)
{
    int i, j;
    struct disk *hd;

    /* Reserva memoria en el Heap para la estructura del disco */
    hd = (struct disk *) kmalloc(sizeof(struct disk));

    hd->device = device;
    hd->part = part;
    
    /* 1. Lee el Superbloque (offset base de la partición + 1024 bytes) */
    hd->sb = ext2_read_sb(hd, part->s_lba * 512);
    
    /* 2. Calcula el tamaño del bloque EXT2 (1024 << s_log_block_size) */
    hd->blocksize = 1024 << hd->sb->s_log_block_size;

    /* 3. Calcula la cantidad total de grupos de bloques del sistema de archivos */
    i = (hd->sb->s_blocks_count / hd->sb->s_blocks_per_group) +
        ((hd->sb->s_blocks_count % hd->sb->s_blocks_per_group) ? 1 : 0);
    j = (hd->sb->s_inodes_count / hd->sb->s_inodes_per_group) +
        ((hd->sb->s_inodes_count % hd->sb->s_inodes_per_group) ? 1 : 0);
    hd->groups = (i > j) ? i : j;

    /* 4. Lee la Tabla de Descriptores de Grupo (Group Descriptors) */
    hd->gd = ext2_read_gd(hd, part->s_lba * 512);

    return hd;
}

/*
 * Lee el Superbloque desde el disco (reside a partir del byte 1024 de la partición).
 */
struct ext2_super_block *ext2_read_sb(struct disk *hd, int s_part)
{
    struct ext2_super_block *sb;

    sb = (struct ext2_super_block *) kmalloc(1024);
    /* Lee 1024 bytes del superbloque mediante el driver de disco */
    disk_read(hd->device, s_part + 1024, (char *) sb, sizeof(struct ext2_super_block));

    return sb;
}

/*
 * Lee la tabla de Descriptores de Grupo (Group Descriptor Table).
 */
struct ext2_group_desc *ext2_read_gd(struct disk *hd, int s_part)
{
    struct ext2_group_desc *gd;
    int offset, gd_size, alloc_size;

    /* Si el bloque es de 1024, la tabla está en el byte 2048; si es mayor, en el 2º bloque */
    offset = (hd->blocksize == 1024) ? 2048 : hd->blocksize;
    gd_size = hd->groups * sizeof(struct ext2_group_desc);

    alloc_size = (gd_size < 512) ? 512 : gd_size;
    gd = (struct ext2_group_desc *) kmalloc(alloc_size);

    /* Lee la tabla completa de descriptores desde el disco */
    disk_read(hd->device, s_part + offset, (char *) gd, gd_size);

    return gd;
}

/*
 * Lee los metadatos de un Inodo específico a partir de su número identificador (i_num).
 */
struct ext2_inode *ext2_read_inode(struct disk *hd, int i_num)
{
    int gr_num, index, offset;
    struct ext2_inode *inode;

    /* Validación de parámetros recibidos */
    if (!hd || !hd->gd || !hd->sb || i_num <= 0) {
        printk("ERROR: ext2_read_inode invalid params (i_num: %d)\n", i_num);
        return NULL;
    }

    inode = (struct ext2_inode *) kmalloc(512);

    /* Identifica a qué Grupo de Bloques pertenece el inodo */
    gr_num = (i_num - 1) / hd->sb->s_inodes_per_group;

    if (gr_num >= hd->groups) {
        gr_num = 0;
    }

    /* Posición (índice) dentro de la tabla de inodos del grupo */
    index = (i_num - 1) % hd->sb->s_inodes_per_group;

    /* Calcula la ubicación exacta en bytes dentro de la partición */
    offset = hd->gd[gr_num].bg_inode_table * hd->blocksize + index * hd->sb->s_inode_size;

    /* Lee el contenido del inodo desde el disco */
    disk_read(hd->device, (hd->part->s_lba * 512) + offset, (char *) inode, hd->sb->s_inode_size);

    return inode;
}

/*
 * Lee un archivo completo del disco EXT2 y lo vuelca en un búfer continuo de memoria RAM.
 */
char *ext2_read_file(struct disk *hd, struct ext2_inode *inode)
{
    char *mmap_base, *mmap_head, *buf;
    int *p, *pp, *ppp;
    int i, j, k, b;
    int n, size;

    /* Búferes temporales para lectura de bloques directos e indirectos */
    buf = (char *) kmalloc(hd->blocksize);
    p   = (int *)  kmalloc(hd->blocksize);  /* Búfer Indirección Simple */
    pp  = (int *)  kmalloc(hd->blocksize);  /* Búfer Indirección Doble */
    ppp = (int *)  kmalloc(hd->blocksize);  /* Búfer Indirección Triple */

    size = inode->i_size;                   /* Tamaño total del archivo en bytes */
    mmap_head = mmap_base = kmalloc(size ? size : 1); /* Reserva el búfer final */

    /* --- 1. Lectura de Bloques Directos (i_block[0] a i_block[11]) --- */
    for (i = 0; i < 12 && size > 0; i++) {
        n = ((size > hd->blocksize) ? hd->blocksize : size);

        if (inode->i_block[i] != 0) {
            /* Lee el bloque directo desde el disco */
            disk_read(hd->device, (hd->part->s_lba * 512) + inode->i_block[i] * hd->blocksize, buf, hd->blocksize);
            memcpy(mmap_head, buf, n);
        } else {
            /* Relleno con ceros si es un archivo con huecos (sparse file) */
            for (b = 0; b < n; b++) {
                mmap_head[b] = 0;
            }
        }

        mmap_head += n;
        size -= n;
    }

    /* --- 2. Lectura de Bloques con Indirección Simple (i_block[12]) --- */
    if (inode->i_block[12] && size > 0) {
        /* Lee el bloque apuntador que contiene la lista de bloques de datos */
        disk_read(hd->device, (hd->part->s_lba * 512) + inode->i_block[12] * hd->blocksize, (char *) p, hd->blocksize);

        for (i = 0; i < hd->blocksize / 4 && size > 0; i++) {
            n = ((size > hd->blocksize) ? hd->blocksize : size);

            if (p[i] != 0) {
                disk_read(hd->device, (hd->part->s_lba * 512) + p[i] * hd->blocksize, buf, hd->blocksize);
                memcpy(mmap_head, buf, n);
            } else {
                for (b = 0; b < n; b++) {
                    mmap_head[b] = 0;
                }
            }

            mmap_head += n;
            size -= n;
        }
    }

    /* --- 3. Lectura de Bloques con Indirección Doble (i_block[13]) --- */
    if (inode->i_block[13] && size > 0) {
        /* Lee el primer nivel de punteros */
        disk_read(hd->device, (hd->part->s_lba * 512) + inode->i_block[13] * hd->blocksize, (char *) p, hd->blocksize);

        for (i = 0; i < hd->blocksize / 4 && size > 0; i++) {
            if (p[i] != 0) {
                /* Lee el segundo nivel de punteros */
                disk_read(hd->device, (hd->part->s_lba * 512) + p[i] * hd->blocksize, (char *) pp, hd->blocksize);

                for (j = 0; j < hd->blocksize / 4 && size > 0; j++) {
                    n = ((size > hd->blocksize) ? hd->blocksize : size);

                    if (pp[j] != 0) {
                        disk_read(hd->device, (hd->part->s_lba * 512) + pp[j] * hd->blocksize, buf, hd->blocksize);
                        memcpy(mmap_head, buf, n);
                    } else {
                        for (b = 0; b < n; b++) {
                            mmap_head[b] = 0;
                        }
                    }

                    mmap_head += n;
                    size -= n;
                }
            }
        }
    }

    /* --- 4. Lectura de Bloques con Indirección Triple (i_block[14]) --- */
    if (inode->i_block[14] && size > 0) {
        /* Lee el nivel 1 de indirección */
        disk_read(hd->device, (hd->part->s_lba * 512) + inode->i_block[14] * hd->blocksize, (char *) p, hd->blocksize);

        for (i = 0; i < hd->blocksize / 4 && size > 0; i++) {
            if (p[i] != 0) {
                /* Lee el nivel 2 de indirección */
                disk_read(hd->device, (hd->part->s_lba * 512) + p[i] * hd->blocksize, (char *) pp, hd->blocksize);

                for (j = 0; j < hd->blocksize / 4 && size > 0; j++) {
                    if (pp[j] != 0) {
                        /* Lee el nivel 3 de indirección */
                        disk_read(hd->device, (hd->part->s_lba * 512) + pp[j] * hd->blocksize, (char *) ppp, hd->blocksize);

                        for (k = 0; k < hd->blocksize / 4 && size > 0; k++) {
                            n = ((size > hd->blocksize) ? hd->blocksize : size);

                            if (ppp[k] != 0) {
                                disk_read(hd->device, (hd->part->s_lba * 512) + ppp[k] * hd->blocksize, buf, hd->blocksize);
                                memcpy(mmap_head, buf, n);
                            } else {
                                for (b = 0; b < n; b++) {
                                    mmap_head[b] = 0;
                                }
                            }

                            mmap_head += n;
                            size -= n;
                        }
                    }
                }
            }
        }
    }

    /* Libera los búferes auxiliares del kernel */
    kfree(buf);
    kfree(p);
    kfree(pp);
    kfree(ppp);

    return mmap_base; /* Devuelve el búfer contiguo con la totalidad del archivo */
}
