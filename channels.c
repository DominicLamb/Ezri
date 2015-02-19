#include "headers/main.h"
#include "headers/users.h"
#include "headers/channels.h"
#include "headers/commands.h"
#include "headers/config.h"

static void delete_channel(IRC_Channel *channel) {
   Channel_User *p;
   Channel_User *next;
   if(channel->users) {
      for(p = channel->users; p != 0; p = p->next) {
         next = p->next;
         free(p);
         p = next;
      }
   }
   free(channel);
   channel = 0;
}

IRC_Channel *get_channel(const char * const channelname) {
   IRC_Channel *channel = channellist;
   if(!channelname) {
      return 0;
   }
   while(channel != 0) {
      if(case_compare(channel->name, channelname)) {
         return channel;
      }
      channel = channel->next;
   }
   return 0;
}

int is_channel(const char * const channel) {
   IRC_Channel *p = channellist;
   while(p != 0) {
      if(case_compare(p->name,channel)) {
         return 1;
      }
      p = p->next;
   }
   return 0;
}

void drop_channel(const char * const channel) {
   IRC_Channel *prev;
   IRC_Channel *p = channellist;
   Reg_Channel *regchannel;
   regchannel = get_reg_channel(channel);
   if(regchannel) {
      regchannel->channel = 0;
   }
   if(channellist) {
      prev = channellist;
      while(p != 0) {
         if(case_compare(p->name, channel)) {
            if(p == channellist) {
               p = p->next;
               delete_channel(channellist);
               channellist = p;
            }
            else
            {
               prev->next = p->next;
               delete_channel(p);
            }
            break;
         }
         prev = p;
         p = p->next;
      }
   }
}

static void delete_reg_channel(Reg_Channel *channel) {
   Channel_Access *access;
   access = channel->access;
   while(channel->access) {
      channel->access = access->next;
      free(access);
   }
   free(channel);
}

void drop_reg_channel(const char * const channelname) {
   Reg_Channel *prev;
   Reg_Channel *p = reg_channel_list;
   IRC_Channel *channel;
   channel = get_channel(channelname);
   if(channel) {
      channel->regdata = 0;
   }
   if(reg_channel_list) {
      prev = reg_channel_list;
      while(p != 0) {
         if(case_compare(p->name, channelname)) {
            if(p == reg_channel_list) {
               reg_channel_list = p->next;
               delete_reg_channel(p);
            }
            else
            {
               prev->next = p->next;
               delete_reg_channel(p);
            }
            break;
         }
         prev = p;
         p = p->next;
      }
   }
}

static void channel_delete_user(IRC_Channel *channel, const char * const username) {
   Channel_User *channel_user;
   Channel_User *prev;
   Channel_User *user_channels;
   User *user;
   if(channel && channel->users != 0) {
      channel_user = channel->users;
      prev = channel_user;
      while(channel_user) {
         if(case_compare(channel_user->user->nick, username)) {
            user = channel_user->user;
            if(channel_user == channel->users) {
               channel->users = channel_user->next;
            }
            else
            {
               prev->next = channel_user->next;
            }
            user_channels = user->channels;
            while(user_channels) {
               if(user_channels == channel_user) {
                  if(user_channels == user->channels) {
                     user->channels = user_channels->next_for_user;
                  }
                  else
                  {
                     prev->next_for_user = user_channels->next_for_user;
                  }
                  break;
               }
               prev = user_channels;
               user_channels = user_channels->next_for_user;
            }
            free(channel_user);
            break;
         }
         prev = channel_user;
         channel_user = channel_user->next;
      }
      channel->user_count--;
      if(channel->user_count == 0 && !channel_is_registered(channel)) {
         drop_channel(channel->name);
      }
   }
}

