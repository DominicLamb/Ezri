#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Register"
#define DESCRIPTION "Allows users to register nicknames and channels"
#include "../headers/extensions.h"

int command_nickname_register(Services_User *client, User *sender, cmd_message *message);
int command_channel_register(Services_User *client, User *sender, cmd_message *message);
int command_user_identify(Services_User *client, User *sender, cmd_message *message);

int handle_register(char *channel, User *sender, Services_User *client);
char *registermodes;

int event_backup_nicknames(void **args);
int event_restore_nicknames(void **args);
int event_backup_users(void **args);
int event_restore_users(void **args);
int event_backup_channels(void **args);
int event_restore_channels(void **args);
int event_introduce_channels(void **args);
int event_nickname_drop(void **args);

int invite_register(void **args);

int help_nickname_register(Services_User *client, User *sender, cmd_message *message);
int help_user_identify(Services_User *client, User *sender, cmd_message *message);
int help_channel_register(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   void *config;
   int *result;
   get_config("Register::RegisterNicknames", &config, RETURN_INT);
   result = config;
   if(result && *result) {
      if(wrapper_add_command("nickserv", "REGISTER", command_nickname_register, get_user) != 1) {
         return 0;
      }
      wrapper_add_help("nickserv", "REGISTER", "main", "Register a nickname on the network.", help_nickname_register);
   }

   get_config("Register::RegisterChannels", &config, RETURN_INT);
   result = config;
   if(result && *result) {
      if(wrapper_add_command("chanserv", "REGISTER", command_channel_register, get_user) != 1) {
         return 0;
      }
      wrapper_add_help("chanserv", "REGISTER", "main", "Register a channel on the network.", help_channel_register);
      get_config("Register::modes", &config, RETURN_CHAR);
      registermodes = config;
      if(!registermodes) {
         registermodes = "+ntr";
      }
      get_config("Register::invite", &config, RETURN_INT);
      result = config;
      if(result && *result) {
         add_event("channel_invite", invite_register);
      }
   }
   if(wrapper_add_command("nickserv", "IDENTIFY", command_user_identify, get_user) != 1) {
      return 0;
   }
   wrapper_add_help("nickserv", "IDENTIFY", "main", "Log in to the services.", help_user_identify);
   event_restore_nicknames(&config);
   event_restore_channels(&config);
   event_restore_users(&config);
   add_event("server_synch", event_introduce_channels);
   return 1;
}

