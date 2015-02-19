#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "SVS_Regular"
#define DESCRIPTION "Standard message handling routine for clients."
#include "../headers/extensions.h"
int enable_forwarding;
int svs_main(Services_User *svsclient,  cmd_message *msg_struct);

int event_ctcps(void **args) {
   char *clientname;
   char *target;
   char *sender;
   char *message;
   Services_User *client;
   if(args) {
      clientname = args[0];
      target = args[1];
      sender = args[2];
      message = args[3];
      if(message) {
         client = get_svsclient(target);
         if(client && *message != '\0' &&case_compare(client->routine_name, "SVS_Regular")) {
            if(case_compare(message, "VERSION\x01")) {
               ctcpreply_user(client, get_user(sender), "VERSION Ezri " EZRI_VERSION " [www.arloria.net | www.team-ezri.org]");
            }
            else if(case_compare_length(message, "PING", 4)) {
               ctcpreply_user(client, get_user(sender), "PING");
            }
         }
      }
   }
   return 1;
}

int extension_load(void) {
   void *result;
   int *value = 0;
   get_config("ChanServ::forwarding", &result, RETURN_INT);
   value = result;
   if(value) {
      enable_forwarding = *value;
   }
   add_event("svsclient_ctcp", event_ctcps);
   add_extension_routine(get_extension(EXTENSION_NAME), svs_main);
   return 1;
}

int extension_unload(void) {
   return 1;
}

int svs_main(Services_User *svsclient, cmd_message *msg_struct) {
   svs_commandhook *command = 0;
   User *user = 0;
   svs_help *help;
   Services_User *temp;
   const char *message = message_get(msg_struct);
   char *target;
   size_t length = 0;
   target = message_get_real_target(msg_struct);
   if(*message == '\01') {
      fire_event("svsclient_ctcp", 4, svsclient, target, message_get_sender(msg_struct), message + 1);
      return 1;
   }
   if(*target == '#' && svsclient_triggered(svsclient, message_get_beginning(msg_struct))) {
      message_seek(msg_struct, strlen(svsclient->trigger));
   }
   else if(enable_forwarding) {
      temp = get_svsclient_by_trigger(message);
      if(temp) {
         svsclient = temp;
         message_seek(msg_struct, strlen(svsclient->trigger));
      }
   }
   if(*target != '#' || svsclient_triggered(svsclient, message_get_beginning(msg_struct))) {
      if(*message_get(msg_struct) == ' ') {
         length = skip_chars((char *)message_get(msg_struct), " ") - message_get(msg_struct);
         if(length > 0) {
            message_seek(msg_struct, length);
         }
      }
      if(*message_get_implied_target(msg_struct) == '\0') {
         message_imply_target(msg_struct);
      }
      command = find_command(svsclient, message_get(msg_struct));
      if(command) {
         user = command->restrictions(message_get_sender(msg_struct));
         if(user) {
            message_seek(msg_struct, strlen(command->input) + 1);
            if(*message_get_implied_target(msg_struct) != '#') {
               message_imply_target(msg_struct);
            }
            command->function(svsclient, user, msg_struct);
         }
         else
         {
            message_user(svsclient, get_user(message_get_sender(msg_struct)), "COMMAND_NO_ACCESS", 0);
         }
         return 1;
      }
      else 
      {
         help = get_help_list(svsclient, message_get(msg_struct));
         if(help) {
            command = get_command(svsclient, "HELP");
            if(command) {
               user = get_user(message_get_sender(msg_struct));
               command->function(svsclient, user, msg_struct);
            }
         }
         else if(*message_get_real_target(msg_struct) != '#') {
            message_user(svsclient, get_user(message_get_sender(msg_struct)), "COMMAND_NOT_FOUND", 2, "/msg", svsclient->nick);
         }
      }
   }
   return 0;

}
