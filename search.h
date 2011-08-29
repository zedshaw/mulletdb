#ifndef __search__
#define __search__

#include <grace/value.h>

enum SearchState { START, READ, WRITE };

class SearchDB {
    public:
        void *open(bool readonly);
        bool close();

        value get(int64_t id);
        bool put(int64_t id, const char *val);
        bool del(int64_t id);
        value query(string expr);
        value find(string words);
        value id(string name);
        bool sync();
        const char *last_error();
        void file(const char *dbf) {
            db_file = strdup(dbf);
        }

        SearchDB() : state(START) { };

        ~SearchDB() {
            close();
            free(db_file);
        }

    private:
        SearchState state;
        void *_db;
        char *db_file;
};

#endif
