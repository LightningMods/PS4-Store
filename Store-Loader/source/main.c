#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include <MsgDialog.h>

#include "Header.h"
#include <ps4sdk.h>
#include <errno.h>
#include "lang.h"
#include "ini.h"

int ret;
int libnetMemId = 0;
int libsslCtxId = 0;
int libhttpCtxId = 0;

struct revocation_list {
	char version[50];
	char hash[50];
};



struct revocation_list revoke_list[] = {
	{"1.00", "339bc526059dbec608b9002edb977d48"},
	{"1.01", "0c8187f5a1b4266c4f796b054231cbec"},
	{"1.02.5", "928dafc24a6e4120acb3925ca77c7edd"},
};


static int (*VerifyRSA)(const char* path, const char* pubkey) = NULL;
static int (*jailbreak_me)(void) = NULL;
static int (*rejail_multi)(void) = NULL;

int64_t sys_dynlib_load_prx(char* prxPath, int* moduleID)
{
	return (int64_t)syscall4(594, prxPath, 0, moduleID, 0);
}

int64_t sys_dynlib_unload_prx(int64_t prxID)
{
	return (int64_t)syscall1(595, (void*)prxID);
}


int64_t sys_dynlib_dlsym(int64_t moduleHandle, const char* functionName, void *destFuncOffset)
{
	return (int64_t)syscall3(591, (void*)moduleHandle, (void*)functionName, destFuncOffset);
}

const unsigned char completeVersion[] = {VERSION_MAJOR_INIT,
										 '.',
										 VERSION_MINOR_INIT,
										 '-',
										 'V',
										 '-',
										 BUILD_YEAR_CH0,
										 BUILD_YEAR_CH1,
										 BUILD_YEAR_CH2,
										 BUILD_YEAR_CH3,
										 '-',
										 BUILD_MONTH_CH0,
										 BUILD_MONTH_CH1,
										 '-',
										 BUILD_DAY_CH0,
										 BUILD_DAY_CH1,
										 'T',
										 BUILD_HOUR_CH0,
										 BUILD_HOUR_CH1,
										 ':',
										 BUILD_MIN_CH0,
										 BUILD_MIN_CH1,
										 ':',
										 BUILD_SEC_CH0,
										 BUILD_SEC_CH1,
										 '\0'};


int copyFile(char* sourcefile, char* destfile)
{
	int src = sceKernelOpen(sourcefile, 0x0000, 0);
	if (src > 0)
	{
		logshit("open");
		int out = sceKernelOpen(destfile, 0x0001 | 0x0200 | 0x0400, 0777);
		if (out > 0)
		{
			size_t bytes;
			char* buffer = (char*)malloc(65536);
			if (buffer != NULL)
			{
				while (0 < (bytes = sceKernelRead(src, buffer, 65536)))
					sceKernelWrite(out, buffer, bytes);
				free(buffer);
			}
			sceKernelClose(out);
		}
		else
           return -1;

		
		sceKernelClose(src);
		return 0;
	}
	else
	{
		logshit("[ELFLOADER] fuxking error\n");
        logshit("[STORE_GL_Loader:%s:%i] ----- src fd = %i---\n", __FUNCTION__, __LINE__, src);
		return -1;
	}
}

int MD5_hash_compare(const char* file1, const char* file2)
{
	unsigned char c[MD5_HASH_LENGTH];
	unsigned char c2[MD5_HASH_LENGTH];
	int i;
	FILE* f1 = fopen(file1, "rb");
	FILE* f2 = fopen(file2, "rb");
	MD5_CTX mdContext;

	MD5_CTX mdContext2;
	int bytes2 = 0;
	unsigned char data2[1024];

	int bytes = 0;
	unsigned char data[1024];

	MD5_Init(&mdContext);
	while ((bytes = fread(data, 1, 1024, f1)) != 0)
		MD5_Update(&mdContext, data, bytes);
	MD5_Final(c, &mdContext);

	MD5_Init(&mdContext2);
	while ((bytes2 = fread(data2, 1, 1024, f2)) != 0)
		MD5_Update(&mdContext2, data2, bytes2);
	MD5_Final(c2, &mdContext2);

	for (i = 0; i < 16; i++)
	{
		if (c[i] != c2[i])
		{
			return DIFFERENT_HASH;
		}
	}

	fclose(f1);
	fclose(f2);

	return SAME_HASH;
}


