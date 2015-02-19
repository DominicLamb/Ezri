#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Hosts"
#define DESCRIPTION "Allows the setting and manipulation of vHosts."
#include "../headers/extensions.h"

typedef struct Group_Access {
   char username[NICK_MAX];
   int group_admin;
   time_t time_granted;
   char granted_by[NICK_MAX];
   struct Group_Access *next;
} Group_Access;

typedef struct Group_Host {
   char hostname[HOST_MAX];
   char description[IRCLINE_MAX];
   char approvedby[NICK_MAX];
   char requestedby[NICK_MAX];
   time_t time_created;
   struct Group_Access *access;
   struct Group_Host *next;
} Group_Host;

unsigned int max_host_length = 0;
IMPORT Group_Host *host_list;
Group_Host *host_list = 0;

int event_host(void **args);
int event_backup(char **args);
int event_load(char **args);

int command_addhost(Services_User *client, User *sender, cmd_message *message);
int command_adduser(Services_User *client, User *sender, cmd_message *message);
int command_delhost(Services_User *client, User *sender, cmd_message *message);
int command_deluser(Services_User *client, User *sender, cmd_message *message);
int command_hosts(Services_User *client, User *sender, cmd_message *message);
int command_info(Services_User *client, User *sender, cmd_message *message);
int command_request(Services_User *client, User *sender, cmd_message *message);
int command_sethost(Services_User *client, User *sender, cmd_message *message);
int command_usehost(Services_User *client, User *sender, cmd_message *message);

int help_adduser(Services_User *client, User *sender, cmd_message *message);
int help_addhost(Services_User *client, User *sender, cmd_message *message);
int help_delhost(Services_User *client, User *sender, cmd_message *message);
int help_deluser(Services_User *client, User *sender, cmd_message *message);
int help_hosts(Services_User *client, User *sender, cmd_message *message);
int help_info(Services_User *client, User *sender, cmd_message *message);
int help_request(Services_User *client, User *sender, cmd_message *message);
int help_sethost(Services_User *client, User *sender, cmd_message *message);
int help_usehost(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   svs_commandhook *command;
   svs_help *help;
   Services_User *client;
   char **temp = 0;
   int i;
   const char commandnames[][8] = {"ADDHOST", "ADDUSER", "DELHOST", "DELUSER", "INFO", "LIST", "REQUEST", "SET", "USE", "\0"};
   User *(*restrictions[])(const char * const nickname) = {is_ircop, is_identified, get_user, get_user, is_ircop, get_user};
   int (*commands[])(Services_User *client, User *user, cmd_message *message) = {
      command_addhost, 
      command_adduser, 
      command_delhost,
      command_deluser,
      command_info, 
      command_hosts, 
      command_request, 
      command_sethost,
      command_usehost
   };
   const char helpdescriptions[][IRCLINE_MAX] = {
      "Add a new vHost group.", 
      "Add a user to a vHost access list.", 
      "Delete a hostname group.",
      "Delete a user from a hostname group.",
      "Get information about a vHost.", 
      "See a list of vHosts.",
      "Request a new vHost group.",
      "Change a user's vHost.",
      "Use a host from the list of groups."
   };
   int (*helpfunctions[])(Services_User *client, User *user, cmd_message *message) = {
      help_addhost, 
      help_adduser,
      help_delhost,
      help_deluser,
      help_info, 
      help_hosts,
      help_request,
      help_sethost,
      help_usehost
   };
   client = get_svsclient_by_type("hostserv");
   if(client) {
      for(i = 0; *commandnames[i] != '\0'; i++) {
         command = make_command(commandnames[i], commands[i]);
         if(command) {
            command_restrict(command, restrictions[i]);
            if(add_command_by_type("hostserv", command)) {
               help = create_help(commandnames[i], "main", helpdescriptions[i], helpfunctions[i]);
               if(help) {
                  svs_add_help_by_type("hostserv", help);
               }
               else
               {
                  log_message("Unable to create help for %s, this help item will not be available.", LOG_NOTICE, commandnames[i]);
               }
            }
            else
            {
               log_message("Unable to load %s command, some aspects of HostServ will be unable to function.", LOG_IMPORTANT, commandnames[i]);
            }
         }
         else
         {
            log_message("Unable to allocate memory to create %s command, HostServ will now attempt to unload.", LOG_CRITICAL, commandnames[i]);
            return 0;
         }
      }
      event_load(temp);
   }
   else
   {
      log_message("No HostServ-type clients found, using minimal HostServ implementation and attaching to NickServ-type clients.", LOG_NOTICE);
      command = make_command("SET HOST", command_sethost);
      if(command) {
         command_restrict(command, is_ircop);
         if(add_command_by_type("nickserv", command)) {
            log_message("Loaded SET HOST command and attached to NickServ.", LOG_DEBUG_CORE);
            help = create_help("HOST", "SET", "Change a user's vHost.", help_sethost);
            if(help) {
               svs_add_help_by_type("hostserv", help);
            }
            else
            {
               log_message("Unable to create help for SET HOST, this help item will not be available.", LOG_NOTICE);
            }
         }
         else
         {
            log_message("Unable to find any NickServ-type clients for basic HostServ functions. Loading will not continue.", LOG_CRITICAL);
            return 1;
         }
      }
   }
   //add_event("user_identify", event_host);
   event_load(temp);
   return 1;
}

