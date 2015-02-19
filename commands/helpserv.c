#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "HelpServ"
#define DESCRIPTION "HelpServ queue manager."
#include "../headers/extensions.h"

typedef struct Help_Queue {
   time_t ticket_date;
   time_t accept_date;
   User *request;
   User *helper;
   char messages[6][150];
   struct Help_Queue *next;
} Help_Queue;

Help_Queue *helpqueue = 0;

static char helpchannel[CHANNEL_MAX];
static int auto_ticket = 1;
static int hold_ticket_on_part = 0;
static int voice_on_pickup = 0;
static int auto_access = 0;
static int auto_manager = 0;
static Reg_Channel *channeldata = 0;

unsigned int num_tickets = 0;

int event_join(void **args);
int event_part(void **args);
int event_kick(void **args);
int event_quit(void **args);

User *is_helper(const char * const nickname);
User *is_help_manager(const char * const nickname);

int queue_drop_user(User *user);
int helper_part(User *user);

int command_help_accept(Services_User *client, User *sender, cmd_message *message);
int command_help_close(Services_User *client, User *sender, cmd_message *message);
int command_help_info(Services_User *client, User *sender, cmd_message *message);
int command_help_list(Services_User *client, User *sender, cmd_message *message);
int command_help_transfer(Services_User *client, User *sender, cmd_message *message);
int command_help_update(Services_User *client, User *sender, cmd_message *message);

int help_accept(Services_User *client, User *sender, cmd_message *message);
int help_close(Services_User *client, User *sender, cmd_message *message);
int help_info(Services_User *client, User *sender, cmd_message *message);
int help_list(Services_User *client, User *sender, cmd_message *message);
int help_transfer(Services_User *client, User *sender, cmd_message *message);
int help_update(Services_User *client, User *sender, cmd_message *message);

int queue_add_user(User *user);

static int determine_access_level(void *data);
static int parse_config_bool(const char * const data);

