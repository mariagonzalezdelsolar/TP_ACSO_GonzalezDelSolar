#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char **argv) {
    // Validación de argumentos --> debe recibir 3 argumentos: num_procesos, valor_inicial, proceso_inicio (4 con el nombre del programa)
    if (argc != 4) { 
        fprintf(stderr, "Uso: %s <num_procesos> <valor_inicial> <proceso_inicio>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parseo y validación de parámetros
    const int num_procesos = atoi(argv[1]);
    const int valor_inicial = atoi(argv[2]);
    const int id_proceso_inicio = atoi(argv[3]) - 1; // Para que empiezen en 0 los índices

    if (num_procesos < 3) {
        fprintf(stderr, "Error: Se necesitan al menos 3 procesos\n");
        exit(EXIT_FAILURE);
    }
    if (id_proceso_inicio < 0 || id_proceso_inicio >= num_procesos) {
        fprintf(stderr, "Error: proceso_inicio debe estar entre 1 y %d\n", num_procesos);
        exit(EXIT_FAILURE);
    }

    // Creación de los pipes para comunicación
    int pipes_comunicacion[num_procesos][2]; // pipes_comunicacion[i][0] para lectura, [1] para escritura
    for (int i = 0; i < num_procesos; i++) {
        if (pipe(pipes_comunicacion[i]) == -1) {
            perror("Error al crear pipe de comunicación");
            exit(EXIT_FAILURE);
        }
    }

    // Creación de los procesos hijos
    pid_t ids_procesos_hijos[num_procesos];
    for (int id_proceso = 0; id_proceso < num_procesos; id_proceso++) {
        ids_procesos_hijos[id_proceso] = fork();
        if (ids_procesos_hijos[id_proceso] == -1) {
            perror("Error al crear proceso hijo");
            exit(EXIT_FAILURE);
        }

        if (ids_procesos_hijos[id_proceso] == 0) { 
            // Código ejecutado solo por los procesos hijos
            
            // Determinar vecinos en el anillo
            const int id_proceso_anterior = (id_proceso - 1 + num_procesos) % num_procesos;
            const int id_proceso_siguiente = id_proceso;

            // Cerrar pipes que no se usarán en este proceso
            for (int j = 0; j < num_procesos; j++) {
                if (j != id_proceso_anterior && j != id_proceso_siguiente) {
                    close(pipes_comunicacion[j][0]); // Cerrar extremo de lectura
                    close(pipes_comunicacion[j][1]); // Cerrar extremo de escritura
                }
            }

            int valor_actual;
            if (id_proceso == id_proceso_inicio) {
                // Proceso inicial usa el valor proporcionado
                valor_actual = valor_inicial;
            } else {
                // Otros procesos reciben el valor del proceso anterior
                if (read(pipes_comunicacion[id_proceso_anterior][0], &valor_actual, sizeof(valor_actual)) <= 0) {
                    perror("Error al recibir valor del proceso anterior");
                    exit(EXIT_FAILURE);
                }
            }

            // Operación principal: incrementar el valor
            valor_actual++;

            // Enviar valor al siguiente proceso
            if (write(pipes_comunicacion[id_proceso_siguiente][1], &valor_actual, sizeof(valor_actual)) <= 0) {
                perror("Error al enviar valor al siguiente proceso");
                exit(EXIT_FAILURE);
            }

            // Cerrar pipes usados antes de terminar
            close(pipes_comunicacion[id_proceso_anterior][0]);
            close(pipes_comunicacion[id_proceso_siguiente][1]);

            exit(EXIT_SUCCESS);
        }
    }

    // Código ejecutado solo por el proceso padre
    
    // Cerrar todos los pipes excepto el necesario para leer el resultado final
    const int id_proceso_final = (id_proceso_inicio - 1 + num_procesos) % num_procesos;
    for (int i = 0; i < num_procesos; i++) {
        if (i != id_proceso_final) {
            close(pipes_comunicacion[i][0]); // Cerrar extremos de lectura no usados
        }
        close(pipes_comunicacion[i][1]); // Cerrar todos los extremos de escritura
    }

    // Leer y mostrar el resultado final
    int resultado_final;
    if (read(pipes_comunicacion[id_proceso_final][0], &resultado_final, sizeof(resultado_final)) <= 0) {
        perror("Error al recibir resultado final");
        exit(EXIT_FAILURE);
    }
    close(pipes_comunicacion[id_proceso_final][0]);

    printf("Resultado final: %d\n", resultado_final);

    // Esperar a que todos los procesos hijos terminen correctamente
    int estado_terminacion;
    for (int i = 0; i < num_procesos; i++) {
        waitpid(ids_procesos_hijos[i], &estado_terminacion, 0);
        if (WIFEXITED(estado_terminacion) && WEXITSTATUS(estado_terminacion) != EXIT_SUCCESS) {
            fprintf(stderr, "Advertencia: Proceso hijo %d terminó con código de error\n", i+1);
        }
    }

    return EXIT_SUCCESS;
}