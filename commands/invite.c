#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "CS_Invite"
#define DESCRIPTION "Allows users to invite themselves into a channel."
#include "../headers/extensions.h"

void help_invite(Services_User *client, User *sender, cmd_message *message);
int command_invite(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("ChanServ", "INVITE", command_invite, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("ChanServ", "INVITE", "main", "Invite yourself to the channel.", help_invite);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("ChanServ", "INVITE");
   drop_help_by_type("ChanServ", "INVITE");
   return 1;
}
int command_invite(Services_User *client, User *sender, cmd_message *message) {
   Reg_Channel *channel;
   if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 1, client->nick);
      return 1;
   }
   if(*message_get_target(message) == '#') {
      channel = get_reg_channel(message_get_target(message));
      if(channel) {
         if(get_channel_access(channel, sender->nick) >= channel->levels.can_invite) {
            if(!user_on_channel(sender, channel->name)) {
               remote_server->invite(client->nick, sender->nick, channel->name);
            }
            else
            {
               message_user(client, sender, "COMMAND_INVITE_ALREADY_THERE", 1, channel->name);
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_ACCESS", 1, channel->name);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_CHANNEL_EMPTY", 1, message_get_target(message));
      }
   }
   return 1;
}

void help_invite(Services_User *client, User *sender, cmd_message *message) {
   IRC_Channel *channel;
   if(message && *message_get_target(message)  == '#') {
      message_user(client, sender, "HELP_INVITE_SYNTAX", 1, message_get_target(message));
      message_user(client, sender, "HELP_INVITE_DESCRIBE", 1, message_get_target(message));
      if(user_is_identified(sender)) {
         channel = get_channel(message_get_target(message));
         if(!channel || channel->user_count == 0) {
            message_user(client, sender, "COMMAND_CHANNEL_EMPTY", 1, message_get_target(message));
         }
         else if(!channel_is_registered(channel)) {
            message_user(client, sender, "COMMAND_CHANNEL_NOT_REGISTERED", 1, message_get_target(message));
         }
      }
   }
   else
   {
      message_user(client, sender, "HELP_INVITE_SYNTAX", 1, "#channel");
      message_user(client, sender, "HELP_INVITE_DESCRIBE", 1, "#channel");
   }
}
      