int extension_load(void) {
   svs_commandhook *command;
   svs_help *help = 0;
   void *config;
   char *data;
   unsigned int i = 0;
   const char commandnames[][32] = {"ACCEPT", "CLOSE", "INFO", "LIST", "TRANSFER", "UPDATE", "\0"};
   User *(*restrictions[])(const char * const nickname) = {is_helper, get_user, get_user, is_helper, is_helper, get_user};
   int (*commands[])(Services_User *client, User *user, cmd_message *message) = {
      command_help_accept, 
      command_help_close, 
      command_help_info, 
      command_help_list, 
      command_help_transfer,
      command_help_update
   };
   const char helpdescriptions[][IRCLINE_MAX] = {
      "Helps a user in the help queue.", 
      "Close a help ticket.", 
      "Get information about a ticket.", 
      "See the list of tickets in the help queue.", 
      "Transfers the user from one helper to another.",
      "Update an active help ticket"
   };
   int (*helpfunctions[])(Services_User *client, User *user, cmd_message *message) = {
      help_accept, 
      help_close, 
      help_info, 
      help_list,
      help_transfer,
      help_update
   };
   if(!get_svsclient_by_type("helpserv")) {
      log_message("HelpServ Queue extension was unable to find a HelpServ client to load.", LOG_CRITICAL);
      return 0;
   }
   else if(!extension_loaded("Help")) {
      log_message("\"Help\" extension has not been loaded. This is a suggested extension for HelpServ.", LOG_IMPORTANT);
   }
   get_config("HelpServ", &config, RETURN_BLOCK);
   data = get_key_value(config, "Channel");
   if(data) {
      strncpy_safe(helpchannel, data, CHANNEL_MAX);
      channeldata = get_reg_channel(helpchannel);
      if(!channeldata) {
         log_message("HelpServ requires a registered channel to be specified for HelpServ::Channel, %s is not registered.", LOG_CRITICAL, helpchannel);
         return 0;
      }
   }
   else
   {
      log_message("HelpServ requires a channel to join, none specified at HelpServ::Channel", LOG_CRITICAL);
      return 0;
   }
   data = get_key_value(config, "HelperLevel");
   if(data) {
      auto_access = determine_access_level(data);
      if(auto_access <= 0 || auto_access > ACCESS_MAX) {
         log_message("HelpServ::HelperLevel contained an invalid value, and the HelpServ extension unable to continue loading.", LOG_CRITICAL);
         return 0;
      }
   }
   else
   {
      log_message("HelpServ::HelperLevel was not specified, HelpServ requires a minimum channel access level to be specified for helpers in order to function.", LOG_CRITICAL);
      return 0;
   }
   data = get_key_value(config, "ManagerLevel");
   if(data) {
      auto_manager = determine_access_level(data);
      if(auto_manager <= 0 || auto_manager > ACCESS_MAX) {
         log_message("HelpServ::ManagerLevel contained an invalid access level, HelpServ manager functions will not be available.", LOG_CRITICAL);
         auto_manager = 0;
      }
   }
   else
   {
      log_message("HelpServ::ManagerLevel was not specified, HelpServ manager functions will not be available.", LOG_IMPORTANT);
   }
   data = get_key_value(config, "VoiceOnPickup");
   if(data) {
      voice_on_pickup = parse_config_bool(data);
   }
   else
   {
      voice_on_pickup = 1;
   }
   for(i = 0; *commandnames[i] != '\0'; i++) {
      command = make_command(commandnames[i], commands[i]);
      if(command) {
         command_restrict(command, restrictions[i]);
         if(add_command_by_type("helpserv", command)) {
            help = create_help(commandnames[i], "main", helpdescriptions[i], helpfunctions[i]);
            if(help) {
               svs_add_help_by_type("helpserv", help);
            }
            else
            {
               log_message("Unable to create help for %s, this help item will not be available.", LOG_NOTICE, commandnames[i]);
            }
         }
         else
         {
            log_message("Unable to load %s command, some aspects of HelpServ will be unable to function.", LOG_IMPORTANT, commandnames[i]);
         }
      }
      else
      {
         log_message("Unable to allocate memory to create %s command, HelpServ will now attempt to unload.", LOG_CRITICAL, commandnames[i]);
         return 0;
      }
   }
   data = get_key_value(config, "AutoTicket");
   if(data) {
      auto_ticket = parse_config_bool(data);
   }
   else
   {
      auto_ticket = 1;
   }
   data = get_key_value(config, "HoldTicket");
   if(data) {
      hold_ticket_on_part = parse_config_bool(data);
   }
   else
   {
      hold_ticket_on_part = 0;
   }
   command = make_command("PICKUP", command_help_accept);
   command_restrict(command, get_user);
   add_command_by_type("helpserv", command);

   add_event("channel_user_join", event_join);
   add_event("channel_user_join_burst", event_join);
   add_event("channel_user_part", event_part);
   add_event("channel_user_kick", event_kick);
   add_event("user_disconnect", event_quit);
   remote_server->join(ezri->name, "HelpServ", helpchannel, 1);
   return 1;
}

int extension_unload(void) {
   const char commands[][32] = {"ACCEPT", "CLOSE", "INFO", "LIST", "TRANSFER", "UPDATE"};
   int i;
   for(i = 0; *commands[i] != '\0'; i++) {
      drop_command_by_type("helpserv",  commands[i]);
      drop_help_by_type("helpserv", commands[i]);
   }
   return 1;
}

