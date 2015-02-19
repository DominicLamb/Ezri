#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "CS_Access"
#define DESCRIPTION "Provides a series of commands for giving users status through services."
#include "../headers/extensions.h"


int event_autostatus(void **args);
int command_channel_devoice(Services_User *client, User *sender, cmd_message *message);
int command_channel_dehalfop(Services_User *client, User *sender, cmd_message *message);
int command_channel_deop(Services_User *client, User *sender, cmd_message *message);
int command_channel_deadmin(Services_User *client, User *sender, cmd_message *message);
int command_channel_deowner(Services_User *client, User *sender, cmd_message *message);

int command_channel_voice(Services_User *client, User *sender, cmd_message *message);
int command_channel_halfop(Services_User *client, User *sender, cmd_message *message);
int command_channel_op(Services_User *client, User *sender, cmd_message *message);
int command_channel_admin(Services_User *client, User *sender, cmd_message *message);
int command_channel_owner(Services_User *client, User *sender, cmd_message *message);

int enablehalfops = 0;
int enableadmins = 0;
int enableowners = 0;

int help_voice(Services_User *client, User *sender, cmd_message *message);
int help_halfop(Services_User *client, User *sender, cmd_message *message);
int help_operator(Services_User *client, User *sender, cmd_message *message);
int help_admin(Services_User *client, User *sender, cmd_message *message);
int help_owner(Services_User *client, User *sender, cmd_message *message);

void determine_access(Services_User *client, Reg_User *user, Reg_Channel *channel) {
   int access;
   int done = 0;
   if(client && user && channel) {
      /* Eventually have the IRCd module send the correct modes, not the access module */
      access = get_channel_access(channel, user->user);
      if(enableowners) {
         if(access >= LEVEL_SUBOWNER) {
            remote_server->modes(client->nick, channel->name, "+q", user->user);
            done++;
         }
      }
      if(!done && enableadmins) {
         if(enableadmins && access >= channel_level_admin(channel)) {
            remote_server->modes(client->nick, channel->name, "+a", user->user);
            done++;
         }
      }
      if(done || access >= channel_level_operator(channel)) {
         remote_server->modes(client->nick, channel->name, "+o", user->user);
         done++;
      }
      else if(enablehalfops && access >= channel_level_halfop(channel)) {
         remote_server->modes(client->nick, channel->name, "+h", user->user);
         done++;
      }
      else if(access >= channel_level_voice(channel)) {
         remote_server->modes(client->nick, channel->name, "+v", user->user);
         done++;
      }
   }
}

int event_autostatus(void **args) {
   char *channelname;
   char *username;
   Reg_User *user = 0;
   Reg_Channel *channel;
   Services_User *client;
   if(args) {
      channelname = args[0];
      username = args[1];
      channel = get_reg_channel(channelname);
      if(channel && is_identified(username)) {
         client = get_svsclient(channel->channel_bot);
         user = get_reg_user(username);
         determine_access(client, user, channel);
      }
   }
   return 1;
}

