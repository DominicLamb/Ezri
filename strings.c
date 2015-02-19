#include "headers/main.h"

char *skip_chars(char * const in, const char * const chars) {
   char *ptr;
   if(!in) {
      return 0;
   }
   ptr = in + strspn(in, chars);
   return ptr;
}

char *skip_string(char *in, const char * const chars) {
   if(!in || !chars) {
      return 0;
   }
   while(strstr(in, chars) == in) {
      in += strlen(chars);
   }
   return in;
}

char *chr_delete(char *in, const char remove) {
   char *orig = in;
   char *out = in;
   do {
      if(*in != remove) {
         *out++ = *in;
      }
   } while(*in++);
   return orig;
}

char *chr_replace(char *in, const char old, const char newchar) {
   char *find;
   find = strchr(in, old);
   while(find) {
      *find = newchar;
      find = strchr(find, old);
   }
   return in;
}

ptrdiff_t get_token_start(const unsigned int token, const char *source, const char *delimiter) {
   const char *p = 0;
   const char *q = source;
   unsigned int i;
   for(i = 1; i < token; i++) {
      if(p != 0) {
         q = p + strlen(delimiter);
      }
      p = strstr(q, delimiter);
      if(p) {
         p = skip_chars((char *)p, delimiter);
      }
      else if(!p && i != token) {
         return -1;
      }
      if(!p || *p == '\0') {
         p = source + strlen(source);
         break;
      }
   }
   if(p - source >= 0) {
      return(p - source);
   }
   return 0;
}

char *strtolower(char *string) {
   char *temp = string;
   if(!temp) {
      return 0;
   }
   while(*temp) {
      *temp = (char)tolower(*temp);
      temp++;
   }
   return string;
}

char *strtoupper(char *string) {
   char *temp = string;
   if(!temp) {
      return 0;
   }
   while(*temp) {
      *temp = (char)toupper(*temp);
      temp++;
   }
   return string;
}

ptrdiff_t stripos(const char *source, const char *string) {
   if(string && source) {
      const char *string2 = string;
      const char *source2 = source;
      while(*string && *source) {
         if(*string != *source && tolower(*string) != tolower(*source)) {
            string = string2;
         }
         string++;
         source++;
      }
      if(*string == '\0') {
         return source - (source2 + strlen(string2));
      }
   }
   return -1;
}

unsigned int str_count(const char *source, const char *string) {
   const char *token = source;
   int i = 0;
   if(source && string) {
      while(token) {
         token = strstr(token, string);
         if(token) {
            token += strlen(string);
            i++;
         }
      }
   }
   return i;
}

char *str_replace_(const char *search, const char *replace, const char *subject, char *buffer, const size_t maxlength) {
   char *p;
   char *next;
   size_t length;
   if(!search || *search == '\0') {
      strncpy_safe(buffer, subject, maxlength);
      return buffer;

   }
   p = strstr(subject, search);
   if(!p) {
      strncpy_safe(buffer, subject, maxlength);
      return buffer;
   }
   length = strlen(search);
   memset(buffer, 0, maxlength);
   strncat(buffer, subject, p - subject);
   while(p != 0) {
      next = strstr(p + length, search);
      strncat(buffer, replace, maxlength);
      if(next) {
         strncat(buffer, p + length, next - p - length);
      }
      else
      {
         strncat(buffer, p + length, maxlength - strlen(buffer));
      }
      p = next;
   }
   return buffer;
}

/*
   stricmp does not exist on all platforms, here is an equivalent.
   TODO: Find a faster way of doing this?
*/
int case_compare(const char *string_1, const char *string_2) {
   for(; *string_1 && *string_2; string_1++, string_2++) {
      if(*string_1 != *string_2 && tolower(*string_1) != tolower(*string_2)) {
         break;
      }
   }
   return (*string_1 == *string_2);
}

int case_compare_length(const char *string_1, const char *string_2, size_t length) {
   for(; length > 0 && *string_1 && *string_2; string_1++, string_2++, length--) {
      if(*string_1 != *string_2 && tolower(*string_1) != tolower(*string_2)) {
         break;
      }
   }
   return (*(string_1 - 1) == *(string_2 - 1) && length == 0);
}

int compare_to(const char * const in, const char * const compare, const char * const string) {
   const ptrdiff_t chr = strstr(in, string) - in;
   if(chr > 0 && !strncmp(in, compare, chr)) {
      return 1;
   }
   return 0;
}

