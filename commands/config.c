#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "OS_Config"
#define DESCRIPTION "Commands for manipulating the configuration file."
#include "../headers/extensions.h"

int command_rehash(Services_User *client, User *sender, cmd_message *message);
int command_getsetting(Services_User *client, User *sender, cmd_message *message);

int help_rehash(Services_User *client, User *sender, cmd_message *message);
int help_setting(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("operserv", "REHASH", command_rehash, is_ircop) != 1) {
      return 0;
   }
   if(wrapper_add_command("operserv", "SETTING", command_getsetting, is_ircop) != 1) {
      return 0;
   }
   wrapper_add_help("operserv", "REHASH", "main", "Reload the configuration file", help_rehash);
   wrapper_add_help("operserv", "SETTING", "main", "Get the value of configuration settings.", help_setting);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("operserv", "REHASH");
   drop_command_by_type("operserv", "SETTING");
   drop_help_by_type("operserv", "SETTING");
   drop_help_by_type("operserv", "SETTING");
   return 1;
}

int command_rehash(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      config_drop(config_list);
      config_list = 0;
      config_load();
      message_user(client, sender, "COMMAND_CONFIG_REHASHED", 0);
   }
   return 1;
}

int command_getsetting(Services_User *client, User *sender, cmd_message *message) {
   void *config;
   char *config_string;
   if(!stripos(message_get(message), "pass")) {
      message_user(client, sender, "COMMAND_CONFIG_NO_PASSWORDS", 0);
   }
   else
   {
      get_config(message_get(message), &config, RETURN_CHAR);
      config_string = config;
      if(config_string) {
         message_user(client, sender, "COMMAND_CONFIG_SETTING_DISPLAY", 1, config_string);
      }
   }
   return 1;
}

int help_rehash(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_CONFIG_REHASH_SYNTAX", 0);
      message_user(client, sender, "HELP_CONFIG_REHASH_DESCRIBE", 0);
   }
   return 1;
}

int help_setting(Services_User *client, User *sender, cmd_message *message) {
   if(message && message_get(message)) {
      sender = is_ircop(sender->nick);
      if(sender) {
         command_getsetting(client, sender, message);
      }
   }
   else
   {
      message_user(client, sender, "HELP_CONFIG_SETTING_SYNTAX", 0);
      message_user(client, sender, "HELP_CONFIG_SETTING_DESCRIBE", 0);
   }
   return 1;
}