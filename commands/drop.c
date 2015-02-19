#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Drop"
#define DESCRIPTION "Drop nicknames and channels"
#include "../headers/extensions.h"

int command_nickname_drop(Services_User *client, User *sender, cmd_message *message);
int command_channel_drop(Services_User *client, User *sender, cmd_message *message);

int help_nickname_drop(Services_User *client, User *sender, cmd_message *message);
int help_channel_drop(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(get_svsclient_by_type("ChanServ")) {
      if(wrapper_add_command("ChanServ", "DROP", command_channel_drop, get_user) != 1) {
         return 0;
      }
      wrapper_add_help("ChanServ", "DROP", "main", "Unregister your current nickname, including all links.", help_channel_drop);
   }
   else
   {
      log_message("ChanServ DROP command requires at least one ChanServ-type client to be specified in ezri.conf. NickServ DROP will not be loaded.", LOG_WARNING);
   }

   if(get_svsclient_by_type("NickServ")) {
      if(wrapper_add_command("NickServ", "DROP", command_nickname_drop, get_user) != 1) {
         return 0;
      }
      wrapper_add_help("NickServ", "DROP", "main", "Unregister your current nickname, including all links.", help_nickname_drop);
   }
   else
   {
      log_message("NickServ DROP requires at least one NickServ-type client to be specified in ezri.conf. NickServ DROP will not be loaded.", LOG_WARNING);
   }
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("nickserv", "DROP");
   drop_command_by_type("chanserv", "DROP");
   drop_help_by_type("nickserv", "DROP");
   drop_help_by_type("chanserv", "DROP");
   return 1;
}

int command_nickname_drop(Services_User *client, User *sender, cmd_message *message) {
   char password[64];
   if(!message) {
      return 0;
   }
   else if(*message_get(message) == '\0') {
      help_nickname_drop(client, sender, message);
   }
   else if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
   }
   else
   {
      copy_to(password, message_get(message), ' ', 64);
      if(strcmp(password, sender->regdata->password) == 0) {
         drop_reg_user(sender->regdata->user);
         message_user(client, sender, "COMMAND_NICKNAME_DROPPED", 1, sender->nick);
      }
      else
      {
         message_user(client, sender, "COMMAND_BAD_PASSWORD", 0);
      }
   }
   return 1;
}

int command_channel_drop(Services_User *client, User *sender, cmd_message *message) {
   Reg_Channel *channel;
   if(!message) {
      return 0;
   }
   else if(*message_get_target(message) == '\0') {
      help_channel_drop(client, sender, message);
   }
   else if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
   }
   else
   {
      channel = get_reg_channel(message_get_target(message));
      if(!channel) {
         message_user(client, sender, "COMMAND_CHANNEL_NOT_REGISTERED", 1, message_get_target(message));
      }
      else if(channel_access_owner(channel, sender->nick)) {
         fire_event("channel_drop", 2, channel, sender->nick);
         drop_reg_channel(channel->name);
         message_user(client, sender, "COMMAND_CHANNEL_DROPPED", 1, message_get_target(message));
      }
      else
      {
         message_user(client, sender, "COMMAND_NO_ACCESS", 1, message_get_target(message));
      }
   }
   return 1;
}

int help_nickname_drop(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_DROP_NS_SYNTAX", 0);
      message_user(client, sender, "HELP_DROP_NS_DESCRIBE", 0);
   }
   return 1;
}

int help_channel_drop(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_DROP_CS_SYNTAX", 0);
      message_user(client, sender, "HELP_DROP_CS_DESCRIBE", 0);
   }
   return 1;
}