int extension_unload(void) {
   const char commandnames[][8] = {"ADDHOST", "ADDUSER", "DELHOST", "DELUSER", "INFO", "LIST", "REQUEST", "SET", "USE", "\0"};
   int i;
   if(get_svsclient_by_type("hostserv")) {
      for(i = 0; *commandnames[i] != '\0'; i++) {
         drop_command_by_type("hostserv", commandnames[i]);
         drop_help_by_type("hostserv", commandnames[i]);
      }
   }
   else
   {
      drop_command_by_type("nickserv", "SET HOST");
      drop_help_by_type("nickserv", "SET HOST");
   }
   return 1;
}

static Group_Host *get_host(const char * const hostname) {
   Group_Host *host = host_list;
   while(host) {
       if(case_compare(host->hostname,hostname)) {
          break;
       }
       host = host->next;
   }
   return host;
}

static void drop_access(Group_Host *host, const char * const nickname) {
   Group_Access *access;
   Group_Access *prev;
   if(host && nickname) {
      access = host->access;
      prev = access;
      while(access) {
         if(case_compare(access->username, nickname)) {
            if(access == host->access) {
               host->access = access->next;
               free(access);
            }
            else
            {
               prev->next = access->next;
               free(access);
            }
            break;
         }
         prev = access;
         access = access->next;
      }
   }
}

static Group_Access *get_access(Group_Host *host, const char * const user) {
   Group_Access *access = 0;
   if(host) {
      access = host->access;
      while(access) {
         if(case_compare(access->username, user)) {
            break;
         }
         access = access->next;
      }
   }
   return access;
}

static int is_group_admin(Group_Host *host, const char * const user) {
   Group_Access *access;
   if(host) {
      access = get_access(host, user);
      if(access) {
         return access->group_admin;
      }
   }
   return 0;
}

static Group_Access *add_access(Group_Host *host, const char * const username, const char * const added_by) {
   Group_Access *access = 0;
   unsigned int count = 0;
   if(host && username) {
      if(host->access) {
         access = host->access;
         count++;
         while(access->next) {
            if(case_compare(access->username,username)) {
               return 0;
            }
            access = access->next;
            count++;
         }
         if(case_compare(access->username,username)) {
            return 0;
         }
         if(count < 50) { /* Use config later */
            access->next = malloc(sizeof(Group_Access));
            access = access->next;
         }
      }
      else
      {
         host->access = malloc(sizeof(Group_Access));
         access = host->access;
      }
      if(access) {
         memset(access, '\0', sizeof(Group_Access));
         strncpy_safe(access->username, username, NICK_MAX);
         strncpy_safe(access->granted_by, added_by, NICK_MAX);
         access->time_granted = time(NULL);
      }
   }
   return access;
}

static void delete_access(Group_Access *access) {
   free(access);
}

static void delete_host(Group_Host *host) {
   Group_Access *access;
   Group_Access *next;
   if(host->access) {
      access = host->access;
      while(access) {
         next = access->next;
         delete_access(access);
         access = next;

      }
   }
   free(host);
}

