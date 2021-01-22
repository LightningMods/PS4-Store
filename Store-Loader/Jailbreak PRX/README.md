# Sample: Jailbreak PRX

[![Version](https://img.shields.io/badge/Version-1.00-brightgreen.svg)](https://github.com/Cryptogenic/OpenOrbis-PS4-Toolchain)

This project contains example code for creating a library with a Jailbreak function. It will generate an output .prx (Playstation Relocatable Executable) for the PS4, as well as a stub .so shared object file for linking on the PC side.

to Compile this PRX correctly you must replace your link.x with this one (from OO) here
`$OO_PS4_TOOLCHAIN/link.x`

[Fix building libraries](https://github.com/sleirsgoevy/OpenOrbis-PS4-Toolchain/commit/f1cda6002d8e3af6f530cfaa5929c5d8b8167b0d)

AND
Replace `$OO_PS4_TOOLCHAIN/bin/linux/create-lib` and `$OO_PS4_TOOLCHAIN/bin/linux/create-eboot` with these

[Fix create-eboot generating non-contiguous relro and data segments #112 create-eboot/create-lib #45](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/actions/runs/503278524)



## Directory structure
```
Jailbreak PRX\Jailbreak
|-- Jailbreak
    |-- x64
        |-- Debug                          // Object files / intermediate directory
    |-- build.bat                          // Batch file for building on Windows
    |-- main.cpp                           // Mira Jailbreak and main function
    |-- mira_header.h
    |-- multi-jb.c                         // all the Jailbreak code for HEN
    |-- multi-jb.h                         // Jailbreak Offsets
    |-- new.s                              // ASM for syscall etc
|-- jb.prx                                 // final library file (not present until built)
|-- jb.so                                   // final library stub (not present until built)
|-- Makefile                               // Make rules for building on Linux
```
The ELF, Orbis ELF (OELF), and object files will all be stored in the intermediate directory `x64/Debug`. The final library prx and stub files will be found in the root directory.



## Libraries used

- libc
- libkernel



## Building

A visual studio project has been included for building on Windows. On Linux, a makefile has been included.

To build this project, the developer will need clang, which is provided in the toolchain. The `OO_PS4_TOOLCHAIN` environment variable will also need to be set to the root directory of the SDK installation.


__Linux__
Run the makefile.
```
make
```

## Reference(s) used

PS4-Payload-SDK - https://github.com/Scene-Collective/ps4-payload-sdk/


## Author(s)
- LM
- Alazif
- Specter
