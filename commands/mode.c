#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "OS_Modes"
#define DESCRIPTION "Sets user and channel modes through OperServ."
#include "../headers/extensions.h"

int command_mode(Services_User *client, User *sender, cmd_message *message);
int help_mode(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("OperServ", "MODE", command_mode, is_ircop) != 1) {
      return 0;
   }
   wrapper_add_help("OperServ", "MODE", "main", "Change the modes of a user or channel", help_mode);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("OperServ", "MODE");
   drop_help_by_type("OperServ", "MODE");
   return 1;
}

int command_mode(Services_User *client, User *sender, cmd_message *message) {
   char *target;
   char *modes;
   char *modestring;
   const char * const msgptr = message_get(message);
   if(!message) {
      return 1;
   }
   if(*message_get_target(message) != '#') {
      target = get_token(1, msgptr, " ");
      modes = get_token(2, msgptr, " ");
      modestring = get_token_remainder(3, msgptr, " ");
   }
   else
   {
      target = get_token(1, msgptr, " ");
      modes = get_token(1, msgptr, " ");
      modestring = get_token_remainder(2, msgptr, " ");
   }
   if(target && modes) {
      if(*target == '#') {
         if(is_channel(target)) {
            remote_server->modes(client->nick, target, modes, modestring);
            message_user(client, sender, "COMMAND_MODE_SET", 2, target, modes);
         }
         else
         {
            message_user(client, sender, "COMMAND_EMPTY_CHANNEL", 1, target);
         }
      }
      else
      {
         if(is_user(target)) {
            remote_server->modes(client->nick, target, modes, modestring);
            message_user(client, sender, "COMMAND_MODE_SET", 2, target, modes);
         }
         else
         {
            message_user(client, sender, "COMMAND_USER_OFFLINE", 1, target);
         }
      }
   }
   else if(target) {
      message_user(client, sender, "COMMAND_MODE_NO_MODES", 1, target);
   }
   else
   {
    // help_mode(client, sender, message);
   }
   if(target) {
      free(target);
   }
   if(modes) {
      free(modes);
   }
   if(modestring) {
      free(modestring);
   }
   return 1;
}

int help_mode(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_MODE_SYNTAX", 0);
      message_user(client, sender, "HELP_MODE_DESCRIBE", 0);
   }
   return 1;
}