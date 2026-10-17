ZBIN = $(ZBIN64)
ASM_TCHAR := @
ASM_TCHAR_OPS := @
CFLAGS += -m64 -fshort-wchar -mcpu=ultrasparc -mno-app-regs -msoft-float -mcmodel=medlow -fno-pic -fno-pie
ASFLAGS += -64 -Av9
LDFLAGS += -m elf64_sparc
INCDIRS := arch/$(ARCH)/include $(INCDIRS)
SRCDIRS += arch/sparc64/core
MAKEDEPS += arch/sparc64/Makefile.$(PLATFORM)
include arch/sparc64/Makefile.$(PLATFORM)

# GCC supplies the V9 software bit-operation helpers.
LDLIBS += $(shell $(CC) -print-libgcc-file-name)
