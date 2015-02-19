#include "headers/main.h"
#include "headers/users.h"
#include "headers/language.h"

int add_language(const char * const shortcode, const char * const name) {
   Language *lang;
   if(!language_list) {
      language_list = malloc(sizeof(Language));
      lang = language_list;
   }
   else
   {
      lang = language_list;
      while(lang->next) {
         lang = lang->next;
      }
      lang->next = malloc(sizeof(Language));
      lang = lang->next;
   }
   if(lang) {
      memset(lang, '\0', sizeof(Language));
      strncpy_safe(lang->shortcode, shortcode, 8);
      strncpy_safe(lang->name, name, LANGUAGE_NAME_MAX);
      return 1;
   }
   return 0;
}

Language *get_language(const char * const name) {
   Language *lang = language_list;
   while(lang) {
      if(case_compare(lang->shortcode, name) || case_compare(lang->name, name)) {
         break;
      }
      lang = lang->next;
   }
   return lang;
}

/*
   A note about return codes:

   0 - String was not added.
   1 - String was added (Concatenated)
   2 - String was added (Extended)
*/

int add_language_substring(Language_String *langstring, const char * const line) {
   char *name;
   if(!langstring || !line) {
      return 0;
   }
   name = langstring->name;
   while(langstring->extend_count) {
      langstring = langstring->extend;
   }
   if(strlen(langstring->line) + strlen(line) < IRCLINE_MAX) {
      strncat(langstring->line, line, IRCLINE_MAX - strlen(langstring->line));
   }
   else
   {
      langstring->extend = malloc(sizeof(Language_String));
      langstring = langstring->extend;
      if(langstring) {
         memset(langstring, '\0', sizeof(Language_String));
         strncpy_safe(langstring->name, name, LANGUAGE_NAME_MAX);
         strncpy_safe(langstring->line, line, IRCLINE_MAX);
         langstring->extend_count++;
         return 1;
      }
   }
   return 0;
}

/*
   A note about return codes:

   0 - String was not added.
   1 - String was added (Normal).
   2 - String was added (Concatenated)
   3 - String was added (Extended)
*/
int add_language_string(const char * const language, const char * const name, const char * const line) {
   Language *lang;
   Language_String *langstring;
   lang = get_language(language);
   if(!language || !name || !line || !lang) {
      return 0;
   }
   if(lang->strings) {
      langstring = lang->strings;
      while(langstring->next) {
         if(case_compare(langstring->name, name)) {
            break;
         }
         langstring = langstring->next;
      }
      if(case_compare(langstring->name, name)) {
         if(strlen(line) + strlen(langstring->line) > IRCLINE_MAX - 1) {
            add_language_substring(langstring, line);
            return 3;
         }
         else
         {
            strncat(langstring->line, "\n", 1);
            strncat(langstring->line, line, IRCLINE_MAX - strlen(langstring->line));
            return 2;
         }
      }
      langstring->next = malloc(sizeof(Language_String));
      langstring = langstring->next;
   }
   else
   {
      lang->strings = malloc(sizeof(Language_String));
      langstring = lang->strings;
   }
   if(langstring) {
      memset(langstring, '\0', sizeof(Language_String));
      strncpy_safe(langstring->name, name, LANGUAGE_NAME_MAX);
      strncpy_safe(langstring->line, line, IRCLINE_MAX);
      langstring->extend_count = 1;
      return 1;
   }
   return 0;
}

int load_language(char * const language) {
   FILE *langfile;
   char path[PATH_MAX];
   char buffer[LINE_MAX + 1];
   char *b;
   char token[LANGUAGE_NAME_MAX];
   if(get_language(language)) {
      return 1;
   }
   if(!add_language(language, language)) {
      return 0;
   }
   create_path(path, "language", language, 0);
   strncat(path, ".lang", 6);
   langfile = fopen(path, "r");
   if(langfile) {
      while(fgets(buffer, LINE_MAX, langfile)) {
         b = skip_chars(buffer, " \t");
         if(strncmp(b, "//", 2) != 0 && *b != '#') {
            b = chr_delete(b, '\r');
            b = chr_delete(b, '\n');
            b += copy_to(token, b, ':', LANGUAGE_NAME_MAX);
            if(*token) {
               b = skip_chars(b, " \t");
               add_language_string(language, token, b);
            }
	      }
      }
      fclose(langfile);
      return 1;
   }
   return 0;
}

