set(srcs "main.c"
         "gpio_init.c"
         "timer_button.c"
         "uart_com.c")

idf_component_register(SRCS "uart_com.c" "${srcs}"
                       INCLUDE_DIRS
                       ".")