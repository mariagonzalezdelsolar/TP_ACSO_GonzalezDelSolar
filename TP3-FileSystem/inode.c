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
    if (fs == NULL || inp == NULL) {
        return -1; // Error: puntero nulo
    }

    // Cantidad de inodos que entran en un bloque
    int inodes_per_block = BLOCK_SIZE / sizeof(struct inode); 
    
    // Verifico que el inodo esté dentro del rango válido (mayor o igual a ROOT_NUMBER y menor o igual a la cantidad de inodos máxima)
    int max_inodes = fs->superblock.s_isize * inodes_per_block; 
    if (inumber < ROOT_INUMBER || inumber > max_inodes){
        return -1; // Error: inodo fuera de rango --> invalido
    }
    
    int block  = INODE_START_SECTOR + (inumber - 1) / inodes_per_block; // Bloque de disco en el que se encuentra el inodo
    int index  = (inumber - 1) % inodes_per_block; // Índice del inodo dentro del bloque de disco
    
    struct inode buf[inodes_per_block]; // Buuffer para leer todos los inodos del bloque

    // Leo el bloque de disco que contiene los inodos
    if (diskimg_readsector(fs->dfd, block, buf) < 0){
        return -1; // Error de lectura en disco
    }
    
    *inp = buf[index]; // Copio el inodo deseado al puntero de salida

    return 0; // Éxito
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    // Verifico punteros nulos
    if (fs == NULL || inp == NULL) {
        return -1; // Error: puntero nulo
    }

    // Verifco que el numero de bloque sea positivo
    if (blockNum < 0) {
        return -1; // Error: número de bloque negativo
    }

    // Calcular cuántos punteros caben en un bloque (cada uno de 16 bits)
    int ptrs_per_block = BLOCK_SIZE / sizeof(uint16_t);

    // -----------------------------------------------------------------------
    // CASO 1: “small file”: (todos los bloques son accesibles directamente desde i_addr)
    if ((inp->i_mode & ILARG) == 0) {

        if (blockNum >= NADDR)
            return -1; // Error: índice de bloque fuera de rango

        // Verifico que el bloque directo sea válido
        if (inp->i_addr[blockNum] == 0) {
            return -1; // Error : bloque no asignado
        }

        return inp->i_addr[blockNum]; // Acceso directo
    }

    // ----------------------------------------------------------------------
    // CASO 2: “large file” (uso de bloques indirectos y doble indirecto)
    uint16_t buf[ptrs_per_block]; // Buffer temporal para leer bloques indirectos
    
    int direct_ptrs      = NADDR - 1;                     // 7 punteros a bloques indirectos simples
    int max_direct_blks  = ptrs_per_block * direct_ptrs;  // Máximo de bloques accesibles con punteros indirectos simples

    // SUBCASO 2a: Bloques indirectos simples
    if (blockNum < max_direct_blks) {
        int which  = blockNum / ptrs_per_block; // Qué puntero indirecto udar (0-6)
        int offset = blockNum % ptrs_per_block; // Offset dentro del bloque apuntado
        
        uint16_t indir_block = inp->i_addr[which]; // Obtengo el bloque indirecto correspondiente

        // Verifico que el bloque indirecto sea válido
        if (indir_block == 0) {
            return -1; // Error: bloque indirecto no asignado
        }

        if (diskimg_readsector(fs->dfd, indir_block, buf) < 0)
            return -1; // Error en la lectura del bloque indirecto

        // Verifico que el bloque apuntado sea válido
        if (buf[offset] == 0) {
            return -1; // Error: bloque apuntado no asignado
        }

        return buf[offset]; // Devuelvo el número de bloque físico correspondiente
    } 

    // SUBCASO 2b: Bloques doblemente indirectos 
    else {
        int idx    = blockNum - max_direct_blks; // Offset dentro del espacio doblemente indirecto
        int first  = idx / ptrs_per_block; // índice al primer nivel
        int second = idx % ptrs_per_block; // índice al segundo nivel

        uint16_t dbl_indir = inp->i_addr[direct_ptrs]; // Último puntero de i_addr apunta al bloque doblemente indirecto

        // Verifico que el bloque doblemente indirecto sea válido
        if (dbl_indir == 0) {
            return -1; // Error: bloque doblemente indirecto no asignado
        }

        // Leo el bloque doblemente indirecto
        if (diskimg_readsector(fs->dfd, dbl_indir, buf) < 0)
            return -1; // Error en la lectura 

        uint16_t indir_block = buf[first]; // Obtengo el bloque indirecto simple correspondiente

        // Verifico que el bloque indirecto simple sea válido
        if (indir_block == 0) {
            return -1; // Error: bloque indirecto simple no asignado
        }

        // Leo el bloque indirecto del segundo nivel
        if (diskimg_readsector(fs->dfd, indir_block, buf) < 0)
            return -1;

        // Verifico que el bloque final sea válido
        if (buf[second] == 0) {
            return -1; // Error: bloque apuntado no asignado
        }
        
        return buf[second]; // Devuelvo el número de bloque físico correspondiente
    }
}


int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