static Language_String *get_language_string(const char * const language, const char * const name) {
   Language *lang;
   Language_String *langstring = 0;
   lang = get_language(language);
   if(lang) {
      langstring = lang->strings;
      while(langstring) {
         if(case_compare(langstring->name, name)) {
            break;
         }
         langstring = langstring->next;
      }
   }
   return langstring;
}

/*
   There are large problems with the following functions
   that require fixing long before the final release
*/
static char *language_escape(char *input) {
   unsigned char *in = (unsigned char *)input;
   char *out;
   char *pos = 0;
   if(!in) {
      return 0;
   }
   out = malloc(str_count(input, "<") + strlen(input) + 1);
   if(out) {
      pos = out;
      while(in[0]) {
         if(strlen(input) > 2 && in[0] == '<' && (isdigit(in[1]) || in[1] == 'b' || in[1] == 'u') && in[2] == '>') {
            out[0] = '<';
            out++;
            out[0] = '!';
         }
         else
         {
            out[0] = in[0];
         }
         in++;
         out++;
      }
      *out = '\0';
   }
   return pos;
}

static char *language_unescape(char *string) {
   char *find;
   if(string) {
      find = strchr(string, '<');
      while(find) {
         if(find[1] && find[1] == '!' && find[2] && (isdigit((unsigned char)find[2]) || find[2] == 'u' || find[2] == 'b') && find[3] && find[3] == '>') {
            memmove(find + 1, find+2, strlen(find));   
         }
         find = strchr(find + 1, '<');
      }
   }
   return string;
}

char *parse_language_string(const char * const language, const char * const string, int count, va_list list) {
   int i = 0;
   char token[5];
   char *arg;
   Language_String *langstring;
   Language_String *langsub;
   char *replace = 0;
   char *buffer = 0;
   if(!language || !string) {
      return 0;
   }
   langstring = get_language_string(language, string);
   if(langstring) {
      replace = malloc(IRCLINE_MAX * langstring->extend_count);
      buffer = malloc(IRCLINE_MAX * langstring->extend_count);
      if(replace && buffer) {
         strncpy_safe(replace, langstring->line, IRCLINE_MAX);
         if(langstring->extend) {
            langsub = langstring->extend;
            while(langsub) {
               strncat(replace, "\n", 1);
               strncat(replace, langsub->line, IRCLINE_MAX);
               langsub = langsub->next;
            }
         }
         strncpy_safe(buffer, replace, IRCLINE_MAX * langstring->extend_count);
         if(count) {
            for(i = 0; i < count && strchr(buffer, '<'); i++) {
               sprintf(token, "<%1d>", i + 1);
               arg = va_arg(list, char *);
               if(arg && *arg) {
                  if(strchr(arg, '<')) {
                     arg = language_escape(arg);
                     str_replace_(token, arg, replace, buffer, IRCLINE_MAX * langstring->extend_count);
                     strncpy_safe(replace, buffer, IRCLINE_MAX * langstring->extend_count);
                     free(arg);
                  }
                  else
                  {
                     str_replace_(token, arg, replace, buffer, IRCLINE_MAX * langstring->extend_count);
                     strncpy_safe(replace, buffer, IRCLINE_MAX * langstring->extend_count);
                  }
               }
               else if(arg) {
                  str_replace_(token, "", replace, buffer, IRCLINE_MAX * langstring->extend_count);
                  strncpy_safe(replace, buffer, IRCLINE_MAX * langstring->extend_count);
               }
	         }
         }
         str_replace_("<u>", "\x1F", replace, buffer, langstring->extend_count * IRCLINE_MAX);
         str_replace_("</u>", "\x1F", buffer, replace, langstring->extend_count * IRCLINE_MAX);
         str_replace_("<col>", "\x03", replace, buffer, langstring->extend_count * IRCLINE_MAX);
         str_replace_("</col>", "\x03", buffer, replace, langstring->extend_count * IRCLINE_MAX);
         str_replace_("<b>", "\x02", replace, buffer, langstring->extend_count * IRCLINE_MAX);
         str_replace_("</b>", "\x02", buffer, replace, langstring->extend_count * IRCLINE_MAX);
         free(replace);
      }
   }
   if(count) {
      return language_unescape(buffer);
   }
   return buffer;
}