bool if_exists(const char* path)
{
	int dfd = open(path, O_RDONLY, 0); // try to open dir
	if (dfd < 0) {
		logshit("path %s, errno %s", path, strerror(errno));
		return false;
	}
	else
		close(dfd);

	return true;
}



bool update_version_by_hash(char* path)
{
	bool failed = false;
    int fd = sceKernelOpen(path, 0, 0);
	if (fd <= 0) {
		logshit("Cant open %s\n", path);
		return false;
	}
	
	// Get the Update into mem
	int size = sceKernelLseek(fd, 0, SEEK_END);
	sceKernelLseek(fd, 0, SEEK_SET);

	void* buffer = NULL;
	int ret = sceKernelMmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0, &buffer);
	if (ret < 0)
		logshit("MMAP Error: %s\n", strerror(errno));

	sceKernelClose(fd);

	if (buffer == NULL) {
		logshit("buffer is null\n");
		return false;
	}
	

	// Create MD5 hash
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, buffer, size);

	unsigned char digest[16] = {0};
	MD5_Final(digest, &ctx);

	char md5_string[33] = {0};

	for(int i = 0; i < 16; ++i) {
	    sprintf(&md5_string[i*2], "%02x", (unsigned int)digest[i]);
	}
	md5_string[32] = 0;

	for (int y = 0; y < (sizeof(revoke_list) / sizeof(revoke_list[0])); y++) {

	     logshit("Update Hash: %s, current checked revoked hash: %s\n", md5_string, revoke_list[y].hash);
         if (strcmp(md5_string, revoke_list[y].hash) == 0) {
			logshit("------- Update IS revoked ---------\n");
			return false;
		}

	}

	sceKernelMunmap(buffer, size);

	return true;
}


bool checkForUpdate(char *cdnbuf)
{
    unlink("/user/app/NPXS39041/homebrew.elf.sig");

	if (if_exists("/user/app/NPXS39041/homebrew.elf") && if_exists("/user/app/NPXS39041/local.md5"))
	{
		logshit("[STORE_GL_Loader:%s:%i] ----- ELF exists ---\n", __FUNCTION__, __LINE__);
		logshit("[STORE_GL_Loader:%s:%i] ----- Comparing Hashs ---\n", __FUNCTION__, __LINE__);
		int comp = MD5_hash_compare("/user/app/NPXS39041/remote.md5", "/user/app/NPXS39041/local.md5");
		if (comp == DIFFERENT_HASH)
             goto copy_update;
        else
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- HASHS ARE THE SAME ---\n", __FUNCTION__, __LINE__);
			return true;
		}
	}
	else
      goto copy_update;
	

copy_update:
    msgok(getLangSTR(UPDATE_REQ));
	loadmsg(getLangSTR(DOWNLOADING_UPDATE));
	unlink("/user/app/NPXS39041/homebrew.elf");
    unlink("/user/app/NPXS39041/local.md5");
        
     if(download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/homebrew.elf") == 200)
     {
	     if (copyFile("/user/app/NPXS39041/remote.md5", "/user/app/NPXS39041/local.md5") != 0)
			 return false;
	     else{  
		   unlink("/user/app/NPXS39041/remote.md5");
		   msgok(getLangSTR(UPDATE_APPLIED));
		   return true;
		 }
     }
     else
		return false;
}

