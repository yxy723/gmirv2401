idf_component_register(SRCS "src/gmirv2401.c"
                            "src/gmirv2401_ctrl.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_uart
                                esp_driver_gpio
                                freertos
                                log
                                esp_timer)
