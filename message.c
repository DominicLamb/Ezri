#include "headers/main.h"
#include "headers/system.h"
#include "headers/users.h"
#include "headers/channels.h"
#include "headers/commands.h"
#include "headers/sockets.h"
#include "headers/servers.h"
#include "headers/language.h"
#include "headers/events.h"

void plain_message_user(Services_User *client, User *user, const char * const message) {
   /* Add registered notice/privmsg preference here */
   remote_server->notice(client->nick, user->nick, message);
}

void plain_message_channel(Services_User *client, Reg_Channel *channel, const char * const message) {
   // Figure out how we're going to cleanly do this
   remote_server->privmsg(client->nick, channel->name, message);
}

void plain_message_channel_level(Services_User *client, Reg_Channel *channel, const char * const message, const int level) {
   if(level) {
      // Figure out how we're going to cleanly do this
      remote_server->privmsg(client->nick, channel->name, message);
   }
}

void message_user(Services_User *client, User *user, const char * const message, const int count, ...) {
   char *real_message;
   char *ptr;
   va_list var_args;
   char token[IRCLINE_MAX];
   if(client && user) {
      va_start(var_args, count);
      real_message = parse_language_string("en", message, count, var_args);
      if(real_message && *real_message != '\0') {
         if(strchr(real_message, '\n')) {
            ptr = real_message;
            while(*real_message) {
               real_message += copy_to(token, real_message, '\n', IRCLINE_MAX);
               plain_message_user(client, user, token);
            }
            real_message = ptr;
         }
         else
         {
            plain_message_user(client, user, real_message);
         }
         free(real_message);
      }
   }
}

void message_channel(Services_User *client, Reg_Channel *channel, const char * const message, const int level, const int count, ...) {
   char *real_message;
   char *ptr;
   va_list var_args;
   char token[IRCLINE_MAX];
   if(client && channel && level) {
      va_start(var_args, count);
      real_message = parse_language_string("en", message, count, var_args);
      if(real_message && *real_message != '\0') {
         if(strchr(real_message, '\n')) {
            ptr = real_message;
            while(*real_message) {
               real_message += copy_to(token, real_message, '\n', IRCLINE_MAX);
               plain_message_channel(client, channel, token);
            }
            real_message = ptr;
         }
         else
         {
            plain_message_channel(client, channel, real_message);
         }
         free(real_message);
      }
   }
}

void ctcp_user(Services_User *client, User *user, const char * const message) {
   char ctcp_message[IRCLINE_MAX];
   snprintf(ctcp_message, IRCLINE_MAX, "\x01%s\x01", message);
   remote_server->privmsg(client->nick, user->nick, ctcp_message);
}

void ctcpreply_user(Services_User *client, User *user, const char * const message) {
   char ctcp_message[IRCLINE_MAX];
   snprintf(ctcp_message, IRCLINE_MAX, "\x01%s\x01", message);
   remote_server->notice(client->nick, user->nick, ctcp_message);
}


int extension_loadfile(const char * const directory, const char * const filename) {
   char full_path[PATH_MAX];
   ExtensionFile dll = 0;
   ExtensionFunction function;
   Extension_Info filestruct;
   Extension_Info (*filedata)(void);
   int result = 0;
   if(!filename || !directory || strlen(filename) < 1 || strlen(filename) + strlen(directory) > FILENAME_MAX) {
      return FILE_INVALID_FILENAME;
   }
   if(strpbrk(filename, "\\/~")) {
      return FILE_INVALID_FILENAME;
   }
   create_path(full_path, directory, filename, 1);
   log_message("Loading extension %s", LOG_WARNING, full_path);
   if(*full_path && file_exists(full_path)) {
      dll = extension_openfile(full_path);
      if(dll && extension_error() == 0) {
         #ifdef _WIN32
            filedata = (Extension_Info (__cdecl *)(void))extension_getfunction(dll, "extension_info");
         #else
            filedata = extension_getfunction(dll, "extension_info");
         #endif
         if(extension_error() == 0 && filedata != NULL) {
            filestruct = filedata();
            filestruct.handle = dll;
            if(add_extension(&filestruct, full_path)) {
               function = extension_getfunction(dll, "extension_load");
               if(function()) {
                  result = FILE_LOADED;
               }
               else
               {
                  result = FILE_SAYS_NO;
                  unload_extension(filename);
               }
            }
            else
            {
               result = FILE_SAYS_NO;
            }
            if(result == FILE_SAYS_NO) {
               function = extension_getfunction(dll, "extension_unload");
               if(function) {
                  function();
               }
            }
         }
         else
         {
            result = FILE_LOADED_NOT_VALID;
         }
      }
      else
      {
         result = FILE_SAYS_NO;
      }
   }
   else
   {
      result = FILE_NOT_FOUND;
   }
   if(result != FILE_LOADED) {
      log_message("Error while loading %s: %d [%s]\n", LOG_CRITICAL, full_path, extension_error(), translate_error(result));
      if(dll) {
         extension_closefile(dll);
      }
   }
   return result;
}

