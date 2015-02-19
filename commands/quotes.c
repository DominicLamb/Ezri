#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "CS_Quotes"
#define DESCRIPTION "Display an entry message when users join the channel"
#include "../headers/extensions.h"

int command_quote(Services_User *client, User *sender, cmd_message *message);

int help_quote(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("ChanServ", "QUOTE", command_quote, is_identified) != 1) {
      return 0;
   }
   wrapper_add_help("ChanServ", "QUOTE", "main", "Display a message when you join a channel.", help_kill);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("ChanServ", "QUOTE");
   drop_help_by_type("ChanServ", "QUOTE");
   return 1;
}

int command_quote(Services_User *client, User *sender, cmd_message *message) {
   Reg_User *user;
   Reg_Channel *channel;
   Channel_Quote *quote;
   int level;
   if(!message || *message_get(message) == '\0') {
      help_quote(client, sender, message);
      return 1;
   }
   else if(!user_is_identified(sender)) {
   }
   channel = get_reg_channel(message_get_target(message));
   if(channel) {
      level = get_channel_access(channel, sender->nick);
      if(level > 0 && level > channel_level_quote(channel)) {
         
      }
   }
   else
   {
   }
   return 1;
}

int help_quote(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_QUOTE_SYNTAX", 0);
      message_user(client, sender, "HELP_QUOTE_DESCRIBE", 0);
   }
   return 1;
}