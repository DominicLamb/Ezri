#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "List"
#define DESCRIPTION "Show a list of regisetered users and channels."
#include "../headers/extensions.h"

int command_ns_list(Services_User *client, User *sender, cmd_message *message);
int command_cs_list(Services_User *client, User *sender, cmd_message *message);

int help_ns_list(Services_User *client, User *sender, cmd_message *message);
int help_cs_list(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("NickServ", "LIST", command_ns_list, get_user) != 1) {
      return 0;
   }
   if(wrapper_add_command("ChanServ", "LIST", command_cs_list, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("NickServ", "LIST", "main", "Get a list of registered users", help_ns_list);
   wrapper_add_help("ChanServ", "LIST", "main", "Get a list of registered channels", help_cs_list);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("NickServ", "LIST");
   drop_command_by_type("ChanServ", "LIST");
   drop_help_by_type("NickServ", "LIST");
   drop_help_by_type("ChanServ", "LIST");
   return 1;
}

int command_ns_list(Services_User *client, User *sender, cmd_message *message) {
   char response[513];
   char args[9];
   char *hostname;
   User *user = userlist;
   Reg_User *reg_users = reg_user_list;
   struct tm *tm;
   char timestring[80];
   if(message) {
      copy_to(args, message_get(message), ' ', 9);
      if(case_compare(args,"CONNECTED")) {
         message_user(client, sender, "COMMAND_LIST_NS_BEGIN", 0);
         while(user != 0) {
            tm = gmtime(&user->connected);
            strftime(timestring, 80, "%a %B %d %Y (%H:%M) %Z", tm);
            if(user_is_ircop(sender) || *(user->vhostname) == '\0') {
               hostname = user->hostname;
            }
            else
            {
               hostname = user->vhostname;
            }
            snprintf(response, 513, "[%*.s] (%s@%s) [Connected: %s]", NICK_MAX, user->nick, user->username, hostname, timestring);
            message_user(client, sender, response, 0);
            user = user->next;
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_LIST_NS_REGISTERED", 0);
         while(reg_users != 0) {
            tm = gmtime(&user->connected);
            strftime(timestring, 80, "%a %B %d %Y (%H:%M) %Z", tm);
            snprintf(response, 513, "[%s] (%s) [Registered: %s]", reg_users->user, reg_users->email, timestring);
            plain_message_user(client, sender, response);
            reg_users = reg_users->next;
         }
      }
   }
   return 1;
}


int command_cs_list(Services_User *client, User *sender, cmd_message *message) {
   char response[513];
   char *args = 0;
   IRC_Channel *channel = channellist;
   Reg_Channel *regchannel = reg_channel_list;
   if(message) {
      args = get_token(1, message_get(message), " ");
      if(args && case_compare(args,"OCCUPIED")) {
         message_user(client, sender, "COMMAND_LIST_CS_OCCUPIED", 0);
         if(channellist != 0) {
            while(channel != 0) {
               snprintf(response, 513, "[%s] [%s]", channel->name, channel->topic);
               plain_message_user(client, sender, response);
               channel = channel->next;
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_LIST_NO_CHANNELS", 0);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_LIST_CS_REGISTERED", 0);
         if(reg_channel_list) {
            while(regchannel != 0) {
               snprintf(response, 513, "[%s] [%s]", regchannel->name, regchannel->founder);
               plain_message_user(client, sender, response);
               regchannel = regchannel->next;
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_LIST_NO_CHANNELS", 0);
         }
      }
   }
   if(args) {
      free(args);
   }
   return 1;
}

int help_ns_list(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_LIST_NS_SYNTAX", 0);
      message_user(client, sender, "HELP_LIST_NS_DESCRIBE", 0);
   }
   return 1;
}

int help_cs_list(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_LIST_CS_SYNTAX", 0);
      message_user(client, sender, "HELP_LIST_CS_DESCRIBE", 0);
   }
   return 1;
}