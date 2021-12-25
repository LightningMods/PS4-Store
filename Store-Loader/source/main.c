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

int ret;
int libnetMemId = 0;
int libsslCtxId = 0;
int libhttpCtxId = 0;
int secure_boot = false;

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



int update_version_by_hash(char* path)
{
	bool failed = false;
    int fd = sceKernelOpen(path, 0, 0);
    if (fd <= 0) {
    	return 2;
	}

	// Get the Update into mem
	int size = sceKernelLseek(fd, 0, SEEK_END);
	sceKernelLseek(fd, 0, SEEK_SET);

	void* buffer = NULL;
	int ret = sceKernelMmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0, &buffer);
	sceKernelClose(fd);

	if (buffer == NULL) {
		return 3;
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
			return 1;
		}

	}

	sceKernelMunmap(buffer, size);

	return 0;
}


int checkForUpdate(char *cdnbuf)
{       int UpdateRet = -1;
	    int fd = sceKernelOpen("/user/app/NPXS39041/homebrew.elf", 0x0000, 0x0000);
        UpdateRet = sceKernelOpen("/user/app/NPXS39041/local.md5", 0x0000, 0x0000);
        unlink("/user/app/NPXS39041/homebrew.elf.sig");

	logshit("[STORE_GL_Loader:%s:%i] ----- ELF fd = %i---\n", __FUNCTION__, __LINE__, fd);
	if (fd > 0 && UpdateRet > 0)
	{   
                close(fd);
                close(UpdateRet);
		logshit("[STORE_GL_Loader:%s:%i] ----- Comparing Hashs ---\n", __FUNCTION__, __LINE__);
		int comp = MD5_hash_compare("/user/app/NPXS39041/remote.md5", "/user/app/NPXS39041/local.md5");
		if (comp == DIFFERENT_HASH)
		{
			
         
			goto copy_update;
		}
		else
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- HASHS ARE THE SAME ---\n", __FUNCTION__, __LINE__);
			return 0;
		}
	}
	else
	{
               
		goto copy_update;
	}

copy_update:
msgok("Update Required\n");
	loadmsg("Downloading Update File...");
	unlink("/user/app/NPXS39041/homebrew.elf");
        unlink("/user/app/NPXS39041/local.md5");
        
     if(download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/homebrew.elf") == 200)
     {

	UpdateRet = copyFile("/user/app/NPXS39041/remote.md5", "/user/app/NPXS39041/local.md5");
	if (UpdateRet != 0)
	{
		return -1;
	}
	else
	{   unlink("/user/app/NPXS39041/remote.md5");
		msgok("Update Has been Applied\n");
                close(UpdateRet);
		return 0;
	}
     }
     else
		return -1;
}





