#include "hash.h"
#include <tcutil.h>


void HashDB::open()
{
    TCMAP *hdb = tcmapnew();
    _db = hdb;
}

const char *HashDB::get(const char *key)
{
    TCMAP *hdb = (TCMAP *)_db;
    return tcmapget2(hdb, key);
}

void HashDB::put(const char *key, const char *value)
{
    TCMAP *hdb = (TCMAP *)_db;
    tcmapput2(hdb, key, value); 
}

bool HashDB::putkeep(const char *key, const char *value)
{
    TCMAP *hdb = (TCMAP *)_db;
    return tcmapputkeep2(hdb, key, value);
}

void HashDB::putcat(const char *key, const char *value)
{
    TCMAP *hdb = (TCMAP *)_db;
    tcmapputcat2(hdb, key, value); 
}

bool HashDB::del(const char *key)
{
    TCMAP *hdb = (TCMAP *)_db;
    return tcmapout2(hdb, key);  
}


