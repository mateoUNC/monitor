/**
 * @file expose_metrics.h
 * @brief Programa para leer el uso de CPU y memoria y exponerlos como métricas de Prometheus.
 */

#include "metrics.h"
#include <errno.h>
#include <prom.h>
#include <promhttp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Para sleep
#include <cjson/cJSON.h>
#include <config.h>

/**
 * @brief Tamaño del buffer utilizado para leer datos del sistema.
 */
#define BUFFER_SIZE 256
/**
 * @brief Actualiza la métrica de uso de CPU.
 */
void update_cpu_gauge(void);

/**
 * @brief Actualiza las métricas de estadísticas de disco para el dispositivo dado.
 * @param device El dispositivo de disco (por ejemplo, "sda") para el cual se actualizarán las métricas.
 */
void update_disk_stats_gauge(const char* device);

/**
 * @brief Actualiza la métrica de uso de memoria.
 */
void update_memory_gauge(void);

/**
 * @brief Actualiza la métrica de cambios de contexto.
 */
void update_context_switches_gauge(void);
/**
 * @brief Obtiene la metrica de numero de procesos.
 */
void update_running_processes_gauge(void);

/**
 * @brief Actualiza las métricas de tráfico de red para la interfaz dada.
 * @param iface La interfaz de red (por ejemplo, "eth0", "wlp1s0") para la cual se actualizarán las métricas.
 */
void update_net_stats_gauge(const char* iface);
/**
 * @brief Función del hilo para exponer las métricas vía HTTP en el puerto 8000.
 * @param arg Argumento no utilizado.
 * @return NULL
 */
void* expose_metrics(void* arg);

/**
 * @brief Inicializar mutex y métricas.
 */
void init_metrics();

/**
 * @brief Destructor de mutex
 */
void destroy_mutex(void);