int channel_add_user(IRC_Channel *channel, User *user) {
   Channel_User *new_user = 0;
   Channel_User *user_map;
   if(channel && user) {
      channel->user_count++;
      new_user = channel->users;
      if(!new_user) {
         channel->users = malloc(sizeof(Channel_User));
         new_user = channel->users;
      }
      else
      {
         while(new_user->next) {
            new_user = new_user->next;
         }
         new_user->next = malloc(sizeof(Channel_User));
         new_user = new_user->next;
      }
      if(new_user) {
         memset(new_user, 0, sizeof(Channel_User));
         new_user->user = user;
         new_user->channel = channel;
         new_user->joined = time(NULL);
         user_map = user->channels;
         if(user_map) {
            while(user_map->next_for_user) {
               user_map = user_map->next_for_user;
            }
            user_map->next_for_user = new_user;
         }
         else
         {
            user->channels = new_user;
         }
         return 1;
      }
   }
   return 0;
}

void channel_delete_bot(IRC_Channel *channel, Services_User *user) {
   Channel_Bot *bot;
   Channel_Bot *prev;
   if(channel) {
      bot = channel->bots;
      prev = bot;
      while(bot) {
         if(!user || case_compare(bot->botdata->nick, user->nick)) {
            if(bot == channel->bots) {
               channel->bots = bot->next;
               free(bot);
               bot = channel->bots;
               prev = bot;
            }
            else
            {
               prev->next = bot->next;
               free(bot);
               bot = prev->next;
            }
         }
         else
         {
            prev = bot;
            bot = bot->next;
         }
      }
   }
}

void delete_channel_bot(const char * const channelname, const char * const bot) {
   IRC_Channel *channel;
   Services_User *user = 0;
   if(channelname) {
      channel = get_channel(channelname);
      if(channel) {
         if(bot) {
            user = get_svsclient(bot);
         }
         channel_delete_bot(channel, user);
      }
   }
}

int channel_add_bot(IRC_Channel *channel, Services_User *user) {
   Channel_Bot *bots;
   if(channel && user) {
      bots = channel->bots;
      if(bots) {
         while(bots->next) {
            bots = bots->next;
         }
         bots->next = malloc(sizeof(Channel_Bot));
         bots = bots->next;
      }
      else
      {
         channel->bots = malloc(sizeof(Channel_Bot));
         bots = channel->bots;
      }
      if(bots) {
         bots->botdata = user;
         bots->channel = channel;
         bots->joined = time(NULL);
         bots->next = 0;
         channel->bot_count++;
         return 1;
      }
   }
   return 0;
}

int add_channel_bot(const char * const channelname, const char * const username) {
   Services_User *user;
   IRC_Channel *channel;
   if(username && channelname) {
      channel = get_channel(channelname);
      if(!channel) {
         channel = add_channel(channelname);
      }
      if(channel) {
         user = get_svsclient(username);
         return channel_add_bot(channel, user);
      }
   }
   return 0;
}

void channel_add_topic(IRC_Channel *channel, const char * const topic) {
   if(channel && topic) {
      strncpy_safe(channel->topic, topic, TOPIC_MAX);
   }
}

void channel_add_topicdata(IRC_Channel *channel, const char * const setter, const char * const u_timestamp) {
   if(channel && setter && u_timestamp) {
      strncpy_safe(channel->topic_setter, setter, TARGET_MAX);
      channel->topicstamp = import_unix_timestamp(u_timestamp);
   }
}
int add_channel_user(const char * const channelname, const char * const str_user) {
   IRC_Channel *channel;
   User *user;
   if(channelname && str_user) {
      channel = get_channel(channelname);
      user = get_user(str_user);
      if(channel && user) {
         return channel_add_user(channel, user);
      }
   }
   return 0;
}

Channel_User *get_channel_user(IRC_Channel *channel, const char * const user) {
   Channel_User *p;
   if(channel->users == 0) {
      return 0;
   }
   p = channel->users;
   while(p != 0) {
      if(case_compare(user, p->user->nick)) {
         return p;
      }
      p = p->next;
   }
   return 0;
}


