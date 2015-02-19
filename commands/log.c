#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Log"
#define DESCRIPTION "Relay log messages to a channel."
#include "../headers/extensions.h"

Reg_Channel *channel;
Services_User *client;
int log_level;

int event_log(void **args);
int logclient_join(void **args) {
   /* 
      Remember, we'll be firing this for each server
      Compare to ezri->name at some point. Simple fix.
   */
   remote_server->join(client->nick, client->nick, channel->name, 1);
   return 1;
}

int extension_load(void) {
   void *config;
   int returnvalue = 1;
   get_config("Log::Channel::Name", &config, RETURN_CHAR);
   if(config) {
      channel = get_reg_channel((char *)config);
      if(!channel) {
         log_message("Log relayer was unable to load, channel specified at Log::Channel has not been registered.", LOG_CRITICAL);
         returnvalue = 0;
      }
   }
   else
   {
      log_message("Log relayer was unable to load, no channel specified at Log::Channel::Name", LOG_CRITICAL);
      returnvalue = 0;
   }
   get_config("Log::Client", &config, RETURN_CHAR);
   if(returnvalue) {
      if(config) {
         client = get_svsclient((char *)config);
         if(client) {
            get_config("Log::Channel::Join", &config, RETURN_INT);
         }
         else
         {
            log_message("Log relayer was unable to load, client specified at Log::Client is not an Ezri Services client", LOG_CRITICAL);
            returnvalue = 0;
         }
      }
      else
      {
         log_message("Log relayer was unable to load, no services client specified at Log::Client", LOG_CRITICAL);
         returnvalue = 0;
      }
   }
   add_event("log_message", event_log);
   add_event("server_synch", logclient_join);
   return returnvalue;
}

int extension_unload(void) {
   if(channel) {
      remote_server->part(client->nick, channel->name, "Extension unloaded");
   }
   drop_event("log_message", event_log);
   drop_event("server_synch", logclient_join);
   return 1;
}

int event_log(void **args) {
   char *message;
   int level;
   if(args && remote_server->synching == 0) {
      message = args[0];
      if(args[1] && ~(*(int *)(args[1])) & LOG_NO_RELAY) {
         level = *(int *)(args[1]);
         plain_message_channel(client, channel, message);
      }
   }
   return 1;
}