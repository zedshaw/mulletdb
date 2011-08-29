#include "storage.h"
#include "search.h"


value Storage::response(string status, int code, string message, value result)
{
    return $("status", status) -> $("message", message)
        -> $("value", result) -> $("code", code);
}


value Storage::run_query(string action, string query) 
{
    value results;
    int rc = 0;
    char *zErrMsg = 0;
    sqlite3_stmt *ppStmt;
    const char *ppTail;
    int rows = 0;
    int i = 0;
    bool needs_row_id = false;

    rc = sqlite3_prepare_v2(sqlite_db, query, query.strlen(), &ppStmt, &ppTail);

    if(rc != SQLITE_OK) {
        results["error"] = "%s" %format (sqlite3_errmsg(sqlite_db));
    } else {
        int cols = sqlite3_column_count(ppStmt);
        value columns;
        value types;
        value data;
        string temp;

        needs_row_id = action == "insert";

        for(i = 0; i < cols; i++) {
            columns[i] = sqlite3_column_name(ppStmt, i);
            types[i] = sqlite3_column_decltype(ppStmt, i);
        }
        
        if(needs_row_id) {
            columns[i] = "_row_id";
            types[i] = "INTEGER";
        }
        
        results["columns"] = columns;
        results["types"] = types;

        for(rows = 0; (rc = sqlite3_step(ppStmt)) == SQLITE_ROW; rows++) {
            for(i = 0; i < cols; i++) {
                switch(sqlite3_column_type(ppStmt, i)) {
                    case SQLITE_INTEGER:
                        data[rows][i] = sqlite3_column_int(ppStmt, i);
                        break;
                    case SQLITE_FLOAT:
                        data[rows][i] = sqlite3_column_double(ppStmt, i);
                        break;
                    case SQLITE_TEXT:
                        data[rows][i] = sqlite3_column_text(ppStmt, i);
                        break;
                    case SQLITE_BLOB:
                        temp = sqlite3_column_text(ppStmt, i);

                        data[rows][i] = $("data_type", "BLOB") ->
                                     $("data_value", temp.encode64());

                        break;
                    case SQLITE_NULL:
                        break;
                    default:
                        data[rows][i] = sqlite3_column_text(ppStmt, i);
                        break;
                }
            }
        }

        if(needs_row_id) {
            data[rows][i] = sqlite3_last_insert_rowid(sqlite_db);
        }

        results["data"] = data;
    }

    if(rc != SQLITE_DONE && rc != SQLITE_OK) {
       results = response("error", OPERATION_FAILED, 
                "%s" %format (sqlite3_errmsg(sqlite_db)), query);
    } else {
        results = response("ok", OK, query, results);
    }

    sqlite3_finalize(ppStmt);
    return results;
}



string Storage::sql_escape(string val) 
{
    value trans;
    trans["'"] = "''";
    val.replace(trans);
    return "'%s'" %format(val);
}



value Storage::sql_params(value data) 
{
    value params;

    foreach(val, data) {
        if(val.itype() == i_string) {
            params.newval() = "%s = %s" %format(val.id(), sql_escape(val));
        } else {
            params.newval() = "%s = %s" %format(val.id(), val);
        }
    }

    return params;
}


value Storage::escape_vars(value vars)
{
    value escaped;

    foreach(node, vars) {
        if(node.itype() == i_string) {
            escaped[node.id()] = sql_escape(node);
        } else {
            escaped[node.id()] = node;
        }
    }

    return escaped;
}


