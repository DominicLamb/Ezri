#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "CS_Kick"
#define DESCRIPTION "Kick a user from the channel."
#include "../headers/extensions.h"

int command_kick(Services_User *client, User *sender, cmd_message *message);
int help_kick(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("ChanServ", "KICK", command_kick, get_user)) {
      return 0;
   }
   wrapper_add_help("ChanServ", "KICK", "main", "Kick a user from the channel.", help_kick);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("chanserv", "KICK");
   drop_help_by_type("chanserv", "KICK"); 
   return 1;
}

int command_kick(Services_User *client, User *sender, cmd_message *message) {
   char username[NICK_MAX];
   char kick_message[IRCLINE_MAX];
   User *irc_user;
   IRC_Channel *channel;
   message_seek(message, copy_to(username, message_get(message), ' ', NICK_MAX));
   if(!message_get_target(message)) {
      help_kick(client, sender, message);
   }
   else if(username) {
      irc_user = get_user(username);
      if(!irc_user) {
         message_user(client, sender, "COMMAND_USER_OFFLINE", 1, username);
      }
      else if(!user_is_identified(sender)) {
         message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
      }
      else
      {
         channel = get_channel(message_get_target(message));
         if(!channel) {
            message_user(client, sender, "COMMAND_CHANNEL_EMPTY", 1, message_get_target(message));
         }
         else if(get_channel_access(channel->regdata, sender->nick) < channel_level_kick(channel->regdata)) {
            message_user(client, sender, "COMMAND_NO_ACCESS", 0);
         }
         else if(!user_on_channel(irc_user, channel->name)) {
            message_user(client, sender, "COMMAND_USER_NOT_ON_CHANNEL", 2, username, channel->name);
         }
         else if(!channel_is_registered(channel)) {
            message_user(client, sender, "COMMAND_CHANNEL_NOT_REGISTERED", 2, channel->name, client->nick);
         }
         else if(*message_get(message)) {
            snprintf(kick_message, IRCLINE_MAX, "Kick by %s (%s)", sender->nick, message_get(message));
            remote_server->kick(client->nick, message_get_target(message), username, kick_message);
         }
         else
         {
            snprintf(kick_message, 513, "Kick by %s (No reason specified)", sender->nick);
            remote_server->kick(client->nick, message_get_target(message), username, kick_message);
         }
      }
   }
   else
   {
      help_kick(client, sender, message);
   }
   if(username) {
      free(username);
   }
   return 1;
}

int help_kick(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_KICK_SYNTAX", 0);
      message_user(client, sender, "HELP_KICK_DESCRIBE", 0);
   }
   return 1;
}