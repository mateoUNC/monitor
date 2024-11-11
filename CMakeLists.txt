cmake_minimum_required(VERSION 3.10)
project(monitoring_project VERSION 1.0.0 DESCRIPTION "Monitor Program" LANGUAGES C)

# Incluir directorios de encabezados
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/prom/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/promhttp/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/deps
)

# Buscar paquetes necesarios
find_package(cJSON REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(MicroHTTPD REQUIRED)

# Definir las bibliotecas 'prom' y 'promhttp' como importadas
add_library(prom STATIC IMPORTED)
set_target_properties(prom PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/prometheus-client-c/prom/libprom.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/prometheus-client-c/prom/include
)

add_library(promhttp STATIC IMPORTED)
set_target_properties(promhttp PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/prometheus-client-c/promhttp/libpromhttp.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/prometheus-client-c/promhttp/include
)

# Añadir el ejecutable con el nombre "metricShell"
add_executable(metricShell
    src/main.c
    src/metrics.c
    src/expose_metrics.c
    src/config.c
)

# Enlazar las librerías
target_link_libraries(metricShell PRIVATE
    cjson::cjson
    prom
    promhttp
    CURL::libcurl
    Threads::Threads
    MicroHTTPD::microhttpd
)

# Opcional: Añadir flags de compilación para advertencias
target_compile_options(metricShell PRIVATE -Wall -Wextra -pedantic)
