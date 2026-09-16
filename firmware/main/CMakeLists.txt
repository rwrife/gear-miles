idf_component_register(
    SRCS
        "main.c"
        "board.c"
        "bus_spi_panel.c"
        "buttons.c"
        "storage_nvs.c"
        "storage_ring.c"
        "ui.c"
        "http_server.c"
        "wifi_setup.c"
    INCLUDE_DIRS "."
    REQUIRES domain panel_hal esp_driver_gpio esp_driver_spi esp_wifi
             esp_http_server nvs_flash esp_timer esp_partition
             esp_event esp_netif mdns
)
