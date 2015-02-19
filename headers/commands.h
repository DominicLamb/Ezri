#define FILE_INVALID_FILENAME -2
#define FILE_NOT_FOUND -1
#define FILE_ACCESS_DENIED 0
#define FILE_LOADED_NOT_VALID 1
#define FILE_SAYS_NO 2
#define FILE_LOADED 3

#define LOG_DEBUG_CORE 1
#define LOG_DEBUG_PROTOCOL 2
#define LOG_NOTICE 4
#define LOG_WARNING 8
#define LOG_CRITICAL 16
#define LOG_NO_RELAY 32
#define LOG_PENDING 1024
#define LOG_DEBUG LOG_DEBUG_CORE + LOG_DEBUG_PROTOCOL

#define COMMAND_MAX 32
#define HELP_MAX COMMAND_MAX
#define EXTENSION_MAX 64

typedef struct cmd_message {
   const char * message;
   const char *ptr;
   char source[CHANNEL_MAX];
   char implied_target[CHANNEL_MAX];
   char target[TARGET_MAX];
   char command[COMMAND_MAX];
   time_t date;
} cmd_message;

typedef int (*svs_routine)(Services_User *svsclient, cmd_message *msg_struct);

typedef struct Extension_Data {
   void *data;
   struct Extension_Data *next;
   char name[EXTENSION_MAX];
} Extension_Data;

typedef struct Extension {
   struct Extension *next;
   ExtensionFile handle;
   Extension_Data *extension_data;
   svs_routine routine;
   int critical;
   char filename[FILENAME_MAX];
   char name[EXTENSION_MAX];
   char author[64];
   char description[150];
} Extension;

typedef struct svs_help {
   struct svs_help *next;
   char list[HELP_MAX];
   char helpname[HELP_MAX];
   char brief[250];
   void (*function)(Services_User *client, User *sender, cmd_message *message);
} svs_help;

typedef struct svs_commandhook {
   struct svs_commandhook *next;
   char input[COMMAND_MAX];
   int (*function)(Services_User *client, User *sender, cmd_message *message);
   User *(*restrictions)(const char * const nickname);
} svs_commandhook;

EXPORT Extension *extensions;

EXPORT void plain_message_user(Services_User *client, User *user, const char * const message);
EXPORT void plain_message_channel(Services_User *client, Reg_Channel *channel, const char * const message);
EXPORT void plain_message_channel_level(Services_User *client, Reg_Channel *channel, const char * const message, const int level);
EXPORT void message_user(Services_User *client, User *user, const char * const message, const int count, ...);
EXPORT void ctcp_user(Services_User *client, User *user, const char * const message);
EXPORT void ctcpreply_user(Services_User *client, User *user, const char * const message);

EXPORT int add_command(Services_User *client, svs_commandhook *command);
EXPORT int add_command_by_type(const char * const type, svs_commandhook *command);
EXPORT int add_command_by_routine(const char * const type, svs_commandhook *command);
EXPORT void command_restrict(svs_commandhook *command, User *(*restriction)(const char * const user));
EXPORT svs_commandhook *make_command(const char * const input, int function(Services_User *client, User *sender, cmd_message *message));
EXPORT svs_commandhook *find_command(Services_User *client, const char * const message);
EXPORT svs_commandhook *get_command(Services_User *client, const char * const message);
EXPORT void drop_command(Services_User *client, const char * const command);
EXPORT void drop_command_by_type(const char * const type, const char * const command);
EXPORT int extension_loadfile(const char * const directory, const char * const filename);
EXPORT int add_extension(Extension_Info *data, const char * const filename);
EXPORT Extension *get_extension_by_filename(const char * const path, const char * const filename);
EXPORT int add_extension_data(Extension *extension, const char * const name, void *data);
EXPORT void add_extension_routine(Extension *extension, svs_routine routine);
EXPORT void drop_help(Services_User *client, const char * const help);
EXPORT void drop_help_by_type(const char * const type, const char * const help);
EXPORT void extension_mark_critical(Extension *extension, const int toggle);
EXPORT void *get_extension_data(const char * const module_name, const char * const data_name);
EXPORT svs_routine *get_extension_function(const char * const module_name);
EXPORT Extension *get_extension(const char * const module_name);
EXPORT int extension_loaded(const char * const module_name);
EXPORT int unload_extension(const char * const name);
EXPORT svs_help *create_help(const char * const name, const char * const brief, const char * const list, void (*function)(Services_User *client, User *sender, cmd_message *message));
EXPORT int svs_add_help(Services_User *client, svs_help *help);
EXPORT int svs_add_help_by_type(const char * const type, svs_help *help);
EXPORT int process_message(const char * const user, const char * const target, const char * const message);
EXPORT svs_help *get_help_list(Services_User *client, const char * const list);
EXPORT svs_help *get_help(Services_User *client, const char * const name);
EXPORT ptrdiff_t message_seek(cmd_message *msg_struct, ptrdiff_t seek);
EXPORT const char *message_get(cmd_message *msg);
EXPORT const char *message_get_beginning(cmd_message *msg);
EXPORT void message_rewind(cmd_message *msg_struct);
EXPORT char *message_get_target(cmd_message *msg);
EXPORT char *message_get_implied_target(cmd_message *msg);
EXPORT char *message_get_real_target(cmd_message *msg);
EXPORT char *message_get_sender(cmd_message *msg);
EXPORT void message_imply_target(cmd_message *msg_struct);
EXPORT void log_message(const char * const message, const int level, ...);
#ifndef COMPILE_EXTENSION
   Extension *extensions;
   FILE *logfile;
   int logging;
#endif
