// main.c
#include "config.h" // Incluir config.h
#include "expose_metrics.h"
#include "memory.h" // Incluir memory.h
#include <cjson/cJSON.h>
#include <fcntl.h>   // For open, O_WRONLY
#include <libgen.h>  // For dirname
#include <limits.h>  // For PATH_MAX
#include <pthread.h> // For pthread_create, pthread_join
#include <stdbool.h>
#include <sys/stat.h> // For mkfifo
#include <unistd.h>   // For write, close, sleep, readlink>


const char* detect_disk() {
    static char device[256];
    FILE *fp;

    // Ejecuta el comando lsblk para obtener el nombre del disco
    fp = popen("lsblk -nd -o NAME | head -n 1", "r");
    if (fp == NULL) {
        perror("Error ejecutando lsblk");
        return "sda"; // Valor predeterminado
    }

    if (fgets(device, sizeof(device), fp) != NULL) {
        device[strcspn(device, "\n")] = 0; // Eliminar salto de línea
    } else {
        strncpy(device, "sda", sizeof(device));
    }

    pclose(fp);
    return device;
}
/**
 * @brief Inicializa la configuración con valores predeterminados.
 */
void init_default_config(config_t* config) {
    config->sampling_interval = 1; // Valor predeterminado
    config->collect_cpu = true;
    config->collect_memory = true;  // Activamos memoria
    config->collect_memory_fragmentation = true; // Activamos fragmentación
    config->collect_disk = false;
    config->collect_net = false;
    config->collect_context_switches = false;
    config->collect_running_processes = false;
    config->allocation_method = FIRST_FIT; // Método predeterminado
    strncpy(config->log_file, "/tmp/metrics.log", sizeof(config->log_file) - 1);
    config->log_file[sizeof(config->log_file) - 1] = '\0';

    printf("Configuración predeterminada cargada:\n");
    printf("  Intervalo de muestreo: %d\n", config->sampling_interval);
    printf("  CPU: %s\n", config->collect_cpu ? "Activado" : "Desactivado");
    printf("  Memoria: %s\n", config->collect_memory ? "Activado" : "Desactivado");
    printf("  Fragmentación de memoria: %s\n", config->collect_memory_fragmentation ? "Activado" : "Desactivado");
    printf("  Disco: %s\n", config->collect_disk ? "Activado" : "Desactivado");
    printf("  Red: %s\n", config->collect_net ? "Activado" : "Desactivado");
    printf("  Cambios de contexto: %s\n", config->collect_context_switches ? "Activado" : "Desactivado");
    printf("  Procesos en ejecución: %s\n", config->collect_running_processes ? "Activado" : "Desactivado");
    printf("  Archivo de log: %s\n", config->log_file);
}

/**
 * @brief Función principal del sistema.
 */
int main(int argc, char* argv[]) {
    config_t config;

    // Determinar la ruta de config.json
    const char* config_path = (argc > 1) ? argv[1] : "config/config.json";
    printf("Intentando abrir %s\n", config_path);

    // Intentar cargar config.json; si falla, usar configuración predeterminada
    if (!load_config(config_path, &config)) {
        fprintf(stderr, "Error al cargar configuración. Usando valores predeterminados.\n");
        init_default_config(&config);
    }

    // Configurar el método de asignación de memoria
    malloc_control(config.allocation_method);

    init_metrics();

    // Inicializar logger si es necesario
    if (config.log_file[0] != '\0') {
        initialize_logger(config.log_file);
    }

    // Crear hilo para el servidor HTTP si es necesario
    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0) {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        return EXIT_FAILURE;
    }

    // Enviar el valor de sampling interval por pipe a la shell
    save_sampling_interval(config.sampling_interval);

    // Bucle principal para actualizar las métricas
    while (true) {
        // Simular actividad de memoria si la fragmentación de memoria está activada
        if (config.collect_memory_fragmentation) {
            simulate_memory_activity();
            update_memory_fragmentation_gauge();
        }

        if (config.collect_cpu) {
            update_cpu_gauge();
        }

        if (config.collect_memory) {
            update_memory_gauge();
        }

        if (config.collect_disk) {
            const char *disk = detect_disk();
            char disk_path[256];
            snprintf(disk_path, sizeof(disk_path), "/dev/%s", disk);
            update_disk_stats_gauge(disk_path);
        }

        if (config.collect_net) {
            update_net_stats_gauge("wlp1s0");
        }

        if (config.collect_context_switches) {
            update_context_switches_gauge();
        }

        if (config.collect_running_processes) {
            update_running_processes_gauge();
        }

        // Enviar las métricas a través del FIFO
        send_metrics(&config);

        // Dormir según el intervalo de muestreo
        sleep(config.sampling_interval);
    }

    finalize_logger();
    return EXIT_SUCCESS;
}
