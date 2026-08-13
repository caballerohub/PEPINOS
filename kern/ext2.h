#include "types.h"

#ifndef __EXT2__
#define __EXT2__

/*
 * Estructura del Superbloque (Superblock) de EXT2:
 * Almacena los metadatos globales de todo el sistema de archivos (bloques totales,
 * inodos, identificadores mágicos y estado del volumen).
 * Se fuerza el empaquetado (__attribute__((packed))) para alinearse a los bytes exactos del disco.
 */
struct ext2_super_block {
	u32 s_inodes_count;	     /* Cantidad total de inodos en la partición */
	u32 s_blocks_count;	     /* Cantidad total de bloques en la partición */
	u32 s_r_blocks_count;	     /* Bloques reservados para el usuario root */
	u32 s_free_blocks_count; /* Bloques libres en el sistema de archivos */
	u32 s_free_inodes_count; /* Inodos libres en el sistema de archivos */
	u32 s_first_data_block;	 /* ID del primer bloque que contiene datos (0 o 1) */
	u32 s_log_block_size;	 /* Exponente para el tamaño de bloque: 1024 << s_log_block_size */
	u32 s_log_frag_size;	 /* Exponente para calcular el tamaño de fragmento */
	u32 s_blocks_per_group;	 /* Cantidad de bloques contenidos en cada Grupo de Bloques */
	u32 s_frags_per_group;	 /* Cantidad de fragmentos por grupo */
	u32 s_inodes_per_group;	 /* Cantidad de inodos por cada grupo de bloques */
	u32 s_mtime;		     /* Fecha/Hora del último montaje */
	u32 s_wtime;		     /* Fecha/Hora de la última escritura */
	u16 s_mnt_count;	     /* Número de montajes desde la última verificación completa */
	u16 s_max_mnt_count;	 /* Límite de montajes antes de forzar una verificación (e2fsck) */
	u16 s_magic;		     /* Firma mágica que identifica a EXT2 (debe ser 0xEF53) */
	u16 s_state;		     /* Estado del sistema de archivos (limpio o con errores) */
	u16 s_errors;		     /* Comportamiento ante errores (continuar, solo lectura, panic) */
	u16 s_minor_rev_level;	 /* Nivel de revisión menor */
	u32 s_lastcheck;	     /* Fecha/Hora del último chequeo del sistema de archivos */
	u32 s_checkinterval;	 /* Intervalo máximo de tiempo permitido entre chequeos */
	u32 s_creator_os;	     /* Sistema operativo creador (Linux = 0, Pepin1 = 5, etc.) */
	u32 s_rev_level;	     /* Nivel de revisión de la versión de EXT2 (= 1) */
	u16 s_def_resuid;	     /* UID por defecto asignado a los bloques reservados */
	u16 s_def_resgid;	     /* GID por defecto asignado a los bloques reservados */
	u32 s_first_ino;	     /* Primer inodo utilizable para archivos de usuario */
	u16 s_inode_size;	     /* Tamaño en bytes de cada estructura de inodo */
	u16 s_block_group_nr;	 /* Número del grupo de bloques que aloja este superbloque */
	u32 s_feature_compat;    /* Banderas de características compatibles */
	u32 s_feature_incompat;  /* Banderas de características incompatibles */
	u32 s_feature_ro_compat; /* Banderas de características de solo lectura */
	u8 s_uuid[16];		     /* Identificador único global del volumen (UUID) */
	char s_volume_name[16];	 /* Nombre asignado al volumen */
	char s_last_mounted[64]; /* Ruta donde se montó por última vez */
	u32 s_algo_bitmap;	     /* Algoritmo de compresión (si aplica) */
	u8 s_padding[820];       /* Relleno para completar el tamaño estándar del superbloque */
} __attribute__ ((packed));

/*
 * Entrada en la Tabla de Particiones (MBR - Master Boot Record):
 * Describe la disposición física de la partición en el disco de almacenamiento.
 */
