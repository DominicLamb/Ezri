#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "CS_Lock"
#define DESCRIPTION "Enforce a channel lock."
#include "../headers/extensions.h"

int command_channel_lock(Services_User *client, User *sender, cmd_message *message);
int help_lock(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("ChanServ", "LOCK", command_channel_lock, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("ChanServ", "LOCK", "main", "Lock the channel with a password for entry.", help_lock);
   return 0;
}

int extension_unload(void) {
   drop_command_by_type("chanserv", "LOCK");
   drop_help_by_type("chanserv", "LOCK");
   return 1;
}

int command_channel_lock(Services_User *client, User *sender, cmd_message *message) {
   char *password;
   char *channel;
   Reg_Channel *regchannel;
   if(!message || *message_get_target(message) != '#') {
      help_lock(client, sender, message);
   }
   else if(*message_get(message) == '\0') {
      message_user(client, sender, "COMMAND_LOCK_NO_PASSWORD", 1, message_get_target(message));
   }
   else if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
   }
   else
   {
      channel = message_get_target(message);
      password = get_token(1, message_get(message), "");
      if(!password || strlen(password) > 23 || strpbrk(password, " ,")) {
         message_user(client, sender, "COMMAND_INVALID_PASSWORD", 0);
      }
      else
      {
         regchannel = get_reg_channel(channel);
         if(regchannel) {
            remote_server->modes(client->nick, channel, "+k", password);
            message_user(client, sender, "COMMAND_LOCK_SET", 1, channel);
         }
         else
         {
            message_user(client, sender, "COMMAND_CHANNEL_NOT_REGISTERED", 1, channel);
         }
      }
      if(password) {
         free(password);
      }
   }
   return 1;
}

int help_lock(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_LOCK_SYNTAX", 0);
      message_user(client, sender, "HELP_LOCK_DESCRIBE", 0);
   }
   return 1;
}