#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Help"
#define DESCRIPTION "View help about a command."
#include "../headers/extensions.h"

int command_help(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command(NULL, "HELP", command_help, get_user) == 1) {
      return 1;
   }
   return 0;
}

int extension_unload(void) {
   drop_command(NULL, "HELP");
   return 1;
}
/*
   Remember to convert these to language strings
*/
int command_help(Services_User *client, User *sender, cmd_message *message) {
   svs_help *help;
   char response[IRCLINE_MAX];
   int helped = 0;
   if(!message || *message_get(message) == '\0' || case_compare(message_get(message),"help")) {
      if(client->help) {
         help = client->help;
         while(help) {
            if(case_compare(help->list, "main")) {
               sprintf(response, "%-32s%-.250s", help->helpname, help->brief);
               plain_message_user(client, sender, response);
               helped = 1;
            }
            help = help->next;
         }
      }
   }
   else
   {
      help = get_help(client, message_get(message));
      if(help) {
         /* Add the help summary as the first line here */
         help->function(client, sender, message);
         helped = 1;
      }
      help = client->help;
      while(help) {
         if(help->list && case_compare(help->list, message_get(message))) {
            message_user(client, sender, "COMMAND_HELP_BEGIN_CATEGORY", 1, message_get(message));
            sprintf(response, "%-s%-.250s", help->helpname, help->brief);
            plain_message_user(client, sender, response);
            helped = 1;
         }
         help = help->next;
      }
   }
   if(helped == 0) {
      if(client->help) {
         message_user(client, sender, "HELP_NOT_AVAILABLE", 1, message_get(message));
      }
      else
      {
         message_user(client, sender, "HELP_NO_CLIENT_HELP", 1, client->nick);
      }
   }
   return 1;
}