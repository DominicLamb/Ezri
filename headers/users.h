#define ACCESS_MAX 1000
/* Defined in commands.h */
struct cmd_message;

#define BAN_NICKNAME 1
#define BAN_USERNAME 2
#define BAN_HOSTNAME 4

typedef struct Reg_User {
   struct Reg_User *next;
   struct Reg_User *next_by_user;
   struct Channel_Access *access;
   struct Extension_Data *extdata;
   time_t registered;
   int dummy;
   char user[NICK_MAX];
   char password[64];
   char email[128];
   char last_ident[16];
   char last_host[HOST_MAX];
   char vhost[HOST_MAX];
} Reg_User;

typedef struct User {
   Reg_User *regdata;
   struct User *next;
   struct Channel_User *channels;
   struct Extension_Data *extdata;
   time_t connected;
   unsigned long id;
   int is_oper;
   char nick[NICK_MAX];
   char hostname[HOST_MAX];
   char vhostname[HOST_MAX];
   char username[16];
   char realname[65];
   char modes[65];
   char server[HOST_MAX];
} User;

typedef struct Services_User {
   struct Services_User *next;
   int (*main_routine)(struct Services_User *client, struct cmd_message *message);
   time_t connected;   
   char nick[NICK_MAX];
   char hostname[HOST_MAX];
   char username[16];
   char trigger[8];
   char realname[64];
   char modes[64];
   char type[16];
   char routine_name[32];
   struct svs_commandhook *command;
   struct svs_help *help;
} Services_User;

EXPORT int introduce_users(void **args);
EXPORT void drop_client(const char * const client);
EXPORT void client_switch_routine(Services_User *client, const char * const routine);
EXPORT void client_set_trigger(Services_User *client, const char * const trigger);
EXPORT int is_reg_user(const char * const username);
EXPORT Reg_User *get_reg_user(const char * const username);
EXPORT User *get_user(const char * const nickname);
EXPORT int is_user(const char * const nickname);
EXPORT void drop_reg_user(const char * const user);
EXPORT User *add_user(const char * const nickname, const char * const ident, const char * const hostname, const char * const realname, const time_t date);
EXPORT Services_User *load_svsuser(const char * const nickname, const char * const username, const char * const hostname, const char * const realname, const char * const type);
EXPORT Services_User *get_svsclient(const char * const in);
EXPORT Services_User *get_svsclient_by_type(const char * const in);
EXPORT int svsclient_triggered(Services_User *client, const char * const message);
EXPORT Services_User *get_svsclient_by_trigger(const char * const trigger);
EXPORT int add_svsclient(Services_User *client);
EXPORT int is_svsclient(const char * const in);
EXPORT User *is_identified(const char * const username);
EXPORT Reg_User *get_identified_user(const char * const user);
EXPORT int user_is_identified(User *user);
EXPORT Reg_User *register_nickname(const char * const nickname, const char * const password, const char * const email);
EXPORT int identify_user(User *user, const char * const username, const char * const password);
EXPORT int auto_identify(User *user);
EXPORT void user_mode(User *user, const char mode, const int remove);
EXPORT int user_has_mode(User *user, const char mode);
EXPORT void user_new_vhost(const char * const username, const char * const hostname);
EXPORT void user_new_ident(const char * const username, const char * const ident);
EXPORT void user_new_realname(const char * const username, const char * const realname);
EXPORT void user_vhost(User *username, const char * const hostname);
EXPORT void drop_user_data(User *user, const char * const name);
EXPORT int add_user_data(User *user, const char * const name, void *data);
EXPORT void *get_user_data(User *user, const char * const name);
EXPORT void drop_reg_user_data(Reg_User *user, const char * const name);
EXPORT int add_reg_user_data(Reg_User *user, const char * const name, void *data);
EXPORT void *get_reg_user_data(Reg_User *user, const char * const name);
EXPORT int user_is_ircop(User *user);
EXPORT void user_ident(User *user, const char * const ident);
EXPORT void user_realname(User *user, const char * const realname);
EXPORT void user_ircop(User *user, const int remove);
EXPORT User *is_ircop(const char * const nickname);
EXPORT User *user_new_nickname(const char * const sender, const char * const nickname);
EXPORT void drop_user(const char * const username);
EXPORT void drop_users_by_server(const char * const server);
EXPORT int reg_user_vhost(Reg_User *user, const char * const hostname);
EXPORT char *user_get_hostname(User *user, const int prefer_real);
EXPORT char *user_banmask(User *user, char * const out, const int ban_type);

EXPORT User *userlist;
EXPORT unsigned long svsid;
EXPORT unsigned long userid;
EXPORT Services_User *svslist;
EXPORT Reg_User *reg_user_list;

#ifndef COMPILE_EXTENSION
   Services_User *svslist;
   unsigned long svsid;
   unsigned long userid;
   User *userlist;
   Reg_User *reg_user_list;
#endif