struct partition {
	u8 bootable;		     /* 0x00 = No booteable, 0x80 = Partición activa / booteable */
	u8 s_head;		         /* Cabezal de inicio (CHSS) */
	u16 s_sector:6;		     /* Sector de inicio (6 bits) */
	u16 s_cyl:10;		     /* Cilindro de inicio (10 bits) */
	u8 id;			         /* Tipo/ID de partición (ej. 0x83 para Linux EXT2) */
	u8 e_head;		         /* Cabezal de fin */
	u16 e_sector:6;		     /* Sector de fin */
	u16 e_cyl:10;		     /* Cilindro de fin */
	u32 s_lba;		         /* Dirección lógica de bloque de inicio (LBA - Logical Block Addressing) */
	u32 size;		         /* Cantidad total de sectores en la partición */
} __attribute__ ((packed));

/*
 * Estructura de abstracción en memoria Kernel para representar un disco activo.
 */
struct disk {
	int device;                  /* Identificador del dispositivo físico (ej. HDD / IDE) */
	struct partition *part;      /* Apuntador a la información de la partición */
	struct ext2_super_block *sb; /* Apuntador al Superbloque cargado en RAM */
	u32 blocksize;               /* Tamaño del bloque calculado en bytes (1024, 2048, 4096) */
	u16 groups;		             /* Número total de Grupos de Bloques en la partición */
	struct ext2_group_desc *gd;  /* Tabla de Descriptores de Grupo cargada en RAM */
};

/*
 * Descriptor de Grupo de Bloques (Block Group Descriptor):
 * Contiene las ubicaciones y métricas para un grupo específico de bloques.
 */
struct ext2_group_desc {
	u32 bg_block_bitmap;	 /* ID del bloque que contiene el mapa de bits de bloques */
	u32 bg_inode_bitmap;	 /* ID del bloque que contiene el mapa de bits de inodos */
	u32 bg_inode_table;	     /* ID del primer bloque de la Tabla de Inodos del grupo */
	u16 bg_free_blocks_count;/* Cantidad de bloques libres en este grupo */
	u16 bg_free_inodes_count;/* Cantidad de inodos libres en este grupo */
	u16 bg_used_dirs_count;	 /* Cantidad de inodos asignados a directorios */
	u16 bg_pad;		         /* Relleno para alinear la estructura a 32 bits */
	u32 bg_reserved[3];	     /* Espacio reservado para expansión futura */
} __attribute__ ((packed));

/*
 * Inodo EXT2:
 * Estructura de metadatos asociada a un archivo o directorio individual.
 */
struct ext2_inode {
	u16 i_mode;		     /* Modo del archivo: Tipo (regular, dir) + Permisos (rwx) */
	u16 i_uid;               /* ID del usuario propietario (User ID) */
	u32 i_size;              /* Tamaño real del archivo en bytes */
	u32 i_atime;             /* Último tiempo de acceso (Access Time) */
	u32 i_ctime;             /* Tiempo de creación/modificación de metadatos (Creation Time) */
	u32 i_mtime;             /* Último tiempo de modificación de contenido (Modification Time) */
	u32 i_dtime;             /* Tiempo de eliminación del archivo (Deletion Time) */
	u16 i_gid;               /* ID del grupo propietario (Group ID) */
	u16 i_links_count;       /* Número de enlaces duros (hard links) que apuntan al inodo */
	u32 i_blocks;		     /* Cantidad de bloques de 512 bytes ocupados en disco */
	u32 i_flags;             /* Banderas de estado del inodo */
	u32 i_osd1;              /* Datos específicos del sistema operativo */

	/* 
	 * Arreglo de punteros a bloques de datos:
	 * [0..11] : Bloques directos (12 punteros)
	 * [12]    : Bloque de Indirección Simple
	 * [13]    : Bloque de Indirección Doble
	 * [14]    : Bloque de Indirección Triple
	 */
	u32 i_block[15];

	u32 i_generation;        /* Número de versión del archivo (usado por NFS) */
	u32 i_file_acl;          /* Lista de control de acceso al archivo (ACL) */
	u32 i_dir_acl;           /* Lista de control de acceso al directorio / Tamaño alto */
	u32 i_faddr;             /* Dirección de fragmento (obsoleto) */
	u8 i_osd2[12];           /* Datos adicionales del SO */
} __attribute__ ((packed));

