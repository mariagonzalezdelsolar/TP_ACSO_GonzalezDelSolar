
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    // Verifico hiperparámetros 
    if (fs == NULL || pathname == NULL || *pathname == '\0') {
        return -1; 
    }

    // Manejo del caso especial del directorio raíz.
    if (strcmp(pathname, "/") == 0) {
        return ROOT_INUMBER; 
    }

    // Verifico que el path no esté vacío o mal formado.
    if (pathname[0] != '/' || strlen(pathname) < 2) {
        return -1; // Error: path inválido.
    }

    // Verifico componentes vacíos
    if (strstr(pathname, "//") != NULL) {
        return -1;
    }

    // Copio el path para evitar modificar el original.
    char path_copy[strlen(pathname) + 1];
    strcpy(path_copy, pathname);

    // Salteo el '/' inicial.
    char *component = strtok(path_copy + 1, "/");
    int current_inumber = ROOT_INUMBER; // Comienzo en el inodo raíz.

    while (component != NULL) { // Recorro los componentes del path.
        struct direntv6 dir_entry;

        // Chequeo que el nombre del componente no sea demasiado largo
        if (strlen(component) > DIRENT_NAME_LEN) {
            return -1; 
        }

        int result = directory_findname(fs, component, current_inumber, &dir_entry);
        
        if (result < 0) {
            return -1; // Error: Componente no encontrado o error de acceso.
        }

        current_inumber = dir_entry.d_inumber;
        component = strtok(NULL, "/");
    }

    return current_inumber;
}
