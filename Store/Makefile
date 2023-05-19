
# Package metadata.
TITLE       := Store
VERSION     := 2.00
TITLE_ID    := NPXS39041
CONTENT_ID  := IV0002-NPXS39041_00-STOREUPD00000000

# Libraries linked into the ELF.
LIBS        := -luser_mem_sys -lc++ -lz -ljbc -lc -lkernel  -lcurl  -lpolarssl -lSceVideoOut -lSceNet -lSceSysmodule -lSceAudioOut -lSceUserService -lScePigletv2VSH -lSceSysmodule -lSceFreeType \
               -lScePad -lSceSystemService -lSceNet -lSceSsl -lSceHttp  -lSDL2  -lSceCommonDialog -lSceMsgDialog \
               -lorbisGl -lorbisGl2 -lorbisPad -lpng -lSceImeDialog -lSceIme -lSceBgft -lSceAppInstUtil -lSceLibcInternal -lfreetype-gl -lsqlite -lSceLncUtil


# Additional compile flags.
EXTRAFLAGS  := -Wall -DFMT_HEADER_ONLY  -fexceptions -frtti  -Wno-int-to-pointer-cast -Wno-c++11-narrowing -Werror -Wno-for-loop-analysis -fcolor-diagnostics -Wall -D__ORBIS__ -D__PS4__

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
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -Iexternal/freetype-gl/include -Iexternal/sqlite/include  -Iinclude -std=c11 -D_DEFAULT_SOURCE -DFMT_HEADER_ONLY 
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

all: Store-R2.pkg

Store-R2.pkg: pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

pkg.gp4: eboot.bin sce_sys/param.sfo


sce_sys/param.sfo: Makefile
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1 
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 32  
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gde'  
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0 
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0  
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

eboot.bin: $(INTDIR) $(OBJS)
	$(LD) $(INTDIR)/*.o -o $(INTDIR)/$(PROJDIR).elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(INTDIR)/$(PROJDIR).elf -out=$(INTDIR)/$(PROJDIR).oelf --eboot "homebrew.elf" --paid 0x3800000000000011 --authinfo 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

$(INTDIR)/%.o: $(PROJDIR)/%.s
	$(ASMLR) -o $@ $<
	
$(INTDIR)/%.o: $(PROJDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: $(PROJDIR)/%.cpp
	$(CCX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f Store-R2.pkg sce_sys/param.sfo eboot.bin \
		$(INTDIR)/$(PROJDIR).elf $(INTDIR)/$(PROJDIR).oelf $(OBJS)
