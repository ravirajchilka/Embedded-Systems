idf_component_register(
    SRCS "hello_world_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio freertos
)
