set(srcs "main.c"
         "input_iot.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS
                       ".")