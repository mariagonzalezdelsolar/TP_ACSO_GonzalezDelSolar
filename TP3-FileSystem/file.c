#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    // Verifico punteros nulos
    if (fs == NULL || buf == NULL) {
        return -1; // Error: puntero nulo
    }

    // Verifico que el número de bloque sea positivo
    if (blockNum < 0) {
        return -1; // Error: número de bloque negativo
    }

    struct inode inp;

    // Errores de inodo
    if (inode_iget(fs, inumber, &inp) < 0){
        return -1; // No se pudo obtener el inodo.
    }
    if ((inp.i_mode & IALLOC) == 0) {
        return -1; // Inodo no asignado.
    }

    // Busco el número de bloque físico correspondiente a `blockNum` lógico
    int blk = inode_indexlookup(fs, &inp, blockNum);
    if (blk < 0){
        return -1; // Error: bloque inválido o no asignado.
    }

    // Leo el bloque físico correspondiente al bloque lógico
    if (diskimg_readsector(fs->dfd, blk, buf) < 0){
        return -1; // Error de lectura en disco
    }

    int fsize  = inode_getsize(&inp); // Tamaño total del archivo en bytes
    int offset = blockNum * BLOCK_SIZE; // Offset dentro del archivo correspondiente a `blockNum`

    // Offset más allá del tamaño del archivo?
    if (offset >= fsize){
        return 0; // El bloque está fuera del final del archivo.
    }

    // Si el bloque está dentro del tamaño del archivo, pero no completo
    if (offset + BLOCK_SIZE > fsize){
        return fsize - offset; // Parte del bloque contiene basura, devuelvo solo los bytes válidos.
    }

    return BLOCK_SIZE; // El bloque está completamente dentro del archivo.
}
