ZBIN = $(ZBIN64)
ASM_TCHAR := @
ASM_TCHAR_OPS := @
CFLAGS += -m64 -mcpu=power4 -msoft-float -mno-altivec -fno-pic -fno-pie -fshort-wchar
LDFLAGS += -m elf64ppc
LDLIBS += $(shell $(CC) -print-libgcc-file-name)
INCDIRS := arch/$(ARCH)/include $(INCDIRS)
SRCDIRS += arch/powerpc64/core
MAKEDEPS += arch/powerpc64/Makefile.$(PLATFORM)
include arch/powerpc64/Makefile.$(PLATFORM)
