ZBIN = $(ZBIN32)
ASM_TCHAR := @
ASM_TCHAR_OPS := @
CFLAGS += -march=1.1 -mdisable-fpregs -mno-space-regs -fno-pic -fno-pie -fshort-wchar -fno-delete-null-pointer-checks
LDFLAGS += -m hppalinux
LDLIBS += $(shell $(CC) -print-libgcc-file-name)
INCDIRS := arch/$(ARCH)/include $(INCDIRS)
SRCDIRS += arch/hppa/core
MAKEDEPS += arch/hppa/Makefile.$(PLATFORM)
include arch/hppa/Makefile.$(PLATFORM)