static int determine_access_level(void *data) {
      /*
         We'll need flags for whether an IRCd supports
         modes +a and +h, +ov can be assumed
      */
   int level = 0;
   if(case_compare((char *)data, "ADMIN") || case_compare((char *)data, "SOP")) {
      level = channel_level_admin(channeldata);
   }
   else if(case_compare((char *)data, "OP") || case_compare((char *)data, "OPERATOR")) {
      level = channel_level_operator(channeldata);
   }
   else if(case_compare((char *)data, "HOP") || case_compare((char *)data, "HALFOP")) {
      level = channel_level_halfop(channeldata);
   }
   else if(case_compare((char *)data, "VOP") || case_compare((char *)data, "VOICE")) {
      level = channel_level_voice(channeldata);
   }
   else
   {
      level = atoi((char *)data);
   }
   return level;
}

static int parse_config_bool(const char * const data) {
   if(data) {
      if(case_compare(data, "OFF") || *data == '0') {
         return 0;
      }
      return 1;
   }
   return 0;
}

User *is_helper(const char * const nickname) {
   int access;
   if(nickname) {
      if(auto_access) {
         access = get_channel_access(channeldata, nickname);
         if(access >= auto_access) {
            return is_identified(nickname);
         }
      }
      else
      {
         /* Change this if we go to a separate access list method */
      }
   }
   return 0;
}

User *is_help_manager(const char * const nickname) {
   int access;
   if(nickname) {
      if(auto_manager) {
         access = get_channel_access(channeldata, nickname);
         if(access >= auto_access) {
            return is_identified(nickname);
         }
      }
      else
      {
         /* Change this if we go to a separate access list method */
      }
   }
   return 0;
}

int event_join(void **args) {
   User *user;
   char tickets[5];
   if(args && auto_ticket == 1) {
      if(case_compare(args[0], helpchannel)) {
         user = get_user(args[1]);
         if(!is_helper(user->nick)) {
            if(queue_add_user(user)) {
               sprintf(tickets, "%d", num_tickets - 1);
               message_user(get_svsclient_by_type("helpserv"), user, "QUEUE_USER_JOIN", 3, helpchannel, user->nick, tickets);
            }
         }
      }
   }
   return 1;
}

int event_part(void **args) {
   User *user;
   if(args && hold_ticket_on_part == 0) {
      if(case_compare(args[0], helpchannel)) {
         user = get_user(args[1]);
         if(!is_helper(user->nick)) {
            if(queue_drop_user(user)) {
               message_user(get_svsclient_by_type("helpserv"), user, "QUEUE_USER_PART", 2, helpchannel, user->nick);
            }
         }
         else
         {
            helper_part(user);
         }
      }
   }
   return 1;
}

int event_quit(void **args) {
   User *user;
   if(args) {
      if(case_compare(args[0], helpchannel)) {
         user = get_user(args[0]);
         if(!is_helper(user->nick)) {
            queue_drop_user(user);
         }
         else
         {
            helper_part(user);
         }
      }
   }
   return 1;
}

int event_kick(void **args) {
   User *user;
   if(args) {
      if(case_compare(args[0], helpchannel)) {
         user = get_user(args[0]);
         if(!is_helper(user->nick)) {
            if(queue_drop_user(user)) {
               message_user(get_svsclient_by_type("helpserv"), user, "QUEUE_USER_KICK", 2, helpchannel, user->nick);
            }
         }
         else
         {
            helper_part(user);
         }
      }
   }
   return 1;
}

int queue_add_user(User *user) {
   Help_Queue *queue = helpqueue;
   int add = 1;
   if(queue) {
      while(queue->next) {
         if(queue->request == user) {
            add = 0;
            break;
         }
         queue = queue->next;
      }
      if(add) {
         queue->next = malloc(sizeof(Help_Queue));
         queue = queue->next;
      }
   }
   else
   {
      helpqueue = malloc(sizeof(Help_Queue));
      queue = helpqueue;
   }
   if(queue && add) {
      memset(queue, '\0', sizeof(Help_Queue));
      queue->request = user;
      queue->ticket_date = time(NULL);
      num_tickets++;
      return 1;
   }
   return 0;
}

