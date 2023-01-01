# STORE UPDATER EBOOT A.K.A THE LOADER
AUTHINFO      := 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
# Libraries linked into the ELF.
LIBS        := -luser_mem_sys -ljbc -lc++ -lz -lc -lkernel  -lcurl  -lpolarssl -lSceVideoOut -lSceNet -lSceSysmodule -lSceAudioOut -lSceUserService -lScePigletv2VSH -lSceSysmodule -lSceFreeType \
               -lScePad -lSceSystemService -lSceNet -lSceSsl -lSceHttp  -lSDL2  -lSceCommonDialog -lSceMsgDialog \
               -lorbisGl -lorbisGl2 -lorbisPad -lpng -lSceImeDialog -lSceIme -lSceBgft -lSceAppInstUtil -lSceLibcInternal -lfreetype-gl -lsqlite


# Additional compile flags.
EXTRAFLAGS  := -Wall -Wno-int-to-pointer-cast -Werror -Wno-for-loop-analysis -fcolor-diagnostics -Wall -D__ORBIS__ -D__PS4__

# Asset and module directories.
LIBMODULES  :=  $(shell find sce_module -type f -name '*.*')

# You likely won't need to touch anything below this point.

# Root vars
TOOLCHAIN   := $(OO_PS4_TOOLCHAIN)
PROJDIR     := source
#FT_LIB      := external/freetype-gl
INTDIR      := build

# Define objects to build
#CFILES      := $(wildcard $(FT_LIB)/*.c)
CFILES      := $(wildcard $(PROJDIR)/*.c)
CPPFILES    := $(wildcard $(PROJDIR)/*.cpp)
ASMFILES 	:= $(wildcard $(PROJDIR)/*.s)
OBJS 		:= $(patsubst $(PROJDIR)/%.s, $(INTDIR)/%.o,  $(ASMFILES)) $(patsubst $(PROJDIR)/%.c, $(INTDIR)/%.o, $(CFILES)) $(patsubst $(PROJDIR)/%.cpp, $(INTDIR)/%.o, $(CPPFILES))
# Define final C/C++ flags
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include   -Iinclude -std=c11 -D_DEFAULT_SOURCE -DFMT_HEADER_ONLY 
CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -std=c++11 -Iexternal/freetype-gl/include -Iexternal/sqlite/include  -Iinclude -DFMT_HEADER_ONLY -Wno-invalid-noreturn
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

# Create the intermediate directory incase it doesn't already exist.
_unused     := $(shell mkdir -p $(INTDIR))

# Check for linux vs macOS and account for clang/ld path
UNAME_S     := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
		CC      := clang
		CCX     := clang++
		LD      := ld.lld
		CDIR    := linux
		ASMLR   := as
endif
ifeq ($(UNAME_S),Darwin)
		CC      := /usr/local/opt/llvm/bin/clang
		CCX     := /usr/local/opt/llvm/bin/clang++
		LD      := /usr/local/opt/llvm/bin/ld.lld
		CDIR    := macos
		ASMLR   := as
endif

all:  eboot.bin
eboot.bin: $(ODIR) $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/eboot.bin $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(INTDIR)/eboot.bin -out=$(INTDIR)/eboot.oelf --eboot "eboot.bin" --authinfo=$(AUTHINFO)

$(INTDIR)/%.o: $(PROJDIR)/%.s
	$(ASMLR) -o $@ $<
	
$(INTDIR)/%.o: $(PROJDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: $(PROJDIR)/%.cpp
	$(CCX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(CONTENT_ID).pkg sce_sys/param.sfo eboot.bin \
		$(INTDIR)/$(PROJDIR).elf $(INTDIR)/$(PROJDIR).oelf $(OBJS)
