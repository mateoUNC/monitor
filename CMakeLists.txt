cmake_minimum_required(VERSION 3.10)
project(monitoring_project VERSION 1.0.0 DESCRIPTION "Monitor Program" LANGUAGES C)

# Buscar paquetes necesarios
find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
find_package(cJSON REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Incluir directorios de encabezados
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

# Incluir directorios de encabezados
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/prom/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/promhttp/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/deps
    ${MICROHTTPD_INCLUDE_DIRS}
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
    ${MICROHTTPD_LIBRARIES}
)

# Opcional: Añadir flags de compilación para advertencias
target_compile_options(metricShell PRIVATE -Wall -Wextra -pedantic)
