# PS4-Store


## Lead Developer (besides the obv)
- [MZ](https://twitter.com/Masterzorag)



## Licensed Under GPLv3

## Store directions

**Please note that the Store app updates automatically via internet without any action by the user**

- the ELFs in the release section are ONLY for CDNs or for locally hosting a Store Server
- You can download The [Package here (PKG-Zone)](https://pkg-zone.com/Store-R2.pkg), **ONLY Download from this Site**
- You can always change App settings without opening the app by adding a settings.ini file to the root of a USB

## Libaries


- [OrbisDev SDK](https://github.com/orbisdev/orbisdev) (MIT)
- [Orbis SQLite](https://github.com/orbisdev/orbisdev-libSQLite) (GPLv3)
- [OOSDK for PRXs](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) (GPLv3)
- [JSMN](https://github.com/zserge/jsmn) (MIT)
- [INI Parser](https://github.com/benhoyt/inih) (New BSD license)
- [RSA Verify (ARMmbed)](https://github.com/ARMmbed/mbedtls) (Apache License 2.0)
- [OpenSSL/MD5](https://github.com/openssl/openssl) (Apache License 2.0)
- [BusyBox](https://elixir.bootlin.com/busybox/0.39/source) (GPLv2)
- [LibPNG & zlib](https://github.com/glennrp/libpng) (PNG Ref. Lib. License ver. 1)
- [log.c](https://github.com/rxi/log.c) (MIT)
- [Game Dumper](https://github.com/Al-Azif/dumper-testing) (GPLv3)
- [libjbc](https://github.com/sleirsgoevy/ps4-libjbc) (License Unknown)

## Refs
- [ShaderToy](shadertoy.com) 



# How to Update your CDN

Replace your homebrew.elf , homebrew.elf.sig and remote.md5 with the Latest from the Release Section of this Github to a folder named "update" on your servers root

# Settings

the ini file is either loaded by the app dir or from USB0 when the app is booted

```
[Settings]
CDN=UR_HOST //OPTIONAL
Secure_Boot=1 //ONLY allows RSA signed updates from this GH OPTIONAL
temppath=PATH_WHERE_TO_DOWNLOAD_THE_PKGS; Required
StoreOnUSB=0 //store pkgs on usb OPTIONAL
TTF_Font=/mnt/usb0/myfont.ttf // TTF Font the store will try to use (embedded font on fail)
BETA_KEY=xxxxxxxxxxxxxxxxxxxxxxxxxxxx //for Beta Builds (define)
Home_Redirection=1 // Enables the Home Menu Redirect
Daemon_on_start=0 // Disables the Daemon from auto starting with the app
Show_install_prog=1 // Enables the Store PKG/APP install Progress
Copy_INI=1 // Copies USB INI to the PS4s app dir.
```

## Daemon

The Store daemon is installed when you open the app for the first time at `/system/vsh/app/ITEM00002`
it gets Updated ONLY by the Store app auto

The daemon settings file is ONLY for internal use by the Store devs
however it also has a ini at `/system/vsh/app/ITEM00002/daemon.ini` with the following ini values

```
[Daemon]
version=4098// Daemon version for Store, Official version is always > 0x1000
```
 
## App details

upon booting up the app will cache all json files from the CDN in the settings.ini
restart is required for changes made to json files (for now)

- all images from jsons download to `/user/app/NPXS39041/storedata/`
- all game covers are cached at `/user/app/NPXS39041/covers`
- all jsons are downloaded to `/user/app/NPXS39041/pages`
- PKG Download folder `/user/app/NPXS39041/downloads` (download folder is cleared every app launch, not yet installed pkgs are there)
- Store log is at `/user/app/NPXS39041/logs/log.txt`

## Dumper details

- Dumper log is at `/user/app/NPXS39041/logs/itemzflow.log`
- Dumper WILL ONLY dump to the first USB it finds (most likely USB0)
- Dumper will try to use the Languages as the Store
- If the dumper gets stuck on a file make SURE **both** discs and **ALL** Languages are installed if available **BEFORE** Dumping
- If your dump has failed then provide us the log `/user/app/NPXS39041/logs/itemzflow.log` in our discord below

## Languages

- The Store's Langs. repo is [HERE](https://github.com/LightningMods/Store-Languages)
- The Store uses the PS4's System software Lang setting



IF the settings file is loaded from USB all settings will be saved to the same USB

ONLY 4 apps can download at once

## Official Discord server

Invite: https://discord.gg/GvzDdx9GTc

## Donations

We accept the following methods

- [Ko-fi](https://ko-fi.com/lightningmods)
- BTC: bc1qgclk220glhffjkgraju7d8xjlf7teks3cnwuu9

if you donate and dont want to the message anymore created this folder after donating ``

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
- [Sleirsgoevy](https://github.com/sleirsgoevy/)
- [AlAzif](https://github.com/al-azif)


