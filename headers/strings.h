#define strncpy_safe(dest, input, length) { strncpy(dest, input, length); dest[length - 1] = '\0'; }

#define NICK_MAX 32
#define CHANNEL_MAX 33
#define HOST_MAX 64
#define TOPIC_MAX 308
#define TARGET_MAX CHANNEL_MAX + HOST_MAX + 1
#define MASK_MAX TARGET_MAX + 10

EXPORT char *skip_chars(char * const in, const char * const chars);
EXPORT char *skip_string(char *in, const char * const chars);
EXPORT ptrdiff_t get_token_start(const unsigned int token, const char *source, const char *delimiter);
EXPORT char *strtolower(char *string);
EXPORT char *strtoupper(char *string);
EXPORT ptrdiff_t stripos(const char *source, const char *string);
EXPORT int case_compare(const char *string_1, const char *string_2);
EXPORT int case_compare_length(const char *string_1, const char *string_2, size_t length);
EXPORT int compare_to(const char * const in, const char * const compare, const char * const string);
EXPORT int case_compare_to(const char * const in, const char * const compare, const char * const string);
EXPORT size_t copy_to(char * const buffer, const char * const in, char copychar, size_t max);
EXPORT int valid_email(const char *email);
EXPORT char *chr_delete(char *in, const char remove);
EXPORT char *chr_replace(char *in, const char old, const char newchar);
EXPORT unsigned int str_count(const char *source, const char *string);
EXPORT char *str_replace_(const char *search, const char *replace, const char *subject, char *buffer, const size_t maxlength);
EXPORT unsigned int rand_range(unsigned int from, unsigned int to);
EXPORT unsigned int file_exists(const char *path);
EXPORT unsigned long parse_timestring(char *in);
EXPORT char *create_banmask(const char * const in, char * const out);
EXPORT int valid_banmask(const char *in);
EXPORT int wildcard_compare(const char * const string, const char * const mask);
EXPORT void make_timestamp(char * const buffer, const time_t date);
EXPORT time_t import_unix_timestamp(const char *input);
EXPORT signed long export_unix_timestamp(const time_t input);
EXPORT time_t import_timestamp(const char *input);