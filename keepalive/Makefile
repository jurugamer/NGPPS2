IOP_BIN = keepalive.irx
IOP_OBJS = keepalive_bdm.o imports.o

IOP_INCS += -I$(PS2SDK)/iop/include -I$(PS2SDK)/common/include
IOP_CFLAGS += -Wall -fno-builtin -O2

all: $(IOP_BIN)

clean:
	rm -f $(IOP_BIN) $(IOP_OBJS)

include $(PS2SDK)/Defs.make
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal