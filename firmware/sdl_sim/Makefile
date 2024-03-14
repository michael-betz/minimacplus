# Makefile for a native build of the emulator
# using SDL2 for sound / video and an .img file for the HD

SRC = ../src

LDLIBS += -lasan -lm -lSDL2 -lstdc++
CFLAGS += -g -Wall -fstack-protector-all -fsanitize=address -I$(SRC) -I$(SRC)/musashi -DHOSTBUILD

MUSASHI_GEN_SRC := $(SRC)/musashi/m68kopac.c $(SRC)/musashi/m68kopdm.c $(SRC)/musashi/m68kopnz.c
MUSASHI_OP_PREGEN_SRC := $(SRC)/musashi/m68kops_pre.c

ALL_SRCS =  $(MUSASHI_GEN_SRC) \
			$(MUSASHI_OP_PREGEN_SRC) \
			$(SRC)/musashi/m68kcpu.c \
			$(wildcard *.c) \
			$(wildcard $(SRC)/*.c) \

ALL_OBJS = $(ALL_SRCS:.c=.o)

all: main

main: $(ALL_OBJS)

autogen:
	$(MAKE) -C $(SRC)/musashi

$(MUSASHI_GEN_SRC): autogen
$(MUSASHI_OP_PREGEN_SRC): autogen

clean:
	$(MAKE) -C $(SRC)/musashi clean
	rm -f sniff.pcap pram.dat
	rm -rf $(ALL_OBJS) main *.out *.dat
