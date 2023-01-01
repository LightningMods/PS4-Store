# PRX metadata
PROJECTNAME   := libsqlite

# Libraries linked into the ELF
LIBS          := -luser_mem_sys -lc -lorbisGl -lorbisGl2  -lSceUserService -lScePigletv2VSH -lSceLibcInternal -lkernel  -lSceFreeType

# Directorys to include
INCLUDES      := -Iinclude

# Additional compile flags
ERRORFLAGS    := -Wall -Wextra 
OTHERFLAGS    := -O3 -std=c11 -D_DEFAULT_SOURCE -D__ORBIS__ -D__PS4__

# -----------------------------------------------------------------------------
# Do not edit below this line unless you know what you are doing
# -----------------------------------------------------------------------------

TOOLCHAIN     := $(OO_PS4_TOOLCHAIN)
ODIR          := build
SDIR          := source
EXTRAFLAGS    := $(INCLUDES) $(ERRORFLAGS) $(OTHERFLAGS)
CFLAGS        := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include $(EXTRAFLAGS)
LFLAGS        := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crtlib.o

CFILES        := $(wildcard $(SDIR)/*.c)
ASMFILES      := $(wildcard $(SDIR)/*.s)
OBJS          := $(patsubst $(SDIR)/%.s, $(ODIR)/%.o, $(ASMFILES)) $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES))
STUBOBJS      := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o.stub, $(CFILES))

TARGET        = $(PROJECTNAME).prx
TARGETSTUB    = $(PROJECTNAME)_stub.so
TARGETSTATIC  = $(PROJECTNAME).a

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	AR        := llvm-ar
	AS        := llvm-mc
	CC        := clang
	LD        := ld.lld
	CDIR      := linux
endif
ifeq ($(UNAME_S),Darwin)
	AR        := /usr/local/opt/llvm/bin/llvm-ar
	AS        := /usr/local/opt/llvm/bin/llvm-mc
	CC        := /usr/local/opt/llvm/bin/clang
	LD        := /usr/local/opt/llvm/bin/ld.lld
	CDIR      := macos
endif

# Make rules
$(TARGET): $(ODIR) $(OBJS)
	$(LD) $(ODIR)/*.o -o $(ODIR)/$(PROJECTNAME).elf $(LFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(ODIR)/$(PROJECTNAME).elf -out=$(ODIR)/$(PROJECTNAME).oelf --lib=$(TARGET) --paid 0x3800000000000011
	@echo Built PRX successfully!

$(TARGETSTATIC): $(ODIR) $(OBJS)
	$(AR) rcs $(TARGETSTATIC) $(ODIR)/*.o
	@echo Built static library successfully!

$(TARGETSTUB): $(ODIR) $(STUBOBJS)
	$(CC) $(ODIR)/*.o.stub -o $(TARGETSTUB) -target x86_64-pc-linux-gnu -shared -fuse-ld=lld -ffreestanding -nostdlib -fno-builtin -L$(TOOLCHAIN)/lib $(LIBS)
	@echo Built stub successfully!

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(ODIR)/%.o: $(SDIR)/%.s
	$(AS) -triple=x86_64-pc-freebsd-elf --filetype=obj -o $@ $<

$(ODIR)/%.o.stub: $(SDIR)/%.c
	$(CC) -target x86_64-pc-linux-gnu -ffreestanding -nostdlib -fno-builtin -fPIC -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include $(EXTRAFLAGS) -o $@ $<

$(ODIR):
	@echo Creating build directory...
	@mkdir $@

.PHONY: all clean install
.DEFAULT_GOAL := all

all: $(TARGET) $(TARGETSTATIC) $(TARGETSTUB)

clean:
	@echo Cleaning up...
	@rm -rf $(TARGET) $(TARGETSTATIC) $(TARGETSTUB) $(ODIR)

install:
	@echo Installing...
	@yes | cp -f $(TARGETSTATIC) $(OO_PS4_TOOLCHAIN)/lib/$(TARGETSTATIC) 2>/dev/null && echo Installed!|| echo Failed to install, is it built?
