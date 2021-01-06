
#include "defines.h"

wchar_t inputTextBuffer[70];
static char storebuffer[70];

char *StoreKeyboard(const char *Title, char *initialTextBuffer)
{
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG);

    wchar_t title[100];
    char    titl [100];

     if(initialTextBuffer && strlen(initialTextBuffer) > 69) return "Too Long";

    memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
    memset(&storebuffer[0], 0, sizeof(storebuffer));

    if(initialTextBuffer)
        sprintf(&storebuffer[0], "%s", initialTextBuffer);
    //converts the multibyte string src to a wide-character string starting at dest.
    mbstowcs(inputTextBuffer, storebuffer, strlen(storebuffer) + 1);
    // use custom title
    if(Title)
        sprintf(&titl[0], "%s", Title);
    else // default
        sprintf(&titl[0], "%s", "Store Keyboard");
    //converts the multibyte string src to a wide-character string starting at dest.
    mbstowcs(title, titl, strlen(titl) + 1);

    OrbisImeDialogParam param;
    memset(&param, 0, sizeof(OrbisImeDialogParam));

    param.maxTextLength   = 70;
    param.inputTextBuffer = inputTextBuffer;
    param.title           = title;
    param.userId          = 0xFE;
    param.type            = ORBIS_IME_TYPE_BASIC_LATIN;
    param.enterLabel      = ORBIS_IME_ENTER_LABEL_DEFAULT;

    int init_log = sceImeDialogInit( & param, NULL);

    int status;
    while(1)
    {
        status = sceImeDialogGetStatus();

        if (status == ORBIS_IME_DIALOG_STATUS_FINISHED)
        {
            OrbisImeDialogResult result;
            memset(&result, 0, sizeof(OrbisImeDialogResult));
            sceImeDialogGetResult(&result);

            if(result.endstatus == ORBIS_IME_DIALOG_END_STATUS_USER_CANCELED) 
                goto Finished;

            if(result.endstatus == ORBIS_IME_DIALOG_END_STATUS_OK)
            {
                klog("status %i, endstatus %i \n", status, result.endstatus);
                wcstombs(&storebuffer[0], inputTextBuffer, 70);
                goto Finished;
            }
        }

        if (status == ORBIS_IME_DIALOG_STATUS_NONE) goto Finished;
    }

Finished:
    sceImeDialogTerm();
    return storebuffer;
}
