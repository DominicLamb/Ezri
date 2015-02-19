#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "OS_Modload"
#define DESCRIPTION "Services opers can load modules into the services at runtime."
#include "../headers/extensions.h"

int command_load(Services_User *client, User *sender, cmd_message *message);
int command_unload(Services_User *client, User *sender, cmd_message *message);
int command_extensions(Services_User *client, User *sender, cmd_message *message);

int help_load(Services_User *client, User *sender, cmd_message *message);
int help_unload(Services_User *client, User *sender, cmd_message *message);
int help_extensions(Services_User *client, User *sender, cmd_message *message);

int extension_load() {
   int loaded = 1;
   if(wrapper_add_command("operserv", "LOAD", command_load, is_ircop) != 1) {
      loaded = 0;
   }
   else if(wrapper_add_command("operserv", "UNLOAD", command_unload, is_ircop) != 1) {
      loaded = 0;
   }
   else if(wrapper_add_command("operserv", "EXTENSIONS", command_unload, is_ircop) != 1) {
      loaded = 0;
   }
   if(loaded) {
      wrapper_add_help("operserv", "LOAD", "", "Load an extension", help_load);
      wrapper_add_help("operserv", "UNLOAD", "", "Unload an extension", help_unload);
      wrapper_add_help("operserv", "EXTENSIONS", "", "List all extensions", help_extensions);
   }
   return loaded;
}

int extension_unload() {
   drop_command_by_type("operserv", "LOAD");
   drop_command_by_type("operserv", "UNLOAD");
   drop_command_by_type("operserv", "EXTENSIONS");
   drop_help_by_type("operserv", "LOAD");
   drop_help_by_type("operserv", "UNLOAD");
   drop_help_by_type("operserv", "EXTENSIONS");
   return 1;
}

int command_load(Services_User *client, User *sender, cmd_message *message) {
   int modloaded;
   Extension *module;
   if(!sender) {
      return 0;
   }
   if(*message_get(message) == '\0') {
      message_user(client, sender, "COMMAND_NO_FILENAME", 0);
      return 0;
   }
   module = get_extension_by_filename("commands",message_get(message));
   if(!module) {
      modloaded = extension_loadfile("commands", message_get(message));
      switch(modloaded) {
         case FILE_INVALID_FILENAME:
            message_user(client, sender, "COMMAND_MODLOAD_BAD_FILENAME", 1, message_get(message));
         break;
         case FILE_NOT_FOUND:
            message_user(client, sender, "COMMAND_MODLOAD_FILE_MISSING", 1, message_get(message));
         break;
         case FILE_ACCESS_DENIED:
            message_user(client, sender, "COMMAND_MODLOAD_NO_PERMISSION", 1, message_get(message));
         break;
         case FILE_LOADED_NOT_VALID:
            message_user(client, sender, "COMMAND_MODLOAD_FILE_INVALID", 1, message_get(message));
         break;
         case FILE_SAYS_NO:
            message_user(client, sender, "COMMAND_MODLOAD_FILE_CANCEL", 1, message_get(message));
         break;
         case FILE_LOADED:
            message_user(client, sender, "COMMAND_MODLOAD_FILE_SUCCESS", 1, message_get(message));
         break;
         default:
            message_user(client, sender, "COMMAND_MODLOAD_FILE_UNKNOWN", 1, message_get(message));
         break;
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_MODLOAD_ALREADY_LOADED", 1, message_get(message));
   }
   return 1;
}

int command_unload(Services_User *client, User *sender, cmd_message *message) {
   Extension *module = 0;
   if(!sender) {
      return 0;
   }
   if(*message_get(message) == '\0') {
      message_user(client, sender, "COMMAND_NO_FILENAME", 0);
      return 0;
   }
   module = get_extension(message_get(message));
   if(!module) {
      module = get_extension_by_filename("commands", message_get(message));
   }
   if(module) {
      if(case_compare(module->name, EXTENSION_NAME)) {
         message_user(client, sender, "COMMAND_MODLOAD_SELF_UNLOAD", 1, EXTENSION_NAME);
      }
      else if(unload_extension(module->name)) {
         message_user(client, sender, "COMMAND_MODLOAD_UNLOAD_SUCCESS", 1, message_get(message));
      }
      else
      {
         message_user(client, sender, "COMMAND_MODLOAD_UNLOAD_FAILURE", 1, module->name);
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_MODLOAD_NOT_LOADED", 1, message_get(message));
   }
   return 1;
}

int command_extensions(Services_User *client, User *sender, cmd_message *message) {
   Extension *p_extensions = extensions;
   char response[IRCLINE_MAX];
   int show_all = 0;
   int i = 0;
   if(case_compare(message_get(message),"ALL")) {
      show_all = 1;
   }
   while(p_extensions != 0) {
      if(p_extensions->name != 0 && p_extensions->author != 0 && (strcmp(p_extensions->author, OFFICIAL_EXTENSION) != 0 || show_all == 1) && p_extensions->description != 0) {
         sprintf(response, "Extension: %s [%s]: %s", p_extensions->name, p_extensions->author, p_extensions->description);
         plain_message_user(client, sender, response);
         i++;
      }
      p_extensions = p_extensions->next;
   }
   if(i == 0) {
      message_user(client, sender, "COMMAND_MODLOAD_NO_EXTENSIONS", 0);
   }
   return 1;
}

int help_load(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_LOAD_LOAD_SYNTAX", 0);
      message_user(client, sender, "HELP_LOAD_LOAD_DESCRIBE", 0);
   }
   return 1;
}

int help_unload(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_LOAD_UNLOAD_SYNTAX", 0);
      message_user(client, sender, "HELP_LOAD_UNLOAD_DESCRIBE", 0);
   }
   return 1;
}
int help_extensions(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_LOAD_EXTENSIONS_SYNTAX", 0);
      message_user(client, sender, "HELP_LOAD_EXTENSIONS_DESCRIBE", 0);
   }
   return 1;
}