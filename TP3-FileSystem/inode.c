#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

// Número de direcciones de bloques directas/indirectas en un inodo.
#define NADDR      8          
#define NDIRECT    (NADDR - 1) 

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    // Verifico punteros nulos
    if (fs == NULL || inp == NULL) return -1; // Error: puntero nulo

    const int inodes_per_block = DISKIMG_SECTOR_SIZE / sizeof(struct inode); // Cantidad de inodos que entran en un bloque
    const int max_inodes = fs->superblock.s_isize * inodes_per_block; // Verifico que el inodo esté dentro del rango válido (mayor o igual a ROOT_NUMBER y menor o igual a la cantidad de inodos máxima)

    if (inumber < ROOT_INUMBER || inumber > max_inodes) return -1; // Error: inodo fuera de rango --> invalido
    
    const int block  = INODE_START_SECTOR + (inumber - 1) / inodes_per_block; // Bloque de disco en el que se encuentra el inodo
    const int index  = (inumber - 1) % inodes_per_block; // Índice del inodo dentro del bloque de disco
    
    struct inode *inode_block = malloc(DISKIMG_SECTOR_SIZE); // Buuffer para leer todos los inodos del bloque
    if (inode_block == NULL) return -1; // Error: no se pudo asignar memoria

    int result = -1;

    // Leo el bloque de disco que contiene los inodos
    if (diskimg_readsector(fs->dfd, block, inode_block) >= 0) { // Si no entra acá --> Error: no se pudo leer el sector
        *inp = inode_block[index]; // Copio el inodo deseado al puntero de salida
        result = 0;
    }
    
    free(inode_block);
    return result;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    // Verifico punteros nulos
    if (fs == NULL || inp == NULL) return -1; // Error: puntero nulo

    // Verifco que el numero de bloque sea positivo
    if (blockNum < 0) return -1; // Error: número de bloque negativo

    // Calculo cuántos punteros caben en un bloque (cada uno de 16 bits)
    const int ptrs_per_block = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);
    uint16_t *block_buf = malloc(DISKIMG_SECTOR_SIZE);  // Buffer temporal para leer bloques indirectos

    if (block_buf == NULL) return -1; // Error: no se pudo asignar memoria

    int result = -1;

    // CASO 1: “small file”: (todos los bloques son accesibles directamente desde i_addr)
    // Verifico que el bloque directo sea válido, y que el bloque esté asignado
    if ((inp->i_mode & ILARG) == 0) {
        if (blockNum < NADDR) result = inp->i_addr[blockNum];
        goto cleanup;
    }

    // CASO 2: “large file” (uso de bloques indirectos y doble indirecto)
    const int direct_ptrs = NDIRECT;  // 7 punteros a bloques indirectos simples
    const int max_direct = ptrs_per_block * direct_ptrs; // Máximo de bloques accesibles con punteros indirectos simples

    // SUBCASO 2a: Bloques indirectos simples
    if (blockNum < max_direct) {
        const int which = blockNum / ptrs_per_block; // Qué puntero indirecto udar (0-6)
        const int offset = blockNum % ptrs_per_block; // Offset dentro del bloque apuntado

        if (inp->i_addr[which] == 0) goto cleanup;  // No asignado
        if (diskimg_readsector(fs->dfd, inp->i_addr[which], block_buf) < 0) goto cleanup; // Error en la lectura del bloque indirecto
        result = block_buf[offset]; // 0 = no asignado
    } 

    // SUBCASO 2b: Bloques doblemente indirectos 
    else {
        const int idx    = blockNum - max_direct; // Offset dentro del espacio doblemente indirecto
        const int first  = idx / ptrs_per_block; // índice al primer nivel
        const int second = idx % ptrs_per_block; // índice al segundo nivel

        if (inp->i_addr[direct_ptrs] == 0) goto cleanup; // Error: bloque doblemente indirecto no asignado
        if (diskimg_readsector(fs->dfd, inp->i_addr[direct_ptrs], block_buf) < 0) goto cleanup; // Error en la lectura 
        if (block_buf[first] == 0) goto cleanup; // Error: bloque indirecto secundario no asignado

        if (diskimg_readsector(fs->dfd, block_buf[first], block_buf) < 0) goto cleanup; // Error: no se pudo leer bloque indirecto
        result = block_buf[second]; // 0 = no asignado
    }

    cleanup:
        free(block_buf);
        return (result == 0) ? -1 : result;; // Si no está asignado (0) --> error
                                             // Si no hay error devuelvo el numero de bloque fisico correspondiente, sino, -1
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