int extension_loaded(const char * const name) {
   Extension *p = extensions;
   if (name != 0 && extensions != 0) {
      while(p != 0) {
         if(case_compare(name, p->name)) {
            return 1;
         }
         p = p->next;
      }
   }
   return 0;
}

static void delete_extension(Extension *module) {
   ExtensionFunction function = 0;
   if(module) {
      function = extension_getfunction(module->handle, "extension_unload");
      if(function) {
         function();
      }
      extension_closefile(module->handle);
      free(module);
   }
}

int unload_extension(const char * const name) {
   Extension *prev;
   Extension *p;
   int count = 0;
   if(name) {
      p = extensions;
      prev = p;
      while(p != 0) {
         if(case_compare(p->name, name)) {
            if(p == extensions) {
               p = p->next;
               delete_extension(extensions);
               extensions = p;
               count++;
            }
            else
            {
               prev->next = p->next;
               delete_extension(p);
               count++;
            }
            break;
         }
         prev = p;
         p = p->next;
      }
   }
   return count;
}

Extension *get_extension(const char * const module_name) {
   Extension *p = extensions;
   if(!module_name) {
      return 0;
   }
   while(p != 0) {
      if(case_compare(p->name,module_name)) {
         return p;
      }
      p = p->next;
   }
   return 0;
}

Extension *get_extension_by_filename(const char * const path, const char * const filename) {
   Extension *p = extensions;
   char full_path[PATH_MAX];
   if(!filename) {
      return 0;
   }
   create_path(full_path, path, filename, 1);
   if(*full_path) {
      while(p != 0) {
         if(case_compare(p->filename, full_path)) {
            return p;
         }
         p = p->next;
      }
   }
   return 0;
}

void extension_mark_critical(Extension *extension, const int toggle) {
   if(extension) {
      extension->critical = toggle;
   }
}

void *get_extension_data(const char * const module_name, const char * const data_name) {
   Extension *p = extensions;
   Extension_Data *ptr;
   if(!module_name) {
      return 0;
   }
   while(p != 0) {
      if(case_compare(p->name,module_name)) {
         ptr = p->extension_data;
         while(ptr) {
            if(case_compare(ptr->name, data_name)) {
               return ptr->data;
            }
            ptr = ptr->next;
         }
         break;
      }
      p = p->next;
   }
   return 0;
}

svs_routine *get_extension_function(const char * const module_name) {
   Extension *p = extensions;
   if(!module_name) {
      return 0;
   }
   while(p != 0) {
      if(case_compare(p->name,module_name)) {
         return &p->routine;
      }
      p = p->next;
   }
   return 0;
}

ptrdiff_t message_seek(cmd_message *msg_struct, ptrdiff_t seek) {
   size_t moved = 0;
   if(!msg_struct->message || *(msg_struct->message) == '\0') {
      return 0;
   }
   if(seek > 0) {
      moved = strlen(msg_struct->ptr);
      if(moved >= (size_t)seek) {
         msg_struct->ptr += seek;
      }
      else
      {
         msg_struct->ptr += moved;
      }
   }
   else
   {
      if(msg_struct->ptr - msg_struct->message > seek) {
         msg_struct->ptr -= seek;
      }
      else
      {
         seek = msg_struct->ptr - msg_struct->message;
         message_rewind(msg_struct);
      }
   }
   return seek;
}

const char *message_get(cmd_message *msg) {
   if(msg) {
      return msg->ptr;
   }
   return 0;
}

const char *message_get_beginning(cmd_message *msg) {
   if(msg) {
      return msg->message;
   }
   return 0;
}
void message_rewind(cmd_message *msg) {
   msg->ptr = (const char *)msg->message;
}

char *message_get_real_target(cmd_message *msg) {
   return msg->target;
}

char *message_get_implied_target(cmd_message *msg) {
   return msg->implied_target;
}
char *message_get_target(cmd_message *msg) {
   if(msg->implied_target && *msg->implied_target == '#') {
      return msg->implied_target;
   }
   return msg->target;
}

