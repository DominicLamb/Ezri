/* 
   This module is for Unreal 3.2 only.
   Tested against 3.2.7 and later 3.2.8, other versions may not work
*/

#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Unreal"
#define DESCRIPTION "Allows services to communicate with Unreal 3.2"
#include "../headers/extensions.h"

static struct {
   int tokens;
   int umode2;
   int nickv2;
   int sjoin;
   int ns;
   int vl;
   char modenormal[64];
   char modeparams[64];
   char modeset[64];
} ircd;

typedef struct ircd_modechange {
   char modechar;
   char settings[64];
   unsigned int remove;
   struct ircd_modechange *next;
} modechange;

void ircd_send_kick(const char * const source, const char * const channel, const char * const target, const char * const reason);
void ircd_send_kill(const char * const source, const char * const target, const char * const reason, const int svskill);
void ircd_send_synch(const char * const in);
void ircd_send_pong(const char * in);
void ircd_send_nick(const char * const source, const char * const target, const char * const new_nickname);
void ircd_send_new_nick(const char * const nickname, const char * const username,  const char * const hostname,  const char * const server, const char * const realname);
void ircd_send_new_host(const char * const sender, const char * const nickname, const char * const hostname);
void ircd_send_new_ident(const char * const nickname, const char * const ident);
void ircd_send_init(const char * const name, const char * const pass, const char * const description);
void ircd_send_privmsg(const char * const sender, const char * const target, const char * const message);
void ircd_send_notice(const char * const sender, const char * const target, const char * const message);
void ircd_send_join(const char * const sender, const char * const target, const char * const channel, const int service);
void ircd_send_part(const char * const sender, const char * const target, const char * const reason);
void ircd_send_invite(const char * const sender, const char * const nickname, const char * const target);
void ircd_send_modes(const char * const sender, const char * const target, const char * const modes, const char * const params);
void ircd_send_version(const char * const nickname);
void ircd_send_whois(const char * const message, const char * nickname);

int valid_vhost(char *hostname) {
   int i = 0;
   if(hostname) {
      while(hostname[i]) {
         if(!isalnum(hostname[i]) && hostname[i] != '.' && hostname[i] != '-' && hostname[i] != ':' && hostname[i] != '_') {
            break;
         }
         i++;
      }
      if(hostname[i] && i < 63) {
         return 0;
      }
      return 1;
   }
   return 0;
}

int mask_is_banned(IRC_Channel *channel, const char * const mask) {
   Channel_Ban *ban;
   if(channel) {
      ban = channel->bans;
      while(ban) {
         if(strpbrk(ban->hostmask, "*?")) {
            if(wildcard_compare(ban->hostmask, mask)) {
               return 1;
            }
         }
         else
         {
            if(case_compare(ban->hostmask, mask)) {
               return 1;
            }
         }
      }
   }
   return 0;
}


void mode_drop(modechange **modes, char mode, char *param) {
   modechange *prev;
   modechange *p = *modes;
   if(modes) {
      prev = *modes;
      while(p != 0) {
         if(p->modechar == mode && !strcmp(p->settings, param)) {
            if(p == *modes) {
               p = p->next;
               free(*modes);
               *modes = p;
            }
            else
            {
               prev->next = p->next;
               free(p);
            }
            break;
         }
         prev = p;
         p = p->next;
      }
   }
}

void mode_add(modechange **modes, char modechar, char *param, unsigned int remove) {
   modechange *p = *modes;
   modechange *next;
   int change = 1;
   if(*modes == 0) {
      *modes = malloc(sizeof(modechange));
      p = *modes;
   }
   else
   {
      while(p != 0 && change == 1) {
         next = p->next;
         if(modechar == p->modechar && (param == 0 || !strcmp(param, p->settings))) {
            change = 0;
            if(p->remove == remove) {
               change = 0;
            }
            else
            {
               mode_drop(modes, modechar, param);
               change = 0;
            }
         }
         if(next != 0) {
            p = next;
         }
         else
         {
            break;
         }
      }
      if(change) {
         p->next = malloc(sizeof(modechange));
         p = p->next;
      }
   }
   if(change) {
      memset(p, 0, sizeof(modechange));
      p->modechar = modechar;
      if(param) {
         strncpy_safe(p->settings, param, 64);
      }
      p->remove = remove;
   }
}

static void handle_bantype(IRC_Channel *channel, char *ban, char mode) {
   if(channel && ban) {
      if(*ban == '~') {
         ban++;
         channel_add_ban(channel, ban + 2, remote_server->name, mode, *ban);
         ban--;
      }
      else
      {
         channel_add_ban(channel, ban, remote_server->name, mode, 0);
      }
   }
}

