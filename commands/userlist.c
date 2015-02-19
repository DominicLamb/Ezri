#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "CS_Userlist"
#include "../headers/extensions.h"

int command_userlist_view(Services_User *client, User *sender, cmd_message *message);
int command_userlist_add(Services_User *client, User *sender, cmd_message *message);
int help_userlist_add(Services_User *client, User *sender, cmd_message *message);
int help_userlist_view(Services_User *client, User *sender, cmd_message *message);

int default_op = 0;
int extension_load(void) {
   void *config;
   if(wrapper_add_command("ChanServ", "USERLIST ADD", command_userlist_add, is_identified) != 1) {
      return 0;
   }
   /*
      Find out whether limiting this to identified users
      at the command level is a good idea
   */
   if(wrapper_add_command("ChanServ", "USERLIST", command_userlist_view, is_identified) != 1) {
      return 0;
   }
   if(wrapper_add_command("ChanServ", "USERLIST VIEW", command_userlist_view, is_identified) != 1) {
      return 0;
   }
   get_config("ChanServ::levels::default_op", &config, 1);
   if(config) {
      default_op = *(int *)config;
   }
   if(!default_op) {
      default_op = 500;
   }
   wrapper_add_help("ChanServ", "ADD", "USERLIST", "Add a user to the list.", help_userlist_add);
   wrapper_add_help("ChanServ", "VIEW", "USERLIST", "View the user list.", help_userlist_view);
   wrapper_add_help("ChanServ", "USERLIST", "main", "View the user list.", help_userlist_view);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("chanserv", "USERLIST");
   drop_command_by_type("chanserv", "USERLIST ADD");
   drop_command_by_type("chanserv", "USERLIST ADD");
   drop_help_by_type("chanserv", "USERLIST");
   drop_help_by_type("chanserv", "USERLIST ADD");
   drop_help_by_type("chanserv", "USERLIST VIEW");
   return 1;
}

int command_userlist_view(Services_User *client, User *sender, cmd_message *message) {
   Reg_Channel *regchannel;
   char response[IRCLINE_MAX];
   Channel_Access *access;
   if(message && *message_get_target(message) == '#') {
      regchannel = get_reg_channel(message_get_target(message));
      if(regchannel) {
         if(regchannel->access != NULL) {
            access = regchannel->access;
            while(access != NULL) {
               sprintf(response, "%-*s%d", NICK_MAX, access->nick, access->level);
               plain_message_user(client, sender, response);
               access = access->next;
            }

         }
         else
         {
            message_user(client, sender, "COMMAND_USERLIST_EMPTY", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_CHANNEL_NOT_REGISTERED", 1,  message_get_target(message));
      }
   }
   return 1;
}


int command_userlist_add(Services_User *client, User *sender, cmd_message *message) {
   char *username = 0;
   char *input_level = 0;
   int level = 0;
   int level_user = 0;
   int access;
   Reg_Channel *regchannel;
   Reg_User *user;
   if(message && *message_get_target(message) == '#') {
      username = get_token(1, message_get(message), " ");
      input_level = get_token(2, message_get(message), " ");
      if(input_level) {
         level = atoi(input_level);
      }
      if(level <= 0 || level > ACCESS_MAX) {
         message_user(client, sender, "COMMAND_USERLIST_ACCESS_LIMIT", 0);
      }
      else if(username && strlen(username) <= NICK_MAX) {
         user = get_reg_user(username);
         if(!user) {
            message_user(client, sender, "COMMAND_NICKNAME_NOT_REGISTERED", 1, username);
         }
         else
         {
            regchannel = get_reg_channel(message_get_target(message));
            if(!regchannel) {
               message_user(client, sender, "COMMAND_CHANNEL_NOT_REGISTERED", 1, message_get_target(message));
            }
            else
            {
               level_user = get_channel_access(regchannel, user->user);
               access = get_channel_access(regchannel, sender->nick);
               if(!user_is_identified(sender) || access < regchannel->levels.can_access) {
                  message_user(client, sender, "COMMAND_USERLIST_NO_ACCESS", 1, regchannel->name);
               }
               else if(level_user > 0) {
                  message_user(client, sender, "COMMAND_USERLIST_ALREADY_ADDED", 3, username, regchannel->name, access);
               }
               else if(user->dummy == 1) {
                  message_user(client, sender, "COMMAND_USERLIST_DUMMY_ACCOUNT", 1, username);
               }
               else if(reg_channel_add_user(regchannel, username, level)) {
                  message_user(client, sender, "COMMAND_USERLIST_SUCCESSFUL_ADD", 3, username, message_get_target(message), input_level);
               }
            }
         }
      }
      else
      {
         if(username) {
            message_user(client, sender, "COMMAND_INVALID_NICKNAME", 0);
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_NICKNAME", 0);
         }
      }
   }
   else
   {
      help_userlist_add(client, sender, message);
   }
   if(username) {
      free(username);
   }
   if(input_level) {
      free(input_level);
   }
   return 1;
}

int help_userlist_add(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_USERLIST_ADD_SYNTAX", 0);
      message_user(client, sender, "HELP_USERLIST_ADD_DESCRIBE", 0);
   }
   return 1;
}

int help_userlist_view(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_USERLIST_VIEW_SYNTAX", 0);
      message_user(client, sender, "HELP_USERLIST_VIEW_DESCRIBE", 0);
   }
   return 1;
}