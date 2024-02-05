include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(srcs "crc16-ccitt.c" "hexdump.c" "mipi.c" "mipi_dsi.c")

idf_component_register(SRCS ${srcs}
                       REQUIRES driver
                       INCLUDE_DIRS ".")