Group_Host *drop_host(const char * const hostname) {
   Group_Host *host = host_list;
   Group_Host *next;
   if(host) {
      while(host) {
         next = host->next;
         if(case_compare(hostname, host->hostname)) {
            delete_host(host);
            host = 0;
            break;
         }
         host = host->next;
      }
   }
   return host;
}

static Group_Host *add_host(const char * const host, const char * const requester) {
   Group_Host *p = host_list;
   if(!host) {
      return 0;
   }
   if(p) {
      while(p->next) {
         if(case_compare(p->hostname, host)) {
            return 0;
         }
         p = p->next;
      }
      if(case_compare(p->hostname, host)) {
         return 0;
      }
      p->next = malloc(sizeof(Group_Host));
      p = p->next;
   }
   else
   {
      host_list = malloc(sizeof(Group_Host));
      p = host_list;
   }
   if(p) {
      memset(p, '\0', sizeof(Group_Host));
      strncpy_safe(p->hostname, host, HOST_MAX);
      if(requester) {
         strncpy_safe(p->requestedby, requester, NICK_MAX);
      }
      else
      {
         strncpy_safe(p->requestedby, "N/A", NICK_MAX);
      }
      if(strlen(host) > max_host_length) {
         max_host_length = strlen(host);
      }
      p->time_created = time(NULL);
   }
   return p;
}

static void approve_vhost(Group_Host *host, const char * const approver) {
   if(host && approver) {
      strncpy_safe(host->approvedby, approver, NICK_MAX);
   }
}

static void describe_vhost(Group_Host *host, const char * const description) {
   if(host && description) {
      strncpy_safe(host->description, description, IRCLINE_MAX);
   }
}
/*
   Function currently not in use.
   Reserved for default network vHost
*/
int event_host(void **args) {
   Reg_User *user;
   if(args) {
      user = get_reg_user(args[0]);
      if(user && *(user->vhost)) {
         
      }
   }
   return 1;
}

int event_backup(char **args) {
   Group_Host *host = host_list;
   Group_Access *access;
   DB_Table *table_hosts;
   DB_Table *table_access;
   char stamp_granted[15];
   char stamp_created[15];
   table_hosts = database->open("hosts");
   table_access = database->open("hostusers");
   if(table_hosts && table_access) {
      while(host) {
         make_timestamp(stamp_created, host->time_created);
         database->write(table_hosts, 5, host->hostname, host->description, host->approvedby, host->requestedby, stamp_created);
         access = host->access;
         while(access) {
            make_timestamp(stamp_granted, access->time_granted);
            database->write(table_access, 5, host->hostname, access->username, access->granted_by, stamp_granted, access->group_admin ? "1" : "0");
         }
         host = host->next;
      }
      database->close(table_hosts);
      database->close(table_access);
   }
   return 1;
}


int event_load(char **args) {
   Group_Host *host = host_list;
   Group_Access *access;
   char **in;
   DB_Table *table;
   table = database->open("hosts");
   if(table) {
      in = database->read(table, 5);
      while(in) {
         if(table->result_count == 5) {
            host = add_host(in[0], in[2]);
            approve_vhost(host, in[1]);
            if(in[3]) {
               host->time_created = import_timestamp(in[3]);
            }
         }
         in = database->read(table, 5);
      }
      database->close(table);
   }
   table = database->open("hostusers");
   if(table) {
      in = database->read(table, 5);
      while(in) {
         if(table->result_count == 5) {
            host = get_host(in[0]);
            if(host) {
               access = add_access(host, in[1], in[3]) ;
               describe_vhost(host, in[2]);
               access->time_granted = import_timestamp(in[4]);
               if(*in[4] == '1') {
                  access->group_admin = 1;
               }
            }
         }
         database->close(table);
      }
   }
   return 1;
}


/*
   TODO: Add page support
*/

