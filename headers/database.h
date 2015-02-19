typedef struct DB_Table {
   FILE *handle;
   char filename[FILENAME_MAX];
   char **result;
   unsigned int result_count;
   int error;
} DB_Table;

typedef struct DB_Format {
   DB_Table *(*open)(const char * const name);
   int (*close)(DB_Table *base);
   int (*clean)(DB_Table *table);
   char **(*read)(struct DB_Table *db, const unsigned int cols);
   int (*write)(struct DB_Table *db, const unsigned int count, ...);
   DB_Table *(*append)(const char * const name, const unsigned int count, ...);
} DB_Format;
EXPORT struct DB_Format *database;
#ifndef COMPILE_EXTENSION
   struct DB_Format *database;
#endif