/*
 * Entrada de Directorio (Directory Entry):
 * Mapea una cadena de texto (nombre de archivo) con su número de inodo correspondiente.
 */
struct directory_entry {
	u32 inode;		     /* Número de inodo asignado (0 si la entrada no está en uso) */
	u16 rec_len;		 /* Longitud total de este registro (offset para la siguiente entrada) */
	u8 name_len;		 /* Longitud real del nombre del archivo en caracteres */
	u8 file_type;        /* Tipo de archivo (regular, directorio, dispositivo, etc.) */
	char name;           /* Arreglo flexible/flexible char array con el nombre del archivo */
} __attribute__ ((packed));


/* --- CONSTANTES Y MÁSCARAS DE CONFIGURACIÓN --- */

/* Banderas para el campo s_errors del Superbloque */
#define	EXT2_ERRORS_CONTINUE	1  /* Ignorar errores y continuar */
#define	EXT2_ERRORS_RO		2  /* Montar el sistema como solo lectura */
#define	EXT2_ERRORS_PANIC	3  /* Provocar un Kernel Panic inmediatamente */
#define	EXT2_ERRORS_DEFAULT	1

/* Máscaras de tipo de archivo para el campo i_mode del Inodo */
#define	EXT2_S_IFMT	0xF000	   /* Máscara global de formato/tipo */
#define	EXT2_S_IFSOCK	0xC000	   /* Socket */
#define	EXT2_S_IFLNK	0xA000	   /* Enlace simbólico (Symbolic Link) */
#define	EXT2_S_IFREG	0x8000	   /* Archivo regular */
#define	EXT2_S_IFBLK	0x6000	   /* Dispositivo de bloques */
#define	EXT2_S_IFDIR	0x4000	   /* Directorio */
#define	EXT2_S_IFCHR	0x2000	   /* Dispositivo de caracteres */
#define	EXT2_S_IFIFO	0x1000	   /* Tubería con nombre (FIFO) */

/* Máscaras de permisos POSIX para el campo i_mode del Inodo */
#define	EXT2_S_ISUID	0x0800	   /* Bit Set-UID (SUID) */
#define	EXT2_S_ISGID	0x0400	   /* Bit Set-GID (SGID) */
#define	EXT2_S_ISVTX	0x0200	   /* Bit de permanencia (Sticky Bit) */
#define	EXT2_S_IRWXU	0x01C0	   /* Máscara de permisos del Usuario Propietario (rwx) */
#define	EXT2_S_IRUSR	0x0100	   /* Lectura Usuario */
#define	EXT2_S_IWUSR	0x0080	   /* Escritura Usuario */
#define	EXT2_S_IXUSR	0x0040	   /* Ejecución Usuario */
#define	EXT2_S_IRWXG	0x0038	   /* Máscara de permisos del Grupo (rwx) */
#define	EXT2_S_IRGRP	0x0020	   /* Lectura Grupo */
#define	EXT2_S_IWGRP	0x0010	   /* Escritura Grupo */
#define	EXT2_S_IXGRP	0x0008	   /* Ejecución Grupo */
#define	EXT2_S_IRWXO	0x0007	   /* Máscara de permisos de Otros (rwx) */
#define	EXT2_S_IROTH	0x0004	   /* Lectura Otros */
#define	EXT2_S_IWOTH	0x0002	   /* Escritura Otros */
#define	EXT2_S_IXOTH	0x0001	   /* Ejecución Otros */

/* Número fijo del Inodo del Directorio Raíz ('/') */
#define EXT2_INUM_ROOT	2

#endif				/* __EXT2__ */

/* Prototipos de funciones públicas expuestas por el módulo ext2.c */
struct disk *ext2_get_disk_info(int, struct partition *);
struct ext2_super_block *ext2_read_sb(struct disk *, int);
struct ext2_group_desc *ext2_read_gd(struct disk *, int);

struct ext2_inode *ext2_read_inode(struct disk *, int);
char *ext2_read_file(struct disk *, struct ext2_inode *);
