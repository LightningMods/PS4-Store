#include "utils.h"
#include "defines.h"
#include "sqlite3.h"

const void* sqlite3_get_sqlite3Apis();
char* err_msg = 0;
int sqlite3_memvfs_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);
sqlite3* db = NULL;

int read_buffer(const char *file_path, uint8_t **buf, size_t *size) {
        FILE *fp;
        uint8_t *file_buf;
        size_t file_size;

        if ((fp = fopen(file_path, "rb")) == NULL)
                return -1;
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        file_buf = (uint8_t *)malloc(file_size);
        fread(file_buf, 1, file_size, fp);
        fclose(fp);

        if (buf)
                *buf = file_buf;
        else
                free(file_buf);
        if (size)
                *size = file_size;

        return 0;
}

static sqlite3* open_sqlite_db(const char* db_path)
{
	uint8_t* db_buf;
	size_t db_size;
	sqlite3 *db;
	const sqlite3_api_routines* api = sqlite3_get_sqlite3Apis();

	// Open an in-memory database to use as a handle for loading the memvfs extension
	if (sqlite3_open(":memory:", &db) != SQLITE_OK)
	{
		log_debug("open :memory: %s", sqlite3_errmsg(db));
		return NULL;
	}

	sqlite3_enable_load_extension(db, 1);
	if (sqlite3_memvfs_init(db, NULL, api) != SQLITE_OK_LOAD_PERMANENTLY)
	{
		log_debug("Error loading extension: memvfs");
		return NULL;
	}

	// Done with this database
	sqlite3_close(db);

	if (read_buffer(db_path, &db_buf, &db_size) != 0)
	{
		log_debug("Cannot open database file: %s", db_path);
		return NULL;
	}

	// And open that memory with memvfs now that it holds a valid database
	char *memuri = sqlite3_mprintf("file:memdb?ptr=0x%p&sz=%lld&freeonclose=1", db_buf, db_size);
	log_debug("Opening '%s'...", memuri);

	if (sqlite3_open_v2(memuri, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, "memvfs") != SQLITE_OK)
	{
		log_debug("Error open memvfs: %s", sqlite3_errmsg(db));
		return NULL;
	}
	sqlite3_free(memuri);

	return db;
}

int callback(void*, int, char**, char**);

bool SQL_Load_DB(const char* dir) {

    db = open_sqlite_db(dir);
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


int int_cb(void *in, int idc, char **argv, char **idc_2)
{
    int *numb = (int *)in;
    *numb = atoi(argv[0]);

    return 0;
}
int SQL_Get_Count(void)
{
    int count = -1;
    sqlite3_stmt *stmt;

	log_debug("command: %s", SQL_COUNT);
	int err = sqlite3_prepare_v2(db, SQL_COUNT, -1, &stmt, 0);
	if(err){
		log_error("Selecting data from DB Failed");	
		return 0;
	}
	
    if (sqlite3_step(stmt) == SQLITE_ERROR) {
        log_debug("Error when counting rows for count %s",sqlite3_errmsg(db));
        return 0;
    }
    else 
       count = sqlite3_column_int(stmt, 0);

    log_debug("SQL COUNT: %d",count);

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

