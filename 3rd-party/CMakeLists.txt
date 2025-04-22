aux_source_directory(./linenoise SOURCES)

add_library(3rd-party ${SOURCES})

target_link_libraries(3rd-party PUBLIC stm32cubemx)

target_include_directories(3rd-party PUBLIC
    # Add user defined include paths
    ./linenoise
)