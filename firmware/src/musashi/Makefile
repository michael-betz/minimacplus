# Generate source code of the cpu emulator

MUSASHI_GEN_SRC := m68kopac.c m68kopdm.c m68kopnz.c
MUSASHI_OP_PREGEN_SRC := m68kops_pre.c

all: $(MUSASHI_GEN_SRC) $(MUSASHI_OP_PREGEN_SRC)

$(MUSASHI_GEN_SRC): m68kmake
	./$<

$(MUSASHI_OP_PREGEN_SRC): m68kops $(MUSASHI_GEN_SRC)
	./$< > $@

clean:
	rm -f $(MUSASHI_GEN_SRC) $(MUSASHI_OP_PREGEN_SRC) m68kmake m68kops