int command_addhost(Services_User *client, User *sender, cmd_message *message) {
   Group_Host *host = host_list;
   char hostname[HOST_MAX];
   copy_to(hostname, message_get(message), ' ', HOST_MAX);
   if(*hostname) {
      host = add_host(hostname, sender->nick);
      if(host) {
         if(add_access(host, sender->nick, sender->nick)) {
            if(user_is_ircop(sender)) {
               approve_vhost(host, sender->nick);
               message_user(client, sender, "COMMAND_HOSTS_NEW_HOST_GROUP", 1, host->hostname);

            }
            else
            {
               message_user(client, sender, "COMMAND_HOSTS_NEW_GROUP_PENDING", 1, host->hostname);
            }
            // Temp
            event_backup(&host);
         }
         else
         {
            drop_host(host->hostname);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_HOSTS_GROUP_EXISTS", 1, hostname);
      }
   }
   else
   {
      help_addhost(client, sender, message);
   }
   return 1;
}

int command_adduser(Services_User *client, User *sender, cmd_message *message) {
   Group_Host *host;
   Reg_User *user;
   char hostname[HOST_MAX];
   char *username;
   copy_to(hostname, message_get(message), ' ', HOST_MAX);
   if(*hostname) {
      username = get_token(2, message_get(message), " ");
      if(username) {
         host = get_host(hostname);
         user = get_reg_user(username);
         if(host) {
            if(user) {
               if(add_access(host, user->user, sender->nick)) {
                  message_user(client, sender, "COMMAND_HOSTS_USER_ADDED", 2, host->hostname, user->user);
               }
            }
            else
            {
               message_user(client, sender, "COMMAND_NICKNAME_NOT_REGISTERED", 1, username);
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_HOSTS_GROUP_NOT_FOUND", 1, host->hostname);
         }
         free(username);
      }
      else
      {
         message_user(client, sender, "COMMAND_HOSTS_NO_HOST_GIVEN", 0);
      }
   }
   else
   {
      help_adduser(client, sender, message);
   }
   return 1;
}


int command_delhost(Services_User *client, User *sender, cmd_message *message) {
   Group_Host *host = host_list;
   char hostname[HOST_MAX];
   copy_to(hostname, message_get(message), ' ', HOST_MAX);
   if(*hostname) {
      host = get_host(hostname);
      if(host) {
         if(is_group_admin(host, sender->nick)) {
            host = drop_host(hostname);
            message_user(client, sender, "COMMAND_HOSTS_GROUP_DROPPED", 1, hostname);
         }
         else
         {
            message_user(client, sender, "COMMAND_HOSTS_GROUP_NOT_ADMIN", 1, hostname);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_HOSTS_GROUP_NOT_FOUND", 1, hostname);
      }
   }
   else
   {
      help_delhost(client, sender, message);
   }
   return 1;
}

int command_deluser(Services_User *client, User *sender, cmd_message *message) {
   Group_Host *host;
   char hostname[HOST_MAX];
   message_seek(message, copy_to(hostname, message_get(message), ' ', HOST_MAX));
   if(*hostname) {
      host = get_host(hostname);
      if(host) {
         if(is_group_admin(host, sender->nick)) {
            if(get_access(host, message_get(message))) {
               drop_access(host, message_get(message));
               message_user(client, sender, "COMMAND_HOSTS_USER_DELETED", 2, hostname, message_get(message));
            }
            else
            {
               message_user(client, sender, "COMMAND_HOSTS_USER_NOT_FOUND", 2, hostname, message_get(message));
            }
         }
         else
         {
            message_user(client, sender, "COMMAND_HOSTS_GROUP_NOT_ADMIN", 1, hostname);
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_HOSTS_GROUP_NOT_FOUND", 1, hostname);
      }
   }
   else
   {
      help_deluser(client, sender, message);
   }
   return 1;
}

int command_hosts(Services_User *client, User *sender, cmd_message *message) {
   struct Group_Host *p = host_list;
   char response[513] = "\0";
   if(host_list) {
      sprintf(response, "%-*s %s", max_host_length + 3, "Host", "Group Leader");
      plain_message_user(client, sender, response);
      while(p) {
         sprintf(response, "%-*s %s", max_host_length + 3, p->hostname, p->requestedby);
         plain_message_user(client, sender, response);
         p = p->next;
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_HOSTS_EMPTY", 0);
   }
   return 1;
}

int command_info(Services_User *client, User *sender, cmd_message *message) {
   Group_Host *host;
   Group_Access *access;
   char hostname[HOST_MAX];
   copy_to(hostname, message_get(message), ' ', HOST_MAX);
   if(*hostname) {
      host = get_host(hostname);
      if(host) {
         access = host->access;
         message_user(client, sender, "COMMAND_HOSTS_INFO_NAME", 1, host->hostname);
         message_user(client, sender, "COMMAND_HOSTS_INFO_REQUEST", 2, host->requestedby, host->approvedby);
         message_user(client, sender, "COMMAND_HOSTS_ACCESS_BEGIN", 1, host->hostname);
         while(access) {
            plain_message_user(client, sender, access->username);
            access = access->next;
         }
      }
      else
      {
         message_user(client, sender, "COMMAND_HOSTS_GROUP_NOT_FOUND", 1, hostname);
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_NO_HOSTNAME", 0);
   }
   return 1;
}

int command_request(Services_User *client, User *sender, cmd_message *message) {
   char hostname[HOST_MAX];
   Group_Host *host;
   copy_to(hostname, message_get(message), ' ', HOST_MAX);
   if(*hostname) {
      host = add_host(hostname, sender->nick);
      if(*message_get(message)) {
         describe_vhost(host, message_get(message));
      }
      if(user_is_ircop(sender)) {
         approve_vhost(host, sender->nick);  
         message_user(client, sender, "COMMAND_HOSTS_NEW_HOST_GROUP", 1, host->hostname);
      }
      else
      {
         message_user(client, sender, "COMMAND_HOSTS_NEW_GROUP_PENDING", 1, host->hostname);
      }
   }
   else
   {
      help_request(client, sender, message);
   }
   return 1;
}

int command_sethost(Services_User *client, User *sender, cmd_message *message) {
   char *username = 0;
   char *hostname = 0;
   Reg_User *user;
   username = get_token(1, message_get(message), " ");
   hostname = get_token(2, message_get(message), " ");
   if(username && hostname) {
      user = get_reg_user(username);
      if(user) {
         reg_user_vhost(user, hostname);
      }
      else
      {
         message_user(client, sender, "COMMAND_NICKNAME_NOT_REGISTERED", 1, username);
      }
   }
   else
   {
       help_sethost(client, sender, message);
   }
   if(hostname) {
      free(hostname);
   }
   if(username) {
      free(username);
   }
   return 1;
}

int command_usehost(Services_User *client, User *sender, cmd_message *message) {
   Group_Host *p = host_list;
   Reg_User *user;
   while(p) {
      if(case_compare(p->hostname, message_get(message))) {
         break;
      }
      p = p->next;
   }
   if(p) {
      user = get_identified_user(sender->nick);
      if(user) {
         remote_server->new_host(client->nick, sender->nick, p->hostname);
         reg_user_vhost(user, p->hostname);
         message_user(client, sender, "COMMAND_HOSTS_HOSTNAME_CHANGED", 1, message_get(message));
      }
   }
   else
   {
      message_user(client, sender, "COMMAND_HOSTS_GROUP_NOT_FOUND", 1, message_get(message));
   }
   return 1;
}

int help_addhost(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_ADDHOST_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_ADDHOST_DESCRIBE", 0);
   }
   return 1;
}

int help_adduser(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_ADDUSER_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_ADDUSER_DESCRIBE", 0);
   }
   return 1;
}

int help_delhost(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_DELHOST_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_DELHOST_DESCRIBE", 0);
   }
   return 1;
}

int help_deluser(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_DELUSER_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_DELUSER_DESCRIBE", 0);
   }
   return 1;
}

int help_hosts(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_LIST_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_LIST_DESCRIBE", 0);
   }
   return 1;
}


int help_info(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_INFO_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_INFO_DESCRIBE", 0);
   }
   return 1;
}

int help_request(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_REQUEST_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_REQUEST_DESCRIBE", 0);
   }
   return 1;
}

int help_sethost(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_SETHOST_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_SETHOST_DESCRIBE", 0);
   }
   return 1;
}

int help_usehost(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_HOSTS_USEHOST_SYNTAX", 0);
      message_user(client, sender, "HELP_HOSTS_USEHOST_DESCRIBE", 0);
   }
   return 1;
} 