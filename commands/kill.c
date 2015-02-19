#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "OS_Kill"
#define DESCRIPTION "Allows opers to disconnect users from the network."
#include "../headers/extensions.h"

int command_kill(Services_User *client, User *sender, cmd_message *message);

int help_kill(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("OperServ", "KILL", command_kill, is_ircop) != 1) {
      return 0;
   }
   wrapper_add_help("OperServ", "KILL", "main", "Terminate a user's IRC session.", help_kill);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("Operserv", "KILL");
   drop_help_by_type("OperServ", "KILL");
   return 1;
}

int command_kill(Services_User *client, User *sender, cmd_message *message) {
   char *username;
   char *kill;
   char kill_message[513];
   username = get_token(1, message_get(message), " ");
   kill = get_token_remainder(2, message_get(message), " ");
   if(username) {
      if(!is_user(username)) {
         message_user(client, sender, "COMMAND_USER_OFFLINE", 1, username);
      }
      else if(kill) {
         if(case_compare(sender->nick, username)) {
            snprintf(kill_message, 513, "Self-kill by %s: %s", sender->nick, kill);
         }
         else
         {
            snprintf(kill_message, 513, "Kill by %s: %s", sender->nick, kill);
         }
      }
      else
      {
         if(case_compare(sender->nick, username)) {
            snprintf(kill_message, 513, "Self-kill by %s", sender->nick);
         }
         else
         {
            snprintf(kill_message, 513, "Killed by %s", sender->nick);
         }
      }
      if(*kill_message != 0) {
         remote_server->kill(client->nick, username, kill_message, 0);
      }
   }
   else
   {
      help_kill(client, sender, message);
   }
   if(username) {
      free(username);
   }
   if(kill) {
      free(kill);
   }
   return 1;
}

int help_kill(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_KILL_SYNTAX", 0);
      message_user(client, sender, "HELP_KILL_DESCRIBE", 0);
   }
   return 1;
}