static void handle_chanmode(IRC_Channel *channel, const char * const sender, const char mode, char *params, const int remove) {
   unsigned long limit = 0;
   switch(mode) {
      case 'l':
         if(remove) {
            channel_user_limit(channel, 0);
         }
         else
         {
            limit = strtoul(params, &params, 10);
            channel_user_limit(channel, limit);
         }
      break;
      case 'k':
         if(remove) {
            channel_password(channel, (char *)0);
         }
         else
         {
            channel_password(channel, params);
         }
      break;
      case 'o':
         fire_event("channel_user_operator", 4, channel->name, params, sender, remove);
         channel_operator(channel, params, remove);
      break;
      case 'a':
         fire_event("channel_user_admin", 4, channel->name, params, sender, remove);
         channel_admin(channel, params, remove);
      break;
      case 'q':
         fire_event("channel_user_owner", 4, channel->name, params, sender, remove);
         channel_owner(channel, params, remove);
      break;
      case 'h':
         fire_event("channel_user_halfop", 4, channel->name, params, sender, remove);
         channel_halfop(channel, params, remove);
      break;
      case 'v':
         fire_event("channel_user_voice", 4, channel->name, params, sender, remove);
         channel_voice(channel, params, remove);
      break;
      case 'b':
         fire_event("channel_ban", 4, channel->name, params, sender, remove);
         if(remove) {
            handle_bantype(channel, params, 'b');
         }
      break;
      case 'e':
         fire_event("channel_ban_exception", 4, channel->name, params, sender, remove);
         if(remove) {
            handle_bantype(channel, params, 'e');
         }
      break;
      case 'I':
         fire_event("channel_invite_exception", 4, channel->name, params, sender, remove);
         if(remove) {
            handle_bantype(channel, params, 'I');
         }
      break;
      default:
         if(remove) {
            channel_delete_mode(channel, mode);
         }
         else
         {
            channel_add_mode(channel, mode);
         }
         fire_event("channel_mode", 4, channel->name, mode, sender, params, remove);
      break;
   }
}
void ircd_chanmode(const char * const sender, const char * const target, const char * const modes, const char * const parameter) {
   unsigned int remove = 0;
   int param = 1;
   const char *p = modes;
   char *token = 0;
   modechange *modechanges = 0;
   modechange *p_modes;
   modechange *p_next;
   IRC_Channel *channel = 0;
   if(remote_server->synching != 0) {
      channel = get_channel(target);
   }
   while(*p != 0) {
      if(*p == '+') {
         remove = 0;
      }
      else if(*p == '-') {
         remove = 1;
      }
      else
      {
         if(strchr(ircd.modeparams, *p) || (remove == 0 && strchr(ircd.modeset, *p))) {
            token = get_token(param, parameter, " ");
            param++;
         }
         if(*p != '+' && *p != '-') {
            if(remote_server->synching == 0) {
               mode_add(&modechanges, *p, token, remove);
            }
            else
            {
               handle_chanmode(channel, sender, *p, token, remove);
            }
         }
      }
      if(token) {
         free(token);
         token = 0;
      }
      p++;
   }
   if(modechanges) {
      channel = get_channel(target);
      p_modes = modechanges;
      p_next = modechanges->next;
      while(p_modes != 0) {
         handle_chanmode(channel, sender, p_modes->modechar, p_modes->settings, remove);
         p_next = p_modes->next;
         free(p_modes);
         p_modes = p_next;
      }
   }
}

static void ircd_serverlink(const char * const buffer) {
   char *server_name;
   char *description;
   server_name = get_token(2, buffer, " ");
   if(!ircd.vl) {
      description = get_token_remainder(4, buffer, " ");
   }
   else
   {
      description = get_token_remainder(5, buffer, " ");
   }
   if(server_name) {
      strncpy_safe(remote_server->name, server_name, 64);
      free(server_name);
   }
   if(description) {
      strncpy_safe(remote_server->description, description, 250);
      free(description);
   }
}

static void ircd_capab(const char * const input) {
   int i;
   char *opt;
   char *data;
   strncpy_safe(ircd.modeparams, "vohaq", 6);
   opt = get_token(2, input, " ");
   for(i = 3; opt != NULL; i++) {
      if(strcmp(opt, "TOKEN") == 0) {
         ircd.tokens = 1;
      }
      else if(strcmp(opt, "UMODE2") == 0) {
         ircd.umode2 = 1;
      }
      else if(case_compare(opt, "NICKv2")) {
         ircd.nickv2 = 1;
      }
      else if(case_compare(opt, "VL")) {
         ircd.vl = 1;
      }
      else if(case_compare(opt, "NS")) {
         ircd.vl = 1;
         ircd.ns = 1;
      }
      else if(strstr(opt, "CHANMODES=") == opt) {
         data = get_token(1, opt + 10, ",");
         if(data) {
            strncat(ircd.modeparams, data, 64 - strlen(ircd.modeparams));
            free(data);
         }
         data = get_token(2, opt, ",");
         if(data) {
            strncat(ircd.modeparams, data, 64 - strlen(ircd.modeparams));
            free(data);
         }
         data = get_token(3, opt, ",");
         if(data) {
            strncpy_safe(ircd.modeset, data, 64);
            free(data);
         }
         data = get_token(4, opt, ",");
         if(data) {
            strncpy_safe(ircd.modenormal, data, 64);
            free(data);
         }
      }
      free(opt);
      opt = get_token(i, input, " ");
   }
}

static char *ircd_token(const char * const input) {
   if(!ircd.tokens || !input) {
      return (char *)input;
   }
   if(*input == 'P') {
      if(strcmp(input, "PRIVMSG") == 0) {
         return "!";
      }
      else if(strcmp(input, "PING") == 0) {
         return "8";
      }
      else if(strcmp(input, "PART") == 0) {
         return "D";
      }
      else if(strcmp(input, "PONG") == 0) {
         return "9";
      }
   }
   else if(*input == 'S') {
      if(*(input + 1) == 'V') {
         if(strcmp(input, "SVSNICK") == 0) {
            return "e";
         }
         else if(strcmp(input, "SVSKILL") == 0) {
            return "h";
         }
         else if(strcmp(input, "SVSJOIN") == 0) {
            return "BX";
         }
         else if(strcmp(input, "SVSPART") == 0) {
            return "BT";
         }
      }
      else if(strcmp(input, "SJOIN") == 0) {
         return "~";
      }
      else if(*(input + 1) == 'E') {
         if(strcmp(input, "SETHOST") == 0) {
            return "AA";
         }
         else if(strcmp(input, "SETIDENT") == 0) {
            return "AD";
         }
         else if(strcmp(input, "SETNAME") == 0) {
            return "AE";
         }
      }
      else if(strcmp(input, "SWHOIS") == 0) {
         return "BA";
      }
      else if(strcmp(input, "SDESC") == 0) {
         return "AG";
      }
   }
   else if(*input == 'C') {
      if(strcmp(input, "CHGHOST") == 0) {
         return "AL";
      }
      else if(strcmp(input, "CHGIDENT") == 0) {
         return "AZ";
      }
      else if(strcmp(input, "CHGNAME") == 0) {
         return "BK";
      }
   }
   else if(*input == 'N') {
      if(strcmp(input, "NICK") == 0) {
         return "&";
      }
      else if(strcmp(input, "NETINFO") == 0) {
         return "AO";
      }
   }
   else if(*input == 'K') {
      if(strcmp(input, "KICK") == 0) {
         return "H";
      }
      else if(strcmp(input, "KILL") == 0) {
         return ".";
      }
   }
   else if(*input == 'I') {
      if(strcmp(input, "INVITE") == 0) {
         return "*";
      }
      else if(strcmp(input, "INFO") == 0) {
         return "/";
      }
   }
   else if(strcmp(input, "MODE") == 0) {
      return "G";
   }
   else if(strcmp(input, "UMODE2") == 0) {
      return "|";
   }
   else if(strcmp(input, "JOIN") == 0) {
      return "C";
   }
   else if(strcmp(input, "TOPIC") == 0) {
      return ")";
   }
   else if(strcmp(input, "QUIT") == 0) {
      return ",";
   }
   else if(strcmp(input, "WHOIS") == 0) {
      return "#";
   }
   else if(strcmp(input, "VERSION") == 0) {
      return "+";
   }
   else if(strcmp(input, "EOS") == 0) {
      return "ES";
   }
   return (char *)input;
}

static int match_token(const char * const in, const char * const token) {
   char *tokenised = 0;
   if(ircd.tokens) {
      tokenised = ircd_token(token);
      if(strcmp(tokenised, in) == 0) {
         return 1;
      }
   }
   else
   {
      if(strcmp(token, in) == 0) {
         return 1;
      }
   }
   return 0;
}

static void handle_join(const char * const str_user, const char *channels) {
   char channel[CHANNEL_MAX];
   User *user;
   IRC_Channel *irc_channel;
   if(str_user && channels) {
      user = get_user(str_user);
      while(*channels != '\0') {
         channels += copy_to(channel, channels, ',', CHANNEL_MAX);
         if(*channel != '\0') {
            if(*channel == '0') {
               channel_partall(str_user);
            }
            else
            {
               irc_channel = get_channel(channel);
               if(!irc_channel) {
                  irc_channel = add_channel(channel);
               }
               if(remote_server->synching == 0) {
                  fire_event("channel_user_join", 2, channel, user->nick);
               }
               else
               {
                  fire_event("channel_user_join_burst", 2, channel, user->nick);
               }
               channel_add_user(irc_channel, user);
            }
         }
      }
      
   }
}

static void ircd_privmsg(const char * const sender, const char * const buffer) {
   char target[TARGET_MAX];
   char *find;
   find = strchr(buffer, ' ');
   if(find) {
      find = strchr(find + 1, ' ');
   }
   if(find && find[1]) {
      copy_to(target, find + 1, ' ', TARGET_MAX);
      find = strstr(find + 1, ":");
      if(find) {
         process_message(sender + 1, target, find + 1);
      }
   }
}

static void ircd_join(const char * const buffer) {
   char *target;
   char *channels;
   target = get_token(1, buffer, " ");
   channels = get_token_remainder(3, buffer, " ");
   handle_join(target + 1, channels);
   if(channels) {
      free(channels);
   }
   if(target) {
      free(target);
   }
}

/*
   Oh hell. 
*/
static void ircd_sjoin(const char * const buffer) {
   IRC_Channel *channelptr;
   char *channel;
   char *item = 0;
   char *modes = 0;
   char *marker;
   char *marker2;
   int token = 6;
   channel = get_token(4, buffer, " ");
   if(channel) {
      channelptr = get_channel(channel);
      if(!channelptr) {
         add_channel(channel);
         channelptr = get_channel(channel);
      }
      if(channelptr) {
         modes = get_token(5, buffer, " ");
         if(*modes == '+') {
            marker = modes + 1;
            while(*marker != '\0') {
               if(strchr(ircd.modeparams, *marker) || strchr(ircd.modeset, *marker)) {
                  item = get_token(token, buffer, " ");
                  token++;
               }
               handle_chanmode(channelptr, remote_server->name, *marker, item, 0);
               if(item) {
                  free(item);
                  item = 0;
               }
               marker++;
            }
            item = get_token(token, buffer, " ");
         }
         else
         {
            item = modes;
            modes = 0;
         }
         while(item) {
            if(*item == '"') {
               item++;
               handle_bantype(channelptr, item, 'e');
               item--;
            }
            else if(*item == '\'') {
               item++;
               handle_bantype(channelptr, item, 'I');
               item--;
            }
            else if(*item == '&') {
               item++;
               handle_bantype(channelptr, item, 'b');
               item--;
            }
            else
            {
               marker = item;
               if(*marker == ':') {
                  marker++;
               }
               marker = skip_chars(marker, "@+%*~");
               handle_join(marker, channel);
               marker2 = item;
               if(*marker2 == ':') {
                  marker2++;
               }
               while(marker2 != marker) {
                  switch(*marker2) {
                     case '+':
                        handle_chanmode(channelptr, remote_server->name, 'v', marker, 0);
                     break;
                     case '%':
                        handle_chanmode(channelptr, remote_server->name, 'h', marker, 0);
                     break;
                     case '@':
                        handle_chanmode(channelptr, remote_server->name, 'o', marker, 0);
                     break;
                     case '~':
                        handle_chanmode(channelptr, remote_server->name, 'a', marker, 0);
                     break;
                     case '*':
                        handle_chanmode(channelptr, remote_server->name, 'q', marker, 0);
                     break;
                  }
                  marker2++;
               }
            }
            free(item);
            token++;
            item = get_token(token, buffer, " ");
         }
      }
   }
   if(item) {
      free(item);
   }
   if(modes) {
      free(modes);
   }
   if(channel) {
      free(channel);
   }
}

void ircd_send_kick(const char * const source, const char * const channel, const char * const target, const char * const reason) {
   char response[IRCLINE_MAX];
   snprintf(response, IRCLINE_MAX, ":%s %s %s %s %s", source, ircd_token("KICK"), channel, target, reason);
   fire_event("user_kick", 1, target);   
   delete_channel_user(channel, target);
   remote_server->send(remote_server->Socket, response);
}

void ircd_send_kill(const char * const source, const char * const target, const char * const reason, const int svskill) {
   char response[IRCLINE_MAX];
   if(svskill == 0) {
      snprintf(response, IRCLINE_MAX, ":%s %s %s %s (%s)", source, ircd_token("KILL"), target, source, reason);
   }
   else
   {
      snprintf(response, IRCLINE_MAX, ":%s %s %s %s", source, ircd_token("SVSKILL"), target, reason);
   }
   fire_event("user_disconnect", 1, target);
   drop_user(target);
   remote_server->send(remote_server->Socket, response);
}

/* 
   If multiple servers send the synch to us, we're going to have a problem 
   Test and fix as needed. 
*/

void ircd_send_synch(const char * const in) {
   if(in) {
      fire_event("server_synch", 1, remote_server->name);
      remote_server->synching = 0;
   }
}

void ircd_send_pong(const char * in) {
   char pong[65];
   char response[IRCLINE_MAX];
   in = strchr(in, ' ');
   copy_to(pong, in + 1, ' ', 65);
   sprintf(response, "%s %s", ircd_token("PONG"), pong);
   remote_server->send(remote_server->Socket, response);
}

void ircd_send_nick(const char * const source, const char * const target, const char * const new_nickname) {
   char response[IRCLINE_MAX];
   sprintf(response, ":%s %s %s %s :%ld", source, ircd_token("SVSNICK"), target, new_nickname, export_unix_timestamp(time(NULL)));
   remote_server->send(remote_server->Socket, response);
}

