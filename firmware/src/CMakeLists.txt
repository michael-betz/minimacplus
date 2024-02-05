set(srcs "hd.c" "hexdump.c" "localtalk.c" "main.c" "snd.c")

if(CONFIG_TME_DISP_MIPI)
    list(APPEND srcs "mipi_lcd.c" "adns9500.c")
endif()

if(CONFIG_TME_DISP_WROVER)
    list(APPEND srcs "spi_lcd.c")
endif()

idf_component_register(SRCS ${srcs})
