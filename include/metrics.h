/**
 * @file metrics.h
 * @brief Funciones para obtener el uso de CPU y memoria desde el sistema de archivos /proc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Tamaño del buffer utilizado para leer datos del sistema.
 */
#define BUFFER_SIZE 256
#define BUFFER_SIZE 256

/**
 * @brief Estructura para almacenar la información de la memoria.
 */
typedef struct
{
    double total_mem; /**< Memoria total en MB. */
    double used_mem;  /**< Memoria usada en MB. */
    double free_mem;  /**< Memoria disponible en MB. */
} memory_info_t;

/**
 * @brief Estructura para almacenar las estadísticas de disco.
 */
typedef struct
{
    char device[32];                /**< Nombre del dispositivo (por ejemplo, "sda"). */
    unsigned long reads_completed;  /**< Número de lecturas completadas exitosamente. */
    unsigned long writes_completed; /**< Número de escrituras completadas exitosamente. */
} disk_stats_t;

/**
 * @brief Obtiene las estadísticas de disco desde /proc/diskstats.
 *
 * @param device Nombre del dispositivo (por ejemplo, "sda").
 * @return Estructura disk_stats_t con los datos del dispositivo, o con reads_completed = -1 en caso de error.
 */
disk_stats_t get_disk_stats(const char* device);

/**
 * @brief Estructura para almacenar las estadísticas de red.
 */
typedef struct
{
    char iface[16];     /**< Nombre de la interfaz de red (por ejemplo, eth0). */
    long long rx_bytes; /**< Bytes recibidos por la interfaz. */
    long long tx_bytes; /**< Bytes transmitidos por la interfaz. */
} net_stats_t;

/**
 * @brief Lee las estadísticas de red desde /proc/net/dev.
 *
 * @param iface Nombre de la interfaz de red.
 * @return Estructura net_stats_t con los datos de la interfaz, o {-1, -1} en caso de error.
 */
net_stats_t get_net_stats(const char* iface);

/**
 * @brief Obtiene el número de cambios de contexto desde /proc/stat.
 *
 * @return Número de cambios de contexto, o -1 en caso de error.
 */
long long get_context_switches(void);

/**
 * @brief Obtiene el número de procesos en ejecución desde /proc/stat.
 *
 * @return Número de procesos en ejecución, o -1 en caso de error.
 */
int get_running_processes(void);

/**
 * @brief Obtiene los datos de uso de memoria desde /proc/meminfo.
 *
 * @return Estructura memory_info_t con los valores de memoria total, usada y disponible en MB,
 *         o {-1.0, -1.0, -1.0} en caso de error.
 */
memory_info_t get_memory_usage(void);

/**
 * @brief Obtiene el porcentaje de uso de CPU desde /proc/stat.
 *
 * @return Uso de CPU como porcentaje (0.0 a 100.0), o -1.0 en caso de error.
 */
double get_cpu_usage(void);
