set(srcs "main.c"
         "gpio_init.c"
         "seven_segment.c"
         "realtime.c")

idf_component_register(SRCS "realtime.c" "${srcs}"
                       INCLUDE_DIRS
                       ".")