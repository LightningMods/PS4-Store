
#include "defines.h"

#include <string.h>
#include <stdio.h>
#include <user_mem.h> 

typedef uint32_t u32;
typedef uint8_t  u8 ;
typedef uint16_t u16;
typedef uint64_t u64;


typedef struct PSF_HEADER {
    char magic[4];
    u32 version;
    u32 keyTabOffs;
    u32 dataTabOffs;
    u32 entryCount;
} psf_header;


typedef struct PSF_PARAM_ENTRY {
    u16 keyOffs;
    u16 paramFmt;
    u32 paramLen;
    u32 paramMax;
    u32 dataOffs;
} psf_param_entry;


typedef uint64_t unat;

enum param_fmt {
    Fmt_StrS = 0x004,
    Fmt_Utf8 = 0x204,
    Fmt_SInt32 = 0x404,

    Fmt_Invalid = 0x000
};



static char buffer[255];

u8 *sfodata = NULL;
int j;


char buff[255];
//int SFODump(int check_app_ver, char *path, item_t *item)
int index_token_from_sfo(item_t *item, char *path, int lang)
{
            sfodata      = orbisFileGetFileContent(path);
    size_t  sfodata_size = _orbisFile_lastopenFile_size;
    char* lang_title[16];

    item_idx_t *t = &item->token_d[0];

    snprintf(&lang_title[0], 15, "TITLE_%i", lang);
    //log_info("SFO TITLE: %s", lang_title);

    if (sfodata)
    {
        psf_header * hdr = (psf_header*)&sfodata[0];

        char tmp[256];
        for (unat n = 0; n < hdr->entryCount; n++)
        {
            psf_param_entry * e = (psf_param_entry *)(&sfodata[0] + sizeof(psf_header) + sizeof(psf_param_entry)*n);

            unat fileOffs = hdr->keyTabOffs + e->keyOffs;

            char *s_key = "INVALID";
            if (fileOffs < sfodata_size) {
                s_key = strdup((const char*)(&sfodata[0] + fileOffs));
            }

            u8 *e_data = NULL;
            fileOffs   = hdr->dataTabOffs + e->dataOffs;
            if (fileOffs + e->paramLen < sfodata_size) {
                e_data = calloc(e->paramLen, sizeof(u8));
                memcpy(&e_data[0], (&sfodata[0] + fileOffs), e->paramLen);
            }
#if 0
            if ((Fmt_StrS == e->paramFmt) || (Fmt_Utf8 == e->paramFmt))
                log_info("Param[%04lu] \"%s\" : \"%s\" ", n, s_key, (const char*)&e_data[0]);
            else if (Fmt_SInt32 == e->paramFmt)
                log_info("Param[%04lu] \"%s\" : %d ", n, s_key, *(u32*)&e_data[0]);
            else
                log_info("Param[%04lu] \"%s\" : %X : %lu bytes ", n, s_key, e->paramFmt, sizeof(e_data));
#endif
            snprintf(&tmp[0], 255, "%s", (const char*)&e_data[0]);
            // 
            
            if (!strcmp(s_key, "TITLE"))
                  buffer_to_off(item, NAME, tmp);
            if (!strcmp(lang_title, s_key))
                   buffer_to_off(item, NAME, tmp);
            if( ! strcmp(s_key, "VERSION" ) )
                buffer_to_off(item, VERSION, tmp);
            if( ! strcmp(s_key, "APP_TYPE") ) 
                buffer_to_off(item, APPTYPE, tmp);

            // release, don't leak
            if(e_data) free(e_data), e_data = NULL;
            if(s_key)  free(s_key),  s_key  = NULL;
        }
    }
    // release
    if(sfodata) free(sfodata), sfodata = NULL;
    //print_text(100, 100, "Param[%04d] \"%s\" : \"%s\" ");
    return 0;
}

