#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "NS_Ajoin"
#define DESCRIPTION "Allows users to manage their auto-join list."
#include "../headers/extensions.h"

int help_ajoin_add(Services_User *client, User *sender, cmd_message *message);
int help_ajoin_clear(Services_User *client, User *sender, cmd_message *message);
int help_ajoin_del(Services_User *client, User *sender, cmd_message *message);
int help_ajoin_list(Services_User *client, User *sender, cmd_message *message);

int command_ajoin_add(Services_User *client, User *sender, cmd_message *message);
int command_ajoin_clear(Services_User *client, User *sender, cmd_message *message);
int command_ajoin_del(Services_User *client, User *sender, cmd_message *message);
int command_ajoin_list(Services_User *client, User *sender, cmd_message *message);

int event_restore_channels(void **args);
int event_do_joins(void **args);
int event_expire(void **args);

typedef struct Join_List {
   char channel[CHANNEL_MAX];
   struct Join_List *next;
} Join_List;

int extension_load(void) {
   void **temp = 0;
   if(get_svsclient_by_type("nickserv")) {
      if(wrapper_add_command("nickserv", "AJOIN ADD", command_ajoin_add, is_identified) != 1) {
         return 0;
      }
      else if(wrapper_add_command("nickserv", "AJOIN CLEAR", command_ajoin_clear, is_identified) != 1) {
         return 0;
      }
      else if(wrapper_add_command("nickserv", "AJOIN DEL", command_ajoin_del, is_identified) != 1) {
         return 0;
      }
      else if(wrapper_add_command("nickserv", "AJOIN LIST", command_ajoin_list, is_identified) != 1) {
         return 0;
      }
      else
      {
         wrapper_add_help("nickserv", "ADD", "AJOIN", "Add a channel to the auto-join list", help_ajoin_add);
         wrapper_add_help("nickserv", "CLEAR", "AJOIN", "Clear the auto-join list", help_ajoin_clear);
         wrapper_add_help("nickserv", "DEL", "AJOIN", "Remove a channel from the auto-join list", help_ajoin_del);
         wrapper_add_help("nickserv", "LIST", "AJOIN", "Show the channels on the auto-join list", help_ajoin_list);
         event_restore_channels(temp);
         add_event("user_identify", event_do_joins);
         add_event("nickname_drop", event_expire);
         return 1;
      }
   }
   else
   {
      log_message("NickServ AJOIN extension require at least one NickServ-type client to be specified in ezri.conf. This extension will be unloaded.", LOG_WARNING);
   }
   return 0;
}

int extension_unload(void) {
   const char commands[][16] = {"AJOIN ADD", "AJOIN CLEAR", "AJOIN_DEL", "AJOIN_LIST", "\0"};
   const char help[][8] = {"ADD", "CLEAR", "DEL", "LIST"};
   int i;
   for(i = 0; *commands[i] != '\0'; i++) {
      drop_command_by_type("nickserv", commands[i]);
      drop_help_by_type("nickserv", help[i]);
   }
   return 1;
}

int event_do_joins(void **args) {
   Reg_Channel *regchannel;
   Reg_User *reguser;
   Join_List *list;
   if(args) {
     reguser = get_identified_user(args[0]);
     if(reguser) {
         list = get_reg_user_data(reguser, "ajoin");
         while(list) {
            remote_server->join(ezri->name, reguser->user, list->channel, 0);
            list = list->next;
         }
      }
   }
   return 1;
}

int event_write_channels(void **args) {
   DB_Table *table = database->open("ajoin");
   Reg_User *user = reg_user_list;
   char line[2048] = "\0";
   Join_List *join;
   if(table && args) {
      database->clean(table);
      while(user) {
         join = get_reg_user_data(user, "ajoin");
         while(join) {
            strncat(line, join->channel, 2048 - strlen(line));
            if(join->next) {
               strncat(line, ",", 2048 - strlen(line));
            }
            join = join->next;
         }
         database->write(table, 2, user->user, line);
         *line = '\0';
         user = user->next;
      }
   }
   database->close(table);
   return 1;
}

