ZBIN = $(ZBIN64)
ASM_TCHAR := @
ASM_TCHAR_OPS := @
CFLAGS += -mcpu=ev6 -mno-fp-regs -fno-pic -fno-pie -fshort-wchar
LDFLAGS += -m elf64alpha
LDLIBS += $(shell $(CC) -print-libgcc-file-name)
INCDIRS := arch/alpha/include $(INCDIRS)
SRCDIRS += arch/alpha/core
MAKEDEPS += arch/alpha/Makefile.$(PLATFORM)
include arch/alpha/Makefile.$(PLATFORM)
