
#include "defines.h"

wchar_t inputTextBuffer[33];
static char storebuffer[34];

char * StoreKeyboard() {

  sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG);

  wchar_t title[100];

  memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
  memset(&storebuffer[0], 0, sizeof(storebuffer));

  mbstowcs(inputTextBuffer, storebuffer, strlen(storebuffer) + 1);
  mbstowcs(title, "Store Keyboard", strlen("Store Keyboard") + 1);

  OrbisImeDialogParam param;
  memset( & param, 0, sizeof(OrbisImeDialogParam));

  param.maxTextLength = 33;
  param.inputTextBuffer = inputTextBuffer;
  param.title = title;
  param.userId = 0xFE;
  param.type = ORBIS_IME_TYPE_BASIC_LATIN;
  param.enterLabel = ORBIS_IME_ENTER_LABEL_DEFAULT;

  int init_log = sceImeDialogInit( & param, NULL);

  while (1) {
    int status = sceImeDialogGetStatus();

    if (status == ORBIS_IME_DIALOG_STATUS_FINISHED) {
      OrbisImeDialogResult result;
      memset( & result, 0, sizeof(OrbisImeDialogResult));
      sceImeDialogGetResult( & result);

      if (result.endstatus == ORBIS_IME_DIALOG_END_STATUS_USER_CANCELED) 
          goto Finished;

      if (result.endstatus == ORBIS_IME_DIALOG_END_STATUS_OK) {
        klog("status %i, endstatus %i \n", status, result.endstatus);
        wcstombs(&storebuffer[0], inputTextBuffer, 33);
         goto Finished;

      }
    }

    if (status == ORBIS_IME_DIALOG_STATUS_NONE) 
            goto Finished;

  }

  Finished:
    sceImeDialogTerm();
    return storebuffer;

}