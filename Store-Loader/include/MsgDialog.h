#ifndef _ORBIS_MSG_H_
#define _ORBIS_MSG_H_
#pragma once

#include <stdint.h>


#ifdef __cplusplus 
extern "C" {
#endif

#include <CommonDialog.h>

#define ORBIS_MSG_DIALOG_MODE_USER_MSG				(1)
#define ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT				(5)
#define ORBIS_MSG_DIALOG_BUTTON_TYPE_OK				(0)

typedef struct OrbisMsgDialogButtonsParam {
	const char *msg1;					
	const char *msg2;					
	char reserved[32];					
} OrbisMsgDialogButtonsParam;

typedef struct OrbisMsgDialogUserMessageParam {
	int32_t buttonType;				
	int :32;										
	const char *msg;								
	OrbisMsgDialogButtonsParam *buttonsParam;		
	char reserved[24];								
													
} OrbisMsgDialogUserMessageParam;

typedef struct OrbisMsgDialogSystemMessageParam {
	int32_t sysMsgType;		
	char reserved[32];								
} OrbisMsgDialogSystemMessageParam;

typedef struct OrbisMsgDialogProgressBarParam {
	int32_t barType;			
	int :32;										
	const char *msg;								
	char reserved[64];								
} OrbisMsgDialogProgressBarParam;



typedef struct OrbisMsgDialogParam {
	OrbisCommonDialogBaseParam baseParam; 	
	size_t size;									
	int32_t mode;							
	int :32;	
	OrbisMsgDialogUserMessageParam *userMsgParam;	
	OrbisMsgDialogProgressBarParam *progBarParam;		
	OrbisMsgDialogSystemMessageParam *sysMsgParam;	
	int32_t userId;					
	char reserved[40];								
	int :32;										
} OrbisMsgDialogParam;



typedef struct OrbisMsgDialogResult {
	int32_t mode;							
													
	int32_t result;									
													
	int32_t buttonId;					
													
	char reserved[32];								
													
} OrbisMsgDialogResult;

int sceMsgDialogTerminate();
int sceMsgDialogInitialize();
static inline
void OrbisMsgDialogParamInitialize(OrbisMsgDialogParam *param)
{
	memset( param, 0x0, sizeof(OrbisMsgDialogParam) );

	_sceCommonDialogBaseParamInit( &param->baseParam );
	param->size = sizeof(OrbisMsgDialogParam);
}
int sceMsgDialogUpdateStatus();
// Add a message to the message dialog progress bar.
// OrbisMsgDialogMode must be initialized with ORBIS_MSG_DIALOG_MODE_PROGRESS_BAR.
int32_t sceMsgDialogProgressBarSetMsg(int target, const char *barMsg);
// Display the message dialog.
int32_t sceMsgDialogOpen(const OrbisMsgDialogParam *param);
// Set the message dialog progress bar immediately without animation.
// OrbisMsgDialogMode must be initialized with ORBIS_MSG_DIALOG_MODE_PROGRESS_BAR.
int32_t sceMsgDialogProgressBarSetValue(int idc, uint32_t rate);

// Get the result of the message dialog after the user closes the dialog.
// This can be used to detect which option was pressed (yes, no, cancel, etc).
int32_t sceMsgDialogGetResult(OrbisMsgDialogResult *result);
#endif

#ifdef __cplusplus
}
#endif