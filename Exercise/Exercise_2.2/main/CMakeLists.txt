set(srcs "main.c"
         "gpio_init.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS
                       ".")