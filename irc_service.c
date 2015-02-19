#include "headers/main.h"
#include "headers/users.h"
#include "headers/sockets.h"
#include "headers/servers.h"
#include "headers/channels.h"
#include "headers/commands.h"
#include "headers/events.h"
#include "headers/config.h"
#include "headers/database.h"
#include "headers/language.h"

static void configure_logging() {
   void *config;
   char path[PATH_MAX];
   logging = 0;
   create_path(path, "logs", "log.txt", 0);
   logfile = fopen(path, "a");
   if(!logfile) {
      printf("Unable to open or create log file at %s", path);
   }
   else
   {
      get_config("core::log_level", &config, RETURN_CHAR);
      if(config) {
         if(case_compare(config, "DEBUG_CORE")) {
            logging = LOG_DEBUG_CORE;
         }
         else if(case_compare(config, "DEBUG_PROTOCOL")) {
            logging = LOG_DEBUG_PROTOCOL;
         }
         else if(case_compare(config, "NOTICE")) {
            logging = LOG_NOTICE;
         }
         else if(case_compare(config, "WARNING")) {
            logging = LOG_WARNING;
         }
         else if(case_compare(config, "CRITICAL")) {
            logging = LOG_CRITICAL;
         }
         if(logging) {
            log_message("Starting Ezri " EZRI_VERSION, LOG_CRITICAL);
         }
      }
   }
}
static int load_svsclients() {
   void *config;
   Services_User *client;
   int modstatus;
   int i;
   get_config("svsclient", &config, RETURN_BLOCK);
   for(i = 0; config; i++) {
      client = load_svsuser(get_key_value(config, "nickname"), get_key_value(config, "username"), get_key_value(config, "hostname"), get_key_value(config, "realname"), get_key_value(config, "type"));
      if(client) {
         client_set_trigger(client, get_key_value(config, "trigger"));
         if(get_key_value(config, "routine") && !get_extension(get_key_value(config, "routine"))) {
            modstatus = extension_loadfile("clients", get_key_value(config, "routine"));
            if(modstatus == FILE_LOADED) {
               client_switch_routine(client, get_key_value(config, "routine"));
            }
         }
         else
         {
            client_switch_routine(client, get_key_value(config, "routine"));
         }
      }
      config = configblock_get_next_name(config);
   }
   return i;
}

static int load_core() {
   void *config;
   struct key_value_pair *key;
   char path[PATH_MAX];
   char *directory;
   int modstatus = FILE_LOADED;
   get_config("core::languages", &config, RETURN_BLOCK);
   key = get_key_pair((configblock *)config, "load");
   if(key) {
      while(key) {
         load_language(key->value);
         key = get_key_pair_next(key, "load");
      }
   }
   else
   {
      log_message("No default language specified, assuming English.", LOG_NOTICE);
      load_language("en");
   }
   get_config("core::database::load", &config, RETURN_CHAR);
   if(config) {
      modstatus = extension_loadfile("databases", config);
      if(modstatus == FILE_LOADED) {
         database = get_extension_data(config, "db_format");
      }
      else
      {
         log_message("Error loading %s database driver. A database driver is required in order to launch Ezri.", LOG_CRITICAL, config);
         return 0;
      }
   }
   else
   {
      log_message("No core::database::load entries detected. Please add a database format to your configuration file.", LOG_CRITICAL);
      return 0;
   }
   get_config("core::extensions", &config, RETURN_BLOCK);
   key = get_key_pair(config, "load");
   while(key != 0) {
      if(strstr(key->value, DIRECTORY_SEPARATOR)) {
         directory = get_token(1, key->value, DIRECTORY_SEPARATOR);
         if(directory) {
            modstatus = extension_loadfile(directory, key->value + strlen(directory) + 1);
            free(directory);
         }
      }
      else
      {
         modstatus = extension_loadfile("commands", key->value);
      }
      if(modstatus != FILE_LOADED) {
         create_path(path, "commands", key->value, 1);
      }
      key = get_key_pair_next(key, "load");
   }
   log_message("Core loaded.", LOG_DEBUG_CORE);
   return 1;
}

