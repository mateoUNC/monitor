#include "metrics.h"
#include <ctype.h>
#include <fcntl.h> // Para open
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
// obtenemos los datos de la red de forma parecida a como obtubimos los datos de memoria.
extern pthread_mutex_t lock; // Asegúrate de que el mutex sea accesible

#define BUFFER_SIZE 256

net_stats_t get_net_stats(const char* iface)
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    net_stats_t stats = {0};
    stats.rx_bytes = -1;
    stats.tx_bytes = -1;

    // Abrir /proc/net/dev
    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/net/dev");
        return stats; // Error
    }

    // Saltar las dos primeras líneas (encabezado)
    fgets(buffer, sizeof(buffer), fp);
    fgets(buffer, sizeof(buffer), fp);

    // Leer cada línea del archivo
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        char* ptr = buffer;
        char* token;
        char* saveptr;

        // Obtener el nombre de la interfaz
        token = strtok_r(ptr, " \t\n", &saveptr);
        if (token == NULL)
            continue;

        // Quitar ':' del nombre de la interfaz
        char* colon = strchr(token, ':');
        if (colon)
            *colon = '\0';

        // Quitar espacios en blanco iniciales
        while (isspace(*token))
            token++;

        if (strcmp(token, iface) == 0)
        {
            unsigned long long values[16];
            int i = 0;
            while (i < 16 && (token = strtok_r(NULL, " \t\n", &saveptr)) != NULL)
            {
                values[i++] = strtoull(token, NULL, 10);
            }
            if (i >= 16)
            {
                stats.rx_bytes = values[0];
                stats.tx_bytes = values[8];
                snprintf(stats.iface, sizeof(stats.iface), "%s", iface);
            }
            else
            {
                fprintf(stderr, "Error al leer los campos para la interfaz %s\n", iface);
            }
            break;
        }
    }

    fclose(fp);

    if (stats.rx_bytes == -1 && stats.tx_bytes == -1)
    {
        fprintf(stderr, "No se encontraron datos para la interfaz %s\n", iface);
    }

    return stats;
}

disk_stats_t get_disk_stats(const char* device)
{
    FILE* fp = fopen("/proc/diskstats", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/diskstats");
        disk_stats_t error_stats = {0};
        strncpy(error_stats.device, device, sizeof(error_stats.device) - 1);
        error_stats.reads_completed = (unsigned long)-1;
        error_stats.writes_completed = (unsigned long)-1;
        return error_stats;
    }

    char buffer[BUFFER_SIZE];
    disk_stats_t stats = {0};
    strncpy(stats.device, device, sizeof(stats.device) - 1);

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        unsigned int major_num, minor_num;
        char dev_name[32];
        unsigned long reads_completed, writes_completed;

        int items = sscanf(buffer, "%u %u %31s %lu %*s %*s %*s %lu", &major_num, &minor_num, dev_name, &reads_completed,
                           &writes_completed);

        if (items >= 5 && strcmp(dev_name, device) == 0)
        {
            stats.reads_completed = reads_completed;
            stats.writes_completed = writes_completed;
            fclose(fp);
            return stats;
        }
    }

    fclose(fp);
    // Si no se encontró el dispositivo
    fprintf(stderr, "No se encontraron datos para el dispositivo %s\n", device);
    stats.reads_completed = (unsigned long)-1;
    stats.writes_completed = (unsigned long)-1;
    return stats;
}

int get_running_processes()
{
    FILE* fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/stat");
        return -1;
    }

    char buffer[256];
    int running_procs = -1;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (strncmp(buffer, "procs_running", 13) == 0)
        {
            sscanf(buffer, "procs_running %d", &running_procs);
            break;
        }
    }

    fclose(fp);

    if (running_procs == -1)
    {
        fprintf(stderr, "No se pudo encontrar 'procs_running' en /proc/stat\n");
    }

    return running_procs;
}

long long get_context_switches()
{
    FILE* fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/stat");
        return -1;
    }

    char buffer[256];
    long long ctxt = -1;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (strncmp(buffer, "ctxt", 4) == 0)
        {
            if (sscanf(buffer, "ctxt %lld", &ctxt) != 1)
            {
                fprintf(stderr, "Error al analizar 'ctxt' en /proc/stat\n");
                ctxt = -1;
            }
            break;
        }
    }

    fclose(fp);

    if (ctxt == -1)
    {
        fprintf(stderr, "No se pudo encontrar 'ctxt' en /proc/stat\n");
    }

    return ctxt;
}
// Estructura para almacenar los datos de memoria
memory_info_t get_memory_usage()
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long total_mem = 0, free_mem = 0;

    // Abrir el archivo /proc/meminfo
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/meminfo");
        return (memory_info_t){-1.0, -1.0, -1.0}; // Devolver error
    }

    // Leer los valores de memoria total y disponible
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %llu kB", &total_mem) == 1)
        {
            continue; // MemTotal encontrado
        }
        if (sscanf(buffer, "MemAvailable: %llu kB", &free_mem) == 1)
        {
            break; // MemAvailable encontrado
        }
    }

    fclose(fp);

    // Verificar si se encontraron ambos valores
    if (total_mem == 0 || free_mem == 0)
    {
        fprintf(stderr, "Error al leer la información de memoria desde /proc/meminfo\n");
        return (memory_info_t){-1.0, -1.0, -1.0}; // Devolver error
    }

    // Calcular la memoria usada
    double used_mem = total_mem - free_mem;

    // Devolver los valores de memoria total, usada y disponible (convertidos a MB)
    memory_info_t memory_info = {
        .total_mem = (double)total_mem / 1024.0, // Convertir de kB a MB
        .used_mem = (double)used_mem / 1024.0,   // Convertir de kB a MB
        .free_mem = (double)free_mem / 1024.0    // Convertir de kB a MB
    };

    return memory_info;
}

double get_cpu_usage()
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long user1, nice1, system1, idle1, iowait1, irq1, softirq1, steal1;
    unsigned long long user2, nice2, system2, idle2, iowait2, irq2, softirq2, steal2;

    // Primera lectura
    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/stat");
        return -1.0;
    }
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        perror("Error al leer /proc/stat");
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    if (sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user1, &nice1, &system1, &idle1, &iowait1, &irq1,
               &softirq1, &steal1) < 4)
    {
        fprintf(stderr, "Error al analizar /proc/stat en la primera lectura\n");
        return -1.0;
    }

    // Esperar un intervalo
    usleep(100000); // 100 milisegundos

    // Segunda lectura
    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/stat");
        return -1.0;
    }
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        perror("Error al leer /proc/stat");
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    if (sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user2, &nice2, &system2, &idle2, &iowait2, &irq2,
               &softirq2, &steal2) < 4)
    {
        fprintf(stderr, "Error al analizar /proc/stat en la segunda lectura\n");
        return -1.0;
    }

    // Calcular los totales
    unsigned long long total1 = user1 + nice1 + system1 + idle1 + iowait1 + irq1 + softirq1 + steal1;
    unsigned long long total2 = user2 + nice2 + system2 + idle2 + iowait2 + irq2 + softirq2 + steal2;

    unsigned long long totald = total2 - total1;
    unsigned long long idled = (idle2 + iowait2) - (idle1 + iowait1);

    if (totald == 0)
    {
        fprintf(stderr, "Totald es cero, no se puede calcular el uso de CPU!\n");
        return -1.0;
    }

    // Calcular el uso de CPU
    double cpu_usage = (1.0 - ((double)idled / (double)totald)) * 100.0;
    return cpu_usage;
}
