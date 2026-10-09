EE_BIN = NGPPS2.elf

# =========================================================================
# OBJETOS DO PROJETO (Launcher Nativo + Core NeoPop)
# =========================================================================
EE_OBJS = main.o \
          src/core/pad.o \
		  src/core/tim2_util.o \
          src/actions/game_launcher.o \
          src/ui/menu_scroll.o \
          src/ui/launcher_view.o \
		  src/ui/lang.o \
          src/neopop/neopop_ps2.o \
          src/neopop/Core/bios.o \
          src/neopop/Core/biosHLE.o \
          src/neopop/Core/dma.o \
          src/neopop/Core/flash.o \
          src/neopop/Core/gfx.o \
          src/neopop/Core/gfx_scanline_colour.o \
          src/neopop/Core/gfx_scanline_mono.o \
          src/neopop/Core/interrupt.o \
          src/neopop/Core/mem.o \
          src/neopop/Core/neopop.o \
          src/neopop/Core/rom.o \
          src/neopop/Core/sound.o \
          src/neopop/Core/state.o \
		  src/neopop/Core/perf_profiler.o \
          src/neopop/Core/Z80_interface.o \
          src/neopop/Core/z80/Z80.o \
          src/neopop/Core/TLCS-900h/TLCS900h_interpret.o

GSKIT = $(PS2DEV)/gsKit

# =========================================================================
# INCLUDES
# =========================================================================
EE_INCS = -Isrc \
          -Isrc/core \
          -Isrc/ui \
          -Isrc/actions \
          -Isrc/neopop \
          -Isrc/neopop/Core \
          -Isrc/neopop/Core/z80 \
          -Isrc/neopop/Core/TLCS-900h \
          -I$(PS2SDK)/ports/include \
		  -I$(GSKIT)/include

EE_LDFLAGS = -Wl,-Ttext=0x00800000 -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib

EE_CFLAGS = -D_EE -O2 -fomit-frame-pointer -finline-functions -freorder-blocks -G0 -Wall -std=gnu99 -ffast-math -DNEWLIB_PORT_AWARE -D__cdecl= \
            -Wno-unused-function -Wno-unused-variable -Wno-char-subscripts \
            -Wno-parentheses -Wno-strict-aliasing -Wno-incompatible-pointer-types \
            -Wno-implicit-function-declaration $(EE_INCS)

# =========================================================================
# BIBLIOTECAS NATIVAS PS2SDK (A ordem importa!)
# -ldraw -lgraph -lmath3d -lpacket -ldma substituem 100% o gsKit e o dmaKit!
# =========================================================================
EE_LIBS = -lgskit -ldmakit -lfileXio -lpatches -lpad -laudsrv -lm -ldebug -lc -lkernel


all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

clean:
	rm -f $(EE_BIN) main.o src/core/*.o src/ui/*.o src/actions/*.o \
	      src/neopop/*.o src/neopop/Core/*.o src/neopop/Core/z80/*.o src/neopop/Core/TLCS-900h/*.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal