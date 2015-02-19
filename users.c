#include "headers/main.h"
#include "headers/users.h"
#include "headers/channels.h"
#include "headers/commands.h"
#include "headers/sockets.h"
#include "headers/servers.h"
#include "headers/config.h"

static void delete_client(Services_User *client) {
   free(client);
}

int introduce_users(void **args) {
   Services_User *client = svslist;
   int clients = 0;
   /* This if currently just satisfies the compiler
   Eventually include the server name for comparison */
   if(args) {
      while(client) {
         if(client->connected == 0) {
            add_svsclient(client);
         }
         client = client->next;
         clients++;
      }
   }
   return clients;
}

Services_User *load_svsuser(const char * const nickname, const char * const username, const char * const hostname, const char * const realname, const char * const type) {
   Services_User *client = svslist;
   if(client != 0) {
      while(client->next != 0) {
         client = client->next;
      }
      client->next = malloc(sizeof(Services_User));
      client = client->next;
   }
   else
   {
      svslist = malloc(sizeof(Services_User));
      client = svslist;
   }
   if(nickname && hostname && realname && type) {
      if(client) {
         memset(client, 0, sizeof(Services_User));
         strncpy_safe(client->nick, nickname, NICK_MAX);
         strncpy_safe(client->hostname, hostname, HOST_MAX);
         strncpy_safe(client->realname, realname, 64);
         strncpy_safe(client->type, type, 16);
         if(username) {
            strncpy_safe(client->username, username, 10);
         }
         else
         {
            strncpy_safe(client->username, nickname, 10);
         }
         return client;
      }
   }
   else
   {
      delete_client(client);
   }
   return 0;
}

void client_switch_routine(Services_User *client, const char * const routine) {
   svs_routine *client_routine;
   if(client) {
      client_routine = get_extension_function(routine);
      if(client_routine) {
         client->main_routine = *client_routine;
         strncpy_safe(client->routine_name, routine, 32);
      }
   }
}

void client_set_trigger(Services_User *client, const char * const trigger) {
   strncpy_safe(client->trigger, trigger, 5);
}

void drop_client(const char * const client) {
   Services_User *prev;
   Services_User *p = svslist;
   if(svslist) {
      prev = svslist;
      while(p != 0) {
         if(case_compare(p->nick, client)) {
            if(p == svslist) {
               svslist = p->next;
               delete_client(p);
            }
            else
            {
               if(p->next) {
                  prev->next = p->next;
               }
               delete_client(p);
            }
            break;
         }
         prev = p;
         p = p->next;
      }
   }
}

int add_svsclient(Services_User *client) {
   if(!client || !client->nick || !client->hostname || !client->username) {
      return 0;
   }
   client->connected = time(NULL);
   remote_server->new_nick(client->nick, client->username, client->hostname, ezri->name, client->realname);
   return 1;
}

int is_svsclient(const char * const in) {
   Services_User *p;
   size_t length;
   unsigned int is_match = 0;
   if(!in) {
      return 0;
   }
   p = svslist;
   length = strcspn(in, "@");
   while(p != 0) {
      if(p->nick) {
         if(case_compare_length(p->nick, in, length)) {
            is_match = 1;
            break;
         }
      }
      p = p->next;
   }
   return is_match;
}

Services_User *get_svsclient(const char * const in) {
   Services_User *p;
   size_t length;
   unsigned int is_match = 0;
   if(!in) {
      return 0;
   }
   p = svslist;
   length = strcspn(in, "@");
   while(p != 0) {
      if(p->nick && case_compare_length(p->nick, in, length)) {
         is_match = 1;
         break;
      }
      p = p->next;
   }
   return p;
}

/* This command is only useful for determining whether a
services client of this type is loaded, as it will not
look past the first match. */

Services_User *get_svsclient_by_type(const char * const in) {
   Services_User *p = svslist;
   if(!in) {
      return 0;
   }
   while(p != 0) {
      if(p->type && case_compare(p->type, in)) {
         break;
      }
      p = p->next;
   }

   return p;
}

int svsclient_triggered(Services_User *client, const char * const message) {
   if(client && message) {
       return (strstr(message, client->trigger) == message);
   }
   return 0;
}

Services_User *get_svsclient_by_trigger(const char * const trigger) {
   Services_User *p = svslist;
   if(!trigger) {
      return 0;
   }
   while(p != 0) {
      if(p->nick && p->trigger && strstr(trigger, p->trigger) == trigger) {
         break;
      }
      p = p->next;
   }
   return p;
}

static void delete_user(User *user) {
   //user->c
   free(user);
   user = 0;
}

