#include "expose_metrics.h"
#include "memory.h"

// Mutex para sincronización de hilos
pthread_mutex_t lock;

static prom_gauge_t* cpu_usage_metric;         // Metrica de Prometheus para el uso de cpu
static prom_gauge_t* rx_bytes_metric;          // datos recibidos
static prom_gauge_t* tx_bytes_metric;          // datos transmitidos
static prom_gauge_t* total_memory_metric;      // Metricas de Prometheus pa
static prom_gauge_t* used_memory_metric;       // Metricas de Prometheus para uso de la memoria
static prom_gauge_t* free_memory_metric;       // Metricas de Memoria Libre
static prom_gauge_t* context_switches_metric;  // Metricas para cambios de contexto
static prom_gauge_t* running_processes_metric; // Metricas para cantidad de procesos corriendo
static prom_gauge_t* disk_reads_completed_metric;
static prom_gauge_t* disk_writes_completed_metric;
// Declaración de las métricas
static prom_gauge_t* cpu_usage_metric;
static prom_gauge_t* rx_bytes_metric;
static prom_gauge_t* tx_bytes_metric;
static prom_gauge_t* memory_fragmentation_metric; // Agrega esta línea

// Esta funcion sirve para actualizar el dato desde /proc/stat para obtener el ultimo valor de Cpu_Usage
void update_cpu_gauge()
{
    double usage = get_cpu_usage();
    if (usage >= 0)
    {
        pthread_mutex_lock(&lock); // Previene condiciones de carrera y asegura la integridad de los datos.
        prom_gauge_set(cpu_usage_metric, usage, NULL);
        pthread_mutex_unlock(&lock); // Libero mutex lock
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso de CPU\n");
    }
}

void update_running_processes_gauge()
{
    int running_procs = get_running_processes();
    if (running_procs >= 0)
    {
        pthread_mutex_lock(&lock); // Previene condiciones de carrera y asegura la integridad de los datos.
        prom_gauge_set(running_processes_metric, running_procs, NULL);
        pthread_mutex_unlock(&lock); // Libero mutex lock
    }
    else
    {
        fprintf(stderr, "Error al obtener el número de procesos en ejecución\n");
    }
}

void update_net_stats_gauge(const char* iface)
{
    net_stats_t stats = get_net_stats(iface);

    if (stats.rx_bytes >= 0 && stats.tx_bytes >= 0)
    {
        pthread_mutex_lock(&lock);

        // Actualizar las métricas con los valores obtenidos
        prom_gauge_set(rx_bytes_metric, stats.rx_bytes, NULL);
        prom_gauge_set(tx_bytes_metric, stats.tx_bytes, NULL);

        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error al obtener estadísticas de red para la interfaz: %s\n", iface);
    }
}

void update_disk_stats_gauge(const char* device)
{
    disk_stats_t stats = get_disk_stats(device);

    if (stats.reads_completed != (unsigned long)-1 && stats.writes_completed != (unsigned long)-1)
    {
        pthread_mutex_lock(&lock);

        // Actualizar las métricas con los valores obtenidos
        prom_gauge_set(disk_reads_completed_metric, (double)stats.reads_completed, NULL);
        prom_gauge_set(disk_writes_completed_metric, (double)stats.writes_completed, NULL);

        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error al obtener estadísticas de disco para el dispositivo: %s\n", device);
    }
}

void update_context_switches_gauge()
{
    long long ctxt = get_context_switches();
    if (ctxt >= 0)
    {
        pthread_mutex_lock(&lock);
        prom_gauge_set(context_switches_metric, (double)ctxt, NULL);
        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error al obtener el número de cambios de contexto\n");
    }
}

void update_memory_gauge()
{
    memory_info_t memory_info = get_memory_usage(); // Llama a get_memory_usage()

    if (memory_info.total_mem >= 0)
    {
        pthread_mutex_lock(&lock);

        // Actualiza las métricas en Prometheus con los valores obtenidos
        prom_gauge_set(total_memory_metric, memory_info.total_mem, NULL);
        prom_gauge_set(used_memory_metric, memory_info.used_mem, NULL);
        prom_gauge_set(free_memory_metric, memory_info.free_mem, NULL);

        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error al obtener la información de memoria\n");
    }
}

void update_memory_fragmentation_gauge() {
    double fragmentation = calculate_memory_fragmentation();
    pthread_mutex_lock(&lock);
    prom_gauge_set(memory_fragmentation_metric, fragmentation, NULL);
    pthread_mutex_unlock(&lock);
}



void* expose_metrics(void* arg)
{
    (void)arg; // Argumento no utilizado

    // Aseguramos que el manejador HTTP esté adjunto al registro por defecto
    promhttp_set_active_collector_registry(NULL);

    // Iniciamos el servidor HTTP en el puerto 8000
    struct MHD_Daemon* daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, 8000, NULL, NULL);
    if (daemon == NULL)
    {
        fprintf(stderr, "Error al iniciar el servidor HTTP\n");
        return NULL;
    }

    // Mantenemos el servidor en ejecución
    while (1)
    {
        sleep(1); // Mantiene el hilo vivo
    }

    // Nunca debería llegar aquí
    MHD_stop_daemon(daemon);
    return NULL;
}