static int load_connect() {
   void *result = 0;
   int module = 0;
   get_config("connect::options::numeric", &result, RETURN_INT);
   if(result) {
      ezri->numeric = *(int *)result;
   }
   get_config("connect::name", &result, RETURN_CHAR);
   if(result) {
      strncpy_safe(ezri->name, (char *)result, HOST_MAX);
   }
   if(*ezri->name == '\0') {
      log_message("Please specify a server to connect to in the connect block for ezri.conf.", LOG_CRITICAL);
      return 0;
   }
   get_config("connect::describe", &result, RETURN_CHAR);
   if(result) {
      strncpy_safe(ezri->description, (char *)result, 256);
   }
   if(*ezri->description == '\0') {
      log_message("Please specify a server description for Ezri in the connect block for ezri.conf.", LOG_CRITICAL);
      return 0;
   }
   get_config("connect::ircd", &result, RETURN_CHAR);
   if(result) {
      module = extension_loadfile("ircd", (char *)result);
      if(module != FILE_LOADED) {
         log_message("IRCd extension %s could not be loaded. An IRCd extension is required in order to launch Ezri.", LOG_CRITICAL, (char *)result);
         return 0;
      }
      remote_server = get_extension_data((char *)result, "protocol");
   }
   else
   {
      log_message("No connect::ircd entry detected. Please add an IRCd protocol to your configuration file.", LOG_CRITICAL);
      return 0;
   }
   get_config("connect::password", &result, RETURN_CHAR);
   if(result) {
      strncpy_safe(ezri->password, result, 64);
   }
   if(*ezri->password == '\0') {
      log_message("No connect::password entry detected. The connect block requires a password in order to link with your server, please add this information to ezri.conf", LOG_CRITICAL);
      return 0;
   }
   get_config("connect::port", &result, RETURN_INT);
   /* Valid port range */
   if(result) {
      ezri->port = *(int *)result;
      if(ezri->port < 1 || ezri->port > 65535) {
         ezri->port = 0;
      }
   }
   if(ezri->port == 0) {
     log_message("Missing or invalid port used in ezri.conf. Please set connect::port to a value between 0 and 65535.", LOG_CRITICAL);
     return 0;
   }
   get_config("connect::server", &result, RETURN_CHAR);
   if(result) {
      strncpy_safe(ezri->link, result, HOST_MAX);
   }
   if(*ezri->link == '\0') {
      log_message("No connect::server entry detected. Please specify a remote server to connect to in the connect block of ezri.conf", LOG_CRITICAL);
      return 0;
   }
   if(remote_server == 0) {
      log_message("The IRCd extension did not specify a set of commands to use. This could be a fault with the IRCd extension you have specified.", LOG_CRITICAL);
      return 0;
   }
   socket_init();
   remote_server->Socket = make_connection(TYPE_IPV4, ezri->link, (unsigned short)ezri->port);
   if(!remote_server->Socket) {
      log_message("Unable to establish a connection to %s on port %d", LOG_CRITICAL, ezri->link, ezri->port);
      return 0;
   }
   remote_server->init(ezri->name, ezri->password, ezri->description);
   return 1;
}

int main() {
   char buffer[IRCLINE_MAX] = "\0";
   int alive = 1;
   time_t time_now = time(NULL);
   debug = 1;
   logging = LOG_CRITICAL;
   if(!config_load()) {
      char path[PATH_MAX];
      create_path(path, NULL, "ezri.conf", 0);
      log_message("%s was not found. Please put ezri.conf in this path to start Ezri.\n", LOG_CRITICAL, path);
      return 1;
   }
   ezri = malloc(sizeof(Ezri));
   if(ezri == 0) {
      /*
         If we're out of memory, we aren't doing ourselves any favours by calling log_message
      */
      puts("Unable to allocate memory to start Ezri (Out of memory?)");
      return 1;
   }
   configure_logging();
   memset(ezri, '\0', sizeof(Ezri));
   srand((unsigned int)time(NULL));

   add_event("server_synch", introduce_users);

   if(!load_connect() || !load_svsclients() || !load_core()) {
      return 1;
   }
   while(alive != -1) {
      alive = remote_server->read(buffer, remote_server->Socket, IRCLINE_MAX - 1);
      while(*buffer != '\0') {
         if(debug) {
            log_message("IN: %s", LOG_DEBUG_CORE, buffer);
         }
         remote_server->main(buffer);
         alive = remote_server->read(buffer, remote_server->Socket, IRCLINE_MAX - 1);
         if(time_now != time(NULL)) {
            time_now = time(NULL);
            check_timers(time_now);
         }
         if(logging & LOG_PENDING) {
            fflush(logfile);
            logging -= LOG_PENDING;
         }
      }
      check_timers(time_now);
      fire_event("services_shutdown", errno);
   }
   end_all_timers(NULL);
   return 0;
}