void drop_user(const char * const username) {
   User *prev;
   User *p = userlist;
   if(userlist) {
      prev = userlist;
      while(p != 0) {
         if(case_compare(p->nick, username)) {
            if(p == userlist) {
               userlist = p->next;
               delete_user(p);
            }
            else
            {
               prev->next = p->next;
               delete_user(p);
            }
            break;
         }
         prev = p;
         p = p->next;
      }
   }
}

void drop_users_by_server(const char * const server) {
   User *prev;
   User *p = userlist;
   if(userlist) {
      prev = userlist;
      while(p != 0) {
         if(case_compare(p->server, server)) {
            if(p == userlist) {
               p = p->next;
               delete_user(userlist);
               userlist = p;
            }
            else
            {
               prev->next = p->next;
               delete_user(p);
               p = prev->next;
            }
         }
         else
         {
            prev = p;
            p = p->next;
         }
      }
   }
}

User *add_user(const char * const nickname, const char * const ident, const char * const hostname, const char * const realname, const time_t date) {
   User *user = userlist;
   if(nickname != 0 && ident != 0 && hostname && realname != 0) {
      if(userlist == 0) {
         userlist = malloc(sizeof(User));
         user = userlist;
      }
      else
      {
         while(user->next != 0) {
            user = user->next;
         }
         user->next = malloc(sizeof(User));
         user = user->next;
      }
      if(user) {
         memset(user, 0, sizeof(User));
         strncpy_safe(user->nick, nickname, NICK_MAX);
         strncpy_safe(user->username, ident, 10);
         strncpy_safe(user->realname, realname, 65);
         strncpy_safe(user->hostname, hostname, HOST_MAX);
         user->connected = date;
         user->next = 0;
         user->id = userid;
         user->regdata = 0;
         user->is_oper = 0;
         userid++;
      }
   }
   return user;
}


User *get_user(const char * const nickname) {
   User *p = userlist;
   while(p != 0) {
      if(p->nick && case_compare(p->nick,nickname)) {
         break;
      }
      p = p->next;
   }
   return p;
}

void user_new_vhost(const char * const username, const char * const hostname) {
   User *user;
   if(username && hostname) {
      user = get_user(username);
      strncpy_safe(user->vhostname, hostname, HOST_MAX);
   }
}

void user_new_ident(const char * const username, const char * const ident) {
   User *user;
   if(username && ident) {
      user = get_user(username);
      strncpy_safe(user->username, ident, 10);
   }
}

void user_new_realname(const char * const username, const char * const realname) {
   User *user;
   if(username && realname) {
      user = get_user(username);
      strncpy_safe(user->realname, realname, 64);
   }
}

void user_vhost(User *user, const char * const hostname) {
   if(user) {
      strncpy_safe(user->vhostname, hostname, HOST_MAX);
   }
}

void user_ident(User *user, const char * const ident) {
   if(user) {
      strncpy_safe(user->username, ident, 10);
   }
}

void user_realname(User *user, const char * const realname) {
   if(user) {
      strncpy_safe(user->realname, realname, 64);
   }
}

void user_ircop(User *user, const int remove) {
   if(remove == 0) {
      user->is_oper = 1;
   }
   else
   {
      user->is_oper = 0;
   }
}

void user_mode(User *user, const char mode, const int remove) {
   if(user && mode != 0) {
      if(!strchr(user->modes, mode) && !remove) {
         strncat(user->modes, &mode, 1);
      }
      else if(remove) {
         chr_delete(user->modes, mode);
      }
   }
}

int user_has_mode(User *user, const char mode) {
   if(user && strchr(user->modes, mode)) {
      return 1;
   }
   return 0;
}

int is_user(const char * const nickname) {
   User *p = userlist;
   while(p != 0) {
      if(p->nick && strcmp(p->nick,nickname) == 0) {
         return 1;
      }
      p = p->next;
   }
   return 0;
}

static void delete_reg_user(Reg_User *user) {
   free(user);
}

void drop_reg_user(const char * const user) {
   Reg_User *prev;
   Reg_User *p = reg_user_list;
   if(reg_user_list) {
      prev = reg_user_list;
      while(p != 0) {
         if(case_compare(p->user, user)) {
            if(p == reg_user_list) {
               reg_user_list = p->next;
            }
            else
            {
               prev->next = p->next;
            }
            delete_reg_user(p);
            break;
         }
         prev = p;
         p = p->next;
      }
   }
}

int is_reg_user(const char * const nickname) {
   Reg_User *p = reg_user_list;
   while(p != 0) {
      if(p->user) {
         if(strcmp(p->user,nickname) == 0) {
            return 1;
         }
      }
      p = p->next;
   }
   return 0;
}

