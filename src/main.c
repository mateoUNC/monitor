/**
 * @file main.c
 * @brief Entry point of the system
 */

#include "config.h" // Incluir config.h
#include "expose_metrics.h"
#include <cjson/cJSON.h>
#include <fcntl.h>   // For open, O_WRONLY
#include <libgen.h>  // For dirname
#include <limits.h>  // For PATH_MAX
#include <pthread.h> // For pthread_create, pthread_join
#include <stdbool.h>
#include <sys/stat.h> // For mkfifo
#include <unistd.h>   // For write, close, sleep, readlink

/**
 * @brief Funcion que busca el disco local.
 */
const char* detect_disk() {
    FILE *fp;
    static char device[256];

    // Ejecuta el comando lsblk para obtener dispositivos
    fp = popen("lsblk -nd -o NAME | head -n 1", "r");
    if (fp == NULL) {
        perror("Error ejecutando lsblk");
        return "sda"; // Valor predeterminado
    }

    if (fgets(device, sizeof(device), fp) != NULL) {
        device[strcspn(device, "\n")] = 0; // Quita el salto de línea
    } else {
        strncpy(device, "sda", sizeof(device)); // Valor predeterminado
    }

    pclose(fp);
    return device;
}

/**
 * @brief Función principal del sistema.
 */
int main(int argc, char* argv[])
{
    config_t config;
    const char* config_path =
        (argc > 1) ? argv[1] : "config/config.json"; // Cambia el valor predeterminado si es necesario
    printf("Intentando abrir %s\n", config_path);
    if (!load_config(config_path, &config))
    {
        fprintf(stderr, "Error al cargar configuración\n");
        return EXIT_FAILURE;
    }

    init_metrics();

    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0)
    {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        return EXIT_FAILURE;
    }
    // Envia el valor de sampling interval por pipe a la shell.
    save_sampling_interval(config.sampling_interval);
    // Bucle principal para actualizar las métricas
    while (true)
    {

        if (config.collect_cpu)
        {
            update_cpu_gauge();
        }

        if (config.collect_memory)
        {
            update_memory_gauge();
        }

        if (config.collect_disk)
        {
            const char *disk = detect_disk();
            char disk_path[256];
            snprintf(disk_path, sizeof(disk_path), "/dev/%s", disk);
            update_disk_stats_gauge(disk_path);
        }

        if (config.collect_net)
        {
            update_net_stats_gauge("wlp1s0");
        }

        if (config.collect_context_switches)
        {
            update_context_switches_gauge();
        }

        if (config.collect_running_processes)
        {
            update_running_processes_gauge();
        }

        // Enviar las métricas a través del FIFO solo una vez por cada intervalo
        send_metrics(&config);

        // Dormir según el intervalo de muestreo
        sleep(config.sampling_interval);
    }
    return EXIT_SUCCESS;
}
