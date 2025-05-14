#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    // Valido inputs de entrada
    if (fs == NULL || buf == NULL) return -1;
    if (blockNum < 0) return -1;

    struct inode inp; // Inicializo el inodo en cero por seguridad

    if (inode_iget(fs, inumber, &inp) < 0) return -1;  // Obtengo el inodo a partir del número de inodo
    if ((inp.i_mode & IALLOC) == 0) return -1; // Me fijo que el inodo esté efectivamente asignado

    // Busco el número de bloque físico correspondiente 
    int blk = inode_indexlookup(fs, &inp, blockNum);
    if (blk < 0) return -1;

    // Leo el bloque físico del disco y lo copio al buffer
    if (diskimg_readsector(fs->dfd, blk, buf) < 0) return -1;

    // Obtengo el tamaño total del archivo
    int fsize = inode_getsize(&inp);
    if (fsize < 0) return -1;

    // Calculo el offset del bloque dentro del archivo
    int offset = blockNum * DISKIMG_SECTOR_SIZE;
    
    if (offset >= fsize) return 0; // Si el offset está más allá del final del archivo, no hay datos que devolver
    if ((offset + DISKIMG_SECTOR_SIZE) > fsize) return fsize - offset; // Si el bloque se extiende más allá del final del archivo, devuelvo solo los bytes útiles restantes
    
    return DISKIMG_SECTOR_SIZE; // Bloque completo
}