int event_restore_channels(void **args) {
   DB_Table *table =database->open("ajoin");
   char **row;
   char *ptr;
   Reg_User *user = 0;
   Join_List *list = 0;
   if(table) {
      do {
         row = database->read(table, 2);
         if(row) {
            if(!user || !case_compare(user->user, row[0])) {
               user = get_reg_user(row[0]);
               list = get_reg_user_data(user, "ajoin");
            }
            ptr = row[1];
            if(user && ptr) {
               while(*ptr) {
                  if(!list) {
                     list = malloc(sizeof(Join_List));
                     add_reg_user_data(user, "ajoin", list);
                  }
                  else
                  {
                     while(list->next) {
                        list = list->next;
                     }
                     list->next = malloc(sizeof(Join_List));
                     list = list->next;
                  }
                  if(list) {
                     ptr += copy_to(list->channel, ptr, ',', CHANNEL_MAX);
                     list->next = 0;
                  }
               }
               
            }
         }
      }
      while(row);
      database->close(table);
   }
   return 1;
}

void delete_ajoin_list(Reg_User *user) {
   Join_List *list;
   Join_List *next;
   list = get_reg_user_data(user, "ajoin");
   if(list) {
      next = list->next;
      while(next) {
         free(list);
         list = next;
         next = list->next;
      }
      free(list);
   }
   drop_reg_user_data(user, "ajoin");
}

int event_expire(void **args) {
   char *username;
   Reg_User *user;
   if(args) {
      username = args[0];
      user = get_reg_user(args[0]);
      delete_ajoin_list(user);
   }
   return 1;
}

static void delete_channel_ajoin(Join_List *join) {
   free(join);
}
static int drop_channel_ajoin(Reg_User *user, const char * const channel) {
   Join_List *p;
   Join_List *prev = 0;
   if(user && channel) {
      p = get_reg_user_data(user, "ajoin");
      if(p) {
         prev = p;
         while(p) {
            if(case_compare(p->channel, channel)) {
               if(prev == p) {
                  if(p->next == 0) {
                     drop_reg_user_data(user, "ajoin");
                  }
                  else
                  {
                     p = p->next;
                     memcpy(prev, p, sizeof(Join_List));
                  }
               }
               else
               {
                  prev->next = p->next;
               }
               delete_channel_ajoin(p);
               return 1;
            }
            prev = p;
            p = p->next;
         }
      }
   }
   return 0;
}

int add_channel_ajoin(const char * const username, const char * const channelname) {
   Join_List *list;
   Reg_User *user;
   int add = 1;
   user = get_reg_user(username);
   if(user) {
      list = get_reg_user_data(user, "ajoin");
      if(list) {
         while(list->next) {
            if(case_compare(list->channel, channelname)) {
               add = 0;
               break;
            }
            list = list->next;
         }
         if(list->next == 0) {
            if(case_compare(list->channel, channelname)) {
               add = 0;
            }
         }
         if(add) {
            list->next = malloc(sizeof(Join_List));
            list = list->next;
         }
         else
         {
            return 0;
         }
      }
      else
      {
         list = malloc(sizeof(Join_List));
         if(list) {
            add_reg_user_data(user, "ajoin", list);
         }
      }
      if(add) {
         if(list) {
            strncpy_safe(list->channel, channelname, CHANNEL_MAX);
            list->next = 0;
            return 1;
         }
      }
      else
      {
         return -1;
      }
   }
   return 0;
}

/*
   One curiosity of Ezri is that these will most likely become the target.
*/