/*
   Do not optimise this to determine a User
   struct, we can access one much faster
   within channel_delete_user
*/
void delete_channel_user(const char * const channel, const char * const username) {
   IRC_Channel *p_channel;
   p_channel = get_channel(channel);
   if(p_channel) {
      if(username) {
         channel_delete_user(p_channel, username);
      }
   }
}

void channel_partall(const char * const username) {
   User *user;
   Channel_User *user_channellist;
   if(username) {
      user = get_user(username);
      if(user) {
         user_channellist = user->channels;
         while(channellist) {
            channel_delete_user(user_channellist->channel, username);
            user_channellist = user->channels;
         }
      }
   }
}

int channel_access_voice(Reg_Channel *channel, const char * const nickname) {
   Channel_Access *access;
   if(channel) {
      access = channel->access;
      while(access) {
         if(case_compare(access->nick, nickname)) {
            break;
         }
         access = access->next;
      }
      if(access) {
         if(access->level >= channel->levels.is_voice) {
            return 1;
         }
      }
   }
   return 0;
}

int channel_access_halfop(Reg_Channel *channel, const char * const nickname) {
   Channel_Access *access;
   if(channel) {
      access = channel->access;
      while(access) {
         if(case_compare(access->nick, nickname)) {
            break;
         }
         access = access->next;
      }
      if(access) {
         if(access->level >= channel->levels.is_halfop) {
            return 1;
         }
      }
   }
   return 0;
}

int channel_access_operator(Reg_Channel *channel, const char * const nickname) {
   Channel_Access *access;
   if(channel) {
      access = channel->access;
      while(access) {
         if(case_compare(access->nick, nickname)) {
            break;
         }
         access = access->next;
      }
      if(access) {
         if(access->level >= channel->levels.is_op) {
            return 1;
         }
      }
   }
   return 0;
}

int channel_access_admin(Reg_Channel *channel, const char * const nickname) {
   Channel_Access *access;
   if(channel) {
      access = channel->access;
      while(access) {
         if(case_compare(access->nick, nickname)) {
            break;
         }
         access = access->next;
      }
      if(access) {
         if(access->level >= channel->levels.is_admin) {
            return 1;
         }
      }
   }
   return 0;
}

int channel_access_owner(Reg_Channel *channel, const char * const nickname) {
   Channel_Access *access;
   if(channel) {
      access = channel->access;
      while(access) {
         if(case_compare(access->nick, nickname)) {
            break;
         }
         access = access->next;
      }
      if(access) {
         if(access->level == ACCESS_MAX) {
            return 1;
         }
      }
   }
   return 0;
}

