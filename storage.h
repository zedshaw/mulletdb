#ifndef __storage__
#define __storage__

#include "mulletdb.h"
#include <sqlite3.h>
#include "hash.h"
#include "search.h"

#define DEFAULT_STORE "/var/storage"

class Storage {
    public:
    
    sqlite3 *sqlite_db;
    HashDB hdb;
    SearchDB search;
    string storage;
    string sqlite_file;

    Storage(value config) {
        storage = config.exists("storage") ? config["storage"] : DEFAULT_STORE;
        storage += "/%s";
        string sqlite_file = storage %format("sql.db");
        string search_db = storage %format("docs");

        sqlite3_open(sqlite_file, &sqlite_db);
        log::write(log::info, "storage", "using database file %s" %format(sqlite_file));


        hdb.open();

        log::write(log::info, "storage", "using docs db: %s" %format(search_db));
        search.file(search_db);
        search.open(false);

    }

    ~Storage(void) {
        search.close();
        sqlite3_close(sqlite_db);
    }

    value response(string status, int code, string message, value result);

    value run_query(string action, string query);

    value sql_params(value data);

    string sql_escape(string val); 

    value escape_vars(value vars);

    value sql(string action, string qstr);

    value mem(string cmd, string qstr);

    value disk(string cmd, string qstr);

    value doc(string cmd, string qstr);
};

#endif
