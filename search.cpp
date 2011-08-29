#include <dystopia.h>
#include "search.h"
#include <grace/strutil.h>


void *SearchDB::open(bool readonly)
{
    int omode = 0;
    TCIDB *idb = NULL;
    bool res = false;

    switch(state) {
        case START:
            omode = readonly ? IDBOREADER : IDBOWRITER;
            idb = tcidbnew();
            res = tcidbopen(idb, db_file, omode | IDBOCREAT);
            state = readonly ? READ : WRITE;
            break;

        case READ:
            if(readonly) {
                return _db;
            } else {
                close();
                idb = tcidbnew();
                res = tcidbopen(idb, db_file, IDBOWRITER);
                state = WRITE;
            }
            break;

        case WRITE:
            if(readonly) {
                close();
                idb = tcidbnew();
                res = tcidbopen(idb, db_file, IDBOREADER);
                state = READ;
            } else {
                return _db;
            }
            break;
    }

    _db = idb;
    return res ? _db : NULL;
}


bool SearchDB::close()
{
    TCIDB *idb = (TCIDB *)_db;

    bool res = tcidbclose(idb);
    tcidbdel(idb);
    _db = NULL;

    state = START;

    return res;
}


value SearchDB::get(int64_t id)
{
    TCIDB *idb = (TCIDB *)open(true);
    char *data = tcidbget(idb, id);

    if(tcidbecode(idb) == TCENOREC) {
        free(data);
        return $("error", last_error());
    } else {
        return $("doc", data) -> $("id", (int)id);
    }
}

bool SearchDB::put(int64_t id, const char *val)
{
    TCIDB *idb = (TCIDB *)open(false);

    return tcidbput(idb, id, val);
}

bool SearchDB::del(int64_t id)
{
    TCIDB *idb = (TCIDB *)open(false);

    return tcidbout(idb, id);
}

value SearchDB::find(string words)
{
    TCIDB *idb = (TCIDB *)open(true);
    uint64_t *rids = NULL;
    int rnum = 0;
    int i = 0;
    value wordlist = strutil::splitquoted(words); 
    value result;

    foreach(word, wordlist) {
        value ids;
        rids = tcidbsearch(idb, word, IDBSSUBSTR, &rnum);

        for(i = 0; i < rnum; i++) {
            // will truncate ids that are too big on some platforms
            ids.newval() = (unsigned int)rids[i];
        }

        free(rids);

        result[word] = ids;
    }

    return $("query", words) -> $("results", result);
}

value SearchDB::query(string expr)
{
    TCIDB *idb = (TCIDB *)open(true);
    uint64_t *rids = NULL;
    int rnum = 0;
    int i = 0;
    value ids;

    rids = tcidbsearch2(idb, expr, &rnum);

    for(i = 0; i < rnum; i++) {
        // will truncate ids that are too big on some platforms
        ids.newval() = (unsigned int)rids[i];
    }

    free(rids);

    return $("query", expr) -> $("ids", ids);
}

const char *SearchDB::last_error()
{
    TCIDB *idb = (TCIDB *)_db;

    return tcidberrmsg(tcidbecode(idb));
}

bool SearchDB::sync()
{
    TCIDB *idb = (TCIDB *)open(false);

    return tcidbsync(idb);
}
