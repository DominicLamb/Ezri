#define LEVEL_MAX 1000
#define LEVEL_SUBOWNER 990

typedef struct Channel_Access {
   struct Channel_Access *next;
   struct Channel_Access *next_for_user;
   int level;
   char nick[NICK_MAX];
   char channel[CHANNEL_MAX];
} Channel_Access;

typedef struct Channel_Levels {
   int is_admin;
   int is_op;
   int is_halfop;
   int is_voice;
   int can_invite;
   int can_cs_ban;
   int can_access;
   int can_kick;
   int can_quote;
} Channel_Levels;

typedef struct Channel_Ban {
   char hostmask[MASK_MAX];
   char setter[TARGET_MAX];
   char type;
   char extban;
   struct Channel_Ban *next;
} Channel_Ban;

typedef struct Reg_Channel {
   struct Reg_Channel *next;
   struct Extension_Data *extdata;
   time_t registered;
   char name[CHANNEL_MAX];
   char founder[NICK_MAX];
   char lock_pass[16];
   struct Channel_Access *access;
   struct Channel_Levels levels;
   struct IRC_Channel *channel;
   char channel_bot[NICK_MAX];
} Reg_Channel;

/*
   Some servers send a full mask for the topic setter 
*/
typedef struct IRC_Channel {
   struct IRC_Channel *next;
   struct Channel_User *users;
   struct Channel_Bot *bots;
   struct Reg_Channel *regdata;
   Channel_Ban *bans;
   time_t founded;
   time_t topicstamp;
   unsigned int user_count;
   unsigned int bot_count;
   unsigned long user_limit;
   char name[CHANNEL_MAX];
   char modes[32];
   char password[32];
   char topic[TOPIC_MAX];
   char topic_setter[TARGET_MAX];
} IRC_Channel;

typedef struct Channel_Bot {
   Services_User *botdata;
   IRC_Channel *channel;
   time_t joined;
   struct Channel_Bot *next;
} Channel_Bot;

typedef struct Channel_User {
   struct Channel_User *next;
   struct Channel_User *next_for_user;
   IRC_Channel *channel;
   User *user;
   time_t joined;
   short is_channelop;
   short is_admin;
   short is_owner;
   short is_voice;
   short is_halfop;
} Channel_User;

EXPORT IRC_Channel *get_channel(const char * const channelname);
EXPORT Reg_Channel *get_reg_channel(const char * const channelname);
EXPORT int channel_add_user(IRC_Channel *channel, User *user);
EXPORT int channel_add_bot(IRC_Channel *channel, Services_User *user);
EXPORT int add_channel_bot(const char * const channelname, const char * const username);
EXPORT void channel_add_topic(IRC_Channel *channel, const char * const topic);
EXPORT void channel_add_topicdata(IRC_Channel *channel, const char * const setter, const char * const u_timestamp);
EXPORT int add_channel_user(const char * const channelname, const char * const str_user);
EXPORT Channel_User *get_channel_user(IRC_Channel *channel, const char * const user);
EXPORT IRC_Channel *add_channel(const char * const name);
EXPORT int is_channel(const char * const channel);
EXPORT void drop_channel(const char * const channel);
EXPORT void drop_reg_channel(const char * const channel);
EXPORT int add_reg_channel_data(Reg_Channel *regchannel, const char * const name, void *data);
EXPORT Channel_User *user_on_channel(User *user, const char * const channel);
EXPORT Reg_Channel *register_channel(const char * const channel, const char * const founder, const  char * const bot);
EXPORT int reg_channel_add_user(Reg_Channel *channel, const char * const user, const int level);
EXPORT int channel_is_registered(IRC_Channel *channel);
EXPORT int get_channel_access(Reg_Channel *channel, const char * const user);
EXPORT int channel_operator(IRC_Channel *target, const char * const user, const unsigned int remove);
EXPORT int channel_halfop(IRC_Channel *target, const char * const user, const unsigned int remove);
EXPORT int channel_voice(IRC_Channel *target, const char * const user, const unsigned int remove);
EXPORT int channel_admin(IRC_Channel *target, const char * const user, const unsigned int remove);
EXPORT int channel_owner(IRC_Channel *target, const char * const user, const unsigned int remove);
EXPORT int channel_is_voice(IRC_Channel *channel, const char * const user);
EXPORT int channel_is_halfop(IRC_Channel *channel, const char * const user);
EXPORT int channel_is_op(IRC_Channel *channel, const char * const user);
EXPORT int channel_is_admin(IRC_Channel *channel, const char * const user);
EXPORT int channel_is_owner(IRC_Channel *channel, const char * const user);
EXPORT int channel_add_mode(IRC_Channel *channel, const char mode);
EXPORT int channel_delete_mode(IRC_Channel *channel, const char mode);
EXPORT void channel_password(IRC_Channel *channel, const char * const password);
EXPORT int channel_user_limit(IRC_Channel *channel, const unsigned long limit);
EXPORT void delete_channel_user(const char * const channel, const char * const username);
EXPORT void channel_partall(const char * const username);
EXPORT int channel_access_voice(Reg_Channel *channel, const char * const nickname);
EXPORT int channel_access_halfop(Reg_Channel *channel, const char * const nickname);
EXPORT int channel_access_operator(Reg_Channel *channel, const char * const nickname);
EXPORT int channel_access_admin(Reg_Channel *channel, const char * const nickname);
EXPORT int channel_access_owner(Reg_Channel *channel, const char * const nickname);
EXPORT void create_ban(Channel_Ban *ban, const char * const mask, const char * const setter, const char type, const char extban);
EXPORT void channel_add_ban(IRC_Channel *channel, const char * const mask, const char * const setter, const char type, const char extban);
EXPORT int channel_level_voice(Reg_Channel *channel);
EXPORT int channel_level_halfop(Reg_Channel *channel);
EXPORT int channel_level_operator(Reg_Channel *channel);
EXPORT int channel_level_admin(Reg_Channel *channel);
EXPORT int channel_level_ban(Reg_Channel *channel);
EXPORT int channel_level_kick(Reg_Channel *channel);
EXPORT int channel_registered(const char * const name);
EXPORT int valid_channel(const char * const channel);

EXPORT Reg_Channel *reg_channel_list;
EXPORT IRC_Channel *channellist;
#ifndef COMPILE_EXTENSION
   Reg_Channel *reg_channel_list;
   IRC_Channel *channellist;
#endif