int is_linked_nickname(const char * const nickname, const char * const link) {
   Reg_User *user;
   Reg_User *next;
   if(!nickname || !link) {
      return 0;
   }
   user = get_reg_user(nickname);
   if(user) {
      if(user->next_by_user != user) {
         next = user;
         do {
            if(case_compare(next->user, link)) {
               return 1;
            }
            next = next->next_by_user;
         }
         while(next != user);
      }
   }
   return 0;
}

int link_nickname(const char * const nickname, const char * const link) {
   Reg_User *user;
   Reg_User *newuser;
   Reg_User *next;
   if(!nickname || !link) {
      return 0;
   }
   user = get_reg_user(nickname);
   if(user) {
      newuser = get_reg_user(link);
      if(!newuser) {
         newuser = malloc(sizeof(Reg_User));
         if(newuser) {
            newuser = memcpy(newuser, user, sizeof(Reg_User));
            strncpy_safe(newuser->user, nickname, NICK_MAX);
            next = user;
            while(next->next_by_user != user) {
               next = next->next_by_user;
            }
            newuser->next = next->next_by_user;
            next->next_by_user = newuser;
            while(next->next) {
               next = next->next;
            }
            next->next = newuser;
         }
      }
   }
   return 0;
}
User *is_identified(const char * const username) {
   User *user;
   if(username) {
      user = get_user(username);
      if(user && user->regdata && user->regdata->user) {
         if(case_compare(user->regdata->user, username)) {
            return user;
         }
      }
   }
   return 0;
}

int user_is_identified(User *user) {
   if(user && user->regdata != 0) {
      if(case_compare(user->regdata->user, user->nick)) {
         return 1;
      }
   }
   return 0;
}

Reg_User *get_identified_user(const char * const username) {
   User *user;
   if(username) {
      user = get_user(username);
      if(user && user->regdata) {
         if(case_compare(user->regdata->user, user->nick)) {
            return user->regdata;
         }
      }
   }
   return 0;
}

User *user_new_nickname(const char * const sender, const char * const nickname) {
   User *user = userlist;
   if(!user || !nickname || !userlist) {
      return 0;
   }
   while(user) {
      if(strcmp(sender,user->nick) == 0) {
         strncpy_safe(user->nick, nickname, NICK_MAX);
         user->regdata = 0;
         return user;
      }
      user = user->next;
   }
   return 0;
}

static void delete_user_data(Extension_Data *start, const char * const name) {
   Extension_Data *data = start;
   Extension_Data *prev;
   prev = data;
   data = data->next;
   while(data) {
      if(case_compare(data->name, name)) {
         prev->next = data->next;
         free(data);
         data = prev->next;
      }
      else
      {
         data = data->next;
      }
   }
}
void drop_user_data(User *user, const char * const name) {
   Extension_Data *data;
   if(user && name && user->extdata) {
      data = user->extdata;
      if(case_compare(user->extdata->name, name)) {
         data = data->next;
         free(user->extdata);
         user->extdata = data;
      }
      else
      {
         delete_user_data(data, name);
      }
   }
}

int add_user_data(User *user, const char * const name, void *data) {
   Extension_Data *extdata;
   if(!user || !name || !data) {
      return 0;
   }
   if(!user->extdata) {
      user->extdata = malloc(sizeof(Extension_Data));
      extdata = user->extdata;
   }
   else
   {
      extdata = user->extdata;
      while(extdata->next) {
         extdata = extdata->next;
      }
      extdata = malloc(sizeof(Extension_Data));
   }
   if(extdata) {
      memset(extdata, '\0', sizeof(Extension_Data));
      strncpy_safe(extdata->name, name, 32);
      extdata->data = data;
      return 1;
   }
   return 0;
}

void *get_user_data(User *user, const char * const name) {
   Extension_Data *data;
   if(!user || !name) {
      return 0;
   }
   data = user->extdata;
   while(data) {
      if(case_compare(data->name, name)) {
         return data->data;
      }
      data = data->next;
   }
   return 0;
}

void drop_reg_user_data(Reg_User *user, const char * const name) {
   Extension_Data *data;
   if(user && name && user->extdata) {
      data = user->extdata;
      if(case_compare(user->extdata->name, name)) {
         if(data == user->extdata) {
            data = data->next;
            free(user->extdata);
            user->extdata = data;
         }
         else
         {
            delete_user_data(data, name);
         }
         data = data->next;
      }
   }
}

