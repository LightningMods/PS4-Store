
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <orbis/libkernel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mira_header.h"

#define ENABLE_MIRA_API 0

 extern "C"
{

#include "multi-jb.h"
}

#define JAILBREAK_FAILED -1
#define SUCCESS 0

int mira_jailbreak()
{

	MiraThreadCredentials get_param;
	get_param.State = GSState::Get;
	get_param.ThreadId = 0;
	get_param.ProcessId = getpid();

	MiraThreadCredentials param;
	param.State = GSState::Set;
	param.ThreadId = 0;
	param.RealUserId = 0;
	param.RealGroupId = 0;
	param.EffectiveUserId = 0;
	param.ProcessId = getpid();
	param.Prison = MiraThreadCredentialsPrison::Root;


	int mira_fd = open("/dev/mira", 0x000, 0x000);
	if (mira_fd >= 0) {

		printf("opened mira now doing a ioctl\n");

		int io_ret = ioctl(mira_fd, 0xC0704D01, &get_param);

		io_ret = ioctl(mira_fd, 0xC0704D01, &param);

		if (io_ret == 0) {

			int ret = sceKernelOpen("/user/.test", 0x0001 | 0x0200, 0777);
			printf("ret %d\n", ret);
			if (ret >= 0)
			{
				close(ret);
				close(mira_fd);
				unlink("/user/.test");

				printf("Jailbreak Successfully\n");
				return SUCCESS;
			}
			close(mira_fd);
		
		}
	}

	return JAILBREAK_FAILED;
}//

extern "C"
{

int jailbreak_me(void)
{

     int ret = JAILBREAK_FAILED;    

#if ENABLE_MIRA_API==1
     if (mira_jailbreak() == JAILBREAK_FAILED)
#endif
        ret = jailbreak_multi();
        

     


    return ret;
}
}
