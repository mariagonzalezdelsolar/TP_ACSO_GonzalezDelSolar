#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt) {
  // Verifico punteros nulos
  if (fs == NULL || name == NULL || dirEnt == NULL) {
    return -1; // Error: puntero nulo
  }
  struct inode dir_inode;

  // MANEJO DE ERRORES
  // Errores de inodos
  if (inode_iget(fs, dirinumber, &dir_inode) < 0) {
    return -1; // Error al leer el inodo del directorio.
  }
  if ((dir_inode.i_mode & IALLOC) == 0) {
    return -1; // Inodo no asignado.
  }
  // Errores de directorio
  if ((dir_inode.i_mode & IFDIR) == 0) {
    return -1; // No es un directorio.
  }

  char block[BLOCK_SIZE];
  int num_blocks = (inode_getsize(&dir_inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;

  // Error al leer el bloque del directorio.
  for (int blk = 0; blk < num_blocks; blk++) {
    int bytes_read = file_getblock(fs, dirinumber, blk, block);
    if (bytes_read <= 0) {
        return -1;
    }
  
  // Cada bloque contiene varias entradas de directorio.
  int num_entries = BLOCK_SIZE / sizeof(struct direntv6);
  struct direntv6 *entries = (struct direntv6 *)block;

  for (int i = 0; i < num_entries; i++) {
    if (entries[i].d_inumber != 0 && strncmp(entries[i].d_name, name, sizeof(entries[i].d_name)) == 0) {
      *dirEnt = entries[i]; // Copio la entrada encontrada.
      return 0; // Éxito.
    }
   }
  }

  return -1; // No se encontró el nombre.
}