int event_identify_status(void **args) {
   User *user;
   Channel_User *channel;
   Reg_Channel *regchannel;
   Services_User *client = 0;
   if(args) {
      user = get_user(args[0]);
      if(user) {
         channel = user->channels;
         while(channel) {
            if(channel_is_registered(channel->channel)) {
               regchannel = channel->channel->regdata;
               if(!client || strcmp(client->nick, regchannel->channel_bot) != 0) {
                  client = get_svsclient(regchannel->channel_bot);
                  determine_access(client, user->regdata, regchannel);
               }
            }
            channel = channel->next_for_user;
         }
      }
   }
   return 1;
}
int extension_load(void) {
   void *config = 0;
   if(wrapper_add_command("chanserv", "VOICE", command_channel_voice, is_identified) != 1) {
      return 0;
   }
   else if(wrapper_add_command("chanserv", "DEVOICE", command_channel_devoice, is_identified) != 1) {
      return 0;
   }
   else if(wrapper_add_command("chanserv", "OP", command_channel_op, is_identified) != 1) {
      return 0;
   }
   else if(wrapper_add_command("chanserv", "DEOP", command_channel_deop, is_identified) != 1) {
      return 0;
   }
   else
   {
      wrapper_add_help("chanserv", "VOICE", "ACCESS", "Set channel operator status on a user", help_voice);
      wrapper_add_help("chanserv", "DEVOICE", "ACCESS", "Remove channel operator status from a user", help_voice);
      wrapper_add_help("chanserv", "OP", "ACCESS", "Set channel operator status on a user", help_operator);
      wrapper_add_help("chanserv", "DEOP", "ACCESS", "Remove channel operator status from a user", help_operator);
      get_config("Access::EnableHalfops", &config, RETURN_INT);
      if(config) {
         enablehalfops = *(int *)config;
      }
      if(enablehalfops) {
         if(wrapper_add_command("chanserv", "HALFOP", command_channel_halfop, is_identified) != 1) {
            return 0;
         }
         else if(wrapper_add_command("chanserv", "HALFOP", command_channel_dehalfop, is_identified) != 1) {
            return 0;
         }
         wrapper_add_help("chanserv", "HALFOP", "ACCESS", "Set channel halfop status on a user", help_halfop);
         wrapper_add_help("chanserv", "DEHALFOP", "ACCESS", "Remove channel halfop status from a user", help_halfop);
      }
      get_config("Access::EnableAdmins", &config, RETURN_INT);
      if(config) {
         enableadmins = *(int *)config;
      }
      if(enableadmins) {
         if(wrapper_add_command("chanserv", "ADMIN", command_channel_admin, is_identified) != 1) {
            return 0;
         }
         else if(wrapper_add_command("chanserv", "DEADMIN", command_channel_deadmin, is_identified) != 1) {
            return 0;
         }
         wrapper_add_help("chanserv", "ADMIN", "ACCESS", "Set channel admin status on a user", help_admin);
         wrapper_add_help("chanserv", "DEADMIN", "ACCESS", "Remove channel admin status from a user", help_admin);
      }
      get_config("Access::EnableOwners", &config, RETURN_INT);
      if(config) {
         enableowners = *(int *)config;
      }
      if(enableowners) {
         if(wrapper_add_command("chanserv", "OWNED", command_channel_owner, is_identified) != 1) {
            return 0;
         }
         else if(wrapper_add_command("chanserv", "DEOWNER", command_channel_deowner, is_identified) != 1) {
            return 0;
         }
         wrapper_add_help("chanserv", "OWNER", "ACCESS", "Set channel owner status on a user", help_owner);
         wrapper_add_help("chanserv", "DEOWNER", "ACCESS", "Remove channel owner status from a user", help_owner);
      }
      add_event("channel_user_join", event_autostatus);
      add_event("user_identify", event_identify_status);
      return 1;
   }
}

int extension_unload(void) {
   char commands[][10] = {"VOICE", "DEVOICE", "HALFOP", "DEHALFOP", "OP", "DEOP", "ADMIN", "DEADMIN", "OWNER", "DEOWNER", "\0"};
   int i = 0;
   while(commands[i]) {
      drop_command_by_type("chanserv", commands[i]);
      drop_help_by_type("chanserv", commands[i]);
   }
   return 1;
}

