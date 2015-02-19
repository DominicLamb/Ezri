#define LANGUAGE_NAME_MAX 32

typedef struct Language_String {
  char name[LANGUAGE_NAME_MAX];
  char line[IRCLINE_MAX];
  struct Language_String *extend;
  int extend_count;
  struct Language_String *next;
} Language_String;

typedef struct Language {
  char name[LANGUAGE_NAME_MAX];
  char shortcode[8];
  struct Language_String *strings;
  struct Language *next;
} Language;

EXPORT Language *language_list;
#ifndef COMPILE_EXTENSION
Language *language_list;
#endif
EXPORT int add_language(const char * const shortcode, const char * const name);
EXPORT Language *get_language(const char * const name);
EXPORT int add_language_string(const char * const language, const char * const name, const char * const line);
EXPORT int load_language(char * const language);
EXPORT char *parse_language_string(const char * const language, const char * const string, int count, va_list list);