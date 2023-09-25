#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#	Recursive Make Considered Harmful
#	http://aegis.sourceforge.net/auug97.pdf
#
OBJDIR := obj
SHELL := /bin/bash

# UEFI firmware definitions
JOS_LOADER_DEPS := LoaderPkg/Loader/*.c LoaderPkg/Loader/*.h LoaderPkg/Loader/*.inf LoaderPkg/*.dsc LoaderPkg/*.dec
JOS_LOADER_BUILD := LoaderPkg/UDK/Build/LoaderPkg
ifeq ($(ARCHS),IA32)
OVMF_FIRMWARE := LoaderPkg/Binaries/OVMF-IA32.fd
JOS_LOADER := LoaderPkg/Binaries/Loader-IA32.efi
JOS_LOADER_DEPS += LoaderPkg/Loader/Ia32/*.nasm LoaderPkg/Loader/Ia32/*.c
JOS_BOOTER := BOOTIa32.efi
else
OVMF_FIRMWARE := LoaderPkg/Binaries/OVMF-X64.fd
JOS_LOADER := LoaderPkg/Binaries/Loader-X64.efi
JOS_LOADER_DEPS += LoaderPkg/Loader/X64/*.c
JOS_BOOTER := BOOTX64.efi
endif
JOS_ESP := LoaderPkg/ESP

# Run 'make V=1' to turn on verbose commands, or 'make V=0' to turn them off.
ifeq ($(V),1)
override V =
endif
ifeq ($(V),0)
override V = @
endif

-include conf/lab.mk
-include conf/env.mk

LABSETUP ?= ./

TOP = .

ifdef JOSLLVM

ifndef CLANGPREFIX
CLANGPREFIX := $(shell if PATH="$$ISP_PATH:$$PATH" which clang 2>&1 >/dev/null 2>&1; \
	then PATH="$$ISP_PATH:$$PATH" which clang | sed s/clang$$//; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find LLVM clang." 1>&2; \
	echo "*** Is the directory with clang in your PATH?" 1>&2; \
	echo "*** If your LLVM clang toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'clang', set your CLANGPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake CLANGPREFIX= ...'." 1>&2; \
	echo "*** Perhaps you wanted to use gcc toolchain, in this case ensure" 1>&2; \
	echo "*** JOSLLVM environment variable is not defined." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

CC	:= $(CLANGPREFIX)clang -target x86_64-gnu-linux -pipe
AS	:= $(CLANGPREFIX)llvm-as
AR	:= $(CLANGPREFIX)llvm-ar
LD	:= $(CLANGPREFIX)ld.lld
OBJCOPY	:= llvm/gnu-objcopy
OBJDUMP	:= $(CLANGPREFIX)llvm-objdump
NM	:= $(CLANGPREFIX)llvm-nm

EXTRA_CFLAGS += -Wno-self-assign -Wno-format-nonliteral -Wno-address-of-packed-member \
                -Wno-frame-address -Wno-unknown-warning-option


GCC_LIB := $(shell A="$$($(CC) $(CFLAGS) -print-resource-dir)/lib"; [ -d "$$A/linux" ] && echo "$$A/linux" || echo "$$A/ispras")/libclang_rt.builtins-x86_64.a

else

# Cross-compiler jos toolchain
#
# This Makefile will automatically use the cross-compiler toolchain
# installed as 'i386-jos-elf-*', if one exists.  If the host tools ('gcc',
# 'objdump', and so forth) compile for a 32-bit x86 ELF target, that will
# be detected as well.  If you have the right compiler toolchain installed
# using a different name, set GCCPREFIX explicitly in conf/env.mk

# try to infer the correct GCCPREFIX
ifndef GCCPREFIX
GCCPREFIX := $(shell if PATH="$$ISP_PATH:$$PATH" x86_64-ispras-elf-objdump -i 2>&1 | grep '^elf64-x86-64$$' >/dev/null 2>&1; \
	then PATH="$$ISP_PATH:$$PATH" which x86_64-ispras-elf-gcc | sed s/gcc$$//; \
	elif objdump -i 2>&1 | grep 'elf64-x86-64' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an x86_64-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with x86_64-ispras-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your x86_64-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'x86_64-ispras-elf-', set your GCCPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake GCCPREFIX= ...'." 1>&2; \
	echo "*** Perhaps you wanted to use llvm toolchain, in this case ensure" 1>&2; \
	echo "*** JOSLLVM environment variable is defined." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

CC	:= $(GCCPREFIX)gcc -fno-pic -pipe
AS	:= $(GCCPREFIX)as
AR	:= $(GCCPREFIX)ar
LD	:= $(GCCPREFIX)ld
OBJCOPY	:= $(GCCPREFIX)objcopy
OBJDUMP	:= $(GCCPREFIX)objdump
NM	:= $(GCCPREFIX)nm

EXTRA_CFLAGS	:= $(EXTRA_CFLAGS) -Wno-unused-but-set-variable

GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

endif

# Native commands
NCC	:= gcc $(CC_VER) -pipe
NATIVE_CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -I$(TOP) -MD -Wall
TAR	:= gtar
PERL	:= perl

# Try to infer the correct QEMU
ifndef QEMU
QEMU := $(shell if PATH="$$ISP_PATH:$$PATH" which qemu-system-x86_64 > /dev/null 2>&1; \
	then PATH="$$ISP_PATH:$$PATH" which qemu-system-x86_64; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/x86_64-softmmu.app/Contents/MacOS/x86_64-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in conf/env.mk?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

# Try to generate a unique GDB port if it is not set already.
ifeq ($(GDBPORT),)
GDBPORT	:= $(shell expr `id -u` % 5000 + 25000)
endif

# Compiler flags
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -fno-builtin -I$(TOP) -MD
ifeq ($(D),1)
CFLAGS += -O0
else
# Only optimize to -O1 to discourage inlining, which complicates backtraces.
CFLAGS += -O1
endif
CFLAGS += -ffreestanding -fno-omit-frame-pointer -mno-red-zone
CFLAGS += -Wall -Wformat=2 -Wno-unused-function -Werror -g -gpubnames -gdwarf-4

# Add -fno-stack-protector if the option exists.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
CFLAGS += $(EXTRA_CFLAGS)
CFLAGS += -mno-sse -mno-sse2 -mno-mmx


ifdef KUBSAN

CFLAGS += -DSAN_ENABLE_KUBSAN

KERN_SAN_CFLAGS += -fsanitize=undefined \
	-fsanitize=implicit-integer-truncation \
	-fno-sanitize=function \
	-fno-sanitize=vptr \
	-fno-sanitize=return

endif

ifdef GRADE3_TEST
CFLAGS += -DGRADE3_TEST=$(GRADE3_TEST)
CFLAGS += -DGRADE3_FUNC=$(GRADE3_FUNC)
CFLAGS += -DGRADE3_FAIL=$(GRADE3_FAIL)
CFLAGS += -DGRADE3_PFX1=$(GRADE3_PFX1)
CFLAGS += -DGRADE3_PFX2=$(GRADE3_PFX2)
.SILENT:
endif

# Common linker flags
LDFLAGS := -m elf_x86_64 -z max-page-size=0x1000 --print-gc-sections --warn-common -z noexecstack

# Linker flags for JOS programs
ULDFLAGS := -T user/user.ld --warn-common

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=

# Make sure that 'all' is the first target
all: .git/hooks/post-checkout .git/hooks/pre-commit

# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS:  $(OBJDIR)/kern/%.o \
	   $(OBJDIR)/lib/%.o $(OBJDIR)/fs/%.o $(OBJDIR)/net/%.o \
	   $(OBJDIR)/user/%.o \
	   $(OBJDIR)/prog/%.o

KERN_CFLAGS := $(CFLAGS) -DJOS_KERNEL -DLAB=$(LAB) -mcmodel=large -m64
USER_CFLAGS := $(CFLAGS) -DLAB=$(LAB) -mcmodel=large -m64
ifeq ($(CONFIG_KSPACE),y)
KERN_CFLAGS += -DCONFIG_KSPACE
USER_CFLAGS += -DCONFIG_KSPACE -DJOS_PROG
else
USER_CFLAGS += -DJOS_USER
endif

# Update .vars.X if variable X has changed since the last make run.
#
# Rules that use variable X should depend on $(OBJDIR)/.vars.X.  If
# the variable's value has changed, this will update the vars file and
# force a rebuild of the rule that depends on it.
$(OBJDIR)/.vars.%: FORCE
	@test -f $@ || touch $@
	$(V)echo "$($*)" | cmp -s - $@ || echo "$($*)" > $@
.PRECIOUS: $(OBJDIR)/.vars.%
.PHONY: FORCE


# Include Makefrags for subdirectories
include kern/Makefrag
include lib/Makefrag
ifeq ($(CONFIG_KSPACE),y)
include prog/Makefrag
else
include user/Makefrag
endif

QEMUOPTS = -hda fat:rw:$(JOS_ESP) -serial mon:stdio -gdb tcp::$(GDBPORT)
QEMUOPTS += -m 512M -d int,cpu_reset,mmu,pcall -no-reboot

QEMUOPTS += $(shell if $(QEMU) -display none -help | grep -q '^-D '; then echo '-D qemu.log'; fi)
IMAGES = $(OVMF_FIRMWARE) $(JOS_LOADER) $(OBJDIR)/kern/kernel $(JOS_ESP)/EFI/BOOT/kernel $(JOS_ESP)/EFI/BOOT/$(JOS_BOOTER)
QEMUOPTS += -bios $(OVMF_FIRMWARE)
# QEMUOPTS += -debugcon file:$(UEFIDIR)/debug.log -global isa-debugcon.iobase=0x402

define POST_CHECKOUT
#!/bin/sh -x
make clean
endef
export POST_CHECKOUT

define PRE_COMMIT
#!/bin/sh

if git diff --cached --name-only --diff-filter=DMR | grep -q grade
then
   echo "FAIL: Don't change grade files."
   exit 1
else
   exit 0
fi
endef
export PRE_COMMIT

.git/hooks/post-checkout:
	@echo "$$POST_CHECKOUT" > $@
	@chmod +x $@

.git/hooks/pre-commit:
	@echo "$$PRE_COMMIT" > $@
	@chmod +x $@

.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

$(OVMF_FIRMWARE):
	LoaderPkg/build_ovmf.sh

$(JOS_LOADER): $(OVMF_FIRMWARE) $(JOS_LOADER_DEPS)
	LoaderPkg/build_ldr.sh

$(JOS_ESP)/EFI/BOOT/kernel: $(OBJDIR)/kern/kernel
	mkdir -p $(JOS_ESP)/EFI/BOOT
	cp $(OBJDIR)/kern/kernel $(JOS_ESP)/EFI/BOOT/kernel

$(JOS_ESP)/EFI/BOOT/$(JOS_BOOTER): $(JOS_LOADER)
	mkdir -p $(JOS_ESP)/EFI/BOOT
	cp $(JOS_LOADER) $(JOS_ESP)/EFI/BOOT/$(JOS_BOOTER)
	# cp $(JOSLOADER)/Loader.debug $(UEFIDIR)/EFI/BOOT/BOOTIA32.DEBUG

pre-qemu: .gdbinit

qemu: $(IMAGES) pre-qemu
	$(QEMU) $(QEMUOPTS)

qemu-nox: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Use Ctrl-a x to exit qemu"
	@echo "***"
	$(QEMU) -display none $(QEMUOPTS)

qemu-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'gdb'." 1>&2
	@echo "***"
	$(QEMU) $(QEMUOPTS) -S

qemu-nox-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'gdb'." 1>&2
	@echo "***"
	$(QEMU) -display none $(QEMUOPTS) -S

print-qemu:
	@echo $(QEMU)

print-gdbport:
	@echo $(GDBPORT)

format:
	@find . -name *.[ch] -not -path "./LoaderPkg/*" -exec $(CLANGPREFIX)clang-format -i {} \;

# For deleting the build
clean:
	rm -rf $(OBJDIR) .gdbinit jos.in qemu.log $(JOS_LOADER) $(JOS_LOADER_BUILD) $(JOS_ESP) kern/kernel.ld

realclean: clean
	rm -rf lab$(LAB).tar.gz \
		jos.out $(wildcard jos.out.*) \
		qemu.pcap $(wildcard qemu.pcap.*)

distclean: realclean
	rm -f .git/hooks/pre-commit .git/hooks/post-checkout $(OVMF_FIRMWARE) LoaderPkg/efibuild.sh

ifneq ($(V),@)
GRADEFLAGS += -v
endif

grade:
	@echo $(MAKE) clean
	@$(MAKE) clean || \
	  (echo "'make clean' failed.  HINT: Do you have another running instance of JOS?" && exit 1)
	ARCHS=X64 ./grade-lab$(LAB) $(GRADEFLAGS)
	@echo $(MAKE) clean
	@$(MAKE) clean || \
	  (echo "'make clean' failed.  HINT: Do you have another running instance of JOS?" && exit 1)
	ARCHS=IA32 ./grade-lab$(LAB) $(GRADEFLAGS)

# For test runs

prep-%:
	$(V)$(MAKE) "INIT_CFLAGS=${INIT_CFLAGS} -DTEST=`case $* in *_*) echo $*;; *) echo user_$*;; esac`" $(IMAGES)

run-%-nox-gdb: prep-% pre-qemu
	$(QEMU) -display none $(QEMUOPTS) -S

run-%-gdb: prep-% pre-qemu
	$(QEMU) $(QEMUOPTS) -S

run-%-nox: prep-% pre-qemu
	$(QEMU) -display none $(QEMUOPTS)

run-%: prep-% pre-qemu
	$(QEMU) $(QEMUOPTS)

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps

always:
	@:

.PHONY: all always clean realclean distclean grade
