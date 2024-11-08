/**
 * @file config.h
 * @brief Funciones para cargar configuraciones segun el config.json
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
/**
 * @brief Estructura de informacion de monitor.
 */
typedef struct {
    int sampling_interval;          /**< Intervalo de muestreo en segundos */
    bool collect_cpu;               /**< Recopila información de CPU si es true */
    bool collect_memory;            /**< Recopila información de memoria si es true */
    bool collect_disk;              /**< Recopila información de disco si es true */
    bool collect_net;               /**< Recopila información de red si es true */
    bool collect_context_switches;  /**< Recopila cambios de contexto si es true */
    bool collect_running_processes; /**< Recopila número de procesos activos si es true */
    char log_file[256];             /**< Ruta al archivo de log */
} config_t;

/**
 * @brief Carga la configuración desde un archivo JSON.
 *
 * Esta función abre el archivo JSON especificado, lo analiza y carga la configuración 
 * en la estructura `config_t`, asignando valores predeterminados si algunas opciones no están presentes.
 *
 * @param filename Nombre del archivo de configuración JSON.
 * @param config Puntero a la estructura `config_t` donde se almacenará la configuración cargada.
 * @return `true` si la configuración se carga correctamente, `false` en caso de error.
 */
bool load_config(const char* filename, config_t* config);

/**
 * @brief Envía las métricas actuales a través de un FIFO.
 *
 * Construye un objeto JSON con las métricas especificadas en `config` y lo envía a un FIFO.
 * Las métricas se recopilan según las opciones activadas en `config`.
 *
 * @param config Puntero a la estructura `config_t` que contiene las métricas a enviar.
 */
void send_metrics(config_t* config);

/**
 * @brief Guarda el intervalo de muestreo en un archivo.
 *
 * Abre o crea el archivo `/tmp/sampling_interval.txt` y escribe el intervalo de muestreo especificado.
 *
 * @param interval Intervalo de muestreo en segundos que se desea guardar.
 */
void save_sampling_interval(int interval);

#endif // CONFIG_H
