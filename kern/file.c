#include "lib.h"
#include "list.h"
#include "kmalloc.h"
#include "file.h"
#include "process.h"


/*
 * Inicializa la estructura del nodo raíz del sistema de archivos ("/").
 * Parámetros:
 *   - disk: Puntero a la información física del disco montado.
 * Retorno: Puntero a la estructura `struct file` del directorio raíz.
 */
struct file *init_root(struct disk *disk)
{
	struct file *fp;

	/* Reserva memoria dinamica en el kernel para la estructura del archivo raíz */
	fp = (struct file *) kmalloc(sizeof(struct file));

	/* Asigna el nombre "/" al nodo raíz */
	fp->name = (char *) kmalloc(sizeof("/"));
	strcpy(fp->name, "/");

	fp->disk = disk;
	fp->inum = EXT2_INUM_ROOT;                  /* En EXT2, la raíz siempre es el inodo #2 */
	fp->inode = ext2_read_inode(disk, fp->inum);/* Lee el inodo correspondiente desde el disco */
	fp->mmap = 0;
	fp->parent = fp;                            /* El padre del nodo raíz es él mismo */

	INIT_LIST_HEAD(&fp->leaf);                  /* Inicializa la lista de hijos (hojas/archivos) */
	get_dir_entries(fp);                        /* Pobla el caché leyendo las entradas del directorio */

	INIT_LIST_HEAD(&fp->sibling);               /* Inicializa la lista de nodos hermanos */

	return fp;
}

/* 
 * Determina si el nodo de archivo especificado es un directorio.
 * Retorno: 1 si es un directorio, 0 en caso contrario.
 */
int is_directory(struct file *fp)
{
	/* Carga diferida (lazy loading): si el inodo no está en memoria, lo lee del disco */
	if (!fp->inode) 
		fp->inode = ext2_read_inode(fp->disk, fp->inum);

	/* Compara el campo de modo con la máscara de directorio EXT2 */
	return (fp->inode->i_mode & EXT2_S_IFDIR) ? 1 : 0;
}

/*
 * Busca un archivo o subdirectorio por su nombre dentro de la lista de elementos 
 * en la memoria caché del directorio padre (`dir->leaf`).
 */
struct file *is_cached_leaf(struct file *dir, char *filename)
{
	struct file *leaf;

	/* Recorre la lista doblemente enlazada de nodos hermanos pertenecientes al directorio */
	list_for_each_entry(leaf, &dir->leaf, sibling){
		if (strcmp(leaf->name, filename) == 0)
			return leaf;                        /* Nodo encontrado en la caché */
	}

	return (struct file *) 0;                   /* No encontrado en la caché */
}

/*
 * Lee el contenido binario de un directorio desde el disco y actualiza la lista de 
 * submódulos/archivos en la caché del kernel.
 */
