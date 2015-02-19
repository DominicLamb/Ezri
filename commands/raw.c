#define EXTENSION_NAME "Raw"
#define DESCRIPTION "Send raw strings to the server."
#include "../headers/extensions.h"

int command_raw(Services_User *client, User *sender, cmd_message *message);
int help_raw(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("OperServ", "RAW", command_raw, is_ircop) != 1) {
      return 0;
   }
   wrapper_add_help("OperServ", "RAW", "main", "Send an unchecked message to the server.", help_raw);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("OperServ", "RAW");
   drop_help_by_type("OperServ", "RAW");
   return 1;
}

int command_raw(Services_User *client, User *sender, cmd_message *message) {
   if(message != 0) {
      remote_server->send(remote_server->Socket, message_get(message));
      plain_message_user(client, sender, "Command sent.");
   }
   return 1;
}

int help_raw(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      plain_message_user(client, sender, "RAW [string]");
      plain_message_user(client, sender, "Sends a raw message to the server. Note that Ezri does not do any checking on the message sent, and a malformed message could have negative results for your network.");
      plain_message_user(client, sender, "It is recommended that you only use this extension in a development environment. Do not use this in a production environment without a very good reason.");
   }
   return 1;
}