#define COMPILE_EXTENSION
#include "main.h"
#include "strings.h"
#include "users.h"
#include "channels.h"
#include "commands.h"
#include "events.h"
#include "sockets.h"
#include "servers.h"
#include "config.h"
#include "database.h"
#ifndef AUTHOR
   #define AUTHOR "Unknown"
#endif
#ifndef VERSION
   #define VERSION "0.0"
#endif
#ifndef DESCRIPTION
   #define DESCRIPTION "None given"
#endif

IMPORT int extension_load(void);
IMPORT int extension_unload(void);
IMPORT char *extension_version(void);
IMPORT Extension_Info extension_info(void);
char *get_token(unsigned int token, const char * const source, const char * const delimiter);
char *get_token_remainder(unsigned int token, const char * const source, const char * const delimiter);
char *str_replace(const char * const search, const char * const replace, const char *subject);

char *extension_version(void) {
  return EZRI_VERSION EZRI_RELEASE;
}

Extension_Info extension_info(void) {
  Extension_Info extinfo;
  strncpy_safe(extinfo.name, EXTENSION_NAME, 64);
  strncpy_safe(extinfo.author, AUTHOR, 64);
  strncpy_safe(extinfo.extension_version, VERSION, 12);
  strncpy_safe(extinfo.description, DESCRIPTION, 150);
  return extinfo;
}

int wrapper_add_command(const char * const clienttype, const char * const commandname, int (*function)(Services_User *client, User *user, cmd_message *message), User *(restrictions)(const char * const username)) {
   svs_commandhook *command;
   command = make_command(commandname, function);
   if(command) {
      command_restrict(command, restrictions);
      if(add_command_by_type(clienttype, command)) {
         return 1;
      }
      else
      {
         log_message("Unable to load %s command - Could not attach to a client of type \"%s\"", LOG_WARNING, commandname, clienttype);
         return 0;
      }
   }
   else
   {
      log_message("Warning: Unable to create %s command as requested from " EXTENSION_NAME " extension.", LOG_CRITICAL, commandname);
      return -1;
   }
}

int wrapper_add_help(const char * const clienttype, const char * const name, const char * const list, const char * const brief, void (*function)(Services_User *client, User *sender, cmd_message *message)) {
   svs_help *help = create_help(name, list, brief, function);
   if(help) {
      if(svs_add_help_by_type(clienttype, help)) {
         return 1;
      }
      else if(clienttype) {
         log_message("Unable to load %s help topic- Could not attach to a client of type \"%s\"", LOG_WARNING, name, clienttype);
      }
      else
      {
         log_message("Unable to load %s help topic- Could not attach to all services clients.", LOG_WARNING, name, clienttype);
      }

   }
   else
   {
      log_message("Warning: Unable to create %s help topic as requested from " EXTENSION_NAME " extension.", LOG_CRITICAL, name);
      return -1;
   }
   return 0;
}