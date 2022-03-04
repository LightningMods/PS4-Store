#ifndef _ORBIS_IME_H_
#define _ORBIS_IME_H_
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dialog.h>

#ifdef __cplusplus 
extern "C" {
#endif


#define ORBIS_COMMON_DIALOG_MAGIC_NUMBER 0xC0D1A109


typedef enum OrbisCommonDialogStatus {
	ORBIS_COMMON_DIALOG_STATUS_NONE				= 0,
	ORBIS_COMMON_DIALOG_STATUS_INITIALIZED		= 1,
	ORBIS_COMMON_DIALOG_STATUS_RUNNING			= 2,
	ORBIS_COMMON_DIALOG_STATUS_FINISHED			= 3
} OrbisCommonDialogStatus;

typedef enum OrbisCommonDialogResult {
	ORBIS_COMMON_DIALOG_RESULT_OK					= 0,
	ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED		= 1,
} OrbisCommonDialogResult;

typedef struct OrbisCommonDialogBaseParam {
	size_t size;
	uint8_t reserved[36];
	uint32_t magic;
} OrbisCommonDialogBaseParam __attribute__ ((__aligned__(8)));


static inline void _sceCommonDialogSetMagicNumber( uint32_t* magic, const OrbisCommonDialogBaseParam* param )
{
	*magic = (uint32_t)( ORBIS_COMMON_DIALOG_MAGIC_NUMBER + (uint64_t)param );
}

static inline void _sceCommonDialogBaseParamInit(OrbisCommonDialogBaseParam *param)
{
	memset(param, 0x0, sizeof(OrbisCommonDialogBaseParam));
	param->size = (uint32_t)sizeof(OrbisCommonDialogBaseParam);
	_sceCommonDialogSetMagicNumber( &(param->magic), param );
}


// Empty Comment
void sceCommonDialogInitialize();
// Empty Comment
void sceCommonDialogIsUsed();


#define ORBIS_SYSMODULE_IME_DIALOG				0x0096

#include <dialog.h>

typedef struct OrbisRtcTick {
	uint64_t tick;
} OrbisRtcTick;

typedef enum
{
	ORBIS_IME_DIALOG_STATUS_NONE = 0,			
													
	ORBIS_IME_DIALOG_STATUS_RUNNING,					
										
	ORBIS_IME_DIALOG_STATUS_FINISHED,					
													
} OrbisImeDialogStatus;


typedef enum
{
	ORBIS_IME_DIALOG_END_STATUS_OK = 0,	
													
	ORBIS_IME_DIALOG_END_STATUS_USER_CANCELED,		
													
	ORBIS_IME_DIALOG_END_STATUS_ABORTED,				
													
} OrbisImeDialogEndStatus;


typedef enum OrbisImeEnterLabel
{
	ORBIS_IME_ENTER_LABEL_DEFAULT= 0,	
	ORBIS_IME_ENTER_LABEL_SEND = 1,	
	ORBIS_IME_ENTER_LABEL_SEARCH = 2,	
	ORBIS_IME_ENTER_LABEL_GO	= 3,	
} OrbisImeEnterLabel;


typedef enum OrbisImeType
{
	ORBIS_IME_TYPE_DEFAULT = 0,	
	ORBIS_IME_TYPE_BASIC_LATIN = 1,	
	ORBIS_IME_TYPE_URL = 2,	
	ORBIS_IME_TYPE_MAIL = 3,	
	ORBIS_IME_TYPE_NUMBER = 4,	
} OrbisImeType;

typedef struct OrbisImeDialogResult {
	OrbisImeDialogEndStatus endstatus;				
													
	int8_t reserved[12];							
													
} OrbisImeDialogResult;


typedef enum OrbisImeInputMethod
{
	ORBIS_IME_INPUT_METHOD_DEFAULT = 0,	
} OrbisImeInputMethod;

typedef int32_t (*OrbisImeTextFilter)(
	wchar_t *outText,			
	uint32_t *outTextLength,	
	const wchar_t *srcText,		
	uint32_t srcTextLength		
);

typedef enum OrbisImeHorizontalAlignment
{
	ORBIS_IME_HALIGN_LEFT	= 0,	
	ORBIS_IME_HALIGN_CENTER	= 1,	
	ORBIS_IME_HALIGN_RIGHT	= 2,	
} OrbisImeHorizontalAlignment;


typedef enum OrbisImeVerticalAlignment
{
	ORBIS_IME_VALIGN_TOP = 0,	
	ORBIS_IME_VALIGN_CENTER = 1,	
	ORBIS_IME_VALIGN_BOTTOM = 2,	
} OrbisImeVerticalAlignment;

typedef struct OrbisImeDialogParam {
	int32_t userId;					
													
	OrbisImeType type;								
													
	uint64_t supportedLanguages;					
													
	OrbisImeEnterLabel enterLabel;					
													
	OrbisImeInputMethod inputMethod;					
													
	OrbisImeTextFilter filter;						
												
	uint32_t option;								
											
	uint32_t maxTextLength;							
													
	wchar_t *inputTextBuffer;						
													
	float posx;										
													
	float posy;										
													
	OrbisImeHorizontalAlignment horizontalAlignment;	
													
	OrbisImeVerticalAlignment verticalAlignment;		
													
	const wchar_t *placeholder;						
													
	const wchar_t *title;							
													
	int8_t reserved[16];							
													
} OrbisImeDialogParam;

typedef struct OrbisImeColor
{
	uint8_t r;	
	uint8_t g;	
	uint8_t b;	
	uint8_t a;	
} OrbisImeColor;

typedef enum OrbisImePanelPriority
{
	ORBIS_IME_PANEL_PRIORITY_DEFAULT		= 0,	
	ORBIS_IME_PANEL_PRIORITY_ALPHABET		= 1,
	ORBIS_IME_PANEL_PRIORITY_SYMBOL		= 2,	
	ORBIS_IME_PANEL_PRIORITY_ACCENT		= 3,	

} OrbisImePanelPriority;

typedef enum OrbisImeKeyboardType
{
	ORBIS_IME_KEYBOARD_TYPE_NONE				=  0,	//<E Unknown keyboard
	ORBIS_IME_KEYBOARD_TYPE_DANISH			=  1,	
	ORBIS_IME_KEYBOARD_TYPE_GERMAN			=  2,	
	ORBIS_IME_KEYBOARD_TYPE_GERMAN_SW		=  3,	
	ORBIS_IME_KEYBOARD_TYPE_ENGLISH_US		=  4,	
	ORBIS_IME_KEYBOARD_TYPE_ENGLISH_GB		=  5,	
	ORBIS_IME_KEYBOARD_TYPE_SPANISH			=  6,	
	ORBIS_IME_KEYBOARD_TYPE_SPANISH_LA		=  7,	
	ORBIS_IME_KEYBOARD_TYPE_FINNISH			=  8,	
	ORBIS_IME_KEYBOARD_TYPE_FRENCH			=  9,	
	ORBIS_IME_KEYBOARD_TYPE_FRENCH_BR		= 10,	
	ORBIS_IME_KEYBOARD_TYPE_FRENCH_CA		= 11,	
	ORBIS_IME_KEYBOARD_TYPE_FRENCH_SW		= 12,	
	ORBIS_IME_KEYBOARD_TYPE_ITALIAN			= 13,	
	ORBIS_IME_KEYBOARD_TYPE_DUTCH			= 14,	
	ORBIS_IME_KEYBOARD_TYPE_NORWEGIAN		= 15,	
	ORBIS_IME_KEYBOARD_TYPE_POLISH			= 16,	
	ORBIS_IME_KEYBOARD_TYPE_PORTUGUESE_BR		= 17,	
	ORBIS_IME_KEYBOARD_TYPE_PORTUGUESE_PT		= 18,	
	ORBIS_IME_KEYBOARD_TYPE_RUSSIAN			= 19,	
	ORBIS_IME_KEYBOARD_TYPE_SWEDISH			= 20,	
	ORBIS_IME_KEYBOARD_TYPE_TURKISH			= 21,	
	ORBIS_IME_KEYBOARD_TYPE_JAPANESE_ROMAN	        = 22,	
	ORBIS_IME_KEYBOARD_TYPE_JAPANESE_KANA		= 23,	
	ORBIS_IME_KEYBOARD_TYPE_KOREAN			= 24,	
	ORBIS_IME_KEYBOARD_TYPE_SM_CHINESE		= 25,	
	ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_ZY		= 26,	
	ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_PY_HK	= 27,	
	ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_PY_TW	= 28,	
	ORBIS_IME_KEYBOARD_TYPE_TR_CHINESE_CG		= 29,	
	ORBIS_IME_KEYBOARD_TYPE_ARABIC_AR		= 30,	
} OrbisImeKeyboardType;

typedef struct OrbisImeKeycode
{
	uint16_t keycode;				
	wchar_t character;				
	uint32_t status;				
	OrbisImeKeyboardType type;		
	int32_t userId;	
	uint32_t resourceId;			
	OrbisRtcTick timestamp;		
} OrbisImeKeycode;

typedef int (*OrbisImeExtKeyboardFilter)(
	const OrbisImeKeycode *srcKeycode,	
	uint16_t *outKeycode,				
	uint32_t *outStatus,				
	void *reserved						
);

typedef struct OrbisImeParamExtended
{
	uint32_t option;							
	OrbisImeColor colorBase;						
	OrbisImeColor colorLine;						
	OrbisImeColor colorTextField;					
	OrbisImeColor colorPreedit;					
	OrbisImeColor colorButtonDefault;				
	OrbisImeColor colorButtonFunction;			
	OrbisImeColor colorButtonSymbol;				
	OrbisImeColor colorText;						
	OrbisImeColor colorSpecial;					
	OrbisImePanelPriority priority;				
	const char *additionalDictionaryPath;		
	OrbisImeExtKeyboardFilter extKeyboardFilter;	
	uint32_t disableDevice;						
	uint32_t extKeyboardMode;					
	int8_t reserved[60];						
} OrbisImeParamExtended;

int sceImeDialogTerm();
int sceImeDialogGetResult( OrbisImeDialogResult* result );
OrbisImeDialogStatus sceImeDialogGetStatus();
int sceImeDialogInit( const OrbisImeDialogParam *param, const OrbisImeParamExtended *extendedParam );



#define ORBIS_SYSMODULE_MESSAGE_DIALOG			0x00a4
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


static inline
void OrbisMsgDialogParamInitialize(OrbisMsgDialogParam *param)
{
	memset( param, 0x0, sizeof(OrbisMsgDialogParam) );

	_sceCommonDialogBaseParamInit( &param->baseParam );
	param->size = sizeof(OrbisMsgDialogParam);
}


char *StoreKeyboard(const char *Title, char *initialTextBuffer);

#endif

#ifdef __cplusplus
}
#endif