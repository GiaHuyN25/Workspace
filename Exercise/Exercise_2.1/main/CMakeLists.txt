set(srcs "main.c"
         "gpio_init.c"
         "timer_button.c"
        )

idf_component_register(SRCS "uart_com.c" "${srcs}"
                       INCLUDE_DIRS
                       ".")