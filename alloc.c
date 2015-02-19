#include "headers/main.h"
static char *string_alloc(const char * const delimiter, const char * const source) {
   char *string;
   size_t bytes;
   if(delimiter && source && *source != '\0') {
      if(strstr(source, delimiter) > source) {
         bytes = (strstr(source, delimiter) - source);
      }
      else
      {
         bytes = strlen(source);
      }
      string = malloc(bytes + 1);
      if(string) {
         strncpy_safe(string, source, bytes + 1);
      }
      return string;
   }
   return 0;
}

char *get_token(const unsigned int token, const char * const source, const char * const delimiter) {
   char *string = 0;
   ptrdiff_t bytes = 0;
   if(token > 1) {
      bytes = get_token_start(token, source, delimiter);
      if(bytes == -1) {
         return 0;
      }
   }
   if(source && delimiter) {
      string = string_alloc(delimiter, source + bytes);
   }
   return string;
}

char *get_token_remainder(const unsigned int token, const char * const source, const char * const delimiter) {
   char *string = 0;
   ptrdiff_t bytes = 0;
   size_t length;
   if(token > 0) {
      bytes = get_token_start(token, source, delimiter);
      if(bytes > 0) {
         length = strlen(source);
         string = string_alloc("", source + bytes);
      }
   }
   return string;
}

char *str_replace(const char * const search, const char * const replace, const char *subject) {
   char *buffer = malloc(IRCLINE_MAX);
   if(buffer) {
      return str_replace_(search, replace, subject, buffer, IRCLINE_MAX);
   }
   return 0;
}
