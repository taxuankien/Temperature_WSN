set(srcs "main.c"
        "../src/mesh_server.c"
        "../src/ds18b20.c"
        "../src/LCD1602.c")

idf_component_register(SRCS "${srcs}"
                        REQUIRES bt
                        REQUIRES driver
                        REQUIRES nvs_flash
                        PRIV_REQUIRES   esp_timer
                        INCLUDE_DIRS  "." "../include")