char *message_get_sender(cmd_message *msg) {
   return msg->source;
}
void message_imply_target(cmd_message *msg_struct) {
   size_t len;
   if(*msg_struct->ptr == '#') {
      len = copy_to(msg_struct->implied_target, msg_struct->ptr, ' ', CHANNEL_MAX);
      if(strchr(msg_struct->implied_target, ',')) {
         *(msg_struct->implied_target) = '\0';
      }
      else
      {
         message_seek(msg_struct, len);
      }
   }
}

static void delete_command(svs_commandhook *command) {
   free(command);
}

/*
   Internal routine to handle the list parsing
   Avoids duplicate code
*/

static void drop_command_(Services_User *client, const char * const command) {
   svs_commandhook *p = client->command;
   svs_commandhook *prev = p;
   while(p != 0) {
      if(case_compare(p->input, command)) {
         if(p == client->command) {
            p = p->next;
            delete_command(client->command);
            client->command = p;
         }
         else
         {
            prev->next = p->next;
            delete_command(p);
         }
         break;
      }
      prev = p;
      p = p->next;
   }
}

void drop_command(Services_User *client, const char * const command) {
   if(client && client->command && command) {
      drop_command_(client, command);
   }
   else
   {
      client = svslist;
      while(client) {
         if(client->command) {
            drop_command_(client, command);
         }
         client = client->next;
      }
   }
}

void drop_command_by_type(const char * const type, const char * const command) {
   svs_commandhook *p;
   Services_User *client = svslist;
   if(type && command) {
      while(client) {
         p = client->command;
         if(!type || *type == '\0' || case_compare(client->type, type)) {
            drop_command_(client, command);
         }
         client = client->next;
      }
   }
}

int add_command(Services_User *client, svs_commandhook *command) {
   svs_commandhook *p;
   if(!command || !client) {
      return 0;
   }
   if(client->command != 0) {
      p = client->command;
      while(p->next != 0) {
         if(case_compare(p->input,command->input)) {
            log_message("Attempted to load duplicate command %s to %s", LOG_WARNING, p->input, client->nick);
            delete_command(command);
            return 0;
         }
         p = p->next;
      }
      p->next = command;
      p = p->next;
   }
   else
   {
      client->command = command;
      p = client->command;
   }
   if(debug) {
      log_message("Added %s command to client %s", LOG_DEBUG_CORE, command->input, client->nick);
   }
   return 1;
}

void command_restrict(svs_commandhook *command, User *(*restriction)(const char * const user)) {
   if(command) {
      command->restrictions = restriction;
   }
}

int add_command_by_type(const char * const type, svs_commandhook *command) {
   svs_commandhook *p;
   Services_User *client = svslist;
   int count = 0;
   int add = 1;
   if(!command) {
      return 0;
   }
   while(client != 0) {
      if(!type || case_compare(client->type, type)) {
         if(client->command) {
            p = client->command;
            while(add && p->next) {
               if(case_compare(p->input,command->input)) {
                  add = 0;
               }
               p = p->next;
            }
            if(add && count == 0) {
               add = 0;
               count++;
               p->next = command;
            }
            else if(add) {
               p->next = malloc(sizeof(svs_commandhook));
               p = p->next;
               memcpy(p, command, sizeof(svs_commandhook));
            }
         }
         else
         {
            if(count == 0) {
               client->command = command;
               count++;
            }
            else
            {
               client->command = malloc(sizeof(svs_commandhook));
               p = client->command;
               if(p) {
                  memcpy(p, command, sizeof(svs_commandhook));
               }
            }
         }
      }
      client = client->next;
      add = 1;
   }
   if(count == 0) {
      delete_command(command);
   }
   return count;
}

int add_command_by_routine(const char * const type, svs_commandhook *command) {
   svs_commandhook *p;
   Services_User *client = svslist;
   int count = 0;
   int add = 1;
   if(!command) {
      return 0;
   }
   while(client != 0) {
      if(!type || case_compare(client->routine_name, type)) {
         if(client->command != 0) {
            p = client->command;
            while(add && p->next != 0) {
               if(case_compare(p->input,command->input)) {
                  add = 0;
               }
               p = p->next;
            }
            if(add && count == 0) {
               add = 0;
               count++;
               p->next = command;
            }
            else if(add) {
               p->next = malloc(sizeof(svs_commandhook));
               p = p->next;
            }
         }
         else
         {
            client->command = malloc(sizeof(svs_commandhook));
            p = client->command;
         }
         if(add && p) {
            memcpy(p, command, sizeof(svs_commandhook));
            count++;
         }
      }
      client = client->next;
      add = 1;
   }
   if(count == 0) {
      delete_command(command);
   }
   return count;
}

static void delete_help(svs_help *help) {
   free(help);
}

