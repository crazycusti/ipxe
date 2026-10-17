ZBIN = $(ZBIN32)
ASM_TCHAR := @
ASM_TCHAR_OPS := @
CFLAGS += -m32 -mcpu=v8 -mno-app-regs -msoft-float -fshort-wchar -fno-pic -fno-pie
ASFLAGS += -32 -Av8
LDFLAGS += -m elf32_sparc
# Distribution multilib libgcc targets V8+, which is not SPARC V8.
# The small required helper set is implemented locally for V8.
INCDIRS := arch/$(ARCH)/include $(INCDIRS)
SRCDIRS += arch/sparc32/core
MAKEDEPS += arch/sparc32/Makefile.$(PLATFORM)
include arch/sparc32/Makefile.$(PLATFORM)
