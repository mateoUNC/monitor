// config.c
#include "config.h"
#include "expose_metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/metrics_fifo"

// Cargar configuración desde config.json usando cJSON
bool load_config(const char* filename, config_t* config) {
    FILE* file = fopen(filename, "r");
    printf("Intentando abrir %s\n", filename);
    if (!file) {
        fprintf(stderr, "Error al abrir el archivo %s\n", filename);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_contents = (char*)malloc(file_size + 1);
    fread(file_contents, 1, file_size, file);
    file_contents[file_size] = '\0';
    fclose(file);

    cJSON* root = cJSON_Parse(file_contents);
    free(file_contents);
    if (!root) {
        fprintf(stderr, "Error al parsear JSON\n");
        return false;
    }

    // Obtener el intervalo de muestreo
    cJSON* interval = cJSON_GetObjectItem(root, "sampling_interval");
    if (cJSON_IsNumber(interval)) {
        config->sampling_interval = interval->valueint;
        printf("Intervalo de muestreo: %d\n", config->sampling_interval);
    } else {
        config->sampling_interval = 1; // Valor predeterminado en caso de error
        printf("Intervalo de muestreo no encontrado, usando valor predeterminado: 1\n");
    }

    // Inicializar todas las métricas en falso
    config->collect_cpu = config->collect_memory = config->collect_disk = config->collect_net = false;
    config->collect_context_switches = config->collect_running_processes = false;

    // Obtener la lista de métricas
    cJSON* metrics = cJSON_GetObjectItem(root, "metrics");
    if (cJSON_IsArray(metrics)) {
        int metrics_count = cJSON_GetArraySize(metrics);
        printf("Número de métricas: %d\n", metrics_count);
        for (int i = 0; i < metrics_count; i++) {
            cJSON* metric = cJSON_GetArrayItem(metrics, i);
            if (cJSON_IsString(metric)) {
                printf("Leyendo métrica: %s\n", metric->valuestring);
                if (strcmp(metric->valuestring, "cpu") == 0) {
                    config->collect_cpu = true;
                } else if (strcmp(metric->valuestring, "memory") == 0) {
                    config->collect_memory = true;
                } else if (strcmp(metric->valuestring, "disk") == 0) {
                    config->collect_disk = true;
                } else if (strcmp(metric->valuestring, "net") == 0) {
                    config->collect_net = true;
                } else if (strcmp(metric->valuestring, "context_switches") == 0) {
                    config->collect_context_switches = true;
                } else if (strcmp(metric->valuestring, "running_processes") == 0) {
                    config->collect_running_processes = true;
                }
            } else {
                printf("Métrica en formato incorrecto\n");
            }
        }
    } else {
        printf("La lista de métricas no es un array o está ausente\n");
    }

    // Obtener el archivo de log (opcional)
    cJSON* log_file = cJSON_GetObjectItem(root, "log_file");
    if (cJSON_IsString(log_file)) {
        strncpy(config->log_file, log_file->valuestring, sizeof(config->log_file) - 1);
        config->log_file[sizeof(config->log_file) - 1] = '\0';
        printf("Archivo de log: %s\n", config->log_file);
    } else {
        printf("Archivo de log no especificado\n");
    }

    // Mostrar los estados de las métricas después de la carga
    printf("Estados de métricas:\n");
    printf("  CPU: %s\n", config->collect_cpu ? "Activado" : "Desactivado");
    printf("  Memoria: %s\n", config->collect_memory ? "Activado" : "Desactivado");
    printf("  Disco: %s\n", config->collect_disk ? "Activado" : "Desactivado");
    printf("  Red: %s\n", config->collect_net ? "Activado" : "Desactivado");
    printf("  Cambios de contexto: %s\n", config->collect_context_switches ? "Activado" : "Desactivado");
    printf("  Procesos en ejecución: %s\n", config->collect_running_processes ? "Activado" : "Desactivado");

    cJSON_Delete(root);
    return true;
}


// Función para construir y enviar métricas a través del FIFO
void send_metrics(config_t* config) {
    // Crear el JSON con las métricas usando cJSON
    cJSON *root = cJSON_CreateObject();
    if (config->collect_cpu) {
        cJSON_AddNumberToObject(root, "cpu_usage", get_cpu_usage());
    }
    if (config->collect_memory) {
        memory_info_t mem_info = get_memory_usage();
        cJSON_AddNumberToObject(root, "total_memory", mem_info.total_mem);
        cJSON_AddNumberToObject(root, "used_memory", mem_info.used_mem);
        cJSON_AddNumberToObject(root, "free_memory", mem_info.free_mem);
    }
    if (config->collect_disk) {
        disk_stats_t disk_info = get_disk_stats("nvme0n1");
        cJSON_AddNumberToObject(root, "disk_reads", disk_info.reads_completed);
        cJSON_AddNumberToObject(root, "disk_writes", disk_info.writes_completed);
    }
    if (config->collect_net) {
        net_stats_t net_info = get_net_stats("wlp1s0");
        cJSON_AddNumberToObject(root, "rx_bytes", net_info.rx_bytes);
        cJSON_AddNumberToObject(root, "tx_bytes", net_info.tx_bytes);
    }
    if (config->collect_context_switches) {
        cJSON_AddNumberToObject(root, "context_switches", get_context_switches());
    }
    if (config->collect_running_processes) {
        cJSON_AddNumberToObject(root, "running_processes", get_running_processes());
    }

    // Convertir el JSON a una cadena
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);

    // Abrir el FIFO en modo escritura (crearlo si no existe)
    int fifo_fd = open(FIFO_PATH, O_WRONLY | O_CREAT, 0666);
    if (fifo_fd == -1) {
        perror("Error al abrir el FIFO");
        free(json_string);
        return;
    }

    // Enviar la cadena JSON a través del FIFO
    write(fifo_fd, json_string, strlen(json_string));
    close(fifo_fd);
    free(json_string); // Liberar la cadena JSON
}


void save_sampling_interval(int interval) {
    FILE *file = fopen("/tmp/sampling_interval.txt", "w");
    if (file != NULL) {
        fprintf(file, "%d\n", interval);
        fclose(file);
    } else {
        perror("Error al abrir el archivo para guardar el intervalo de muestreo");
    }
}