int channel_is_voice(IRC_Channel *channel, const char * const user) {
   Channel_User *channel_user;
   if(channel == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(channel, user);
   if(channel_user == 0) {
      return 0;
   }
   return channel_user->is_voice;
}

int channel_is_halfop(IRC_Channel *channel, const char * const user) {
   Channel_User *channel_user;
   if(channel == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(channel, user);
   if(channel_user == 0) {
      return 0;
   }
   return channel_user->is_halfop;
}

int channel_is_op(IRC_Channel *channel, const char * const user) {
   Channel_User *channel_user;
   if(channel == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(channel, user);
   if(channel_user == 0) {
      return 0;
   }
   return channel_user->is_channelop;
}

int channel_is_admin(IRC_Channel *channel, const char * const user) {
   Channel_User *channel_user;
   if(channel == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(channel, user);
   if(channel_user == 0) {
      return 0;
   }
   return channel_user->is_admin;
}

int channel_is_owner(IRC_Channel *channel, const char * const user) {
   Channel_User *channel_user;
   if(channel == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(channel, user);
   if(channel_user == 0) {
      return 0;
   }
   return channel_user->is_owner;
}

int channel_delete_mode(IRC_Channel *channel, const char mode) {
   if(channel) {
      chr_delete(channel->modes, mode);
      return 1;
   }
   return 0;
}

int channel_add_mode(IRC_Channel *channel, const char mode) {
   if(channel && !strstr(channel->modes, &mode)) {
      channel->modes[strlen(channel->modes) - 1] = mode;
      return 1;
   }
   return 0;
}
int channel_user_limit(IRC_Channel *channel, const unsigned long limit) {
   if(channel) {
      channel->user_limit = limit;
      return 1;
   }
   return 0;
}

void channel_password(IRC_Channel *channel, const char * const password) {
   if(channel) {
      if(password) {
         strncpy_safe(channel->password, password, 32);
      }
      else
      {
         memset(channel->password, 0, sizeof(channel->password));
      }
   }
}

int channel_voice(IRC_Channel *target, const char *const user, const unsigned int rescind) {
   Channel_User *channel_user;
   if(target == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(target, user);
   if(channel_user == 0) {
      return 0;
   }
   if(rescind == 0) {
      channel_user->is_voice = 1;
   }
   else
   {
      channel_user->is_voice = 0;
   }

   return 1;
}

int channel_halfop(IRC_Channel *target, const char * const user, const unsigned int rescind) {
   Channel_User *channel_user;
   if(target == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(target, user);
   if(channel_user == 0) {
      return 0;
   }
   if(rescind == 0) {
      channel_user->is_halfop = 1;
   }
   else
   {
      channel_user->is_halfop = 0;
   }

   return 1;
}

int channel_operator(IRC_Channel *target, const char * const user, const unsigned int rescind) {
   Channel_User *channel_user;
   if(target == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(target, user);
   if(channel_user == 0) {
      return 0;
   }
   if(rescind == 0) {
      channel_user->is_channelop = 1;
   }
   else
   {
      channel_user->is_channelop = 0;
   }

   return 1;
}

int channel_admin(IRC_Channel *target, const char * const user, const unsigned int rescind) {
   Channel_User *channel_user;
   if(target == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(target, user);
   if(channel_user == 0) {
      return 0;
   }
   if(rescind == 0) {
      channel_user->is_admin = 1;
   }
   else
   {
      channel_user->is_admin = 0;
   }

   return 1;
}

int channel_owner(IRC_Channel *target, const char * const user, const unsigned int rescind) {
   Channel_User *channel_user;
   if(target == 0 || user == 0) {
      return 0;
   }
   channel_user = get_channel_user(target, user);
   if(channel_user == 0) {
      return 0;
   }
   if(rescind == 0) {
      channel_user->is_owner = 1;
   }
   else
   {
      channel_user->is_owner = 0;
   }

   return 1;
}

IRC_Channel *add_channel(const char * const name) {
   IRC_Channel *channel = channellist;
   Reg_Channel *regchannel = reg_channel_list;
   if(name == 0 || strlen(name) > CHANNEL_MAX) {
      return 0;
   }
   if(channellist == 0) {
      channellist = malloc(sizeof(IRC_Channel));
      channel = channellist;
   }
   else
   {
      channel = channellist;
      while(channel->next != 0) {
         if(case_compare(channel->name,name)) {
            return 0;
         }
         channel = channel->next;
      }
      channel->next = malloc(sizeof(IRC_Channel));
      channel = channel->next;
   }
   if(channel) {
      memset(channel, 0, sizeof(IRC_Channel));
      strncpy_safe(channel->name, name, CHANNEL_MAX);
      regchannel = get_reg_channel(name);
      if(regchannel) {
         regchannel->channel = channel;
         channel->regdata = regchannel;
      }
   }
   return channel;
}

int add_reg_channel_data(Reg_Channel *channel, const char * const name, void *data) {
   Extension_Data *extdata;
   if(!channel || !name || !data) {
      return 0;
   }
   if(!channel->extdata) {
      channel->extdata = malloc(sizeof(Extension_Data));
      extdata = channel->extdata;
   }
   else
   {
      extdata = channel->extdata;
      if(extdata) {
         while(extdata->next) {
            extdata = extdata->next;
         }
      }
      extdata = malloc(sizeof(Extension_Data));
   }
   if(extdata) {
      memset(extdata, '\0', sizeof(Extension_Data));
      strncpy_safe(extdata->name, name, 32);
      extdata->data = data;
      return 1;
   }
   return 0;
}

Channel_User *user_on_channel(User *user, const char * const channel) {
   Channel_User *channels;
   if(!channel || user == 0 || !user->channels) {
      return 0;
   }
   channels = user->channels;
   while(channels) {
      if(case_compare(channels->channel->name, channel)) {
         return channels;
      }
      channels = channels->next_for_user;
   }
   return 0;
}
Reg_Channel *register_channel(const char * const channel, const char * const founder, const char * const bot) {
   Reg_Channel *new_channel = reg_channel_list;
   void *config = 0;
   if(!channel || strlen(channel) > CHANNEL_MAX) {
      /* Invalid channel length */
      return 0;
   }
   if(!founder || strlen(founder) > NICK_MAX) {
      /* Invalid user length */
      return 0;
   }
   if(reg_channel_list == 0) {
      reg_channel_list = malloc(sizeof(Reg_Channel));
      new_channel = reg_channel_list;
   }
   else
   {
      while(new_channel->next != 0) {
         new_channel = new_channel->next;
      }
      new_channel->next = malloc(sizeof(Reg_Channel));
      new_channel = new_channel->next;
   }
   if(new_channel) {
      memset(new_channel, 0, sizeof(Reg_Channel));
      strncpy_safe(new_channel->name, channel, CHANNEL_MAX);
      if(founder) {
         strncpy_safe(new_channel->founder, founder, NICK_MAX);
      }
      if(bot && is_svsclient(bot)) {
         strncpy_safe(new_channel->channel_bot, bot, NICK_MAX);
      }
      else
      {
         config = get_svsclient_by_type("chanserv");
         if(config) {
            strncpy_safe(new_channel->channel_bot, (char *)config, NICK_MAX);
         }
      }
      new_channel->registered = time(NULL);
      /* This needs moving */
      get_config("ChanServ::levels::default_cs_ban", &config, RETURN_INT);
      new_channel->levels.can_cs_ban = *(int *)config;
      get_config("ChanServ::Levels::default_invite", &config, RETURN_INT);
      new_channel->levels.can_invite = *(int *)config;
      get_config("ChanServ::Levels::default_voice", &config, RETURN_INT);
      new_channel->levels.is_voice = *(int *)config;
      get_config("ChanServ::Levels::default_halfop", &config, RETURN_INT);
      new_channel->levels.is_halfop = *(int *)config;
      get_config("ChanServ::Levels::default_op", &config, RETURN_INT);
      new_channel->levels.is_op = *(int *)config;
      get_config("ChanServ::Levels::default_admin", &config, RETURN_INT);
      new_channel->levels.is_admin = *(int *)config;
      get_config("ChanServ::Levels::default_access", &config, RETURN_INT);
      new_channel->levels.can_access = *(int *)config;
      get_config("ChanServ::Levels::default_kick", &config, RETURN_INT);
      new_channel->levels.can_access = *(int *)config;
      new_channel->channel = get_channel(channel);
      if(new_channel->channel) {
         new_channel->channel->regdata = new_channel;
      }
   }
   return new_channel;
}

Reg_Channel *get_reg_channel(const char * const channelname) {
   Reg_Channel *channel = reg_channel_list;
   if(!channelname) {
      return 0;
   }
   while(channel != 0) {
      if(case_compare(channel->name, channelname)) {
         return channel;
      }
      channel = channel->next;
   }
   return 0;
}

void create_ban(Channel_Ban *ban, const char * const mask, const char * const setter, const char type, const char extban) {
   if(ban) {
      memset(ban, 0, sizeof(Channel_Ban));
      strncpy(ban->hostmask, mask, MASK_MAX);
      strncpy(ban->setter, setter, TARGET_MAX);
      ban->type = type;
      ban->extban = extban;
   }
}

void channel_add_ban(IRC_Channel *channel, const char * const mask, const char * const setter, const char type, const char extban) {
   Channel_Ban *ban;
   if(channel && mask && setter && type) {
      ban = channel->bans;
      if(channel->bans) {
         while(ban->next) {
            ban = ban->next;
         }
         ban->next = malloc(sizeof(Channel_Ban));
         ban = ban->next;
      }
      else
      {
         channel->bans = malloc(sizeof(Channel_Ban));
         ban = channel->bans;
      }
      create_ban(ban, mask, setter, type, extban);
   }
}

int channel_level_quote(Reg_Channel *channel) {
   if(!channel) {
      return ACCESS_MAX + 1;
   }
   return channel->levels.can_quote;
}

int channel_level_voice(Reg_Channel *channel) {
   if(!channel) {
      return ACCESS_MAX + 1;
   }
   return channel->levels.is_voice;
}

int channel_level_halfop(Reg_Channel *channel) {
   if(!channel) {
      return ACCESS_MAX + 1;
   }
   return channel->levels.is_halfop;
}

int channel_level_operator(Reg_Channel *channel) {
   if(!channel) {
      return ACCESS_MAX + 1;
   }
   return channel->levels.is_op;
}

int channel_level_admin(Reg_Channel *channel) {
   if(!channel) {
      return ACCESS_MAX + 1;
   }
   return channel->levels.is_admin;
}

int channel_level_ban(Reg_Channel *channel) {
   if(!channel) {
      return ACCESS_MAX + 1;
   }
   return channel->levels.can_cs_ban;
}
int channel_level_kick(Reg_Channel *channel) {
   if(!channel) {
      return ACCESS_MAX + 1;
   }
   return channel->levels.can_kick;
}

int channel_registered(const char * const name) {
   Reg_Channel *channel = reg_channel_list;
   while(channel != NULL) {
      if(case_compare(name,channel->name)) {
         return 1;
      }
      channel = channel->next;
   }
   return 0;
}

int channel_is_registered(IRC_Channel *channel) {
   if(channel) {
      return (channel->regdata != NULL);
   }
   return 0;
}

int reg_channel_add_user(Reg_Channel *channel, const char * const user, const int level) {
   Channel_Access *user_access = 0;
   if(level <= 0) {
      /* This is a delete command */
   }
   if(channel == 0 || user == 0) {
      return 0;
   }

   if(channel->access == 0) {
      channel->access = malloc(sizeof(Channel_Access));
      user_access = channel->access;
   }
   else
   {
      user_access = channel->access;
      while(user_access->next != 0) {
         user_access = user_access->next;
      }   
      user_access->next = malloc(sizeof(Channel_Access));
      user_access = user_access->next;
   }
   if(user_access) {
      strncpy_safe(user_access->nick, user, NICK_MAX);
      strncpy_safe(user_access->channel, channel->name, CHANNEL_MAX);
      user_access->level = level;
      user_access->next = 0;
      return 1;
   }
   return 0;
}

int get_channel_access(Reg_Channel *channel, const char * const user) {
   Channel_Access *user_access;
   if(channel == 0 || user == 0) {
      return 0;
   }
   if(channel->access == 0) {
      return 0;
   }
   else
   {
      user_access = channel->access;
      while(user_access != 0) {
         if(case_compare(user_access->nick, user)) {
            return user_access->level;
         }
         user_access = user_access->next;
      }
   }
   return 0;
}

int valid_channel(const char * const channel) {
   if(channel && *channel == '#') {
      if(!strpbrk(channel, "\",'")) {
         if(strlen(channel) < CHANNEL_MAX) { 
            return 1;
         }
      }
   }
   return 0;
}