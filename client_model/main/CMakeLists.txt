set(srcs "main.c"
        "board.c"
        "../src/mesh_client.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  "." "../include")
