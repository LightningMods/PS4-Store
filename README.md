# PS4-Store


## Lead Developer (besides the obv)
- [MZ](https://twitter.com/Masterzorag)


## Licensed Under GPLv3



## Libaries


- [OrbisDev SDK](https://github.com/orbisdev/orbisdev) 
- [OOSDK for PRXs](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) 
- [JSMN](https://github.com/zserge/jsmn) (MIT)
- [INI Parser](https://github.com/0xe1f/psplib/tree/master/libpsp) (Apache License 2.0)
- [RSA Verify (ARMmbed)](https://github.com/ARMmbed/mbedtls) (Apache License 2.0)
- [OpenSSL/MD5](https://github.com/openssl/openssl) (Apache License 2.0)
- [BusyBox](https://elixir.bootlin.com/busybox/0.39/source) (GPLv2)
- [LibPNG](https://github.com/glennrp/libpng) 
- Libz

## Refs
- [ShaderToy](shadertoy.com) 



# How to Update your CDN

Replace your homebrew.elf , homebrew.elf.sig and remote.md5 with the Latest from the Release Section of this Github to a folder named "update" on your servers root

# Settings

the ini file is either loaded by the app dir or from USB0 when the app is booted

```
CDN=UR_HOST //OPTIONAL
Secure_Boot=1 //ONLY allows RSA signed updates from this GH OPTIONAL
temppath=PATH_WHERE_TO_DOWNLOAD_THE_PKGS; Required
StoreOnUSB=0 //store pkgs on usb OPTIONAL
Adavnced_Enabled=1 // need it for advanced settings optional
CDN_For_Devkit_Modules=CDN_FOR_ONLY_DEVKIT_MODULES //optional
```
 
## App details

upon booting up the app will cache all json files from the CDN in the settings.ini
restart is required for changes made to json files (for now)

all images from jsons download to /user/app/STORE_ID/storedata/
all jsons are downloaded to /user/app/STORE_ID/

IF the settings file is loaded from USB all settings will be saved to USBs

ONLY 3/4 apps can download at once

## Credits

- [Flatz](https://twitter.com/flat_z)
- [SocraticBliss](https://twitter.com/SocraticBliss)
- [TheoryWrong](https://twitter.com/TheoryWrong)
- Znullptr
- [Xerpi](https://twitter.com/xerpi)
- [TOXXIC407](https://twitter.com/TOXXIC_407)
- [Masterzorag](https://twitter.com/masterzorag)
- [Psxdev/BigBoss](https://twitter.com/psxdev)
- [Specter](https://twitter.com/SpecterDev)


