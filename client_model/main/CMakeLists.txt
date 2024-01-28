set(srcs "main.c"
			"../src/mesh_client.c")

idf_component_register(	SRCS "${srcs}"
			REQUIRES bt 
			REQUIRES nvs_flash
			REQUIRES protocol_examples_common
			REQUIRES mqtt
			INCLUDE_DIRS "." 
						"../include")