void ircd_send_new_nick(const char * const nickname, const char * const username,  const char * const hostname,  const char * const server, const char * const realname) {
   char response[IRCLINE_MAX];
   sprintf(response, "%s %s %d %ld %s %s %s %d :%s", ircd_token("NICK"), nickname, 1, export_unix_timestamp(time(NULL)), username, hostname, server, 0, realname);
   remote_server->send(remote_server->Socket, response);
}

void ircd_send_new_host(const char * const sender, const char * const nickname, const char * const hostname) {
   char response[IRCLINE_MAX];
   if(*hostname) {
      sprintf(response, ":%s %s %s %-.63s", sender, ircd_token("CHGHOST"), nickname, hostname);
      remote_server->send(remote_server->Socket, response);
   }
   else
   {
      ircd_send_modes(sender, nickname, "-xt+x", (char *)0);
   }
}

void ircd_send_new_ident(const char * const nickname, const char * const ident) {
   char response[IRCLINE_MAX];
   sprintf(":%s %s %s", nickname, ircd_token("SETIDENT"), ident);
   remote_server->send(remote_server->Socket, response);
}

void ircd_usermode(const char * const sender, const char * const target,  const char * const modes) {
   int remove = 0;
   const char *p = modes;
   User *user = 0;
   if(sender) {
      user = get_user(target);
   }
   if(user) {
      while(*p != 0) {
         if(*p == '+') {
            remove = 0;
         }
         else if(*p == '-') {
            remove = 1;
         }    
         else if(*p == 'o') {
            user_ircop(user, remove);
            fire_event("user_oper", 3, target, sender, remove);
         }
         else if(remove == 0 && *p == 'r') {
            if(!auto_identify(user)) {
               ircd_send_modes(ezri->name, user->nick, "-r", (char *)0);
            }
         }
         else
         {
            user_mode(user, *p, remove);
            fire_event("user_mode", 4, target, sender, p, remove);
         }
         p++;
      }
   }
}

void ircd_new_nick(const char * const buffer) {
   char *nickname;
   time_t date;
   char *ident;
   char *mem;
   char *modes;
   char *hostname;
   char *service_stamp;
   char *realname;
   User *user;
   nickname = get_token(2, buffer, " ");
   mem = get_token(3, buffer, " ");
   if(nickname) {
      mem = get_token(4, buffer, " ");
      if(mem) {
         date = import_unix_timestamp(mem);
         ident = get_token(5, buffer, " ");
         hostname = get_token(6, buffer, " ");
         service_stamp = get_token(7, buffer, " ");
         if(!ircd.nickv2) {
            realname = get_token_remainder(9, buffer, " ");
         }
         else
         {
            realname = get_token_remainder(11, buffer, " ");
         }
         if(strlen(nickname) <= NICK_MAX) {
            user = add_user(nickname, ident, hostname, realname + 1, date);
            if(!remote_server->synching) {
               fire_event("user_connect", 1, user->nick);
            }
            if(ircd.nickv2) {
               modes = get_token(9, buffer, " ");
               ircd_usermode(nickname, nickname, modes);
               if(modes) {
                  free(modes);
               }
            }
         }
         else
         {
            log_message("Nickname %s is longer than NICK_MAX, ignored.\n", LOG_CRITICAL);
         }
         if(ident) {
            free(ident);
         }
         if(hostname) {
            free(hostname);
         }
         if(service_stamp) {
            free(service_stamp);
         }
         if(realname) {
            free(realname);
         }
      }
   }
   if(nickname) {
      free(nickname);
   }
   if(mem) {
	   free(mem);
   }
}

static void ircd_nick(const char * const buffer) {
   char *sender;
   char *new_sender;
   User *user;
   sender = get_token(1, buffer, " ");
   new_sender = get_token(3, buffer, " ");
   user = user_new_nickname(sender + 1, new_sender);
   if(user) {
      fire_event("user_change_nick", 2, sender + 1, new_sender);
   }
   if(sender) {
      free(sender);
   }
   if(new_sender) {
      free(new_sender);
   }
} 

static void ircd_topicburst(const char * const buffer) {
   char *sender;
   char *channel;
   char *timestamp;
   char *topic;
   IRC_Channel *channeldata;
   channel = get_token(2, buffer, " ");
   sender = get_token(3, buffer, " ");
   timestamp = get_token(4, buffer, " ");
   if(channel && sender && timestamp) {
      topic = get_token_remainder(5, buffer, " ");
      channeldata = get_channel(channel);
       if(channeldata) {
         channel_add_topic(channeldata, topic + 1);
         channel_add_topicdata(channeldata, sender, timestamp);
      }
      fire_event("channel_topic_burst", 4, sender + 1, channel, timestamp, topic + 1);
      if(topic) {
         free(topic);
      }
   }
   if(channel) {
      free(channel);
   }
   if(sender) {
      free(sender);
   }
   if(timestamp) {
      free(timestamp);
   }
}

