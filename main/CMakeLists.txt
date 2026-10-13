idf_component_register(
    SRCS "range_finder.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer esp_driver_gpio esp_driver_tsens
)