value Storage::sql(string action, string qstr)
{
    string sql_query;
    value data;

    data.fromjson(qstr);

    if(data.exists("vars")) {
        data["vars"] = escape_vars(data["vars"]);
    }

    if(action == "query") {
        // * query {"sql": "", "vars": {}}
        sql_query = strutil::valueparse(data["sql"], data["vars"]);
    } else if(action == "select") {
        // * select {"table": "", "vars": {}, "what": "*", "where":"", "order":"", 
        // "group": "", "limit": "", "offset": "", "having": ""}

        string query_format = "select $what$ from $table$ ";

        if(!data.exists("what")) data["what"] = "*";

        if(data.exists("where")) {
            query_format += "where $where$ ";
            data["where"] = strutil::valueparse(data["where"], data["vars"]);
        }

        if(data.exists("order")) query_format += "order by $order$ ";

        if(data.exists("group")) {
            query_format += "group by $group$ ";

            if(data.exists("having")) query_format += "having $having$ ";
        }

        if(data.exists("limit")) query_format += "limit $limit$ ";

        if(data.exists("offset")) query_format += "offset $offset$ ";

        sql_query = strutil::valueparse(query_format, data);

    } else if(action == "where") {
        // * where {"table": "", "values": {}}
        string query_format = "select * from $table$ where ";
        query_format += sql_params(data["values"]).join();
        sql_query = strutil::valueparse(query_format, data);

    } else if(action == "insert") {
        // * insert {"table":, "", "values": {}}
        string query_format = "insert into $table$ $columns$ values $values$";
        value columns;
        value values;

        foreach(node, data["values"]) {
            columns.newval() = node.id();
            if(node.isbuiltin("string")) {
                values.newval() = sql_escape(node);
            } else {
                values.newval() = node;
            }
        }
        
        data["columns"] = columns.join(",", "(", ")");
        data["values"] = values.join(",", "(", ")");
        sql_query = strutil::valueparse(query_format, data);
    } else if(action == "update") {
        // * update {"table": "", "where": "", "vars": {}, "values": {}}
        string query_format = "update $table$ set $values$ where $where$";
        data["values"] = sql_params(data["values"]).join(",");
        data["where"] = strutil::valueparse(data["where"], data["vars"]);
        sql_query = strutil::valueparse(query_format, data);
    } else if(action == "delete") {
        // * delete {"table": "", "where": "", "using": "", vars={}}
        string query_format;

        if(data.exists("where")) {
            data["where"] = strutil::valueparse(data["where"], data["vars"]);

            query_format = "delete from $table$ where $where$";
        }

        if(data.exists("using")) {
            data["using"] = strutil::valueparse(data["using"], data["vars"]);
            query_format = "delete from $table$ where $where$ using $using$";
        }

        sql_query = strutil::valueparse(query_format, data);
    } else {
        return response("error", INVALID_REQUEST, "action not supported", action);
    }

    return run_query(action, sql_query);
}


value Storage::mem(string cmd, string qstr)
{
    if (cmd == "put") {
        string key = qstr.cutat(' ');
        hdb.put(key, qstr);
        return response("ok", OK, cmd, "");
    } else if (cmd == "get") {
        return response("ok", OK, cmd, hdb.get(qstr));
    } else if (cmd == "putkeep") {
        string key = qstr.cutat(' ');
        return response("ok", OK, cmd, hdb.putkeep(key, qstr));
    } else if (cmd == "putcat") {
        string key = qstr.cutat(' ');
        hdb.putcat(key, qstr);
        return response("ok", OK, cmd, "");
    } else if (cmd == "del") {
        return response("ok", OK, cmd, hdb.del(qstr)); 
    } else if (cmd == "store") {
        string tfile = storage %format(qstr);
        const char *value = hdb.get(qstr);

        if(value != NULL) {
            fs.save(tfile, value);
            return response("ok", OK, cmd, hdb.del(qstr)); 
        } else {
            return response("error", NOT_FOUND, "record not found", qstr);
        }
    } else {
        return response("error", INVALID_REQUEST, "invalid mem command", cmd);
    }
}


