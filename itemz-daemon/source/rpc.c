#include "defines.h"
#include <utils.h>
#include <sys/signal.h>
#include "log.h"
#include <ps4sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/socket.h>

uint8_t CONNECT_TEST(struct clientArgs* client, uint8_t** mode, uint32_t* length)
{
	// Re-allocate memory
	*mode = realloc(*mode, 99);

	sprintf((char*)(*mode), "Connected\0");
	*length = strlen((char*)(*mode));

	return NO_ERROR;
}

extern bool is_enabled;


void handleIPC(struct clientArgs* client, uint8_t* buffer, uint32_t length)
{
	uint8_t error = NO_ERROR;
	uint8_t* outputBuffer = malloc(sizeof(uint8_t*));
	uint32_t outputLength = 0;


	uint8_t method = buffer[0];
	switch (method)
	{

	case CONNECTION_TEST: {
		log_info("[Daemon IPC][client %i] command CONNECTION_TEST() called", client->cl_nmb);
		error = CONNECT_TEST(client, &outputBuffer, &outputLength);
		break;
	}
	case DISABLE_HOME_REDIRECT: {
		log_info("[Daemon IPC][client %i] command DISABLE_HOME_REDIRECT() called", client->cl_nmb);
		is_enabled = false;
		error = NO_ERROR;
		break;
	}
	case  ENABLE_HOME_REDIRECT: {
		log_info("[Daemon IPC][client %i] command ENABLE_HOME_REDIRECT() called", client->cl_nmb);
		is_enabled = true;
		error = NO_ERROR;
		break;
	}
	case DEAMON_UPDATE: {
		log_info("[Daemon IPC][client %i] command DEAMON_UPDATE() called", client->cl_nmb);
		unlink("/system_tmp/IPC_init");
		unlink("/system_tmp/IPC_Socket");
		log_info("[Daemon IPC][client %i] Reloading Daemon ...", client->cl_nmb);
		sceSystemServiceLoadExec("/app0/eboot.bin", 0);
		error = NO_ERROR;
		break;
	}

	default:{
	  log_info("[Daemon IPC][client %i] command %i called", client->cl_nmb, buffer[0]);
	  error = INVALID;
	  break;
	}


	}


	uint8_t* outputBufferFull = malloc(outputLength + 1);

	outputBufferFull[0] = error; // First byte is always error byte

	memcpy(&outputBufferFull[1], outputBuffer, outputLength);


	// Send response
	networkSendData(client->socket, outputBufferFull, outputLength + 1);


	// Free allocated memory
	free(outputBuffer);
	outputBuffer = NULL;
	free(outputBufferFull);
	outputBufferFull = NULL;
}

void* ipc_client(void* args)
{
	struct clientArgs* cArgs = (struct clientArgs*)args;


	log_debug("[Daemon IPC] Thread created, Socket %i", cArgs->socket);


	uint32_t readSize = 0;
	uint8_t buffer[512];
	while ((readSize = networkReceiveData(cArgs->socket, buffer, 512)) > 0)
	{
		// Handle buffer
		handleIPC(cArgs, buffer, readSize);
		memset(buffer, 0, 512);
	}


	log_debug("[Daemon IPC][client %i] IPC Connection disconnected, Shutting down ...", cArgs->cl_nmb);


	networkCloseConnection(cArgs->socket);
	scePthreadExit(NULL);
	free(args);
	return NULL;
}
