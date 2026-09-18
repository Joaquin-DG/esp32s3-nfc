idf_component_register(SRCS "nfc.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_client
                        mbedtls
                        esp_wifi
                        esp_event
                        nvs_flash
                        esp_netif
                        esp_driver_gpio 
                        esp_driver_uart 
                        espressif__esp-idf-cxx 
                        jef-sure__pn532
                    )