value Storage::disk(string cmd, string qstr)
{
    if(cmd == "put") {
        string key = qstr.cutat(' ');
        string tfile = storage %format(key);

        return response("ok", OK, cmd, fs.save(tfile, qstr));
    } if(cmd == "putkeep") {
        string key = qstr.cutat(' ');
        string tfile = storage %format(key);

        if(fs.exists(tfile)) {
            return response("ok", OK, cmd, false);
        } else {
            return response("ok", OK, cmd, fs.save(tfile, qstr));
        }
    } else if (cmd == "get") {
        string tfile = storage %format(qstr);

        if(fs.exists(tfile)) {
            return response("ok", OK, cmd, fs.load(tfile));
        } else {
            return response("error", NOT_FOUND, "file not found", tfile);
        }
    } else if (cmd == "delete") {
        string tfile = storage %format(qstr);

        if(fs.exists(tfile)) {
            return response("ok", OK, cmd, fs.rm(tfile));
        } else {
            return response("error", NOT_FOUND, "file not found", tfile);
        }
    } else if (cmd == "info") {
        string tfile = storage %format(qstr);

        if(fs.exists(tfile)) {
            value info = fs.getinfo(tfile);
            return response("ok", OK, cmd, $("path", info["path"]) ->
                    $("inode", info["inode"]) ->
                    $("size", info["size"]) ->
                    $("type", info["type"]));
        } else {
            return response("error", NOT_FOUND, "file not found", tfile);
        }
    } else if (cmd == "cp") {
        string from = storage %format(qstr.cutat(' '));
        string to = storage %format(qstr);

        if(fs.exists(from)) {
            if(fs.cp(from, to)) {
                return response("ok", OK, cmd, true);
            } else {
                return response("error", OPERATION_FAILED, "copy failed",
                        $("from", from) -> $("to", to));
            }
        } else {
            return response("error", NOT_FOUND, "file not found", from);
        }
    } else if (cmd == "mv") {
        string from = storage %format(qstr.cutat(' '));
        string to = storage %format(qstr);

        if(fs.exists(from)) {
            if(fs.mv(from, to)) {
                return response("ok", OK, cmd, true);
            } else {
                return response("error", OPERATION_FAILED, "failed to move",
                        $("from", from) -> $("to", to));
            }
        } else {
            return response("error", NOT_FOUND, "file not found", from);
        }
    } else if(cmd == "load") {
        string from = storage %format(qstr);
        if(fs.exists(from)) {
            hdb.put(qstr.cval(), fs.load(from)->cval());
            return response("ok", OK, cmd, true);
        } else {
            return response("error", NOT_FOUND, "file not found", from);
        }
    } else {
        return response("error", INVALID_REQUEST, "invalid disk command", cmd);
    }
}


value Storage::doc(string cmd, string query)
{
    value result;

    if(cmd == "get") {
        int64_t id = query.toint();

        value res = search.get(id);

        if(res.exists("error")) {
            result = response("error", NOT_FOUND, "doc id not found", cmd);
        } else {
            result = response("ok", OK, cmd, res);
        }

    } else if (cmd == "put") {
        string key = query.cutat(' ');
        int64_t id = key.toint();
        bool res = false;

        res = search.put(id, query);

        if(res) {
            result = response("ok", OK, cmd, true);
        } else {
            result = response("error", OPERATION_FAILED, search.last_error(), cmd);
        }
    } else if (cmd == "del") {
        int64_t id = query.toint();
        bool res = false;

        res = search.del(id);

        if(res) {
            result = response("ok", OK, cmd, true);
        } else {
            result = response("error", OPERATION_FAILED, search.last_error(), cmd);
        }
    } else if (cmd == "query") {
        value res = search.query(query);

        result = response("ok", OK, cmd, res);
    } else if (cmd == "find") {
        value res = search.find(query);

        result = response("ok", OK, cmd, res);
    } else if (cmd == "index") {
        string key = query.cutat(' ');
        int64_t id = key.toint();
        string inf = storage %format(query);

        if(fs.exists(inf)) {
            string data = fs.load(inf);
            bool res = search.put(id, data);

            if(res) {
                result = response("ok", OK, cmd, $("id", (unsigned int)id) -> $("key", key));
            } else {
                result = response("error", OPERATION_FAILED, search.last_error(), cmd);
            }
        } else {
            result = response("error", NOT_FOUND, "file not found", inf);
        }
    } else {
        result = response("error", INVALID_REQUEST, "invalid doc command", cmd);
    }

    return result;
}