int case_compare_to(const char * const in, const char * const compare, const char * const string) {
   const ptrdiff_t chr = strstr(in, string) - in;
   if(chr > 0 && case_compare_length(in, compare, chr)) {
      return 1;
   }
   return 0;
}

size_t copy_to(char * const buffer, const char * const in, char copychar, size_t max) {
   size_t result = 0;
   const char * const temp = strchr(in, copychar);
   if(temp) {
      result = temp - in;
   }
   if(result > 0) {
      if(result + 1 <= max) {
         strncpy_safe(buffer, in, result + 1);
         return result + 1;
      }
   }
   else
   {
      strncpy_safe(buffer, in, max);
      return strlen(buffer);
   }
   return 0;
}

/* 
   Loose check only. 
   It should be up to the user to provide a legitimate address to send to. 
*/

int valid_email(const char *email) {
   unsigned int i;
   const unsigned char *p = (const unsigned char *)email;
   int at_pos = 0;
   if(!email) {
      return 0;
   }
   for(i = 0; *p != '\0'; i++) {
      if(!isalnum(*p) && (email[i] != '.' && email[i] != '-' && email[i] != '_' && email[i] != '@')) {
         return 0;
      }
      else if(email[i] == '@') {
         if(at_pos) {
            return 0;
         }
         at_pos = i;
      }
      else if(email[i] == '.') {
         if(i == 0 || email[i - 1] == '.') {
            return 0;
         }
      }
      p++;
   }
   p--;
   if(at_pos && isalpha(*p)) {
      p = (unsigned char *)strrchr(email, '.');
      if(p && (char *)p - email > at_pos + 1) {
         return 1;
      }
   }
   return 0;
}

unsigned long parse_timestring(char *in) {
   unsigned long length = 0;
   unsigned long duration = 0;
   while(in && *in != '\0' && length < 157680000) {
      if(isdigit(*(unsigned char *)in)) {
         duration = strtoul(in, &in, 10);
      }
      if(duration) {
         if(*in == 'm' && duration < 2628000) {
           duration = duration * 60;
         }
         else if(*in == 'h' && duration < 43800) {
            duration = duration * 3600;
         }
         else if(*in == 'd' && duration <= 1900) {
            duration = duration * 86400;
         }
         else if(*in == 'w' && duration <= 275) {
            duration = duration * 604800;
         }
         else if(*in == 'M' && duration <= 60) {
            duration = duration * 2620800;
         }
         else if(*in == 'y' && duration <= 5) {
            duration = duration * 31449600;
         }
         in++;
      }
      if(duration + length > 157680000) {
         length = 157680000;
         break;
      }
      else
      {
         length += duration;
      }
      duration = 0;
   }
   return length;
}

/*
  TODO: String representation of duration
*/
void timestring_duration(char *buffer, char *in) {
   unsigned long duration;
   if(buffer) {
      duration = parse_timestring(in);
   }
}

unsigned int rand_range(unsigned int from, unsigned int to) {
   return(from + (int)((to - from + 1) * rand() / (RAND_MAX + 1.0)));
}

unsigned int file_exists(const char *path) {
   FILE *file;
   file = fopen(path, "r");
   if(file != NULL) {
      fclose(file);
      return 1;
   }
   return 0;
}

char *create_banmask(const char * const in, char * const out) {
   char nickname[NICK_MAX] = "\0";
   char ident[11] = "\0";
   char host[HOST_MAX] = "\0";
   char *find_first;
   const char *find;
   size_t length = 0;
   find_first = strchr(in, '!');
   if(find_first) {
      copy_to(nickname, in, '!', NICK_MAX);
   }
   find = strchr(in, '@');
   if(find) {
      length = find - (in + strlen(nickname) + 1);
      if(length > 11) {
         length = 11;
      }
      if(*nickname) {
         strncpy_safe(ident, (in + (find_first - in)) + 1, length);
         strncpy_safe(host, find + 1, HOST_MAX);
      }
      else
      {
         strncpy_safe(ident, in + 1, length);
         strncpy_safe(host, find + 1, HOST_MAX);
      }
   }
   else if(!find && !find_first) {
      if(strchr(in, '.')) {
         strncpy_safe(host, in, HOST_MAX);
      }
      else
      {
         strncpy_safe(nickname, in, NICK_MAX);
      }
   }
   if(*nickname == '\0') {
      *nickname = '*';
   }
   if(*ident == '\0') {
      *ident = '*';
   }
   if(*host == '\0') {
      *host = '*';
   }
   snprintf(out, MASK_MAX, "%s!%s@%s", nickname, ident, host);
   return out;
}