void drop_help(Services_User *client, const char * const help) {
   svs_help *prev;
   svs_help *p;
   if(client && client->help && help) {
      p = client->help;
      prev = p;
      while(p != 0) {
         if(case_compare(p->helpname, help)) {
            if(p == client->help) {
               p = p->next;
               delete_help(client->help);
               client->help = p;
            }
            else
            {
               prev->next = p->next;
               delete_help(p);

            }
            break;
         }
         prev = p;
         p = p->next;
      }
   }
}

void drop_help_by_type(const char * const type, const char * const help) {
   Services_User *client = svslist;
   while(client) {
      if(client->help && (!type || *type == '\0' || case_compare(client->type, type))) {
         drop_help(client, help);
      }
      client = client->next;
   }
}

svs_help *create_help(const char * const name, const char * const list, const char * const brief, void (*function)(Services_User *client, User *sender, cmd_message *message)) {
   svs_help *help;
   if(!name || !brief || !function) {
      log_message("Invalid arguments passed to create_help", LOG_NOTICE);
      return 0;
   }
   help = malloc(sizeof(svs_help));
   if(help) {
      memset(help, 0, sizeof(svs_help));
      strncpy_safe(help->helpname, name, HELP_MAX);
      if(list) {
         strncpy_safe(help->list, list, HELP_MAX);
      }
      strncpy_safe(help->brief, brief, 250);
      strtoupper(help->helpname);
      strtoupper(help->list);
      help->function = function;
   }
   else
   {
      log_message("Unable to allocate %d bytes to create new help item", LOG_CRITICAL, sizeof(svs_help));
   }
   return help;
}

svs_help *get_help_list(Services_User *client, const char * const list) {
   svs_help *help;
   if(!list || !client) {
      return 0;
   }
   help = client->help;
   while(help != 0) {
      if(help->list != 0 && case_compare(help->list,list)) {
         break;
      }
      help = help->next;
   }
   return help;
}
svs_help *get_help(Services_User *client, const char * const name) {
   svs_help *help;
   if(!name || !client) {
      return 0;
   }
   help = client->help;
   while(help != 0) {
      if(help->helpname != 0 && case_compare(help->helpname,name)) {
         break;
      }
      help = help->next;
   }
   return help;
}


int svs_add_help(Services_User *client, svs_help *help) {
   svs_help *p = client->help;
   if(!help) {
      return 0;
   }
   if(client) {
      delete_help(help);
      return 0;
   }
   if(client->help != 0) {
      while(p->next != 0) {
         p = p->next;
      }
      p->next = help;
   }
   else
   {
      client->help = help;
   }
   return 1;
}

int svs_add_help_by_type(const char * const type, svs_help *help) {
   svs_help *p;
   Services_User *client = svslist;
   unsigned int count = 0;
   if(!help || !type) {
      return 0;
   }
   while(client != 0) {
      if(case_compare(type, client->type)) {
         if(client->help) {
            p = client->help;
            while(p->next != 0) {
               if(strcmp(help->helpname, p->helpname) == 0 && strcmp(help->list, p->list) == 0) {
                  break;
               }
               p = p->next;
            }
            if(strcmp(help->helpname, p->helpname) != 0 || strcmp(help->list, p->list) != 0) {
               if(count == 0) {
                  p->next = help;
                  count++;
               }
               else
               {
                  p->next = malloc(sizeof(svs_help));
                  p = p->next;
                  if(p) {
                     memcpy(p, help, sizeof(svs_help));
                     count++;
                  }
               }
            }
         }
         else
         {
            if(count == 0) {
               client->help = help;
               count++;
            }
            else
            {
               client->help = malloc(sizeof(svs_help));
               if(client->help) {
                  memcpy(client->help, help, sizeof(svs_help));
                  count++;
               }
            }
         }
      }
      client = client->next;
   }
   if(count == 0) {
      delete_help(help);
   }
   return count;
}

svs_commandhook *make_command(const char * const input, int function(Services_User *client, User *sender, cmd_message *message)) {
   svs_commandhook *command;
   if(!input) {
      return 0;
   }
   command = malloc(sizeof(svs_commandhook));
   if(command) {
      memset(command, 0, sizeof(svs_commandhook));
      command->function = function;
      strncpy_safe(command->input, input, COMMAND_MAX);
      strtoupper(command->input);
   }
   return command;
}

svs_commandhook *find_command(Services_User *client, const char * const message) {
   svs_commandhook *command;
   const char *check;
   command = 0;
   if(client != 0 && client->command != 0) {
      command = client->command;
      while(command != 0) {
         if(command->input) {
            if(stripos(message,command->input) == 0) {
               check = message + strlen(command->input);
               if(*check == '\0' || *check == ' ') {
                  break;
               }
            }
         }
         command = command->next;
      }
   }
   return command;
}

