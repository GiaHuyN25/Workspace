set(srcs "main.c"
         "gpio_init.c"
         "seven_segment.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS
                       ".")