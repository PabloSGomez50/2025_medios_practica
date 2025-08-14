# Crear la biblioteca estática "adf4351" con los archivos fuente
add_library(adf4351 STATIC
    src/adf4351.c
)

# Linkeo dependencias de la bibliotecas
target_link_libraries(adf4351
    pico_stdlib
    hardware_spi
)

# Incluir las cabeceras de la biblioteca
target_include_directories(adf4351 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)
