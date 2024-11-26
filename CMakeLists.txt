cmake_minimum_required(VERSION 3.10)
project(monitoring_project VERSION 1.0.0 DESCRIPTION "Monitor Program" LANGUAGES C)

# Buscar paquetes necesarios
find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
find_package(cJSON REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Añadir el ejecutable con el nombre "metricShell"
add_executable(metricShell
    src/main.c
    src/metrics.c
    src/expose_metrics.c
    src/config.c
)

# Agregar la biblioteca de memoria
# Supongamos que la ruta relativa es correcta

# Enlazar las librerías
target_link_libraries(metricShell PRIVATE
    cjson::cjson
    prom
    promhttp
    CURL::libcurl
    Threads::Threads
    ${MICROHTTPD_LIBRARIES}
    memory # Asegurarse de que 'memory_lib' es el nombre correcto de la biblioteca
)

# Especificar directorios de inclusión para 'metricShell'
target_include_directories(metricShell PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/prom/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/promhttp/include
    ${CMAKE_SOURCE_DIR}/prometheus-client-c/deps
    ${MICROHTTPD_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/memory/include  # Incluir encabezados de memoria
)

# Asegurarse de que las rutas relativas son correctas
