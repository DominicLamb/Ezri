#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "NS_NickProtect"
#define DESCRIPTION "Enables nickname protection."
#include "../headers/extensions.h"

int timer_nickprotect(void **input, const int timer_id);
int event_change_nickname(void **input);
int event_connect(void **input);
int command_ghost(Services_User *client, User *sender, cmd_message *message);
int help_ghost(Services_User *client, User *sender, cmd_message *message);

char *ghostclient;
unsigned int time_length = 0;

int extension_load(void) {
   void *config;
   get_config("Ghost::GhostClient", &config, RETURN_CHAR);
   ghostclient = config;
   get_config("Ghost::RecoveryTime", &config, RETURN_INT);
   if(!ghostclient || *ghostclient == '\0' || strlen(ghostclient) > NICK_MAX || time_length < 10) {
      return 0;
   }
   if(wrapper_add_command("NickServ", "GHOST", command_ghost, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("NickServ", "GHOST", "main", "Disconnect a user with your nickname.", help_ghost);
   add_event("user_change_nick", event_change_nickname);
   add_event("user_connect", event_connect);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("NickServ", "GHOST");
   drop_help_by_type("NickServ", "GHOST");
   drop_event("user_change_nick", event_change_nickname);
   drop_event("user_connect", event_connect);
   end_all_timers("ghost");
   return 1;
}

int timer_nickprotect(void **input, const int timer_id) {
   char guestnickname[NICK_MAX];
   User *user = get_user(input[0]);
   if(!user_is_identified(user)) {
      snprintf(guestnickname, NICK_MAX, "Guest%d", timer_id);
      message_user(get_svsclient(ghostclient), user, "NICKNAME_CHANGE", 1, guestnickname);
      remote_server->change_nick(input[1], input[0], guestnickname);
   }
   return TIMER_DELETE;
}

static int handle_nickname(User *user, Services_User *client) {
   Reg_User *regdata;
   if(user) {
      regdata = get_reg_user(user->nick);
      if(regdata != 0) {
         message_user(client, user, "NICKNAME_PROTECTED", 2, user->nick, ghostclient);
         add_timer("ghost", timer_nickprotect, time_length, 2, user->nick, client->nick);
      }
   }
   return EVENT_CONTINUE;
}

int event_change_nickname(void **input) {
   Services_User *client;
   User *user = 0;
   if(input) {
      client = get_svsclient(ghostclient);
      user = get_user(input[1]);
      handle_nickname(user, client);
   }
   return EVENT_CONTINUE;
}

int event_connect(void **input) {
   Services_User *client;
   User *user = 0;
   if(input) {
      client = get_svsclient(ghostclient);
      user = get_user(input[0]);
      handle_nickname(user, client);
   }
   return EVENT_CONTINUE;
}

int command_ghost(Services_User *client, User *sender, cmd_message *message) {
   Reg_User *user;
   char *pass;
   char *nickname;
   if(!message) {
      help_ghost(client, sender, message);
      return 1;
   }
   if(*message_get_target(message) == '#') {
      message_user(client, sender, "COMMAND_NOT_IN_CHANNEL", 0);
   }
   nickname = get_token(1, message_get(message), " ");
   if(!nickname) {
      help_ghost(client, sender, message);
   }
   else if(!case_compare(nickname, sender->nick)) {
      user = get_reg_user(nickname);
      if(!user) {
         message_user(client, sender, "COMMAND_NICKNAME_NOT_REGISTERED", 1);
      }
      else
      {
         pass = get_token(2, message_get(message), " ");
         if(!pass || strcmp(pass, user->password) != 0) {
            message_user(client, sender, "COMMAND_BAD_PASSWORD", 0);
         }
         else
         {
            message_user(client, sender, "COMMAND_GHOST_DISCONNECTED", 1);
            remote_server->kill(ghostclient, nickname, "Disconnected by %s (Ghost command used)", 1);
         }
         if(pass) {
            free(pass);
         }
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_GHOST_SELF_KILL", 0);
   }
   if(nickname) {
      free(nickname);
   }
   return 1;
}

int help_ghost(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_GHOST_SYNTAX", 0);
      message_user(client, sender, "HELP_GHOST_DESCRIBE", 0);
   }
   return 1;
}