static void ircd_topic(const char * const buffer) {
   char *sender;
   char *channel;
   char *nickhost;
   char *timestamp;
   char *topic;
   IRC_Channel *channeldata;
   sender = get_token(1, buffer, " ");
   channel = get_token(3, buffer, " ");
   nickhost = get_token(4, buffer, " ");
   timestamp = get_token(5, buffer, " ");
   topic = get_token_remainder(6, buffer, " ");
   channeldata = get_channel(channel);
   if(channeldata) {
      channel_add_topic(channeldata, topic + 1);
      channel_add_topicdata(channeldata, sender, timestamp);
   }
   fire_event("channel_topic_change", 3, sender, channel, topic + 1);
   if(sender) {
      free(sender);
   }
   if(channel) {
      free(channel);
   }
   if(nickhost) {
      free(nickhost);
   }
   if(timestamp) {
      free(timestamp);
   }
   if(topic) {
      free(topic);
   }
}

static void ircd_quit(const char * const buffer) {
   char *nickname;
   char *message;
   nickname = get_token(1, buffer + 1, " ");
   message = get_token_remainder(3, buffer, " ");
   fire_event("user_disconnect", 1, nickname);
   drop_user(nickname);
   if(nickname) {
      free(nickname);
   }
   if(message) {
      free(message);
   }
}

static void ircd_part(const char * const buffer) {
   char *nickname;
   char *channel;
   char *message;
   nickname = get_token(1, buffer + 1, " ");
   channel = get_token(3, buffer, " ");
   message = get_token_remainder(4, buffer, " ");
   if(nickname && channel && message) {
      fire_event("channel_user_part", 3, channel, nickname, message + 1);
      delete_channel_user(channel, nickname);
   }
   else
   {
      fire_event("channel_user_part", 3, channel, nickname, NULL);
      delete_channel_user(channel, nickname);
   }
   if(nickname) {
      free(nickname);
   }
   if(channel) {
      free(channel);
   }
   if(message) {
      free(message);
   }
}

static void ircd_kick(const char * const in) {
   char *nickname;
   char *target;
   char *channel;
   nickname = get_token(1, in + 1, " ");
   channel = get_token(3, in, " ");
   target = get_token(4, in, " ");
   fire_event("channel_user_kick", 3, channel, target, nickname);
   delete_channel_user(channel, nickname);
   if(nickname) {
      free(nickname);
   }
   if(channel) {
      free(channel);
   }
   if(target) {
      free(target);
   }
}

static void ircd_mode(const char * const buffer) {
   char *sender;
   char *type;
   char *target;
   char *modes;
   char *parameter;
   sender = get_token(1, buffer + 1, " ");
   type = get_token(2, buffer, " ");
   if(type && !match_token(type, "UMODE2")) {
      target = get_token(3, buffer, " ");
      modes = get_token(4, buffer, " ");
      parameter = get_token_remainder(5, buffer, " ");
   }
   else
   {
      target = sender;
      modes = get_token(3, buffer, " ");
      parameter = get_token_remainder(4, buffer, " "); 
   }
   if(*target == '#') {
      ircd_chanmode(sender, target, modes, parameter);
   }
   else
   {
      ircd_usermode(sender, target, modes);
   }
   if(target != sender) {
      if(target) {
         free(target);
      }
      if(sender) {
         free(sender);
      }
   }
   else if(sender) {
      free(sender);
   }
   if(modes) {
      free(modes);
   }
   if(parameter) {
      free(parameter);
   }
}

static void ircd_invite(const char * const in) {
   char *nickname;
   char *target;
   char *channel;
   nickname = get_token(1, in + 1, " ");
   target = get_token(3, in, " ");
   channel = get_token(4, in, " ");
   if(channel) {
      fire_event("channel_invite", 3, target, channel + 1, nickname);
      free(channel);
   }
   if(target) {
      free(target);
   }
   if(nickname) {
      free(nickname);
   }
}

static void ircd_sethost(const char * const in) {
   char *source;
   char *host;
   source = get_token(1, in, " ");
   host = get_token(3, in, " ");
   if(source && host) {
      user_new_vhost(source + 1, host);
   }
   if(source) {
      free(source);
   }
   if(host) {
      free(host);
   }
}

static void ircd_setident(const char * const in) {
   char *source;
   char *ident;
   source = get_token(1, in + 1, " ");
   ident = get_token(3, in, " ");
   if(source && ident) {
      user_new_ident(source, ident);
   }
   if(source) {
      free(source);
   }
   if(ident) {
      free(ident);
   }
}

static void ircd_setname(const char * const in) {
   char *source;
   char *realname;
   source = get_token(1, in + 1, " ");
   realname = get_token(3, in, " ");
   if(source && realname) {
      user_new_realname(source, realname);
   }
   if(source) {
      free(source);
   }
   if(realname) {
      free(realname);
   }
}

static void ircd_chghost(const char * const in) {
   char *target;
   char *host;
   target = get_token(3, in, " ");
   host = get_token(4, in, " ");
   if(target && host) {
      user_new_vhost(target, host);
   }
   if(target) {
      free(target);
   }
   if(host) {
      free(host);
   }
}

