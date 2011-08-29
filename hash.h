#ifndef __hash__
#define __hash__


class HashDB {
    public:
        void open();
        const char *get(const char *key);
        void put(const char *key, const char *value);
        bool putkeep(const char *key, const char *value);
        void putcat(const char *key, const char *value);
        bool del(const char *key);


    private:
        void *_db;
};

#endif
