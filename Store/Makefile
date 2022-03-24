ifndef ORBISDEV
$(error ORBISDEV, is not set)
endif

target ?= ps4_elf_sce
TargetFile=homebrew.elf

include $(ORBISDEV)/make/ps4sdk.mk

LinkerFlags +=  -ldumper -lc++ -lc++abi -lc++experimental -luser_mem_sys -lkernel_stub -lSceLibcInternal_stub -lSceSysmodule_stub -lSceSystemService_stub -lSceNet_stub -lSceUserService_stub -lScePigletv2VSH_stub -lSceVideoOut_stub  -lSceImeDialog_stub -lSceGnmDriver_stub  -lorbis -lorbisGl -lScePad_stub -lSceCommonDialog_stub -lSceMsgDialog_stub -lSceAudioOut_stub -lSceIme_stub -lSceAppInstUtil_stub -lSceBgft_stub -lSceSysUtil_stub -lSceHttp_stub -lSceSsl_stub -lSceNetCtl_stub -lfreetype-gl -lSceFreeTypeOl_stub -lkernel_sys_stub -lSceLncUtil_stub -lSQLite 

CompilerFlags += -D__PS4__ -D__ORBIS__ -fstack-protector-all # -DHAVE_SHADER_COMPILER
CompilerFlagsCpp += -std=c++17 -DHAVE_CONFIG_H -D_U_="__attribute__((unused))" -D__ORBIS__=1 -D_ORBIS_ -DNDEBUG -D_BSD_SOURCE -fexceptions -frtti

IncludePath += -I$(ORBISDEV)/usr/include -I$(ORBISDEV)/usr/include/c++/v1 -I$(ORBISDEV)/usr/include/orbis
IncludePath += -I$(ORBISDEV)/usr/include/orbis/freetype-gl


install:
	@cp $(OutPath)/homebrew.self /hostapp/homebrew.self
	@echo "Installed!"
oelf:
	orbis-elf-create bin/homebrew.elf bin/homebrew.oelf >/dev/null 2>&1


AUTH_INFO = 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

eboot:
	python2 $(ORBISDEV)/bin/make_fself.py --auth-info $(AUTH_INFO) bin/homebrew.oelf bin/homebrew.self
pkg_build:
	cp bin/homebrew.self pkg/eboot.bin && cd pkg && pkgTool pkg_build Project.gp4 . && cp *.pkg ../bin/