static void ircd_chgident(const char * const in) {
   char *target;
   char *ident;
   target = get_token(3, in, " ");
   ident = get_token(4, in, " ");
   if(target && ident) {
      user_new_ident(target, ident);
   }
   if(target) {
      free(target);
   }
   if(ident) {
      free(ident);
   }
}

static void ircd_chgname(const char * const in) {
   char *target;
   char *realname;
   target = get_token(3, in, " ");
   realname = get_token(4, in, " ");
   if(target && realname) {
      user_new_realname(target, realname);
   }
   if(target) {
      free(target);
   }
   if(realname) {
      free(realname);
   }
}

void ircd_send_init(const char * const name, const char * const pass, const char * const description) {
   char response[513];
   sprintf(response, "PASS :%s", pass);
   remote_server->send(remote_server->Socket, response);
   remote_server->send(remote_server->Socket,"PROTOCTL NOQUIT TOKEN NICKv2 UMODE2 VHP VL NS SJOIN SJ3");
   sprintf(response, "SERVER %s 1 :%s", name, description);
   fire_event("server_handshake", 0);
   remote_server->send(remote_server->Socket, response);
}

void ircd_send_privmsg(const char * const sender, const char * const target, const char * const message) {
   char input[IRCLINE_MAX];
   snprintf(input, IRCLINE_MAX, ":%s %s %s :%s", sender, ircd_token("PRIVMSG"), target, message);
   remote_server->send(remote_server->Socket, input);
}

void ircd_send_notice(const char * const sender, const char * const target, const char * const message) {
   char input[IRCLINE_MAX];
   snprintf(input, IRCLINE_MAX, ":%s %s %s :%s", sender, ircd_token("NOTICE"), target, message);
   remote_server->send(remote_server->Socket, input);
}

void ircd_send_join(const char * const sender, const char * const target, const char * const channel, const int service) {
   char input[IRCLINE_MAX];
   if(service) {
      add_channel_bot(channel, target);
      snprintf(input, IRCLINE_MAX, ":%s %s %s", target, ircd_token("JOIN"), channel);
      fire_event("channel_svsuser_join", 2, channel, target);
   }
   else
   {
      snprintf(input, IRCLINE_MAX, ":%s %s %s %s", sender, ircd_token("SVSJOIN"), target, channel);
   }
   remote_server->send(remote_server->Socket, input);
}

void ircd_send_part(const char * const sender, const char * const target, const char * const reason) {
   char input[IRCLINE_MAX];
   if(reason != 0) {
      snprintf(input, IRCLINE_MAX, ":%s %s %s :%s", sender, ircd_token("PART"), target, reason);
   }
   else
   {
      snprintf(input, IRCLINE_MAX, ":%s %s %s", sender, ircd_token("PART"), target);
   }

   remote_server->send(remote_server->Socket, input);
}

void ircd_send_invite(const char * const sender, const char * const nickname, const char * const target) {
   char input[IRCLINE_MAX];
   sprintf(input, ":%s %s %s %s", sender, ircd_token("INVITE"), nickname, target);
   remote_server->send(remote_server->Socket, input);
}

void ircd_send_modes(const char * const sender, const char * const target, const char * const modes, const char * const params) {
   char input[IRCLINE_MAX];
   if(strcspn(target,"#") == 0) {
      if(params) {
         sprintf(input, ":%s %s %s %s %s", sender, ircd_token("MODE"), target, modes, params);
      }
      else
      {
         sprintf(input, ":%s %s %s %s", sender, ircd_token("MODE"), target, modes);
      }
      ircd_chanmode(sender, target, modes, params);
    }
    else
    {
       if(params) {
          sprintf(input, ":%s %s %s %s %s", sender, ircd_token("SVS2MODE"), target, modes, params);
       }
       else
       {
            sprintf(input, ":%s %s %s %s", sender, ircd_token("SVS2MODE"), target, modes);
       }
       ircd_usermode(sender, target, modes);
    }
    remote_server->send(remote_server->Socket, input);
}

void ircd_send_version(const char * const nickname) {
   char input[IRCLINE_MAX];
   snprintf(input, IRCLINE_MAX, ":%s 351 %s %s", ezri->name, nickname + 1, "Ezri Services v" EZRI_VERSION "-" EZRI_RELEASE " @ http://www.team-ezri.org/");
   remote_server->send(remote_server->Socket, input);
}