static int print_ini_info(void* user, const char* section, const char* name,
	const char* value)
{
	static char prev_section[50] = "";

	if (strcmp(section, prev_section)) {
		logshit("%s[%s]\n", (prev_section[0] ? "\n" : ""), section);
		strncpy(prev_section, section, sizeof(prev_section));
		prev_section[sizeof(prev_section) - 1] = '\0';
	}
	logshit("%s = %s\n", name, value);

	StoreLoaderOptions* pconfig = (StoreLoaderOptions*)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	if (MATCH("Settings", "Secure_Boot")) {
		pconfig->SECURE_BOOT = atoi(value);
	}
	else if (MATCH("Settings", "CDN")) {
		pconfig->opt[CDN_URL] = strdup(value);
	}
	else if (MATCH("Settings", "Copy_INI")) {
		pconfig->Copy_INI = atoi(value);
	}

	return 1;
}

static bool touch_file(char* destfile)
{
	int fd = open(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd > 0) {
		close(fd);
		return true;
	}
	else
		return false;
}

int main()
{

	init_STOREGL_modules();

	int libjb = -1, librsa = -1, dl_ret = 0;
	char cdnbuf[255];

	sceSystemServiceHideSplashScreen();
	sceCommonDialogInitialize();
    sceMsgDialogInitialize();

	ret = netInit();
	if (ret < 0) 
		logshit("netInit() error: 0x%08X\n", ret);
	
	libnetMemId = ret;

	ret = sceSslInit(SSL_HEAP_SIZE);
	if (ret < 0)
		logshit("sceSslInit() error %x\n", ret);

	libsslCtxId = ret;

	logshit("libsslCtxId = %x\n", libsslCtxId);


	ret = sceHttpInit(libnetMemId, libsslCtxId, HTTP_HEAP_SIZE);
	if (ret < 0)
		logshit("sceHttpInit() error %x\n", ret);

	libhttpCtxId = ret;


	if (!LoadLangs(PS4GetLang())) {
		if (!LoadLangs(0x01)) {
			msgok("Failed to load Backup Lang...\nThe App is unable to continue"); 
			goto exit_sec;
		}
		else
			logshit("Loaded the backup, lang %i failed to load", PS4GetLang());
	}

     sys_dynlib_load_prx("/app0/Media/rsa.sprx", &librsa);
     sys_dynlib_load_prx("/app0/Media/jb.prx", &libjb);

     if (!sys_dynlib_dlsym(libjb, "jailbreak_me", &jailbreak_me))
     {
          logshit("jailbreak_me resolved from PRX\n");
   
            if((ret = jailbreak_me()) != 0){//
				msgok("%s: %x\n", getLangSTR(FATAL_JB), ret); goto exit_sec;
            }
            else 
				printf("jailbreak_me() returned %i\n", ret);
     }
     else{
        msgok("%s: %x\n", getLangSTR(FATAL_JB),ret); goto exit_sec;
	 }

	 if (!sys_dynlib_dlsym(libjb, "rejail_multi", &rejail_multi))
	     logshit("rejail_multi resolved from PRX\n");
	 else {
		msgok("%s: %x\n", getLangSTR(FATAL_REJAIL),ret); goto exit_sec;
	 }
		


	if (jailbreak_me != NULL && rejail_multi != NULL)
	{
		logshit("After jb\n");
		mkdir("/user/app/NPXS39041/", 0777);
		mkdir("/user/app/NPXS39041/storedata/", 0777);
		mkdir("/user/app/NPXS39041/logs/", 0777);
		unlink("/user/app/NPXS39041/logs/loader.log");
		StoreLoaderOptions config;
		

		logshit("[STORE_GL_Loader:%s:%i] -----  All Internal Modules Loaded  -----\n", __FUNCTION__, __LINE__);
		logshit("------------------------ Store Loader[GL] Compiled Time: %s @ %s EST -------------------------\n", __DATE__, __TIME__);
		logshit("[STORE_GL_Loader:%s:%i] -----  STORE Version: %s  -----\n", __FUNCTION__, __LINE__, completeVersion);
		logshit("----------------------------------------------- -------------------------\n");

		config.opt[CDN_URL] = "http://api.pkg-zone.com";
		config.SECURE_BOOT = true;
		config.Copy_INI = false;

 		if (if_exists("/mnt/usb0/settings.ini"))
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- FOUND USB RECOVERY INI ---\n", __FUNCTION__, __LINE__);

			int error = ini_parse("/mnt/usb0/settings.ini", print_ini_info, &config);
			if (error) {
				printf("Bad config file (first error on line %d)!\n", error);
			}
			else {

				if (config.Copy_INI)
					copyFile("/mnt/usb0/settings.ini", "/user/app/NPXS39041/settings.ini");
			}

			logshit("[STORE_GL_Loader:%s:%i] ----- USB INI CDN: %s Secure Boot: %i ---\n", __FUNCTION__, __LINE__, config.opt[CDN_URL], config.SECURE_BOOT);
		} 
        else
        {
		    if (!if_exists("/user/app/NPXS39041/settings.ini"))
		    {
				logshit("[STORE_GL_Loader:%s:%i] ----- APP INI Not Found, Making ini ---\n", __FUNCTION__, __LINE__);

				char* buff[1024];
				memset(buff, 0, 1024);
				snprintf(buff, 1023, "[Settings]\nCDN=http://api.pkg-zone.com\nSecure_Boot=1\ntemppath=/user/app/NPXS39041/downloads\nStoreOnUSB=0\nShow_install_prog=1\nHomeMenu_Redirection=0\nDaemon_on_start=1\nLegacy=0\n");

				int fd = open("/user/app/NPXS39041/settings.ini", O_WRONLY | O_CREAT | O_TRUNC, 0777);
				if (fd >= 0)
				{
					write(fd, buff, strlen(buff));
					close(fd);
				}
				else
					logshit("Could not make create INI File");

				mkdir("/user/app/NPXS39041/downloads", 0777);
				config.SECURE_BOOT = true;

		    }			
		    else
		    {
				logshit("[STORE_GL_Loader:%s:%i] ----- INI FOUND ---\n", __FUNCTION__, __LINE__);
				int error = ini_parse("/user/app/NPXS39041/settings.ini", print_ini_info, &config);
				if (error) {
					printf("Bad config file (first error on line %d)!\n", error);
				}
		    }
           
      }
		


	start:

		logshit("[STORE_GL_Loader:%s:%i] ----- CDN Url = %s ---\n", __FUNCTION__, __LINE__, config.opt[CDN_URL]);

		ret = pingtest(libnetMemId, libhttpCtxId, config.opt[CDN_URL]);
		if (ret != 0)
			msgok("%s: 0x%x\n",getLangSTR(PING_FAILED), ret);
		else if (ret == 0)
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- Ping Successfully ---\n", __FUNCTION__, __LINE__);

			snprintf(cdnbuf, 254, "%s/update/remote.md5", config.opt[CDN_URL]);

			sceMsgDialogTerminate();

			if ((dl_ret = download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/remote.md5")) == 200){

				if (!sys_dynlib_dlsym(librsa, "VerifyRSA", &VerifyRSA) && VerifyRSA != NULL)
				{
					logshit("[STORE_GL_Loader:%s:%i] ----- CheckForUpdate() ---\n", __FUNCTION__, __LINE__);

					snprintf(cdnbuf, 254, "%s/update/homebrew.elf", config.opt[CDN_URL]);
					if (checkForUpdate(cdnbuf))
					{
						mkdir("/data/self", 0777);
						unlink("/data/self/eboot.bin");

						if (config.SECURE_BOOT)
						{
							snprintf(cdnbuf, 254, "%s/update/homebrew.elf.sig", config.opt[CDN_URL]);
							logshit("[STORE_GL_Loader:%s:%i] ----- Secure Boot is ENABLED ---\n", __FUNCTION__, __LINE__);
							logshit("[STORE_GL_Loader:%s:%i] ----- Checking Revocation list..... ---\n", __FUNCTION__, __LINE__);

							if (update_version_by_hash("/user/app/NPXS39041/homebrew.elf"))
								logshit("[STORE_GL_Loader:%s:%i] ----- Update is NOT part of the revoked listed ---\n", __FUNCTION__, __LINE__);
							else{
                                msgok("%s\n\n%s", getLangSTR(LOADER_FATAL),getLangSTR(SWU_ERROR));
								goto exit_sec;
							}


							logshit("[STORE_GL_Loader:%s:%i] ----- Downloading RSA Sig CDN: %s ---\n", __FUNCTION__, __LINE__, cdnbuf);
							if (download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/homebrew.elf.sig") == 200)
							{
								loadmsg(getLangSTR(RSA_LOAD));

								logshit("VerifyRSA resolved from PRX\n");

								if ((ret = VerifyRSA("/user/app/NPXS39041/homebrew.elf", "/mnt/sandbox/NPXS39041_000/app0/Media/rsa.pub")) != 0){
								   msgok("%s\n\n%s: %x\n", getLangSTR(LOADER_FATAL),getLangSTR(RSA_FAILED),ret);
								   unlink("/user/app/NPXS39041/homebrew.elf");
								   unlink("/user/app/NPXS39041/homebrew.elf.sig");
								   unlink("/user/app/NPXS39041/remote.md5");
								   unlink("/user/app/NPXS39041/local.md5");
								   goto exit_sec;
								}
								else
									logshit("Success!\n\n RSA Check has passed\n");

							   logshit("VerifyRSA from PRX return %x\n", ret);
							}
							else {
								msgok("%s\n\n %s", getLangSTR(LOADER_FATAL),getLangSTR(SECURE_FAIL));
								goto exit_sec;
							}
						}

						logshit("[STORE_GL_Loader:%s:%i] ----- Cleaning Up Network ---\n", __FUNCTION__, __LINE__);

						sceHttpTerm(libhttpCtxId);
						sceSslTerm(libsslCtxId);

						sceNetPoolDestroy();
						sceNetTerm();

						if (copyFile("/user/app/NPXS39041/homebrew.elf", "/data/self/Store.self") != 0) goto err;

						logshit("[STORE_GL_Loader:%s:%i] ----- calling rejail_multi ---\n", __FUNCTION__, __LINE__);
						rejail_multi();

						logshit("[STORE_GL_Loader:%s:%i] ----- Launching() ---\n", __FUNCTION__, __LINE__);
						if (sceSystemServiceLoadExec("/data/self/Store.self", 0) == 0)
							logshit("[STORE_GL_Loader:%s:%i] ----- Launched (shouldnt see) ---\n", __FUNCTION__, __LINE__);
					}
					else
						msgok(getLangSTR(REINSTALL_PKG));

				}
				else
					logshit("[STORE_GL_Loader:%s:%i] -----  RSA Function is NULL! and could not be resolved  -----\n", __FUNCTION__, __LINE__);
			}
			else 
				goto err;
		}//else if (ret == 0)
	} // else if (ret == 0)
	else
    	logshit("[STORE_GL_Loader:%s:%i] -----  JAILBREAK FAILED  -----\n", __FUNCTION__, __LINE__);

err:
   msgok("%s\n\n%s\n", getLangSTR(LOADER_ERROR), getLangSTR(MORE_INFO));

exit_sec:
   if (rejail_multi != NULL)
   {
	   logshit("Rejailing App");
	   rejail_multi();
	   printf("App rejailed\n");
   }

   return sceSystemServiceLoadExec("exit", 0);
}

void catchReturnFromMain(int exit_code)
{
}