svs_commandhook *get_command(Services_User *client, const char * const message) {
   svs_commandhook *command;
   const char *check;
   command = 0;
   if(client != 0 && client->command != 0) {
      command = client->command;
      while(command != 0) {
         if(command->input) {
            if(case_compare(message,command->input)) {
               check = message + strlen(command->input);
               if(*check == '\0' || *check == ' ') {
                  break;
               }
            }
         }
         command = command->next;
      }
   }
   return command;
}

int process_message(const char * const user, const char * const target, const char * const message) {
   Services_User *svsclient;
   Reg_Channel *channel;
   cmd_message message_struct;
   Channel_Bot *bot;
   memset(&message_struct, 0, sizeof(cmd_message));
   message_struct.date = time(NULL);
   if(target) {
      strncpy_safe(message_struct.target, target, CHANNEL_MAX);
   }
   if(user) {
      strncpy_safe(message_struct.source, user, NICK_MAX);
   }
   if(message) {
      message_struct.message = message;
      message_struct.ptr = message;
   }
   if(*message_struct.target == '#') {
      channel = get_reg_channel(message_struct.target);
      if(channel && channel->channel && channel->channel->bots) {
         bot = channel->channel->bots;
         while(bot) {
            if(bot->botdata && bot->botdata->main_routine) {
               if(bot->botdata->main_routine(bot->botdata, &message_struct)) {
                  break;
               }
            }
            bot = bot->next;
         }
      }
   }
   else if(is_svsclient(message_struct.target) && !is_svsclient(user)) {
      svsclient = get_svsclient(message_struct.target);
      if(svsclient != 0 && svsclient->main_routine != 0) {
         svsclient->main_routine(svsclient, &message_struct);
      }
   }
   return 0;
}

int add_extension(Extension_Info *data, const char * const filename) {
   Extension *p_extension = extensions;
   if(!data || !filename) {
     return 0;
   }
   if(extensions == 0) {
      extensions = malloc(sizeof(Extension));
      p_extension = extensions;
   }
   else
   {
      while(p_extension->next != 0) {
         if(case_compare(data->name, p_extension->name)) {
            return 0;
         }
         p_extension = p_extension->next;
      }
      p_extension->next = malloc(sizeof(Extension));
      p_extension = p_extension->next;
   }
   if(p_extension) {
      memset(p_extension, 0, sizeof(Extension));
      strncpy_safe(p_extension->name, data->name, 64);
      strncpy_safe(p_extension->author, data->author, 64);
      strncpy_safe(p_extension->description, data->description, 150);
      strncpy_safe(p_extension->filename, filename, FILENAME_MAX);
      p_extension->handle = data->handle;
      return 1;
   }
   return 0;
}

int add_extension_data(Extension *extension, const char * const name, void *data) {
   Extension_Data *extdata;
   Extension_Data *ptr;
   if(data &&extension) {
      extdata = malloc(sizeof(Extension_Data));
      if(extdata) {
         strncpy_safe(extdata->name, name, EXTENSION_MAX);
         extdata->data = data;
         ptr = extension->extension_data;
         if(!ptr) {
            extension->extension_data = extdata;
         }
         else
         {
            while(ptr->next) {
               ptr = ptr->next;
            }
            ptr->next = extdata;
         }
      }
   }
   return 0;
}

void add_extension_routine(Extension *extension, svs_routine data) {
   extension->routine = data;
}

void log_message(const char * const message, const int level, ...) {
   struct tm *timestruct;
   char tempstring[LINE_MAX];
   const time_t now = time(NULL);
   va_list args;
   if(message && (debug || level ^ LOG_DEBUG)&& (level | LOG_PENDING) >= (logging | LOG_PENDING)) {
      timestruct = gmtime(&now);
      strftime(tempstring, 20, "%Y-%m-%d %H:%M:%S", timestruct);
      fputc('[', logfile);
      fputs(tempstring, logfile);
      fputc(']', logfile);
      fputc(' ', logfile);
      if(strchr(message, '%')) {
         va_start(args, level);
         vsprintf(tempstring, message, args);
         va_end(args);
         fputs(tempstring, logfile);
         fire_event("log_message", 2, tempstring, &level);
         puts(tempstring);
      }
      else
      {
         fputs(message, logfile);
         fire_event("log_message", 2, message, &level);
         puts(message);
      }
      fputc('\n', logfile);
      logging |= LOG_PENDING;
   }
   
}