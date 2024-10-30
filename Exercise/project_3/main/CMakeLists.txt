set(srcs "main.c"
         "input_iot.c"
         "output_iot.c")

idf_component_register(SRCS "output_iot.c" "${srcs}"
                       INCLUDE_DIRS
                       ".")