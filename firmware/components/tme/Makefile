#Makefile for a native (SDL) tme build
TARGET := sdl/main

MUSASHI_GEN_SRC := musashi/m68kopac.c musashi/m68kopdm.c musashi/m68kopnz.c
MUSASHI_OP_PREGEN_SRC := musashi/m68kops_pre.c

OBJ :=	$(MUSASHI_GEN_SRC:%.c=%.o) \
		$(MUSASHI_OP_PREGEN_SRC:%.c=%.o) \
		musashi/m68kcpu.o \
		sdl/disp.o \
		sdl/hd.o \
		sdl/main.o \
		sdl/snd.o \
		emu.o \
		iwm.o \
		via.o \
		macrtc.o \
		ncr.o \
		scc.o \
		mouse.o \
		hexdump.o \
		network/localtalk.o \
		network/ddp.o \
		network/ethertalk.o \
		network/basiliskif.o \
		network/sniff.o \
# 		musashi/m68kdasm.o

LDLIBS += -lm -lSDL2 -lstdc++
CFLAGS += -I. -I./musashi -I../../.pio/build/minimac/config -g -DHOSTBUILD

all: $(TARGET)

autogen: $(MUSASHI_OP_PREGEN_SRC)

$(TARGET): $(OBJ)

$(MUSASHI_GEN_SRC): musashi/m68kmake
	(cd $(dir $<) && ./$(notdir $<))

$(MUSASHI_OP_PREGEN_SRC): musashi/m68kops $(MUSASHI_GEN_SRC)
	$< > $@

musashi/m68kmake:
musashi/m68kops:
	$(MAKE) -C $(dir $@) $(notdir $@)

clean:
	rm -f $(MUSASHI_GEN_SRC) $(MUSASHI_OP_PREGEN_SRC) musashi/m68kmake musashi/m68kops
	rm -f sniff.pcap pram.dat
	rm -f $(OBJ) $(TARGET)
