#include "utils.h"
#include "defines.h"
#include "sqlite3.h"

char* err_msg = 0;
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
extern "C" const void* sqlite3_get_sqlite3Apis();
extern "C" int sqlite3_memvfs_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

static sqlite3* open_sqlite_db(const char* db_path)
{
	uint8_t* db_buf;
	size_t db_size;
	sqlite3 *db;
	const sqlite3_api_routines* api = (sqlite3_api_routines*)sqlite3_get_sqlite3Apis();

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
		sqlite3_close(db);  // Ensure any partially allocated db resources are freed
                sqlite3_free(memuri);  // Also ensure memuri is freed before returning
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
#include <unordered_map>
// index (write) all used_tokens
int sql_index_tokens(std::shared_ptr<layout_t>  &l, int count)
{
    int idx = 0, num = 0; // Initialize idx to 0 instead of l->item_c
    std::unordered_map<std::string, int> columnIndexMap;
    sceMsgDialogTerminate();
    if(set.auto_load_cache)
       progstart(getLangSTR(CHECKING_FOR_UPDATES)); // make a setting

    for (int i = 1; i <= count; i++) {
        l->item_d[idx].token_c = TOTAL_NUM_OF_TOKENS;
        // dynalloc for SQL tokens
        if (l->item_d[idx].token_d.empty())
            l->item_d[idx].token_d.resize(l->item_d[idx].token_c);

        std::string sql = fmt::format("SELECT * FROM homebrews WHERE pid LIKE {} LIMIT 1;", i);
        sqlite3_stmt* stmt;

       if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            int columnCount = sqlite3_column_count(stmt);
            for (int i = 0; i < columnCount; ++i) {
                std::string colName = sqlite3_column_name(stmt, i);
                columnIndexMap[colName] = i;
            }

            int stepResult = sqlite3_step(stmt);
            if (stepResult == SQLITE_ROW) {
                for (int j = 0; j < static_cast<int>(TOTAL_NUM_OF_TOKENS); j++) {
                    int colIndex = columnIndexMap[used_token[j]];
                    const char* columnText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, colIndex));
                    l->item_d[idx].token_d[j].off = columnText ? std::string(columnText) : std::string();
                    //log_info("columnText: %s", columnText);
                }
            }
            else 
               log_error("Error fetching data: %s", sqlite3_errmsg(db));

            sqlite3_finalize(stmt);
        } else 
           log_error("Error preparing statement: %s", sqlite3_errmsg(db));

        idx++;
    }
    l->item_c = count;
    if(!set.auto_load_cache){
        log_debug("auto_load_cache is disabled skipping ...");
        return count;
    }
    idx = 0;
    size_t fmem = 0;

    if(!set.auto_load_cache)
       return count;
       
    for(auto &t: l->item_d){
        sceKernelAvailableFlexibleMemorySize(&fmem);
        //fill up the available VRAM with PNGs (to reduce loading times) but keep enough to do other things
        if(fmem < MB(100))
            break;

        if(t.token_d.empty() || t.token_d.size() <  PICPATH || t.token_d[PICPATH].off.empty())
           continue;

        // see if t.token_d[PICPATH].off includes the string /user/app where images are cached
        if(t.token_d[PICPATH].off.find("/user/app/NPXS39041/") == std::string::npos)
            continue;

        // calc the progress %
        ProgSetMessagewText(((num * 100) / count), getLangSTR(PRE_LOADING_CACHE));
 
        // pre-load all available textures
        if(if_exists(t.token_d[PICPATH].off.c_str())){
            t.texture = load_png_asset_into_texture(t.token_d[PICPATH].off.c_str());
            if(t.texture == GL_NULL){
                log_error("Failed to load texture for %s", t.token_d[PICPATH].off.c_str());
            }
            else
              idx++;
        }
        num++;
    }
    log_info("Loaded %d textures", idx);
    //sceMsgDialogTerminate();
    return count;
}

