set(srcs "main.cpp"
         "board.c"
         "app_device.cpp")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS
                       ".")