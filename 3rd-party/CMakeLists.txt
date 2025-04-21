aux_source_directory(./linenoise SOURCES)

add_library(3rdParty ${SOURCES})

target_link_libraries(3rdParty PUBLIC stm32cubemx)

target_include_directories(3rdParty PUBLIC
    # Add user defined include paths
    ./linenoise
)