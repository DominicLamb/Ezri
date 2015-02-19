#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Say"
#define DESCRIPTION "Send messages through the service bots."
#include "../headers/extensions.h"

int command_say(Services_User *client, User *sender, cmd_message *message);
int help_say(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command(NULL, "SAY", command_say, is_ircop) != 1) {
      return 0;
   }
   wrapper_add_help(NULL, "SAY", "main", "Send a message through the client.", help_say);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type(NULL, "SAY");
   drop_help_by_type(NULL, "SAY");
   return 1;
}

int command_say(Services_User *client, User *sender, cmd_message *message) {
   char *target = 0;
   const char *msgptr;
   /* Satisfies compiler */
   msgptr = message_get(message);

   if(msgptr && *msgptr != '\0' && (is_ircop(sender->nick) || case_compare(client->type, "ChanServ"))) {
      if(*message_get_implied_target(message) != '\0') {
         target = get_token(1, message_get_implied_target(message), "");
      }
      else
      {
         if(is_ircop(sender->nick) && *msgptr == '-') {
            msgptr++;
            target = get_token(1, msgptr, " ");
            if(!target || !is_user(target)) {
                  free(target);
                  target = 0;
            }
            else
            {
               msgptr += strlen(target) + 1;
            }
         }
         if(!target) {
            if(*message_get_target(message) == '#') {
              target = get_token(1, message_get_target(message), "");
            }
         }
      }
      if(target) {
         if(*msgptr != '\0' && strlen(msgptr) > 1) {
            remote_server->privmsg(client->nick, target, msgptr);
         }
         else
         {
            message_user(client, sender, "COMMAND_SAY_NO_MESSAGE", 0);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_SAY_NO_TARGET", 0);
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_SAY_NO_MESSAGE", 0);
   }
   if(target) {
      free(target);
   }
   return 1;
}

int help_say(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_SAY_SYNTAX", 0);
      message_user(client, sender, "HELP_SAY_DESCRIBE", 0);
   }
   return 1;
}