int handle_register(char *channel, User *sender, Services_User *client) {
   char modestring[IRCLINE_MAX];
   Reg_Channel *new_channel;
   IRC_Channel *channel_ircd;
   if(valid_channel(channel)) {
      if(channel_registered(channel)) {
         message_user(client, sender, "COMMAND_REGISTER_CS_IS_TAKEN", 2, channel, client->nick);
      }
      else if(!user_on_channel(sender, channel)) {
         message_user(client, sender, "COMMAND_NOT_JOINED", 1, channel);
      }
      else if(user_is_identified(sender)) {
         channel_ircd = get_channel(channel);
         if(!channel_ircd || channel_is_op(channel_ircd, sender->nick)) {
            new_channel = register_channel(channel, sender->nick, client->nick);
            reg_channel_add_user(new_channel, sender->nick, 1000);
            message_user(client, sender, "COMMAND_REGISTER_REGISTERED", 1, channel);
            /* Move these out later. Make event_backup_channels call event_backup_users, this makes sense */
            event_backup_channels(&modestring);
            event_backup_users(&modestring);
            remote_server->join(client->nick, client->nick, channel, 1);
            sprintf(modestring, "%s %s %s", sender->nick, client->nick, client->nick);
            remote_server->modes(client->nick, channel, registermodes, (char *)0);
            /* Will need a change for IRCds which do not support these modes */
            remote_server->modes(client->nick, channel, "+qao", modestring);
         }
         else
         {
            message_user(client, sender, "COMMAND_REGISTER_NOT_OPERATOR", 1, channel);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_CHANNEL_INVALID", 0);
   }
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("nickserv", "REGISTER");
   drop_command_by_type("chanserv", "REGISTER");
   drop_command_by_type("nickserv", "IDENTIFY");
   drop_help_by_type("nickserv", "REGISTER");
   drop_help_by_type("chanserv", "REGISTER");
   drop_help_by_type("nickserv", "IDENTIFY");
   return 1;
}

int command_nickname_register(Services_User *client, User *sender, cmd_message *message) {
   char *password = 0;
   char *email = 0;
   if(!message || *message_get_target(message) == '#') {
      message_user(client, sender, "COMMAND_NOT_IN_CHANNEL", 0);
      fire_event("user_fail_id", 1, sender->nick);
   }
   else if(*message_get(message) == '\0') {
      help_nickname_register(client, sender, message);
      return 1;
   }
   else
   {
      password = get_token(1, message_get(message), " ");
      email = get_token(2, message_get(message), " ");
      if(!password || !email) {
         help_nickname_register(client, sender, message);
      }
      else if(case_compare(sender->nick,password)) {
         message_user(client, sender, "COMMAND_REGISTER_BAD_PASSWORD", 0);
      }
      else if(email != 0 && valid_email(email)) {
         if(register_nickname(sender->nick, password, email)) {
            remote_server->modes(client->nick, sender->nick, "+rd", "10549");
            message_user(client, sender, "COMMAND_REGISTER_SUCCESS", 1, email);
            event_backup_nicknames(&sender->nick);
         }
         else
         {
            message_user(client, sender, "COMMAND_REGISTER_NS_IS_TAKEN", 2, sender->nick, client->nick);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_INVALID_EMAIL", 0);
      }
   }
   if(password) {
      free(password);
   }
   if(email) {
      free(email);
   }
   return 1;
}

int command_channel_register(Services_User *client, User *sender, cmd_message *message) {
   char *channel;
   channel = message_get_target(message);
   handle_register(channel, sender, client);
   return 1;
}

int command_user_identify(Services_User *client, User *sender, cmd_message *message) {
   char password[64];
   if(!message || *message_get(message) == '\0') {
      help_user_identify(client, sender, message);
      return 1;
   }
   if(*message_get_target(message) == '#') {
      message_user(client, sender, "COMMAND_NOT_IN_CHANNEL", 0);
      fire_event("user_fail_id", 1, sender->nick);
   }
   else
   {
      copy_to(password, message_get(message), ' ', 64);
      if(*password == '\0') {
         message_user(client, sender, "COMMAND_INVALID_PASSWORD", 0);
         fire_event("user_fail_id", 1, sender->nick);
      }
      else if(identify_user(sender, sender->nick, password)) {
         /*
            This is a temporary measure, Unreal uses these.
            Must be handled in the IRCd extension later.
         */
         remote_server->modes(client->nick, sender->nick, "+rd", "10549");
         if(*(sender->vhostname) == '\0' || strcmp(sender->vhostname, sender->regdata->vhost)) {
            remote_server->new_host(ezri->name, sender->nick, sender->regdata->vhost);
         }
         message_user(client, sender, "COMMAND_REGISTER_ID_SUCCESSFUL", 0);
         fire_event("user_identify", 2, sender->nick, client->nick);
      }
      else
      {
         message_user(client, sender, "COMMAND_REGISTER_ID_FAILURE", 1, client->nick);
      }
   }
   return 1;
}

int event_backup_nicknames(void **args) {
   DB_Table *table;
   Reg_User *user = reg_user_list;
   if(args) {
      table = database->open("users");
      database->clean(table);
      while(user) {
         database->write(table, 7, user->user, user->password, user->email, user->vhost, user->dummy, user->last_ident, user->last_host);
         user = user->next;
      }
      database->close(table);
   }
   return 1;
}

int event_backup_channels(void **args) {
   DB_Table *table;
   Reg_Channel *channel = reg_channel_list;
   char date[15];
   if(args) {
      table = database->open("channels");
      database->clean(table);
      while(channel) {
         make_timestamp(date, channel->registered);
         database->write(table, 5, channel->name, channel->founder, date, channel->lock_pass, channel->channel_bot);
         channel = channel->next;
      }
      database->close(table);
   }
   return 1;
}

int event_backup_users(void **args) {
   DB_Table *table;
   Reg_Channel *channel = reg_channel_list;
   Channel_Access *user;
   if(args) {
      table = database->open("chusers");
      database->clean(table);
      while(channel) {
         user = channel->access;
         while(user) {
            database->write(table, 3, user->channel, &user->level, user->nick);
            user = user->next;
         }
         channel = channel->next;
      }
      database->close(table);
   }
   return 1;
}

int event_restore_nicknames(void **args) {
   DB_Table *table;
   char **in;
   Reg_User *user = reg_user_list;
   if(args) {
     table = database->open("users");
      if(table) {
         in = database->read(table, 7);
         while(in) {
            if(in[0] && in[1] && in[2]) {
	            user = register_nickname(in[0], in[1], in[2]);
               if(user) {
                  if(in[3]) {
                     strncpy_safe(user->vhost, in[3], HOST_MAX);
                  }
	               if(in[4]) {
	                  user->dummy = atoi(in[4]);
   	            }
                  if(in[5]) {
                     strncpy_safe(user->last_ident, user->last_ident, 16);
                  }
                  if(in[6]) {
                     strncpy_safe(user->last_host, user->last_host, HOST_MAX);
                  }
               }
	         }
   	      in = database->read(table, 7);
         }
      }
      database->close(table);
   }
   return 1;
}

int event_nickname_drop(void **args) {
   char *username;
   /*
      Add channel drop here
      Cycle through the registered user's access list
      And find all channels with OWNER_MAX and only
      one owner. Perhaps add a new count function
      that counts the number of people with an
      access level?
         /*
            Perhaps we can also add a function to
            get the next user within a range, which
            would be very useful for a "TRIM" command.
         **
   */
   return 1;
}
int event_restore_channels(void **args) {
   DB_Table *table;
   char **in;
   Reg_Channel *channel = reg_channel_list;
   if(args) {
      table = database->open("channels");
      in = database->read(table, 5);
      while(in) {
         if(in[0] && in[1] && in[4]) {
            channel = register_channel(in[0], in[1], in[4]);
            channel->registered = import_unix_timestamp(in[2]);
         }
         in = database->read(table, 5);
      }
      database->close(table);
   }
   return 1;
}

int event_restore_users(void **args) {
   DB_Table *table;
   char **in;
   int access = 0;
   Reg_Channel *channel;
   if(args) {
      table = database->open("channels");
      if(table) {
         in = database->read(table, 7);
         while(in) {
            if(in[0] && in[1] && in[2]) {
	            access = atoi(in[2]);
               if(access > ACCESS_MAX) {
                  access = ACCESS_MAX;
               }
               channel = get_reg_channel(in[0]);
               if(channel) {
                  reg_channel_add_user(channel, in[1], access);
               }
            }
            in = database->read(table, 3);
         }
      }
   }
   return 1;
}
int invite_register(void **args) {
   char *target;
   char *channel;
   char *nickname;
   Services_User *client;
   User *user;
   if(args) {
      target = args[0];
      channel = args[1];
      nickname = args[2];
      client = get_svsclient(target);
      if(client) {
         user = get_user(nickname);
         handle_register(channel, user, client);
      }
   }
   return 1;
}

int event_introduce_channels(void **args) {
   Reg_Channel *channel = reg_channel_list;
   if(args) {
      while(channel) {
         if(channel->channel_bot && *channel->channel_bot) {
            remote_server->join(channel->channel_bot, channel->channel_bot, channel->name, 1);
            remote_server->modes(channel->channel_bot, channel->name, "+o", channel->channel_bot);
         }
         channel = channel->next;
      }
   }
   return 1;
}

int help_nickname_register(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_REGISTER_NS_SYNTAX", 0);
      message_user(client, sender, "HELP_REGISTER_NS_DESCRIBE", 0);
   }
   return 1;
}

int help_channel_register(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_REGISTER_CS_SYNTAX", 0);
      message_user(client, sender, "HELP_REGISTER_CS_DESCRIBE", 0);
   }
   return 1;
}

int help_user_identify(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_REGISTER_IDENTIFY_SYNTAX", 0);
      message_user(client, sender, "HELP_REGISTER_IDENTIFY_DESCRIBE", 0);
   }
   return 1;
}