int get_dir_entries(struct file *dir)
{
    struct directory_entry *dentry;
    struct file *leaf;
    u32 dsize;
    char *filename;
    int f_toclose;        

    /* Garantiza que los metadatos del inodo del directorio estén en RAM */
    if (!dir->inode) 
        dir->inode = ext2_read_inode(dir->disk, dir->inum);

    /* Asegura que la estructura evaluada sea efectivamente un directorio */
    if (!is_directory(dir)) {
        printk("get_dir_entries() error: %s is not a directory\n", dir->name);
        return -1;
    }

    /* Si el contenido del directorio no ha sido mapeado a memoria, lo lee del disco */
    if (!dir->mmap) {
        dir->mmap = ext2_read_file(dir->disk, dir->inode);
        f_toclose = 1;                           /* Marca que debe liberarse la memoria al finalizar */
    } else {
        f_toclose = 0;
    }

    dsize = dir->inode->i_size;                 /* Tamaño total del bloque del directorio */
    dentry = (struct directory_entry *) dir->mmap;

    /* Recorre la secuencia de estructuras `directory_entry` mapeadas en memoria */
    while (dsize > 0 && dentry->rec_len > 0) {
        
        /* Solo procesa las entradas cuyo inodo sea válido (diferente de 0) */
        if (dentry->inode != 0) {
            filename = (char *) kmalloc(dentry->name_len + 1);
            memcpy(filename, &dentry->name, dentry->name_len);
            filename[dentry->name_len] = 0;     /* Asegura la terminación en nulo del string */

            /* Ignora las entradas especiales "." (directorio actual) y ".." (directorio padre) */
            if (strcmp(".", filename) && strcmp("..", filename)) {
                /* Si el archivo no está aún en la caché de hijos del directorio, crea la entrada */
                if (!(leaf = is_cached_leaf(dir, filename))) {    
                    leaf = (struct file *) kmalloc(sizeof(struct file));
                    leaf->name = (char *) kmalloc(dentry->name_len + 1);
                    strcpy(leaf->name, filename);

                    leaf->disk = dir->disk;
                    leaf->inum = dentry->inode;
                    leaf->inode = 0;             /* Inodo en estado diferido (Lazy allocation) */
                    leaf->mmap = 0;
                    leaf->parent = dir;          /* Enlaza al directorio actual como su padre */
                    INIT_LIST_HEAD(&leaf->leaf);
                    list_add(&leaf->sibling, &dir->leaf); /* Añade a la lista de caché de 'dir' */
                }
            }

            kfree(filename);
        }

        /* Desplaza el puntero y resta los bytes procesados mediante la longitud variable rec_len */
        dsize -= dentry->rec_len;
        dentry = (struct directory_entry *) ((char *) dentry + dentry->rec_len);
    }

    /* Si los datos se leyeron temporalmente para esta llamada, se libera el búfer */
    if (f_toclose == 1) {
        kfree(dir->mmap);
        dir->mmap = 0;
    }

    return 0;
}

/*
 * Resuelve una cadena de texto que contiene una ruta de archivo (ej. "/usr/bin/sh" o "docs/file.txt")
 * y retorna el puntero a la estructura `struct file` correspondiente cargada en la caché del VFS.
 */
struct file *path_to_file(char *path)
{
	char *name, *beg_p, *end_p;
	struct file *fp;

	/* Determina si la ruta es relativa o absoluta */
	if (path[0] != '/')
		fp = current->pwd;                      /* Ruta Relativa: Inicia desde el PWD del proceso actual */
	 else
		fp = f_root;                            /* Ruta Absoluta: Inicia desde la raíz del VFS ('/') */

	/* Avanza los punteros ignorando las barras diagonales iniciales */
	beg_p = path;
	while (*beg_p == '/')
		beg_p++;
	end_p = beg_p + 1;

	/* Recorre cada uno de los componentes de la ruta delimitados por '/' */
	while (*beg_p != 0) {	
		/* Verifica que el nodo actual tenga cargado su inodo y sea un directorio válido */
		if (!fp->inode)
			fp->inode = ext2_read_inode(fp->disk, fp->inum);

		if (!is_directory(fp))
			return (struct file *) 0;           /* Error: Un elemento intermedio de la ruta no es directorio */

		/* Extrae el nombre del subdirectorio o archivo de la cadena actual */
		while (*end_p != 0 && *end_p != '/')
			end_p++;
		name = (char *) kmalloc(end_p - beg_p + 1);
		memcpy(name, beg_p, end_p - beg_p);
		name[end_p - beg_p] = 0;

		/* Navegación según el nombre del componente */
		if (strcmp("..", name) == 0) {		    /* '..' -> Mueve la navegación al nodo padre */
			fp = fp->parent;
		} else if (strcmp(".", name) == 0) {	/* '.' -> Se mantiene en el nodo actual */
			/* Sin cambios (nop) */
		} else {
			get_dir_entries(fp);                 /* Carga o actualiza las entradas del directorio en caché */
			if (!(fp = is_cached_leaf(fp, name))) {
				kfree(name);
				return (struct file *) 0;       /* Archivo o subdirectorio no encontrado */
			}
		}

		/* Avanza los punteros al siguiente componente de la ruta */
		beg_p = end_p;
		while (*beg_p == '/')
			beg_p++;
		end_p = beg_p + 1;

		kfree(name);
	}

	return fp;                                  /* Retorna el nodo `struct file` final localizado */
}