void init_metrics()
{
    // Inicializa el mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "Error al inicializar el mutex\n");
        return;
    }

    // Inicializamos el registro de coleccionistas de Prometheus
    if (prom_collector_registry_default_init() != 0)
    {
        fprintf(stderr, "Error al inicializar el registro de Prometheus\n");
        return;
    }

    // Creamos la métrica para el uso de CPU
    cpu_usage_metric = prom_gauge_new("cpu_usage_percentage", "Porcentaje de uso de CPU", 0, NULL);
    if (cpu_usage_metric == NULL)
    {
        fprintf(stderr, "Error al crear la métrica de uso de CPU\n");
        return;
    }

    // Creamos nuevas métricas para memoria total, usada y disponible
    total_memory_metric = prom_gauge_new("total_memory_mb", "Memoria total en MB", 0, NULL);
    used_memory_metric = prom_gauge_new("used_memory_mb", "Memoria usada en MB", 0, NULL);
    free_memory_metric = prom_gauge_new("free_memory_mb", "Memoria disponible en MB", 0, NULL);

    if (total_memory_metric == NULL || used_memory_metric == NULL || free_memory_metric == NULL)
    {
        fprintf(stderr, "Error al crear las métricas de memoria\n");
        return;
    }

    // Registramos las métricas en el registro por defecto
    if (prom_collector_registry_must_register_metric(cpu_usage_metric) == 0 ||
        prom_collector_registry_must_register_metric(total_memory_metric) == 0 ||
        prom_collector_registry_must_register_metric(used_memory_metric) == 0 ||
        prom_collector_registry_must_register_metric(free_memory_metric) == 0)
    {
        fprintf(stderr, "Error al registrar las métricas\n");
        return;
    }
    // Crear métricas para las estadísticas de disco
    disk_reads_completed_metric =
        prom_gauge_new("disk_reads_completed", "Numero de lecturas completadas exitosamente", 0, NULL);
    disk_writes_completed_metric =
        prom_gauge_new("disk_writes_completed", "Numero de escrituras completadas exitosamente", 0, NULL);

    if (disk_reads_completed_metric == NULL || disk_writes_completed_metric == NULL)
    {
        fprintf(stderr, "Error al crear las métricas de estadísticas de disco\n");
        return;
    }

    if (prom_collector_registry_must_register_metric(disk_reads_completed_metric) == 0 ||
        prom_collector_registry_must_register_metric(disk_writes_completed_metric) == 0)
    {
        fprintf(stderr, "Error al registrar las métricas de estadísticas de disco\n");
        return;
    }

    rx_bytes_metric = prom_gauge_new("rx_bytes", "Bytes recibidos por la interfaz de red", 0, NULL);
    tx_bytes_metric = prom_gauge_new("tx_bytes", "Bytes transmitidos por la interfaz de red", 0, NULL);

    if (rx_bytes_metric == NULL || tx_bytes_metric == NULL)
    {
        fprintf(stderr, "Error al crear las métricas de tráfico de red\n");
        return;
    }

    if (prom_collector_registry_must_register_metric(rx_bytes_metric) == 0 ||
        prom_collector_registry_must_register_metric(tx_bytes_metric) == 0)
    {
        fprintf(stderr, "Error al registrar las métricas de tráfico de red\n");
        return;
    }
    // Metrica para cantidad de procesos en ejecucion.
    running_processes_metric =
        prom_gauge_new("running_processes", "Numero de procesos en ejecución en el sistema", 0, NULL);
    if (running_processes_metric == NULL)
    {
        fprintf(stderr, "Error al crear la métrica de procesos en ejecución\n");
        return;
    }

    // Registrar la métrica
    if (prom_collector_registry_must_register_metric(running_processes_metric) == 0)
    {
        fprintf(stderr, "Error al registrar la métrica de procesos en ejecución\n");
        return;
    }

    context_switches_metric =
        prom_gauge_new("custom_context_switches_per_second", "Tasa de cambios de contexto por segundo", 0, NULL);
    if (context_switches_metric == NULL)
    {
        fprintf(stderr, "Error al crear la métrica de cambios de contexto\n");
        return;
    }

    if (prom_collector_registry_must_register_metric(context_switches_metric) == 0)
    {
        fprintf(stderr, "Error al registrar la métrica de cambios de contexto\n");
        return;
    }

    memory_fragmentation_metric = prom_gauge_new("heap_memory_fragmentation_percentage", "Fragmentacion del heap en porcentaje", 0, NULL);
    if (memory_fragmentation_metric == NULL) {
        fprintf(stderr, "Error al crear la métrica de fragmentación de memoria\n");
        return;
    }

    // Registrar la métrica de fragmentación de memoria
    if (prom_collector_registry_must_register_metric(memory_fragmentation_metric) == 0) {
        fprintf(stderr, "Error al registrar la métrica de fragmentación de memoria\n");
        return;
    }
}


void destroy_mutex()
{
    pthread_mutex_destroy(&lock);
}
