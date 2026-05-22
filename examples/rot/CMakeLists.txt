add_executable(rot main.c)
target_link_libraries(rot weact_epd)
pico_add_extra_outputs(rot)
