

#include "utils.h"
#include "defines.h"
#include <sqlite3.h>

char* err_msg = 0;

sqlite3* db = NULL;
int count = 0;
sqlite3* SQLiteOpenDb(char* path)
{
    int ret;
    struct stat sb;

    //check if our sqlite db is already present in our path
    ret = sceKernelStat(path, &sb);
    if (ret != 0)
    {
        log_error("[ORBIS][ERROR][SQLLDR][%s][%d] sceKernelStat(%s) failed: 0x%08X", __FUNCTION__, __LINE__, path, ret);
        goto sqlite_error_out;
    }
    ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
sqlite_error_out:
    //return error is something went wrong errmsg on klogs
    if (ret != SQLITE_OK)
    {
        log_error("[ORBIS][ERROR][SQLLDR][%s][%d] ERROR 0x%08X: %s", __FUNCTION__, __LINE__, ret, sqlite3_errmsg(db));
        return NULL;
    }
    else
       return db;
    
}


int callback(void*, int, char**, char**);


bool SQL_Load_DB(const char* dir) {

    db = SQLiteOpenDb(dir);
    if (db == NULL) {
        log_error("[SQLLDR] Cannot open database: %s", sqlite3_errmsg(db));
        return false;
    }

    return true;
}

bool SQL_Exec(const char* smt, int (*cb)(void*, int, char**, char**))
{
    log_info("Executing SQL Statement: %s", smt);
    if (sqlite3_exec(db, smt, cb, 0, &err_msg) != SQLITE_OK) {

        log_info("Failed to select data");
        log_info("SQL error: %s", err_msg);

        sqlite3_free(err_msg);

        return false;
    }
    else
        return true;
}


int page_cb(void*, int, char** argv, char**) {

    count = atoi(argv[0]);

    return 0;
}

int SQL_Get_Count(void)
{

    log_info("Executing SQL Statement: "SQL_COUNT);
    if (sqlite3_exec(db, SQL_COUNT, page_cb, 0, &err_msg) != SQLITE_OK) {

        log_info("Failed to select data");
        log_info("SQL error: %s", err_msg);

        sqlite3_free(err_msg);

        return 0;
    }
    else
        return count;
}

/*
 * Arguments:
 *
 *   unused - Ignored in this case, see the documentation for sqlite3_exec
 *    count - The number of columns in the result set
 *     data - The row's data
 *  columns - The column names
 */
static int token_cb(void* in, int count, char** data, char** columns)
{
    char** result_str = (char**)in;
    *result_str = sqlite3_mprintf("%w", data[0]);
    return 0;
}

// index (write) all used_tokens
int sql_index_tokens(layout_t* l, int count)
{
    char buf[255];
    int idx = l->item_c;

    item_idx_t* token = NULL;

    // grab last added index from current item count
    for (int i = 1; i <= count; i++)
    {
        l->item_d[idx].token_c = NUM_OF_USER_TOKENS;
        // dynalloc for SQL tokens
        if (!l->item_d[idx].token_d)
            l->item_d[idx].token_d = calloc(l->item_d[idx].token_c, sizeof(item_idx_t)); /// XXX
        
        token = l->item_d[idx].token_d;
        
        for (int j = 0; j < NUM_OF_USER_TOKENS; j++) {

            snprintf(buf, 254, "SELECT %s FROM homebrews WHERE pid LIKE %i", used_token[j], i);

            if (sqlite3_exec(db, buf, token_cb, &token[j].off, &err_msg) != SQLITE_OK) {

                log_info("Failed to select data");
                log_info("SQL error: %s", err_msg);

                sqlite3_free(err_msg);
                return 0;
            }
            if(token[j].off != NULL)
               token[j].len = strlen(token[j].off);
        }

        // save current index from current item count
        idx++;
    }

    l->item_c = count;

#if 0
    idx = 0;
    // grab last added index from current item count
    for (int i = 1; i < count; i++)
    {
        for (int j = 0; j < NUM_OF_USER_TOKENS; j++) {
            log_info("l->item_d[%i].token_d[%i].off: %s", idx, j,l->item_d[idx].token_d[j].off);
        }

        idx++;
    }

#endif


    return count;
}

