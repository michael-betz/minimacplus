include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(MUSASHI_SRC "musashi/m68kcpu.c" "musashi/m68kops_pre.c" "musashi/m68kopac.c" "musashi/m68kopdm.c" "musashi/m68kopnz.c")
set(EMU_SRC "emu.c" "iwm.c" "via.c" "macrtc.c" "ncr.c" "scc.c" "mouse.c")

set(COMPONENT_SRCS ${MUSASHI_SRC} ${EMU_SRC})
set(COMPONENT_ADD_INCLUDEDIRS "." "./musashi")
register_component()
