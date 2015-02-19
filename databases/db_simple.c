#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "DB_Simple"
#define DESCRIPTION "Very simple file format."
#include "../headers/extensions.h"

DB_Table *db_open(const char * const name);
int db_close(DB_Table *base);
int db_writeline(DB_Table *base, const unsigned int count, ...);
char **db_readline(DB_Table *base, const unsigned int cols);
DB_Table *db_append(const char * const table, const unsigned int count, ...);
int db_clean(DB_Table *table);

int extension_load(void) {
   DB_Format *format;
   format = malloc(sizeof(DB_Format));
   if(format) {
      format->open = db_open;
      format->close = db_close;
      format->read = db_readline;
      format->write = db_writeline;
      format->clean = db_clean;
      format->append = db_append;
      add_extension_data(get_extension(EXTENSION_NAME), "db_format", format);
      return 1;
   }
   return 0;
}

int extension_unload(void) {
  return 1;
}

static void result_free(char **result, const size_t count) {
   size_t i = 0;
   if(result) {
      while(i < count) {
         if(result[i]) {
            free(result[i]);
         }
         i++;
      }
      memset(result, '\0', count * sizeof(char *));
   }
}

DB_Table *db_open(const char * const name) {
   DB_Table *base;
   char path[PATH_MAX];
   base = malloc(sizeof(DB_Table));
   if(base) {
      errno = 0;
      memset(base, '\0', sizeof(DB_Table));
      create_path(path, "dbfiles", "", 0);
      snprintf(base->filename, 255, "%s%s.db", path, name);
      base->handle = fopen(base->filename, "rb+");
      /* Assume the file does not exist */
      if(base->handle == NULL) {
         base->handle = fopen(base->filename, "wb+");
         if(base->handle == NULL) {
            free(base);
            return NULL;
         }
      }
      base->result = 0;
   }
   return base;
}

int db_close(DB_Table *base) {
   if(base) {
      if(base->result) {
         result_free(base->result, base->result_count);
         free(base->result);
      }
      if(fclose(base->handle) == EOF) {
         free(base);
         return 0;
      }
      free(base);
   }
   return 1;
}

int db_writeline(DB_Table *base, const unsigned int count, ...) {
   va_list args;
   char *p2;
   unsigned int i;
   if (base == NULL || count == 0) {
      return -1;
   }
   va_start(args, count);
   fseek(base->handle, 0, SEEK_END);
   for(i = 0; i < count; i++) {
      p2 = va_arg(args, char *);
      if(p2 && *p2) {
         fwrite(p2, 1, strlen(p2), base->handle);
      }
      fputc('\0', base->handle);
   }
   fputc('\n', base->handle);
   va_end(args);
   return i;
}

char **db_readline(DB_Table *base, const unsigned int cols) {
   char buffer[8192] = "\0";
   char **result = 0;
   unsigned int i,j;
   if (base == NULL || cols == 0) {
      return NULL;
   }
   if(base->result && cols == base->result_count) {
      result_free(base->result, base->result_count);
      result = base->result;
   }
   else
   {
      result = (char **)malloc(cols * sizeof(char *));
      if(result) {
         base->result = result;
         base->result_count = cols;
         memset(result, '\0', cols * sizeof(char *));
      }
   }
   if(result && fgets(buffer, sizeof(buffer), base->handle)) {
      for(i = 0, j = 0; buffer[i] != '\n' && j < cols; i++, j++) {
         result[j] = get_token(1, buffer + i, "");
         if(result[j]) {
            i += strlen(result[j]);
         }
      }
   }
   else
   {
	   result = 0;
   }
   return result;
}

DB_Table *db_append(const char * const table, const unsigned int count, ...) {
   DB_Table *base;
   va_list args;
   char *p2;
   unsigned int i;
   if (!table || count == 0) {
      return 0;
   }
   base = malloc(sizeof(DB_Table));
   if(base) {
      memset(base, '\0', sizeof(DB_Table));
      snprintf(base->filename, 255, "dbfiles/%s.db", table);
      base->handle = fopen(base->filename, "a");
      if(base->handle) {
         va_start(args, count);
         for(i = 0; i < count; i++) {
            p2 = va_arg(args, char *);
            if(p2 && *p2) {
               fwrite(p2, 1, strlen(p2), base->handle);
            }
            fputc('\0', base->handle);
         }
         fputc('\n', base->handle);
         va_end(args);
      }
   }
   else
   {
      base->error = errno;
   }
   return base;
}

int db_clean(DB_Table *table) {
   if(!table || !table->handle) {
      return -1;
   }
   fclose(table->handle);
   table->handle = fopen(table->filename, "wb+");
   if(table->handle) {
      return 1;
   }
   return 0;
}

int db_error(DB_Table *table) {
   if(!table) {
      return 0;
   }
   return table->error;
}
