add_executable(tetris main.c)
target_link_libraries(tetris weact_epd)
pico_add_extra_outputs(tetris)
