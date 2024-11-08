cmake_minimum_required(VERSION 3.10)
project(monitoring_project VERSION 1.0.0 DESCRIPTION "Monitor Program" LANGUAGES C)

# Incluir directorios de encabezados
include_directories(${CMAKE_SOURCE_DIR}/monitor/include)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Añadir el ejecutable con el nombre "metricShell"
add_executable(metricShell
    src/main.c
    src/metrics.c
    src/expose_metrics.c
    src/config.c    
)

# Enlazar las librerías gestionadas por Conan y las librerías locales
target_link_libraries(metricShell PRIVATE cjson::cjson /usr/local/lib/libprom.so /usr/local/lib/libpromhttp.so)

# Opcional: Añadir flags de compilación para advertencias
target_compile_options(metricShell PRIVATE -Wall -Wextra -pedantic)
