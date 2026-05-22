add_executable(fonts main.c)
target_link_libraries(fonts weact_epd)
pico_add_extra_outputs(fonts)