int main()
{

	init_STOREGL_modules();

	int libjb = -1, librsa = -1, advanced_settings = 0, dl_ret = 0;
    char Devkit_M[250];
	char cdninpute[255];
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



        sys_dynlib_load_prx("/app0/Media/rsa.sprx", &librsa);
        sys_dynlib_load_prx("/app0/Media/jb.prx", &libjb);

        ret = sys_dynlib_dlsym(libjb, "jailbreak_me", &jailbreak_me);
        if (!ret)
        {
          logshit("jailbreak_me resolved from PRX\n");
   
            if(jailbreak_me() != 0){
              msgok("FATAL Jailbreak failed with code: %x\n", ret); goto exit_sec;
            }
            else printf("jailbreak_me() returned %i\n", ret);
        }
        else{
          msgok("FATAL Jailbreak failed with code: %x\n", ret); goto exit_sec;}


		ret = sys_dynlib_dlsym(libjb, "rejail_multi", &rejail_multi);
		if (!ret)
		   logshit("rejail_multi resolved from PRX\n");
		else {
			msgok("FATAL rejail_multi failed with code: %x\n", ret); goto exit_sec;
		}
		


	if (!ret)
	{
		logshit("After jb\n");
		mkdir("/user/app/NPXS39041/storedata/", 0777);
		mkdir("/user/app/NPXS39041/logs/", 0777);
		unlink("/user/app/NPXS39041/logs/loader.log");
		mkdir("/user/app/NPXS39041/", 0777);

		

		logshit("[STORE_GL_Loader:%s:%i] -----  All Internal Modules Loaded  -----\n", __FUNCTION__, __LINE__);
		logshit("------------------------ Store Loader[GL] Compiled Time: %s @ %s EST -------------------------\n", __DATE__, __TIME__);
		logshit("[STORE_GL_Loader:%s:%i] -----  STORE Version: %s  -----\n", __FUNCTION__, __LINE__, completeVersion);
		logshit("----------------------------------------------- -------------------------\n");

		pl_ini_file init;

        int fd = sceKernelOpen("/mnt/usb0/settings.ini", 0x0000, 0);
 		if (fd > 0)
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- FOUND USB RECOVERY INI ---\n", __FUNCTION__, __LINE__);

			pl_ini_load(&init, "/mnt/usb0/settings.ini");

			pl_ini_get_string(&init, "Settings", "CDN", "http://api.pkg-zone.com", cdninpute, 255);

		    secure_boot = pl_ini_get_int(&init, "Settings", "Secure_boot", 1);


            if(pl_ini_get_int(&init, "Settings", "Copy_INI", 0))
				copyFile("/mnt/usb0/settings.ini", "/user/app/NPXS39041/settings.ini");


			logshit("[STORE_GL_Loader:%s:%i] ----- USB INI CDN: %s Secure Boot: %i ---\n", __FUNCTION__, __LINE__, cdninpute, secure_boot);
			
		} 
        else
        {
			
		fd = sceKernelOpen("/user/app/NPXS39041/settings.ini", 0x0000, 0);
		if (fd < 0)
		{

				logshit("[STORE_GL_Loader:%s:%i] ----- APP INI Not Found, Making ini ---\n", __FUNCTION__, __LINE__);
                
                pl_ini_file file;
			    pl_ini_create(&file);
			    pl_ini_set_string(&file, "Settings", "CDN", "http://api.pkg-zone.com");
			    pl_ini_set_int(&file, "Settings", "Secure_boot", 1);
			    pl_ini_set_string(&file, "Settings", "temppath", "/user/app");
			    pl_ini_set_int(&file, "Settings", "StoreOnUSB", 0);
			    pl_ini_save(&file, "/user/app/NPXS39041/settings.ini");

				snprintf(cdninpute, 255, "http://api.pkg-zone.com");

				secure_boot = true;

		}
					
		else
		{
				logshit("[STORE_GL_Loader:%s:%i] ----- INI FOUND ---\n", __FUNCTION__, __LINE__);

				pl_ini_load(&init, "/user/app/NPXS39041/settings.ini");

				pl_ini_get_string(&init, "Settings", "CDN", "http://api.pkg-zone.com", cdninpute, 255);

				secure_boot = pl_ini_get_int(&init, "Settings", "Secure_boot", 1);

				advanced_settings = pl_ini_get_int(&init, "Settings", "Advanced_enabled", 0);
				if (advanced_settings)
				    pl_ini_get_string(&init, "Settings", "CDN_For_Devkit_Modules", "http://api.pkg-zone.com/", Devkit_M, 255);
				



		}
           
      }
		
#if IS_INTERNAL == 1
		snprintf(cdninpute, 254, "http://192.168.123.178");
#endif


	start:

		logshit("[STORE_GL_Loader:%s:%i] ----- CDN Url = %s ---\n", __FUNCTION__, __LINE__, cdninpute);

		ret = pingtest(libnetMemId, libhttpCtxId, cdninpute);
		if (ret != 0)
			msgok("Connection to the CDN Failed with %x\n", ret);
		else if (ret == 0)
		{
			logshit("[STORE_GL_Loader:%s:%i] ----- Ping Successfully ---\n", __FUNCTION__, __LINE__);


#if IS_INTERNAL == 1
			int dl_ret = download_file(libnetMemId, libhttpCtxId, cdnbuf, "/data/homebrew.elf.new");
#else
			snprintf(cdnbuf, 254, "%s/update/remote.md5", cdninpute);

			dl_ret = download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/remote.md5");
			if (dl_ret != 200)
				goto err;


			fd = sceKernelOpen("/user/app/NPXS39041/pig.sprx", 0x0000, 0);
			if (fd < 0)
			{
				loadmsg("Downloading Devkit Modules...\n");
				logshit("[STORE_GL_Loader:%s:%i] ----- Piglet NOT Found Downloading.... ---\n", __FUNCTION__, __LINE__);

				if (advanced_settings != 0)
					snprintf(cdnbuf, 255, "%s/pig.sprx", Devkit_M);
				else
					snprintf(cdnbuf, 255, "%s/pig.sprx", cdninpute);

				dl_ret = download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/pig.sprx");
				if (dl_ret != 200)
					goto err;
			}
			else
				logshit("[STORE_GL_Loader:%s:%i] ----- Piglet already downloaded ---\n", __FUNCTION__, __LINE__);

			fd = sceKernelOpen("/user/app/NPXS39041/shacc.sprx", 0x0000, 0);
			if (fd < 0)
			{
				logshit("[STORE_GL_Loader:%s:%i] ----- Shacc NOT Found Downloading.... ---\n", __FUNCTION__, __LINE__);

				if (advanced_settings != 0)
					snprintf(cdnbuf, 255, "%s/shacc.sprx", Devkit_M);
				else
					snprintf(cdnbuf, 255, "%s/shacc.sprx", cdninpute);

				dl_ret = download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/shacc.sprx");

				if (dl_ret != 200)
					goto err;
			}
			else
				logshit("[STORE_GL_Loader:%s:%i] ----- Shacc already downloaded ---\n", __FUNCTION__, __LINE__);

			sceMsgDialogTerminate();

#endif

			logshit("[STORE_GL_Loader:%s:%i] ----- dl_ret = %i ---\n", __FUNCTION__, __LINE__, dl_ret);

			if (dl_ret != 200)
			{
				logshit("[STORE_GL_Loader:%s:%i] ----- dl_ret = %i ---\n", __FUNCTION__, __LINE__, dl_ret);
				int usb_fd = -1;
				while (usb_fd < 0)
				{
					msgok("CDN Invaild\n\n CDN Url = %s\n\n Error = %i\n\n provide a new settings.ini file via USB root with a correct CDN", cdninpute, dl_ret);

					usb_fd = sceKernelOpen("/mnt/usb0/settings.ini", O_RDONLY, O_RDONLY);
					if (usb_fd > 0)
					{
						pl_ini_load(&init, "/mnt/usb0/settings.ini");

						pl_ini_get_string(&init, "Settings", "CDN", "N/A", cdninpute, 255);

						if (strstr(cdninpute, "N/A") != NULL)
						{
							msgok("the CDN URL on the settings.ini file is INVAILD \n insert a settings file with a VAILD CDN");

							usb_fd = -1;
						}
						else if (strstr(cdninpute, "N/A") == NULL)
						{
							goto start;
						}
					}

					sleep(1);
				}
			}

			else
			{

				if (true)
				{
					logshit("[STORE_GL_Loader:%s:%i] ----- CheckForUpdate() ---\n", __FUNCTION__, __LINE__);

					snprintf(cdnbuf, 254, "%s/update/homebrew.elf", cdninpute);
					if (checkForUpdate(cdnbuf) == 0)
					{
						mkdir("/data/self", 0777);
						unlink("/data/self/eboot.bin");

						if (secure_boot)
						{
							snprintf(cdnbuf, 254, "%s/update/homebrew.elf.sig", cdninpute);
							logshit("[STORE_GL_Loader:%s:%i] ----- Secure Boot is ENABLED ---\n", __FUNCTION__, __LINE__);

							logshit("[STORE_GL_Loader:%s:%i] ----- Checking Revocation list..... ---\n", __FUNCTION__, __LINE__);

							if (update_version_by_hash("/user/app/NPXS39041/homebrew.elf") == 0)
								logshit("[STORE_GL_Loader:%s:%i] ----- Update is NOT part of the revoked listed ---\n", __FUNCTION__, __LINE__);
							else
							{

								msgok("FATAL!\n\n [SWU Error] Update IS Revoked\n\n to use this Update turn off Secure Boot");
								goto exit_sec;
							}


							logshit("[STORE_GL_Loader:%s:%i] ----- Downloading RSA Sig CDN: %s ---\n", __FUNCTION__, __LINE__, cdninpute);
							if (download_file(libnetMemId, libhttpCtxId, cdnbuf, "/user/app/NPXS39041/homebrew.elf.sig") == 200)
							{
								loadmsg("Checking RSA Sig...\n");

								ret = sys_dynlib_dlsym(librsa, "VerifyRSA", &VerifyRSA);
								if (!ret)
								{
									logshit("VerifyRSA resolved from PRX\n");

									if (ret = VerifyRSA("/user/app/NPXS39041/homebrew.elf", "/mnt/sandbox/NPXS39041_000/app0/Media/rsa.pub") != 0)
									{

										msgok("FATAL!\n\n RSA Check has failed with Error Code %x\n", ret);
										goto exit_sec;
									}
									else
										logshit("Success!\n\n RSA Check has passed\n");


									logshit("VerifyRSA from PRX return %x\n", ret);
								}
								else {
									msgok("FATAL!\n\n could not resolve RSA PRX\n"); goto exit_sec;
								}


							}
							else
							{
								msgok("FATAL!\n\nSecure Boot is enabled but we couldnt\n Download the Sig file from the CDN"); goto exit_sec;
							}



						}

						logshit("[STORE_GL_Loader:%s:%i] ----- Cleaning Up Network ---\n", __FUNCTION__, __LINE__);

						sceHttpTerm(libhttpCtxId);
						sceSslTerm(libsslCtxId);

						sceNetPoolDestroy();
						sceNetTerm();

						copyFile("/user/app/NPXS39041/homebrew.elf", "/data/self/eboot.bin");

						logshit("[STORE_GL_Loader:%s:%i] ----- calling rejail_multi ---\n", __FUNCTION__, __LINE__);
						rejail_multi();

						logshit("[STORE_GL_Loader:%s:%i] ----- Launching() ---\n", __FUNCTION__, __LINE__);
						ret = sceSystemServiceLoadExec("/data/self/eboot.bin", 0);
						if (ret == 0)
							logshit("[STORE_GL_Loader:%s:%i] ----- Launched (shouldnt see) ---\n", __FUNCTION__, __LINE__);
					}
					else
						msgok("Update has FAILED please reinstall the pkg\n");

				}
				else
					logshit("[STORE_GL_Loader:%s:%i] -----  JAILBREAK FAILED  -----\n", __FUNCTION__, __LINE__);

			}


		}//else if (ret == 0)
	} // else if (ret == 0)
	else
		logshit("[STORE_GL_Loader:%s:%i] -----  loadModulesGl() FAILED  -----\n", __FUNCTION__, __LINE__);

err:
   msgok("The Store Loader has encountered an error\n\nFor more info check: /user/app/NPXS39041/logs/loader.log\nThe App will now Close");

exit_sec:
   if (rejail_multi != NULL)
   {
	   logshit("Rejailing App");
	   rejail_multi();
	   printf("App rejailed\n");
   }

  sceSystemServiceLoadExec("exit", 0);

return -1;
}

void catchReturnFromMain(int exit_code)
{
}