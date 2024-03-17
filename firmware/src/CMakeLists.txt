# Define the source files to compile

FILE(GLOB SRCS *.c)
FILE(GLOB SRCS_ESP esp/*.c)
FILE(GLOB SRCS_WIFI esp/wifi/*.c)

set(SRCS_AUTOGEN
	musashi/m68kopac.c
	musashi/m68kopdm.c
	musashi/m68kopnz.c
	musashi/m68kops_pre.c
)

list(APPEND ALL_SRCS ${SRCS})
list(APPEND ALL_SRCS ${SRCS_ESP})
list(APPEND ALL_SRCS ${SRCS_WIFI})
list(APPEND ALL_SRCS musashi/m68kcpu.c)
list(APPEND ALL_SRCS ${SRCS_AUTOGEN})

idf_component_register(SRCS ${ALL_SRCS})

# auto-generate source files for 68k CPU
# ... doesn't work, will use script in platform.ini as a workaround

# add_custom_command(OUTPUT ${SRCS_AUTOGEN}
#     COMMAND make
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/musashi
# )
# add_custom_target(autogen DEPENDS ${SRCS_AUTOGEN})
# add_dependencies(app autogen)