int command_ajoin_add(Services_User *client, User *sender, cmd_message *message) {
   char channel_list[IRCLINE_MAX];
   char channel[CHANNEL_MAX];
   char *cptr = channel_list;
   int result;
   if(!message || *message_get_target(message) != '#') {
      help_ajoin_add(client, sender, message);
   }
   if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
      return 1;
   }
   else if(*message_get(message) == '#') {
      copy_to(channel_list, message_get(message), ' ', CHANNEL_MAX);
   }
   else if(*message_get_target(message) == '#') {
      strncpy_safe(channel_list, message_get_target(message), CHANNEL_MAX);
   }
   while(*cptr == '#') {
      cptr += copy_to(channel, cptr, ',', CHANNEL_MAX);
      if(*channel == '#') {
         result = add_channel_ajoin(sender->nick, channel);
         if(result == 1) {
            message_user(client, sender, "COMMAND_AJOIN_CHANNEL_ADDED", 1, channel);
         }
         else if(result == 0) {
            message_user(client, sender, "COMMAND_AJOIN_CHANNEL_EXISTS", 1, channel);
         }
         else
         {
            message_user(client, sender, "COMMAND_ERROR", 0);
         }
      }
      /* Temp */
     event_write_channels(&cptr);
   }
   return 1;
}

int command_ajoin_del(Services_User *client, User *sender, cmd_message *message) {
   char channel_list[IRCLINE_MAX];
   char channel[CHANNEL_MAX];
   char *cptr = channel_list;
   if(!message || *message_get_target(message) != '#') {
      help_ajoin_del(client, sender, message);
   }
   if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
      return 1;
   }
   else if(*message_get(message) == '#') {
      copy_to(channel_list, message_get(message), ' ', CHANNEL_MAX);
   }
   else if(*message_get_target(message) == '#') {
      strncpy_safe(channel_list, message_get_target(message), CHANNEL_MAX);
   }

   while(*cptr == '#') {
      cptr += copy_to(channel, cptr, ',', CHANNEL_MAX);
      if(*channel == '#') {
         drop_channel_ajoin(get_identified_user(sender->nick), channel);
         message_user(client, sender, "COMMAND_AJOIN_CHANNEL_REMOVED", 1, channel);
      }
      /* Temp */
     event_write_channels(&cptr);
   }
   return 1;
}

/*
   Consider the possibility of services admin clearing the list, too.
*/
int command_ajoin_clear(Services_User *client, User *sender, cmd_message *message) {
   Reg_User *user;
   if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
   }
   else
   {
      user = get_identified_user(sender->nick);
      if(user) {
         delete_ajoin_list(user);

      }
   }
   return 1;
}

int command_ajoin_list(Services_User *client, User *sender, cmd_message *message) {
   Reg_User *user;
   Join_List *list;
   int i = 0;
   char response[IRCLINE_MAX];
   char channelcount[4];
   if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
   }
   else
   {
      user = get_identified_user(sender->nick);
      if(user) {
         list = get_reg_user_data(user, "ajoin");
         if(list) {
            message_user(client, sender, "COMMAND_AJOIN_LIST_START", 1, user->user);
            for(i = 0; list; i++, list = list->next) {
               snprintf(response, IRCLINE_MAX, "%s", list->channel);
               plain_message_user(client, sender, response);
            }
            snprintf(channelcount, 4, "%d", i);
            message_user(client, sender, "COMMAND_AJOIN_LIST_COUNT", 1, channelcount);
         }
         else
         {
            message_user(client, sender, "COMMAND_AJOIN_LIST_EMPTY", 0);
         }
      }
   }
   return 1;
}

int help_ajoin_add(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_AJOIN_ADD_SYNTAX", 0);
      message_user(client, sender, "HELP_AJOIN_ADD_DESCRIBE", 0);
   }
   return 1;
}

int help_ajoin_clear(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_AJOIN_CLEAR_SYNTAX", 0);
      message_user(client, sender, "HELP_AJOIN_CLEAR_DESCRIBE", 0);
   }
   return 1;
}

int help_ajoin_del(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_AJOIN_DEL_SYNTAX", 0);
      message_user(client, sender, "HELP_AJOIN_DEL_DESCRIBE", 0);
   }
   return 1;
}

int help_ajoin_list(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_AJOIN_LIST_SYNTAX", 0);
      message_user(client, sender, "HELP_AJOIN_LIST_DESCRIBE", 0);
   }
   return 1;
}