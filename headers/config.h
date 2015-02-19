#define CONFIG_LINE_MAX 512
#define CONFIG_NAME_MAX 32
#define RETURN_SHORT 1
#define RETURN_USHORT 2
#define RETURN_INT 3
#define RETURN_UINT 4
#define RETURN_LONG 5
#define RETURN_ULONG 6
#define RETURN_CHAR 7
#define RETURN_BLOCK 8

struct key_value_pair {
   char key[CONFIG_NAME_MAX];
   char value[CONFIG_LINE_MAX];
   struct key_value_pair *next;
};

typedef struct configblock {
	struct configblock *next;
	char name[CONFIG_NAME_MAX];
	struct key_value_pair *keys;
	struct configblock *sub;
   struct configblock *child;
} configblock;

EXPORT configblock *config_list;
#ifndef COMPILE_EXTENSION
   configblock *config_list;
#endif
EXPORT int config_load();
EXPORT void config_drop(configblock *config);
EXPORT struct key_value_pair *get_key_pair(configblock *config, const char * const name);
EXPORT char *get_key_value(configblock *config, const char * const name);
EXPORT char *get_key_value_next(struct key_value_pair *keys, const char * const name);
EXPORT struct key_value_pair *get_key_pair_next(struct key_value_pair *keys, const char * const name);
EXPORT configblock *configblock_get(configblock *config, const char * const name);
EXPORT configblock *configblock_get_next_name(configblock *config);
EXPORT void get_config(const char *get, void **dest, const int type);