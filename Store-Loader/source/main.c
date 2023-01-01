#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <MsgDialog.h>
#include <sys/stat.h>	
#include "Header.h"
#include <orbis/SystemService.h>

#include <errno.h>
#include "lang.h"
#include "ini.h"
#include "../external/Jailbreak PRX/include/multi-jb.h"
#include <curl/curl.h>

int ret;

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
	if(!f1) return DIFFERENT_HASH;
	
	FILE* f2 = fopen(file2, "rb");
	if(!f2){
	  fclose(f1);
          return DIFFERENT_HASH;
	}
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
	progstart(getLangSTR(DOWNLOADING_UPDATE));
	unlink("/user/app/NPXS39041/homebrew.elf");
    unlink("/user/app/NPXS39041/local.md5");
        
     if(download_file(cdnbuf, "/user/app/NPXS39041/homebrew.elf") == 200)
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

	StoreLoaderOptions* pconfig = (StoreLoaderOptions*)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0


	logshit("%s = %s\n", name, value);

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
void loader_rooted(){
	StoreLoaderOptions config;
	int dl_ret = 0;
	char cdnbuf[255];
    char buff[600];

	    logshit("======== HELLO FROM ROOTED LOADER ======\n");
		mkdir("/user/app/NPXS39041/", 0777);
		mkdir("/user/app/NPXS39041/storedata/", 0777);
		mkdir("/user/app/NPXS39041/logs/", 0777);
		unlink("/user/app/NPXS39041/logs/loader.log");
		

		logshit("[STORE_GL_Loader:%s:%i] -----  All Internal Modules Loaded  -----\n", __FUNCTION__, __LINE__);
		logshit("------------------------ Store Loader[GL] Compiled Time: %s @ %s EST -------------------------\n", __DATE__, __TIME__);
		logshit("[STORE_GL_Loader:%s:%i] -----  STORE Version: %s  -----\n", __FUNCTION__, __LINE__, completeVersion);
		logshit("----------------------------------------------- -------------------------\n");

		config.opt[CDN_URL] = "https://api.pkg-zone.com";
		config.SECURE_BOOT = true;
		config.Copy_INI = false;

 		if (if_exists("/mnt/usb0/settings.ini"))
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- FOUND USB RECOVERY INI ---\n", __FUNCTION__, __LINE__);

			int error = ini_parse("/mnt/usb0/settings.ini", print_ini_info, &config);
			if (error) {
				logshit("Bad config file (first error on line %d)!\n", error);
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

				memset(&buff[0], 0, sizeof buff);
				snprintf(&buff[0], sizeof buff, "[Settings]\nCDN=https://api.pkg-zone.com\nSecure_Boot=1\ntemppath=/user/app/NPXS39041/downloads\nStoreOnUSB=0\nShow_install_prog=1\nHomeMenu_Redirection=0\nDaemon_on_start=1\nLegacy=0\n");

				int fd = open("/user/app/NPXS39041/settings.ini", O_WRONLY | O_CREAT | O_TRUNC, 0777);
				if (fd >= 0)
				{
					write(fd, &buff[0], strlen(&buff[0]));
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
		

		logshit("[STORE_GL_Loader:%s:%i] ----- CDN Url = %s ---\n", __FUNCTION__, __LINE__, config.opt[CDN_URL]);

		ret = pingtest(config.opt[CDN_URL]);
		if (ret != 0)
			msgok("%s: %s\n",getLangSTR(PING_FAILED), curl_easy_strerror(ret));
		else if (ret == 0)
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- Ping Successfully ---\n", __FUNCTION__, __LINE__);

			snprintf(cdnbuf, 254, "%s/update/remote.md5", config.opt[CDN_URL]);

			sceMsgDialogTerminate();

			if ((dl_ret = download_file(cdnbuf, "/user/app/NPXS39041/remote.md5")) == 200){
				logshit("[STORE_GL_Loader:%s:%i] ----- Downloaded remote.md5 ---\n", __FUNCTION__, __LINE__);
				if (VerifyRSA)
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
							if (download_file(cdnbuf, "/user/app/NPXS39041/homebrew.elf.sig") == 200)
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
								else{
									logshit("Success!\n\n RSA Check has passed\n");
								}

							   logshit("VerifyRSA from PRX return %x\n", ret);
							}
							else {
								msgok("%s\n\n %s", getLangSTR(LOADER_FATAL),getLangSTR(SECURE_FAIL));
								goto exit_sec;
							}
						}

						if (copyFile("/user/app/NPXS39041/homebrew.elf", "/data/self/Store.self") != 0) 
						    goto err;
						else
						   return;
					}
					else
						msgok(getLangSTR(REINSTALL_PKG));

				}
				else
					logshit("[STORE_GL_Loader:%s:%i] -----  RSA Function is NULL! and could not be resolved  -----\n", __FUNCTION__, __LINE__);
			}
			else 
				goto err;
	} // else if (ret == 0)
	else
    	logshit("[STORE_GL_Loader:%s:%i] -----  JAILBREAK FAILED  -----\n", __FUNCTION__, __LINE__);

err:
   msgok("%s\n\n%s\n", getLangSTR(LOADER_ERROR), getLangSTR(MORE_INFO));
exit_sec:
   sceSystemServiceLoadExec("exit", 0);
}

int main(int argc, char* argv[])
{
	int librsa = -1;

	init_STOREGL_modules();

	sceSystemServiceHideSplashScreen();
	sceCommonDialogInitialize();
    sceMsgDialogInitialize();

    curl_global_init(CURL_GLOBAL_ALL);

	if (!LoadLangs(PS4GetLang())) {
		if (!LoadLangs(0x01)) {
			msgok("Failed to load Backup Lang...\nThe App is unable to continue"); 
			goto exit_sec;
		}
		else
			logshit("Loaded the backup, lang %i failed to load", PS4GetLang());
	}

    librsa = sceKernelLoadStartModule("/app0/Media/rsa.prx", 0, NULL, 0, 0, 0);
    int ret = sceKernelDlsym(librsa, "VerifyRSA", (void**)&VerifyRSA);
    if (ret >= 0){
        logshit("VerifyRSA resolved from PRX\n");
        if (!VerifyRSA){
             msgok("%s: %x\n", getLangSTR(FATAL_JB),ret); 
			 goto err;
		}
		else{
			jbc_run_as_root(loader_rooted, NULL, CWD_ROOT);
			logshit("jbc_run_as_root() succesful\n");
		}
    }
	else{
        msgok("%s: %x\n", getLangSTR(FATAL_JB),ret); 
		goto err;
	}

	logshit("[STORE_GL_Loader:%s:%i] ----- Launching() ---\n", __FUNCTION__, __LINE__);
	if (sceSystemServiceLoadExec("/data/self/Store.self", (const char**)argv) == 0)
	    logshit("[STORE_GL_Loader:%s:%i] ----- Launched (shouldnt see) ---\n", __FUNCTION__, __LINE__);

err:
   msgok("%s\n\n%s\n", getLangSTR(LOADER_ERROR), getLangSTR(MORE_INFO));
exit_sec:
   return sceSystemServiceLoadExec("exit", 0);
}
void catchReturnFromMain(int exit_code)
{
}