void ircd_send_whois(const char * const message, const char * nickname) {
   char *target;
   char buffer[IRCLINE_MAX];
   Services_User *client;
   target = get_token(3, message, " ");
   if(target) {
      client = get_svsclient(target);
      if(client) {
         snprintf(buffer, IRCLINE_MAX, "311 %s %s %s %s * :%s", nickname, client->nick, client->username, client->hostname, client->realname);
         remote_server->send(remote_server->Socket, buffer);
         snprintf(buffer, IRCLINE_MAX, "378 %s %s :is connecting from *@%s", nickname, client->nick, client->hostname);
         remote_server->send(remote_server->Socket, buffer);
      }
      else
      {
         snprintf(buffer, IRCLINE_MAX, "378 %s %s :is connecting from *@%s", nickname, client->nick, client->hostname);
         remote_server->send(remote_server->Socket, buffer);
      }
      snprintf(buffer, IRCLINE_MAX, "318 %s %s :End of /WHOIS list.", nickname, target);
      remote_server->send(remote_server->Socket, buffer);
      free(target);
   }
}
int parse_message(const char * const message) {
   static char word[NICK_MAX + 1];
   static char token[NICK_MAX + 1];
   char *tokenised = 0;
   if(!message) {
      return 0;
   }
   copy_to(word, message, ' ', TARGET_MAX + 1);
   if(*word == '\0') {
      return 0;
   }
   if(*word != '\0' && (*word != ':' && *word != '@')) {
      tokenised = ircd_token(word);
      if(tokenised) {
         if(match_token(tokenised, "PING")) {
            ircd_send_pong(message);
         }
         else if(match_token(tokenised, "NICK")) {
            ircd_new_nick(message);
         }
         else if(match_token(tokenised, "TOPIC")) {
            ircd_topicburst(message);
         }
         else if(match_token(tokenised, "SWHOIS")) {
         }
         /* Never tokenised */
         else if(strcmp(word, "PROTOCTL") == 0) {
            ircd_capab(message);
         }
         else if(strcmp(word, "SERVER") == 0) {
            if(remote_server->synching == 1) {
               ircd_serverlink(message);
            }
         }
         else if(strcmp(tokenised, ircd_token("NETINFO")) == 0) {
         }
      }
   }
   else
   {
      if(strlen(message) > strlen(word) + 1) {
         copy_to(token, message + strlen(word) + 1, ' ', NICK_MAX + 1);
         tokenised = ircd_token(token);
         if(match_token(tokenised, "PRIVMSG")) {
            ircd_privmsg(word, message);
         }
         else if(match_token(tokenised, "JOIN")) {
            ircd_join(message); 
         }
         else if(match_token(tokenised, "QUIT")) {
            ircd_quit(message);
         }
         else if(match_token(tokenised, "PART")) {
            ircd_part(message);
         }
         else if(match_token(tokenised, "NICK")) {
            ircd_nick(message);
         }
         else if(match_token(tokenised, "MODE") || match_token(tokenised, "UMODE2")) {
            ircd_mode(message);
         }
         else if(match_token(tokenised, "TOPIC")) {
            ircd_topic(message);
         }
         else if(match_token(tokenised, "KICK")) {
            ircd_kick(message);
         }
         else if(match_token(tokenised, "INVITE")) {
            ircd_invite(message);
         }
         else if(match_token(tokenised, "SJOIN")) {
            ircd_sjoin(message);
         }
         else if(match_token(tokenised, "CHGHOST")) {
            ircd_chghost(message);
         }
         else if(match_token(tokenised, "CHGIDENT")) {
            ircd_chgident(message);
         }
         else if(match_token(tokenised, "CHGNAME")) {
            ircd_chgname(message);
         }
         else if(match_token(tokenised, "CHGIDENT")) {
            ircd_setident(message);
         }
         else if(match_token(tokenised, "SETNAME")) {
            ircd_setname(message);
         }
         else if(match_token(tokenised, "SETHOST")) {
            ircd_sethost(message);
         }
         else if(match_token(tokenised, "WHOIS")) {
            ircd_send_whois(message, word + 1);
         }
         else if(match_token(tokenised, "VERSION")) {
            ircd_send_version(word);
         }
         else if(remote_server->synching && match_token(tokenised, "EOS")) {
            ircd_send_synch(message);
         }
         else
         {
            printf("%s (%s) is currently not handled.\n", word, token);
         }
      }
   }
   return 1;
}

int extension_load(void) {
   Server_Connect *commands;
   commands = malloc(sizeof(Server_Connect));
   if(commands) {
      memset(commands, 0, sizeof(Server_Connect));
      commands->synching = 1;
      commands->init = ircd_send_init;
      commands->main = parse_message;
      commands->read = socket_readline;
      commands->send = socket_sendline;
      commands->part = ircd_send_part;
      commands->privmsg = ircd_send_privmsg;
      commands->notice = ircd_send_notice;
      commands->modes = ircd_send_modes;
      commands->invite = ircd_send_invite;
      commands->kill = ircd_send_kill;
      commands->new_nick = ircd_send_new_nick;
      commands->change_nick = ircd_send_nick;
      commands->join = ircd_send_join;
      commands->new_host = ircd_send_new_host;
      commands->kick = ircd_send_kick;
      add_extension_data(get_extension(EXTENSION_NAME), "protocol", commands);
   }
   return 1;
}

int extension_unload(void) {
   return 1;
}
