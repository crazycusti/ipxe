ZBIN = $(ZBIN32)
ASM_TCHAR := @
ASM_TCHAR_OPS := @
CFLAGS += -m32 -Upowerpc -mcpu=powerpc -msoft-float -mno-altivec -msdata=none -fno-pic -fno-pie -fshort-wchar
ASFLAGS += -mppc -m32
LDFLAGS += -m elf32ppc
LDLIBS += $(shell $(CC) -print-libgcc-file-name)
INCDIRS := arch/$(ARCH)/include $(INCDIRS)
SRCDIRS += arch/powerpc/core
MAKEDEPS += arch/powerpc/Makefile.$(PLATFORM)
include arch/powerpc/Makefile.$(PLATFORM)
