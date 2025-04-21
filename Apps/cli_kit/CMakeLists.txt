aux_source_directory(./ SOURCES)

add_library(cli_kit ${SOURCES})

target_link_libraries(cli_kit PUBLIC 
    stm32cubemx
    Libraries-includes
    Libraries-tonly-bsp
)

target_include_directories(cli_kit PUBLIC
    # Add user defined include paths
    ./
)