int valid_banmask(const char *mask) {
   size_t exc_pos = 0;
   size_t at_pos = 0;
   size_t i;
   const unsigned char *in = (const unsigned char *)mask;
   if(!in) {
      return 0;
   }
   for(i = 0; in[i]; i++) {
      if(in[i] == '!' || in[i] == '@') {
         if(in[i] == '@') {
            if(at_pos) {
               return 0;
            }
            at_pos = i;
         }
         else if(in[i] == '!') {
            if(exc_pos) {
               return 0;
            }
            exc_pos = i;
         }
      }
      else if(isalnum(in[i]) || in[i] == '*' || in[i] == '?' || in[i] == '.' || in[i] == '-' || in[i] == '_' || in[i] == '~' || in[i] == ':') {
      }
      else
      {
         return 0;
      }
   }
   if(i) {
      return 1;
   }
   return 0;
}

int wildcard_compare(const char * const string, const char * const mask) {
   int i = 0;
   int j = 0;
   if(!string || !mask) {
      return 0;
   }
   while(string && string[i] && mask[j]) {
      if(string[i] != mask[j] && tolower(string[i]) != tolower(mask[j])) {
         if(mask[j] != '?' && mask[j] != '*') {
            break;
         }
      }
      if(mask[j] == '*' || mask[j] == '?') {
         while(mask[j] && string[i] != '\0' && (mask[j] == '?' || mask[j + 1] == '*')) {
            if(mask[j] == '?') {
               i++;
               j++;
            }
            else
            {
               while(mask[j] == '*') {
                  j++;
               }
            }
         }
         if(mask[j] && mask[j + 1]) {
            while(string) {
               if(!i) {
                  i++;
               }
               if(wildcard_compare(strchr(string + i, mask[j + 1]), mask + j + 1)) {
                  return 1;
               }
            }
         }
         else
         {
            return 1;
         }
      }
      else
      {
         i++;
      }
      j++;
   }
   if(string && string[i] == '\0' && mask[j] == '\0') {
      return 1;
   }
   return 0;
}

void make_timestamp(char * const buffer, const time_t date) {
   struct tm *tm;
   tm = gmtime(&date);
   if(tm) {
      snprintf(buffer, 15, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
   }
   else
   {
      *buffer = '\0';
   }
}

time_t import_timestamp(const char *input) {
   struct tm *tm;
   char backup[5];
   const time_t temp = time(NULL);
   if(input) {
      tm = gmtime(&temp);
      strncpy_safe(backup, input, 5);
      input += 4;
      tm->tm_year = atoi(backup) - 1900;
      strncpy_safe(backup, input, 3);
      input += 2;
      tm->tm_mon = atoi(backup) - 1;
      strncpy_safe(backup, input, 3);
      input += 2;
      tm->tm_mday = atoi(backup);
      strncpy_safe(backup, input, 3);
      input += 2;
      tm->tm_hour = atoi(backup);
      strncpy_safe(backup, input, 3);
      input += 2;
      tm->tm_min = atoi(backup);
      strncpy_safe(backup, input, 3);
      tm->tm_sec = atoi(backup);
      return mktime(tm);
   }
   else
   {
      return temp;
   }
}

signed long export_unix_timestamp(const time_t input) {
	struct tm *tm;
	signed long output = 0;
	tm = gmtime(&input);
	output = tm->tm_sec + (tm->tm_min * 60) + (tm->tm_hour * 3600) + (tm->tm_yday  * 86400);
	output += ((tm->tm_year - 70) * 31536000) + (int)((tm->tm_year - 68) / 4) * 86400;
	return output;
}

time_t import_unix_timestamp(const char *input) {
   struct tm *tm;
   const time_t temp = time(NULL);
   long intinput = atol(input);
   tm = gmtime(&temp);
   tm->tm_mon = 0;
   tm->tm_year = 70;
   tm->tm_hour = 0;
   tm->tm_min = 0;
   tm->tm_mday = 1;
   tm->tm_mday += (int)(intinput / 86400);
   intinput = (int)intinput % 86400;
   tm->tm_sec = intinput;

   return gmmktime(tm);
}