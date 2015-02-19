#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "CS_Ban"
#define DESCRIPTION "Allows users to place channel bans through the services."
#include "../headers/extensions.h"

int command_ban(Services_User *client, User *sender, cmd_message *message);
int command_timedban(Services_User *client, User *sender, cmd_message *message);
int help_ban(Services_User *client, User *sender, cmd_message *message);
int help_tban(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("ChanServ", "BAN", command_ban, get_user) != 1) {
      return 0;
   }
   else if(wrapper_add_command("ChanServ", "TBAN", command_timedban, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("ChanServ", "BAN", "main", "Place a channel ban on a user.", help_ban);
   wrapper_add_help("ChanServ", "TBAN", "main", "Place a timed channel ban on a user.", help_tban);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("chanserv", "BAN");
   drop_help_by_type("chanserv", "BAN");
   drop_command_by_type("chanserv", "TBAN");
   drop_help_by_type("chanserv", "TBAN");
   return 1;
}

int permission_ban(Services_User *client, User *sender, IRC_Channel *channel, char *username) {
   int can_ban = 0;
   Reg_Channel *regchannel;
   if(user_is_identified(sender)) {
      if(channel) {
         regchannel = get_reg_channel(channel->name);
         if(get_channel_access(regchannel, sender->nick) >= channel_level_ban(regchannel)) {
            if(get_channel_access(regchannel, username) < get_channel_access(regchannel, sender->nick)) {
               can_ban = 1;
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, channel->name);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_CHANNEL_EMPTY", 1, channel);
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 1, client->nick);
   }
   return can_ban;
}

int timer_unban(void **args, const int timer_id) {
   char *channel;
   char *ban;
   char *client;
   if(args) {
      channel = args[0];
      client = args[1];
      ban = args[2];
      if(channel && ban && client) {
         remote_server->modes(client, channel, "-b", ban);
      }
   }
   return TIMER_DELETE;
}

int command_timedban(Services_User *client, User *sender, cmd_message *message) {
   size_t ban_length;
   char username[MASK_MAX];
   char banmask[MASK_MAX];
   char bantime[16];
   IRC_Channel *channel;
   User *user;
   if(!message || *message_get(message) == '\0' || *message_get_target(message) != '#') {
      help_ban(client, sender, message);
      return 1;
   }
   channel = get_channel(message_get_target(message));
   if(isdigit(*message_get(message))) {
      message_seek(message, copy_to(bantime, message_get(message), ' ', 16));
      copy_to(username, message_get(message), ' ', MASK_MAX);
   }
   else
   {
      message_seek(message, copy_to(username, message_get(message), ' ', MASK_MAX));
      copy_to(bantime, message_get(message), ' ', 16);
   }
   if(username && permission_ban(client, sender, channel, username)) {
      /* Guaranteed mask characters */
      if(strpbrk(username, "!@?*.")) {
         create_banmask(username, banmask);
      }
      else
      {
         user = get_user(username);
         if(user) {
            user_banmask(user, banmask, BAN_HOSTNAME);
         }
         else
         {
            /* We know it's a hostname */
            create_banmask(username, banmask);
         }
      }
      if(valid_banmask(banmask)) {
         remote_server->modes(client->nick, message_get_target(message), "+b", banmask);
         ban_length = parse_timestring(bantime);
         if(ban_length > 0) {
            if(add_timer("unban", timer_unban, ban_length, 3, channel->name, client->nick, banmask)) {
            }
         }
      }
   }
   else
   {
      help_tban(client, sender, message);
   }
   return 1;
}
int command_ban(Services_User *client, User *sender, cmd_message *message) {
   char *username = 0;
   char banmask[MASK_MAX];
   IRC_Channel *channel;
   User *user;
   if(!message || *message_get(message) == '\0' || *message_get_target(message) != '#') {
      help_ban(client, sender, message);
      return 1;
   }
   channel = get_channel(message_get_target(message));
   username = get_token(1, message_get(message), " ");
   if(username && permission_ban(client, sender, channel, username)) {
      /* Guaranteed mask characters */
      if(strpbrk(username, "!@?*.")) {
         create_banmask(username, banmask);
      }
      else
      {
         user = get_user(username);
         if(user) {
            user_banmask(user, banmask, BAN_HOSTNAME);
         }
         else
         {
            create_banmask(username, banmask);
         }
      }
      if(valid_banmask(banmask)) {
         remote_server->modes(client->nick, message_get_target(message), "+b", banmask);
      }
   }
   return 1;
}

int help_ban(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_BAN_BAN_SYNTAX", 0);
      message_user(client, sender, "HELP_BAN_BAN_DESCRIBE", 0);
   }
   return 1;
}

int help_tban(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_BAN_TBAN_SYNTAX", 0);
      message_user(client, sender, "HELP_BAN_TBAN_DESCRIBE", 0);
   }
   return 1;
}
