ZBIN = $(ZBIN64)
ASM_TCHAR := %
ASM_TCHAR_OPS := %%
# Like other EFI targets, explicitly request PIE so the common Makefile
# does not append -fno-PIE and cancel position-independent compilation.
CFLAGS += -fpie -fshort-wchar -mfixed-range=f32-f127 -mno-sdata -G0
LDLIBS += $(shell $(CC) -print-libgcc-file-name)
INCDIRS := arch/$(ARCH)/include $(INCDIRS)
SRCDIRS += arch/ia64/core
MAKEDEPS += arch/ia64/Makefile.$(PLATFORM)
include arch/ia64/Makefile.$(PLATFORM)

ASFLAGS += -xauto
