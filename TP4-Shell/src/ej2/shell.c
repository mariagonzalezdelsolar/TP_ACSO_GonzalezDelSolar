#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

/* Constantes de configuración */
#define MAX_COMANDOS 200       // Máximo número de comandos en un pipeline
#define MAX_ARGUMENTOS 64      // Máximo número de argumentos por comando (63 + NULL)
#define MAX_ENTRADA 1024       // Tamaño máximo del input del usuario

/**
 * Elimina espacios en blanco al inicio y final de un string
 * @param str String a procesar (se modifica in-place)
 * @return Puntero al string modificado
 */
char* trim_espacios(char* str) {
    char* fin;
    
    // Eliminar espacios al inicio
    while (*str && isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str; // Solo espacios
    
    // Eliminar espacios al final
    fin = str + strlen(str) - 1;
    while (fin > str && isspace((unsigned char)*fin)) fin--;
    
    // Terminar el string correctamente
    *(fin + 1) = '\0';
    
    return str;
}

/**
 * Parsea un comando en argumentos individuales, mnejando comillas
 * @param comando String con el comando completo
 * @param args Array donde almacenar los argumntos parseados
 * @return Número de argumentos, o -1 en error
 */
int parsear_argumentos(char* comando, char** args) {
    int contador_args = 0;
    char* cursor = comando;
    char* inicio_arg;
    char caracter_comilla = 0;  // Variable simplificada (eliminado en_comllas no usado)
    
    while (*cursor && contador_args < MAX_ARGUMENTOS - 1) {
        // Saltar espacios entre argumentos
        while (*cursor && isspace((unsigned char)*cursor)) cursor++;
        if (!*cursor) break;
        
        inicio_arg = cursor;
        
        // Manejar argumentos entre comillas
        if (*cursor == '"' || *cursor == '\'') {
            caracter_comilla = *cursor;
            cursor++; // Saltar comilla inicial
            inicio_arg = cursor; // Argumento comienza después
            
            // Buscar comilla de cierre
            while (*cursor && *cursor != caracter_comilla) cursor++;
            
            if (!*cursor) {
                fprintf(stderr, "Error: Comilla sin cerrar\n");
                return -1;
            }
            
            *cursor = '\0'; // Terminar argumento
            args[contador_args++] = inicio_arg;
            cursor++; // Saltar comilla final
        } else {
            // Argumento normal (sin comillas)
            while (*cursor && !isspace((unsigned char)*cursor)) cursor++;
            
            if (*cursor) {
                *cursor = '\0';
                cursor++;
            }
            args[contador_args++] = inicio_arg;
        }
    }
    
    // Verificar límite de argumentos
    if (contador_args >= MAX_ARGUMENTOS - 1) {
        fprintf(stderr, "Error: Demasiados argumentos\n");
        return -1;
    }
    
    args[contador_args] = NULL; // Terminar array con NULL
    return contador_args;
}

/**
 * Valida la sintaxis de un pipeline (uso correcto de pipes '|')
 * @param entrada String con el comando completo
 * @return 1 si es válido, 0 si hay error
 */
int validar_sintaxis_pipes(char* entrada) {
    char* cursor = trim_espacios(entrada);
    
    // No puede comenzar o terminar con pipe
    if (*cursor == '|') {
        fprintf(stderr, "Error: El comando no puede comenzar con pipe\n");
        return 0;
    }
    
    int longitud = strlen(cursor);
    if (longitud > 0 && cursor[longitud-1] == '|') {
        fprintf(stderr, "Error: El comando no puede terminar con pipe\n");
        return 0;
    }
    
    // Verificar pipes consecutivos o comandos vacíos
    char* pos_pipe = strchr(cursor, '|');
    while (pos_pipe) {
        // No permitir pipes dobles
        if (*(pos_pipe + 1) == '|') {
            fprintf(stderr, "Error: Sintaxis de pipe inválida\n");
            return 0;
        }
        
        // Debe haber contenido antes del pipe
        char* antes = pos_pipe - 1;
        while (antes >= cursor && isspace((unsigned char)*antes)) antes--;
        if (antes < cursor) {
            fprintf(stderr, "Error: Comando vacío antes del pipe\n");
            return 0;
        }
        
        // Debe haber contenido después del pipe
        char* despues = pos_pipe + 1;
        while (*despues && isspace((unsigned char)*despues)) despues++;
        if (!*despues) {
            fprintf(stderr, "Error: Comando vacío después del pipe\n");
            return 0;
        }
        
        pos_pipe = strchr(pos_pipe + 1, '|');
    }
    
    return 1;
}

int main() {
    char entrada_usuario[MAX_ENTRADA];
    char copia_entrada[MAX_ENTRADA];
    char *comandos[MAX_COMANDOS];
    int num_comandos = 0;
    int pipes[MAX_COMANDOS-1][2];
    pid_t pids_hijos[MAX_COMANDOS];

    // Mostrar mensaje inicial solo en terminal interactivo
    if (isatty(STDIN_FILENO)) {
        printf("Shell iniciado. Escriba 'exit' para salir.\n");
    }

    while (1) {
        // Mostrar prompt en terminal interactivo
        if (isatty(STDIN_FILENO)) {
            printf("Shell> ");
            fflush(stdout);
        }
        
        // Leer entrada del usuario
        if (fgets(entrada_usuario, sizeof(entrada_usuario), stdin) == NULL) {
            if (isatty(STDIN_FILENO)) {
                printf("\nShell finalizado.\n");
            }
            break;
        }
        
        // Eliminar salto de línea
        entrada_usuario[strcspn(entrada_usuario, "\n")] = '\0';

        // Eliminar espacios innecesarios
        char* entrada_limpia = trim_espacios(entrada_usuario);
        
        // Ignorar líneas vacías
        if (strlen(entrada_limpia) == 0) {
            continue;
        }

        // Comando para salir del shell
        if (strcmp(entrada_limpia, "exit") == 0) {
            if (isatty(STDIN_FILENO)) {
                printf("Shell finalizado.\n");
            }
            break;
        }

        // Validar sintaxis de pipes
        if (!validar_sintaxis_pipes(entrada_limpia)) {
            continue;
        }

        // Hacer copia para tokenización
        strcpy(copia_entrada, entrada_limpia);

        // Dividir por pipes para obtener comandos individuales
        num_comandos = 0;
        char *token_comando = strtok(copia_entrada, "|");
        while (token_comando != NULL && num_comandos < MAX_COMANDOS) {
            // Limpiar espacios alrededor de cada comando
            token_comando = trim_espacios(token_comando);
            if (strlen(token_comando) > 0) {
                // Reservar memoria para el comando
                comandos[num_comandos] = malloc(strlen(token_comando) + 1);
                if (comandos[num_comandos] == NULL) {
                    perror("Error en malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(comandos[num_comandos], token_comando);
                num_comandos++;
            }
            token_comando = strtok(NULL, "|");
        }

        if (num_comandos == 0) {
            continue;
        }

        // Caso 1: Un solo comando (sin pipes)
        if (num_comandos == 1) {
            pid_t pid = fork();
            if (pid == 0) {
                // Proceso hijo
                char *argumentos[MAX_ARGUMENTOS];
                char *copia_comando = malloc(strlen(comandos[0]) + 1);
                if (copia_comando == NULL) {
                    perror("Error en malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(copia_comando, comandos[0]);
                
                // Parsear argumentos del comando
                int num_args = parsear_argumentos(copia_comando, argumentos);
                if (num_args <= 0) {
                    free(copia_comando);
                    exit(EXIT_FAILURE);
                }
                
                // Ejecutar comando
                execvp(argumentos[0], argumentos);
                
                // Solo se llega aquí si execvp falla
                fprintf(stderr, "%s: comando no encontrado\n", argumentos[0]);
                free(copia_comando);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                // Proceso padre espera al hijo
                int estado;
                waitpid(pid, &estado, 0);
            } else {
                perror("Error en fork");
            }
        } else {
            // Caso 2: Pipeline con múltiples comandos
            
            // Crear pipes para la comunicación
            int error_pipes = 0;
            for (int i = 0; i < num_comandos - 1; i++) {
                if (pipe(pipes[i]) == -1) {
                    perror("Error al crear pipe");
                    error_pipes = 1;
                    break;
                }
            }

            if (!error_pipes) {
                // Crear procesos hijos para cada comando
                int forks_exitosos = 1;
                for (int i = 0; i < num_comandos; i++) {
                    pids_hijos[i] = fork();
                    if (pids_hijos[i] == 0) {
                        // Proceso hijo
                        
                        // Conectar stdin al pipe anterior (si no es el primer comando)
                        if (i > 0) {
                            if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                                perror("Error en dup2");
                                exit(EXIT_FAILURE);
                            }
                        }
                        
                        // Conectar stdout al pipe siguiente (si no es el último comando)
                        if (i < num_comandos - 1) {
                            if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                                perror("Error en dup2");
                                exit(EXIT_FAILURE);
                            }
                        }
                        
                        // Cerrar todos los pipes en el hijo
                        for (int j = 0; j < num_comandos - 1; j++) {
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                        
                        // Parsear y ejecutar el comando
                        char *argumentos[MAX_ARGUMENTOS];
                        char *copia_comando = malloc(strlen(comandos[i]) + 1);
                        if (copia_comando == NULL) {
                            perror("Error en malloc");
                            exit(EXIT_FAILURE);
                        }
                        strcpy(copia_comando, comandos[i]);
                        
                        int num_args = parsear_argumentos(copia_comando, argumentos);
                        if (num_args <= 0) {
                            free(copia_comando);
                            exit(EXIT_FAILURE);
                        }
                        
                        execvp(argumentos[0], argumentos);
                        fprintf(stderr, "%s: comando no encontrado\n", argumentos[0]);
                        free(copia_comando);
                        exit(EXIT_FAILURE);
                    } else if (pids_hijos[i] == -1) {
                        perror("Error en fork");
                        forks_exitosos = 0;
                        break;
                    }
                }

                // Cerrar todos los pipes en el padre
                for (int i = 0; i < num_comandos - 1; i++) {
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }

                // Esperar que todos los hijos terminen
                if (forks_exitosos) {
                    for (int i = 0; i < num_comandos; i++) {
                        if (pids_hijos[i] > 0) {
                            int estado;
                            waitpid(pids_hijos[i], &estado, 0);
                        }
                    }
                }
            }
        }

        // Liberar memoria de los comandos
        for (int i = 0; i < num_comandos; i++) {
            free(comandos[i]);
        }
    }
    
    return 0;
}