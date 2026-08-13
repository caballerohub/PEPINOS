#include "types.h"
#include "list.h"
#include "lib.h"
#include "gdt.h"
#include "screen.h"
#include "io.h"
#include "idt.h"
#include "mm.h"
#include "process.h"
#include "boot.h"
#include "disk.h"
#include "kmalloc.h"
#include "ext2.h"
#include "elf.h"
#include "file.h"

/* Imprime "[ OK ]" en verde en la columna 40 */
void ok_msg(void)
{
    kX = 40;        /* Columna 40 */
    kattr = 0x0A;   /* Color verde */
    printk("OK\n");
    kattr = 0x07;   /* Restaura color normal */
}

/* Punto de entrada principal del kernel */
void kmain(struct multiboot_info *mbi)
{
    printk("Pepin is booting...\n");
    printk("RAM detected : %uk (lower), %uk (upper)\n", mbi->low_mem, mbi->high_mem);

    cli; /* Desactiva interrupciones */

    /* 1. Carga la GDT */
    printk("Loading GDT");
    init_gdt();
    /* Configura la pila del kernel (esp) */
    asm("    movw $0x18, %%ax \n \
        movw %%ax, %%ss \n \
        movl %0, %%esp"::"i" (KERN_STACK));
    ok_msg();

    /* 2. Carga la IDT */
    printk("Loading IDT");
    init_idt();
    ok_msg();

    /* 3. Configura el PIC 8259 */
    printk("Configure PIC");
    init_pic();
    ok_msg();

    /* 4. Carga el registro de tarea (TR) con el selector TSS */
    printk("Loading Task Register");
    asm("    movw $0x38, %ax; ltr %ax");
    ok_msg();

    /* 5. Activa paginación */
    printk("Enabling paging");
    init_mm(mbi->high_mem);
    ok_msg();

    hide_hw_cursor(); /* Oculta el cursor de hardware */

    {
        struct partition *p1;
        struct disk *hd;
        struct file *fp;
        struct terminal tty1;

        /* 6. Lee la tabla de particiones MBR */
        p1 = (struct partition *) kmalloc(sizeof(struct partition));
        disk_read(0, 0x01BE, (char *) p1, 16);
        printk("Partition found on block: %d, size: %d blocks, bootable: %x\n",
             p1->s_lba, p1->size, p1->bootable);

        /* 7. Obtiene metadatos ext2 de la partición */
        hd = ext2_get_disk_info(0, p1);

        /* 8. Monta la partición raíz (ext2) */
        printk("Mount root partition (ext2fs)");
        f_root = init_root(hd);
        ok_msg();

        /* 9. Inicializa la terminal */
        tty1.pread = tty1.pwrite = 0;
        current_term = &tty1;

        /* 10. Configura el proceso inicial (PID 0) */
        current = &p_list[0];
        current->pid = 0;
        current->state = 1;                  /* Estado activo */
        current->regs.cr3 = (u32) pd0;       /* Directorio de páginas */
        current->console = 0;
        current->pwd = f_root;               /* Directorio raíz '/' */
        current->parent = current;
        INIT_LIST_HEAD(&current->child);     /* Lista de procesos hijos */

        /* 11. Carga el ejecutable del shell de usuario */
        fp = path_to_file("/bin/shell");
        if (!fp) {
            printk("ERROR: /bin/shell no encontrado!\n");
        } else {
            /* Lee el inodo y carga la tarea en memoria */
            fp->inode = ext2_read_inode(hd, fp->inum);
            printk("DEBUG: inum=%d, size=%d\n", fp->inum, fp->inode->i_size);
            load_task(hd, fp->inode, 0, 0);
        }

        /* Mensaje de sistema listo */
        kattr = 0x47;
        printk("Interrupts are enable. System is ready !\n\n");
        kattr = 0x07;

        sti; /* Activa interrupciones */

        /* 12. Bucle principal del kernel */
        while (1) {
            /* Si no hay procesos activos, reejecuta el shell */
            if (n_proc == 0) {
                cli;
                load_task(hd, fp->inode, 0, 0);
                sti;
            }
        }
    }
}
