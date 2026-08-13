# Pépin OS - Implementación de Señales y Gestión de Procesos

Este repositorio contiene la implementación y extensión del kernel educacional **Pépin OS**, un sistema operativo en modo protegido x86 (32 bits). El proyecto abarca la inicialización del kernel, gestión de memoria paginada, llamadas al sistema, manejo de señales e integración de una interfaz de consola interactiva (`minishell`) en espacio de usuario.

---

## Estructura del Proyecto

* `kern/` : Código fuente del núcleo del sistema operativo (interrupciones, GDT, IDT, gestor de procesos, planificación y manejo de señales).
* `userland/` : Aplicaciones y librerías que se ejecutan en espacio de usuario (`shell`, `cat`, utilidades de prueba).
* `c.img` : Imagen de disco duro virtual formateada en sistema de archivos EXT2.
* `Makefile` : Configuración de reglas para la compilación del kernel y binarios de usuario.
* `shell_to_signals.diff` : Archivo de diferencias que documenta los cambios e implementación de señales.

---

## Requisitos del Sistema

Para compilar y ejecutar este proyecto se requieren las siguientes herramientas en un entorno Linux:

* **GCC** (con soporte para compilación en 32 bits `-m32`)
* **NASM** (Netwide Assembler)
* **QEMU** (`qemu-system-i386`)

---

## Compilación y Ejecución

Para iniciar la emulación del sistema operativo directamente con la imagen de disco y el kernel emulado, ejecuta el siguiente comando en la raíz del proyecto:

```bash
make run
