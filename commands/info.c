#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Info"
#define DESCRIPTION "View information about registered users and channels."
#include "../headers/extensions.h"

void help_cs_info(Services_User *client, User *sender, cmd_message *message);
void help_ns_info(Services_User *client, User *sender, cmd_message *message);
int command_ns_info(Services_User *client, User *sender, cmd_message *message);
int command_cs_info(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("NickServ", "INFO", command_ns_info, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("NickServ", "INFO", "main", "Get information about a nickname.", help_ns_info);
   if(wrapper_add_command("ChanServ", "INFO", command_cs_info, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("ChanServ", "INFO", "main", "Get information about a channel.", help_cs_info);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("ChanServ", "INFO");
   drop_command_by_type("NickServ", "INFO");
   drop_help_by_type("ChanServ", "INFO");
   drop_help_by_type("NickServ", "INFO");
   return 1;
}

int command_ns_info(Services_User *client, User *sender, cmd_message *message) {
   User *user;
   Reg_User *reguser = 0;
   const char *msgptr = 0;
   struct tm *timestruct;
   char timestring[80];
   msgptr = message_get(message);
   if(!message || *msgptr == '\0') {
      reguser = get_reg_user(message_get(message));
   }
   else if(strpbrk(msgptr, " ,") || strlen(msgptr) > NICK_MAX) {
      message_user(client, sender, "COMMAND_ONE_NICKNAME", 0);
      return 1;
   }
   else
   {
      reguser = get_reg_user(message_get(message));
   }
   if(reguser) {
      timestruct = gmtime(&reguser->registered);
      strftime(timestring, 80, "%a %B %d %Y (%H:%M) %Z", timestruct);
      message_user(client, sender, "COMMAND_INFO_REGISTERED_DATE", 2, reguser->user, timestring);
      user = get_user(message_get(message));
      if(user && user_is_identified(user)) {
         if(user->is_oper) {
            message_user(client, sender, "COMMAND_INFO_NS_IRCOP", 2, user->nick);
            message_user(client, sender, "COMMAND_INFO_NS_ONLINE", 3, user->nick, user->username, user->hostname);
         }
         else
         {
            if(*(user->vhostname) != '\0') {
               message_user(client, sender, "COMMAND_INFO_NS_ONLINE", 3, user->nick, user->username, user->vhostname);
            }
            else
            {
               message_user(client, sender, "COMMAND_INFO_NS_ONLINE", 3, user->nick, user->username, user->hostname);
            }
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_INFO_NS_OFFLINE", 1, msgptr);
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_INFO_NS_NOT_REGISTERED", 1, message_get(message));
   }
   return 1;
}

int command_cs_info(Services_User *client, User *sender, cmd_message *message) {
   Reg_Channel *channel;
   IRC_Channel *channeldata;
   struct tm *timestruct;
   char timestring[80];
   char users[6];
   char bots[3];
   if(*message_get_target(message) != '#') {
      help_cs_info(client, sender, message);
   }
   else
   {
      channel = get_reg_channel(message_get_target(message));
      if(channel) {
         channeldata = get_channel(channel->name);
         snprintf(users, 6, "%d", channeldata->user_count);
         snprintf(bots, 3, "%d", channeldata->bot_count);
         message_user(client, sender, "COMMAND_INFO_CS_IN_USE", 3, channel->name, users, bots);
         timestruct = gmtime(&channel->registered);
         strftime(timestring, 80, "%a %B %d %Y (%H:%M) %Z", timestruct);
         message_user(client, sender, "COMMAND_INFO_REGISTERED_DATE", 2, channel->name, timestring);
         if(channeldata->topic && strlen(channeldata->topic) > 0) {
            timestruct = gmtime(&channeldata->topicstamp);
            strftime(timestring, 80, "%a %B %d %Y (%H:%M) %Z", timestruct);
            message_user(client, sender, "COMMAND_INFO_CS_TOPIC", 3, channeldata->topic, timestring, channeldata->topic_setter);
         }
         else
         {
            message_user(client, sender, "COMMAND_INFO_CS_NO_TOPIC", 0);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_CHANNEL_NOT_REGISTERED", 1, message_get_target(message));
      }
   }
   return 1;
}

void help_ns_info(Services_User *client, User *sender, cmd_message *message) {
   if(!message || *message_get(message) == '\0') {
      message_user(client, sender, "HELP_INFO_NS_SYNTAX", 0);
      message_user(client, sender, "HELP_INFO_NS_DESCRIBE", 0);
   }
}

void help_cs_info(Services_User *client, User *sender, cmd_message *message) {
   if(!message || *message_get(message) == '\0') {
      message_user(client, sender, "HELP_INFO_CS_SYNTAX", 0);
      message_user(client, sender, "HELP_INFO_CS_DESCRIBE", 0);
   }
}