Help_Queue *get_ticket(const char * const nickname) {
   Help_Queue *queue = helpqueue;
   while(queue) {
      if(case_compare(nickname, queue->request->nick)) {
         return queue;
      }
      queue = queue->next;
   }
   return 0;
}

int queue_drop_user(User *user) {
   Help_Queue *queue = helpqueue;
   Help_Queue *prev = helpqueue;
   if(queue) {
      while(queue) {
         if(queue->request == user) {
            if(queue == helpqueue) {
               queue = queue->next;
               free(helpqueue);
               helpqueue = queue;
            }
            else
            {
               prev->next = queue->next;
               free(queue);
            }
            num_tickets--;
            return 1;
            break;
         }
         else
         {
            prev = queue;
            queue = queue->next;
         }
      }
   }
   return 0;
}

int helper_part(User *user) {
   Help_Queue *queue = helpqueue;
   int i = 0;
   if(user && queue) {
      for(i = 0; queue; i++) {
         if(queue->helper == user) {
            message_user(get_svsclient_by_type("helpserv"), queue->request, "QUEUE_HELPER_PART", 2, helpchannel, queue->helper->nick);
            queue->helper = 0;
         }
         queue = queue->next;
      }
   }
   return i;
}

int command_help_accept(Services_User *client, User *sender, cmd_message *message) {
   Help_Queue *queue = helpqueue;
   const char *msgptr = message_get(message);
   if(queue && msgptr && *msgptr != '\0') {
      queue = get_ticket(msgptr);
      if(queue) {
         if(queue->helper) {
            if(queue->helper == sender) {
               message_user(client, sender, "COMMAND_QUEUE_ACCEPT_YOURS", 1, queue->request->nick);
            }
            else
            {
               message_user(client, sender, "COMMAND_QUEUE_ACCEPT_TAKEN", 2, queue->request->nick, queue->helper->nick);
            }
         }
         else
         {
            queue->helper = sender;
            queue->accept_date = time(NULL);
            message_user(client, sender, "COMMAND_QUEUE_ACCEPT_NEW", 2, queue->request->nick, num_tickets);
            message_user(client, queue->request, "QUEUE_ASSIGNED", 2, queue->helper->nick, num_tickets);
            if(voice_on_pickup) {
               remote_server->modes(client->nick, helpchannel, "+v", queue->request->nick);
            }
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_QUEUE_NOT_FOUND", 1, msgptr);
      }
   }
   else if(queue) {
      /* Take the next user */
      while(queue) {
         if(!queue->helper) {
            break;
         }
         queue = queue->next;
      }
      if(queue) {
         message_user(client, sender, "COMMAND_QUEUE_ACCEPT_NEW", 2, queue->request->nick, num_tickets);
         message_user(client, queue->request, "QUEUE_ASSIGNED", 2, queue->request->nick, num_tickets);
         queue->helper = sender;
         queue->accept_date = time(NULL);
      }
      else
      {
         message_user(client, sender, "COMMAND_QUEUE_NO_QUEUE", 0);
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_QUEUE_NO_TICKETS", 0);
   }
   return 1;
}

int command_help_close(Services_User *client, User *sender, cmd_message *message) {
   Help_Queue *queue = helpqueue;
   const char *msgptr = message_get(message);
   if(queue) {
      if(is_helper(sender->nick)) {
         if(msgptr && *msgptr != '\0') {
            queue = get_ticket(msgptr);
            if(queue) {
               if(queue->helper) {
                  if(queue->helper == sender) {
                     message_user(client, sender, "COMMAND_QUEUE_CLOSE_DONE", 1, queue->request->nick);
                     queue_drop_user(queue->request);
                  }
                  else
                  {
                     message_user(client, sender, "COMMAND_QUEUE_CLOSE_HELPER", 2, queue->request->nick, queue->helper->nick);
                  }
               }
               else
               {
                  message_user(client, sender, "COMMAND_QUEUE_CLOSE_NEW", 1, queue->request->nick);
               }
            }
         }
         else
         {
            help_close(client, sender, message);
         }
      }
      else
      {
         queue = get_ticket(sender->nick);
         if(queue) {
            message_user(client, sender, "COMMAND_QUEUE_CLOSE_OWN_DONE", 0);
            if(queue->helper) {
               message_user(client, queue->helper, "QUEUE_USER_CLOSED_TICKET", 1, queue->request->nick);
            }
            queue_drop_user(queue->request);
            return 1;
         }
         else
         {
            message_user(client, sender, "COMMAND_QUEUE_NO_OPEN_TICKET", 0);
         }
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_QUEUE_NO_TICKETS", 0);
   }
   return 1;
}

int command_help_info(Services_User *client, User *sender, cmd_message *message) {
   Help_Queue *queue;
   struct tm *timestruct;
   char timestring[64];
   char nickname[NICK_MAX];
   int i;
   copy_to(nickname, message_get(message), NICK_MAX, ' ');
   queue = get_ticket(message_get(message));
   if(!nickname) {
      help_info(client, sender, message);
   }
   else if(queue) {
      message_user(client, sender, "COMMAND_QUEUE_INFO_USER", 3, queue->request->nick, queue->request->username, queue->request->hostname);
      timestruct = gmtime(&queue->ticket_date);
      strftime(timestring, 64, "%a %B %d %Y (%H:%M) %Z", timestruct);
      message_user(client, sender, "COMMAND_QUEUE_INFO_OPENED", 1, timestring);
      if(queue->helper) {
         timestruct = gmtime(&queue->accept_date);
         strftime(timestring, 80, "%a %B %d %Y (%H:%M) %Z", timestruct);
         message_user(client, sender, "COMMAND_QUEUE_INFO_PICKUP", 2, timestring, queue->helper->nick);
      }
      if(*queue->messages[0] != '\0') {
         message_user(client, sender, "COMMAND_QUEUE_INFO_MESSAGES", 0);
         for(i = 0; i < 5 && *queue->messages[i] != '\0'; i++) {
            plain_message_user(client, sender, queue->messages[i]);
         }
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_QUEUE_NO_TICKET", 1, nickname);
   }
   return 1;
}

int command_help_list(Services_User *client, User *sender, cmd_message *message) {
   Help_Queue *queue = helpqueue;
   char buffer[IRCLINE_MAX];
   int i;
   char ticketcount[5];
   struct tm *timestruct;
   char timestring[64];
   if(queue && message) {
      snprintf(buffer, IRCLINE_MAX, "%-*s%-30s%s", NICK_MAX, "User", "Ticket opened", "Helped by");
      plain_message_user(client, sender, buffer);
      for(i = 0; queue != 0; i++) {
         timestruct = gmtime(&queue->ticket_date);
         strftime(timestring, 64, "%B %d %Y (%H:%M) %Z", timestruct);
         /*
            This part could really benefit from
            showing the time that the ticket
            has been open, rather than the time
            the ticket was opened.
         */
         if(queue->helper) {
            snprintf(buffer, IRCLINE_MAX, "%-*s%-64s%s", NICK_MAX, queue->request->nick, timestring, queue->helper->nick);
         }
         else
         {
            snprintf(buffer, IRCLINE_MAX, "%-*s%-64s%s", NICK_MAX, queue->request->nick, timestring, "No helper");
         }
         plain_message_user(client, sender, buffer);
         queue = queue->next;
      }
      snprintf(ticketcount, 5, "%d", i);
      message_user(client, sender, "COMMAND_QUEUE_LIST_END", 1, ticketcount);
   }
   else
   {
      message_user(client, sender, "COMMAND_QUEUE_NO_TICKETS", 0);
   }
   return 1;
}

int command_help_transfer(Services_User *client, User *sender, cmd_message *message) {
   char *ticketname;
   char *helpername;
   Help_Queue *queue;
   User *helper;
   if(*message_get(message) == '\0') {
      help_transfer(client, sender, message);
   }
   else if(!helpqueue) {
      message_user(client, sender, "COMMAND_QUEUE_NO_TICKETS", 0);
   }
   else
   {
      ticketname = get_token(1, message_get(message), " ");
      helpername = get_token(2, message_get(message), " ");
      if(ticketname && helpername) {
         queue = get_ticket(ticketname);
         if(queue) {
            helper = get_user(helpername);
            if(helper) {
               if(queue->helper != helper) {
                  queue->helper = helper;
                  message_user(client, sender, "COMMAND_QUEUE_TRANSFERRED", 2, queue->request->nick, queue->helper->nick);
                  if(sender != queue->helper) {
                     message_user(client, queue->helper, "QUEUE_NEW_TICKET", 3, sender->nick, queue->request->nick, queue->helper->nick);
                  }
                  message_user(client, queue->request, "QUEUE_NEW_HELPER", 2, queue->request->nick, queue->helper->nick);
               }
               else
               {
                  message_user(client, sender, "COMMAND_QUEUE_TRANSFER_SAME", 2, queue->request->nick, helper->nick);
               }
            }
            else
            {
               message_user(client, sender, "COMMAND_QUEUE_USER_OFFLINE", 1, helpername);
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_QUEUE_NO_TICKET", 1, ticketname);
         }
      }
      else
      {
         help_transfer(client, sender, message);
      }
      if(ticketname) {
         free(ticketname);
      }
      if(helpername) {
         free(helpername);
      }
   }
   return 1;
}

int command_help_update(Services_User *client, User *sender, cmd_message *message) {
   Help_Queue *queue = helpqueue;
   unsigned int i;
   const char *msgptr = message_get(message);
   if(msgptr && *msgptr != '\0') {
      if(helpqueue) {
         queue = get_ticket(sender->nick);
         if(queue) {
            for(i = 0; i < 5; i++) {
               if(*(queue->messages[i]) == '\0') {
                  strncpy_safe(queue->messages[i], msgptr, IRCLINE_MAX);
                  message_user(client, sender, "COMMAND_QUEUE_UPDATE_SUCCESS", 0);
                  if(queue->helper) {
                     message_user(client, sender, "QUEUE_TICKET_UPDATE", 1, queue->request->nick);
                  }
                  break;
               }
            }
            if(i == 5) {
               message_user(client, sender, "COMMAND_QUEUE_UPDATE_MAXIMUM", 0);
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_QUEUE_NO_TICKET", 0);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_QUEUE_NO_TICKETS", 0);
      }
   }
   else
   {
      help_update(client, sender, message);
   }
   return 1;
}

int help_accept(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_QUEUE_ACCEPT_SYNTAX", 0);
      message_user(client, sender, "HELP_QUEUE_ACCEPT_DESCRIBE", 0);
   }
   return 1;
}

int help_close(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_QUEUE_CLOSE_SYNTAX", 0);
      message_user(client, sender, "HELP_QUEUE_CLOSE_DESCRIBE", 0);
   }
   return 1;
}

int help_info(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_QUEUE_INFO_SYNTAX", 0);
      message_user(client, sender, "HELP_QUEUE_INFO_DESCRIBE", 0);
   }
   return 1;
}

int help_list(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_QUEUE_LIST_SYNTAX", 0);
      message_user(client, sender, "HELP_QUEUE_LIST_DESCRIBE", 0);
   }
   return 1;
}

int help_transfer(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_QUEUE_TRANSFER_SYNTAX", 0);
      message_user(client, sender, "HELP_QUEUE_TRANSFER_DESCRIBE", 0);
   }
   return 1;
}

int help_update(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_QUEUE_UPDATE_SYNTAX", 0);
      message_user(client, sender, "HELP_QUEUE_UPDATE_DESCRIBE", 0);
   }
   return 1;
}