int add_reg_user_data(Reg_User *user, const char * const name, void *data) {
   Extension_Data *extdata;
   if(!user || !name || !data) {
      return 0;
   }
   if(!user->extdata) {
      user->extdata = malloc(sizeof(Extension_Data));
      extdata = user->extdata;
   }
   else
   {
      extdata = user->extdata;
      if(extdata) {
         while(extdata->next) {
            extdata = extdata->next;
         }
      }
      extdata = malloc(sizeof(Extension_Data));
   }
   if(extdata) {
      memset(extdata, '\0', sizeof(Extension_Data));
      strncpy_safe(extdata->name, name, 32);
      extdata->data = data;
      return 1;
   }
   return 0;
}

void *get_reg_user_data(Reg_User *user, const char * const name) {
   Extension_Data *data;
   if(!user || !user->extdata || !name) {
      return 0;
   }
   data = user->extdata;
   do {
      if(case_compare(data->name, name)) {
         return data->data;
      }
      data = data->next;
   }
   while(data);
   return 0;
}

User *is_ircop(const char * const nickname) {
   User *user;
   user = get_user(nickname);
   if(user && user->is_oper == 1) {
      return user;
   }
   return 0;
}

int user_is_ircop(User *user) {
   if(user) {
      if(user->is_oper == 1) {
         return 1;
      }
   }
   return 0;
}

Reg_User *get_reg_user(const char * const username) {
   Reg_User *user = reg_user_list;
   if(user == 0 || username == 0) {
      return 0;
   }
   while(user != 0) {
      if(case_compare(user->user,username)) {
         break;
      }
      user = user->next;
   }
   return user;
}

int identify_user(User *user, const char * const username, const char * const password) {
   Reg_User *account_data;
   if(!user || !password) {
      return 0;
   }
   account_data = get_reg_user(username);
   if(!account_data || !account_data->password) {
      return 0;
   }
   if(strcmp(password,account_data->password) == 0) {
      user->regdata = account_data;
      strncpy_safe(account_data->last_host, user->hostname, HOST_MAX);
      strncpy_safe(account_data->last_ident, user->username, 16);
      if(*account_data->vhost != '\0') {
         remote_server->new_host(ezri->name, user->nick, account_data->vhost);
      }
      return 1;
   }
   return 0;
}

int auto_identify(User *user) {
   Reg_User *regdata;
   regdata = get_reg_user(user->nick);
   if(regdata) {
      if(strcmp(user->username, regdata->last_ident) == 0 && strcmp(user->hostname, regdata->last_host) == 0) {
         user->regdata = regdata;
         return 1;
      }
   }
   return 0;
}

Reg_User *register_nickname(const char * const nickname, const char * const password, const char * const email) {
   Reg_User *user = reg_user_list;
   if(!nickname || strlen(nickname) > NICK_MAX) {
      return 0;
   }
   if(!password) {
      return 0;
   }
   if(reg_user_list == 0) {
      reg_user_list = malloc(sizeof(Reg_User));
      user = reg_user_list;
   }
   else
   {
      while(user->next != 0) {
         if(case_compare(nickname, user->user)) {
            break;
         }
         user = user->next;
      }
      if(!user->next && !case_compare(nickname, user->user)) {
         user->next = malloc(sizeof(Reg_User));
         user = user->next;
      }
      else
      {
         user = 0;
      }
   }
   if(user) {
      memset(user, 0, sizeof(Reg_User));
      strncpy_safe(user->user, nickname, NICK_MAX);
      strncpy_safe(user->password, password, NICK_MAX);
      strncpy_safe(user->email, email, NICK_MAX);
      user->registered = time(NULL);
      user->next = 0;
      user->dummy = 0;
      return user;
   }
   return 0;
}

int reg_user_vhost(Reg_User *user, const char * const hostname) {
   if(user) {
      strncpy_safe(user->vhost, hostname, HOST_MAX);
      return 1;
   }
   return 0;
}

char *user_get_hostname(User *user, const int prefer_real) {
   if(!user) {
      return 0;
   }
   else if(prefer_real) {
      return user->hostname;
   }
   else if(*user->vhostname) {
      return user->vhostname;
   }
   return user->hostname;
}

char *user_banmask(User *user, char * const out, const int ban_type) {
   if(user && out) {
      memset(out, '\0', MASK_MAX);
      if(ban_type & BAN_NICKNAME) {
         strncpy_safe(out, user->nick, NICK_MAX);
      }
      else
      {
         *out = '*';
      }
      strncat(out, "!", 1);
      if(ban_type & BAN_USERNAME) {
         strncat(out, user->username, 10);
      }
      else
      {
         *out = '*';
      }
      strncat(out, "@", 1);
      if(ban_type & BAN_HOSTNAME) {
         if(*(user->vhostname) != '\0') {
            strncat(out, user->vhostname, HOST_MAX);
         }
         else
         {
            strncat(out, user->hostname, HOST_MAX);
         }
      }
   }
   return out;
}