int command_channel_devoice(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(case_compare(find_user, sender->nick) || get_channel_access(channel->regdata, sender->nick) >= channel_level_halfop(channel->regdata)) {
            if(channel_is_voice(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "-v", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_LACKS_ACCESS", 3, find_user, "voice", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_voice(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(get_channel_access(channel->regdata, sender->nick) >= channel_level_halfop(channel->regdata)) {
            if(!channel_is_voice(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "+v", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_HAS_ACCESS", 3, find_user, "voice", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_dehalfop(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(case_compare(find_user, sender->nick) || get_channel_access(channel->regdata, sender->nick) >= channel_level_operator(channel->regdata)) {
            if(channel_is_halfop(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "-h", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_LACKS_ACCESS", 3, find_user, "halfop", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_halfop(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(get_channel_access(channel->regdata, sender->nick) >= channel_level_halfop(channel->regdata)) {
            if(!channel_is_halfop(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "+h", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_HAS_ACCESS", 3, find_user, "halfop", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_deop(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(case_compare(find_user, sender->nick) || get_channel_access(channel->regdata, sender->nick) >= channel_level_operator(channel->regdata)) {
            if(channel_is_op(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "-o", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_LACKS_ACCESS", 3, find_user, "operator", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_op(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(get_channel_access(channel->regdata, sender->nick) >= channel_level_operator(channel->regdata)) {
            if(!channel_is_op(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "+o", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_HAS_ACCESS", 3, find_user, "operator", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_deadmin(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(case_compare(find_user, sender->nick) || get_channel_access(channel->regdata, sender->nick) == channel_level_admin(channel->regdata)) {
            if(channel_is_admin(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "-a", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_LACKS_ACCESS", 3, find_user, "admin", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_admin(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(get_channel_access(channel->regdata, sender->nick) >= channel_level_admin(channel->regdata)) {
            if(!channel_is_admin(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "+a", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_HAS_ACCESS", 3, find_user, "admin", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}


int command_channel_deowner(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(case_compare(find_user, sender->nick) || get_channel_access(channel->regdata, sender->nick) == ACCESS_MAX) {
            if(channel_is_owner(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "-q", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_LACKS_ACCESS", 3, find_user, "owner", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int command_channel_owner(Services_User *client, User *sender, cmd_message *message) {
   char *find_user = 0;
   IRC_Channel *channel;
   if(!message || *message_get_target(message) != '#') {
      message_user(client, sender, "COMMAND_NO_CHANNEL", 0);
   }
   else
   {
      find_user = get_token(1, message_get(message), " ");
      if(!find_user) {
         find_user = get_token(1, sender->nick, "");
      }
      channel = get_channel(message_get_target(message));
      if(channel && channel->regdata) {
         if(get_channel_access(channel->regdata, sender->nick) >= LEVEL_SUBOWNER) {
            if(!channel_is_owner(channel, find_user)) {
               remote_server->modes(client->nick, message_get_target(message), "+q", find_user);
            }
            else
            {
               message_user(client, sender, "ACCESS_HAS_ACCESS", 3, find_user, "owner", message_get_target(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_NO_CHANNEL_ACCESS", 1, message_get_target(message));
         }
      }
      else
      {
         message_user(client, sender, "CHANNEL_NOT_REGISTERED", 2, message_get_target(message), client->nick);
      }
      
   }
   if(find_user) {
      free(find_user);
   }
   return 1;
}

int help_voice(Services_User *client, User *sender, cmd_message *message) {
   if(!message || *message_get(message) == '\0') {
      message_user(client, sender, "HELP_ACCESS_VOICE_SYNTAX", 0);
      message_user(client, sender, "HELP_ACCESS_VOICE_DESCRIBE", 0);
   }
   return 1;
   /* Add "to voice (nickname)" under else */
}

int help_halfop(Services_User *client, User *sender, cmd_message *message) {
   if(!message || *message_get(message) == '\0') {
      message_user(client, sender, "HELP_ACCESS_HALFOP_SYNTAX", 0);
      message_user(client, sender, "HELP_ACCESS_HALFOP_DESCRIBE", 0);
   }
   return 1;
}

int help_operator(Services_User *client, User *sender, cmd_message *message) {
   if(!message || *message_get(message) == '\0') {
      message_user(client, sender, "HELP_ACCESS_OP_SYNTAX", 0);
      message_user(client, sender, "HELP_ACCESS_OP_DESCRIBE", 0);
   }
   return 1;
}

int help_admin(Services_User *client, User *sender, cmd_message *message) {
   if(!message || *message_get(message) == '\0') {
      message_user(client, sender, "HELP_ACCESS_ADMIN_SYNTAX", 0);
      message_user(client, sender, "HELP_ACCESS_ADMIN_DESCRIBE", 0);
   }
   return 1;
}

int help_owner(Services_User *client, User *sender, cmd_message *message) {
   if(!message || *message_get(message) == '\0') {
      message_user(client, sender, "HELP_ACCESS_OWNER_SYNTAX", 0);
      message_user(client, sender, "HELP_ACCESS_OWNER_DESCRIBE", 0);
   }
   return 1;
}