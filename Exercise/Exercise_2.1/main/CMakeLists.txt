set(srcs "main.c"
         "gpio_init.c"
	 "timer_button.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS
                       ".")