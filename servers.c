#include "headers/main.h"
#include "headers/servers.h"
#include "headers/events.h"

Server *server_list;
Server *get_server(const char * const server_name, Server *list) {
   Server *server;
   Server *ptr = 0;
   if(!server_name || !list || !server_list) {
      return 0;
   }
   server = list;
   while(server) {
      if(case_compare(server_name, server->name)) {
         break;
      }
      if(server->leaf) {
         ptr = get_server(server_name, server->leaf);
         if(ptr) {
            server = ptr;
            break;
         }
      }
      server = server->next;
   }
   return server;
}

Server *get_server_by_numeric(const int numeric, Server *list) {
   Server *server;
   Server *ptr = 0;
   if(!numeric || !list || !server_list) {
      return 0;
   }
   server = list;
   while(server) {
      if(numeric == server->numeric) {
         break;
      }
      if(server->leaf) {
         ptr = get_server_by_numeric(numeric, server->leaf);
         if(ptr) {
            server = ptr;
            break;
         }
      }
      server = server->next;
   }
   return server;
}

static void delete_server(Server *server) {
   Server *p;
   static unsigned int depth = 0;
   depth++;
   if(depth == 1) {
      if(server->leaf) {
         delete_server(server->leaf);
      }
      free(server);      
   }
   else
   {
      while(server) {
         p = server->next;
         if(server->leaf) {
            delete_server(server->leaf);
         }
         free(server);
         server = p;
      }
   }
   depth--;
}

void drop_server(const char * const server_name) {
   Server *server;
   Server *p;
   Server *prev;
   if(server_name) {
      server = get_server(server_name, server_list);
      if(server) {
         if(server->parent) {
            p = server->parent;
            p = server->leaf;
            prev = p;
            while(p->next) {
               if(case_compare(p->name, server_name)) {
                  break;
               }
               prev = p;
               p = p->next;
            }
            if(p == prev) {
               server->parent->leaf = server->parent->leaf->next;
            }
            else
            {
               prev->next = p->next;
            }
         }
         else if(server == server_list) {
            server_list = 0;
         }
         delete_server(server);
      }
   }
}
Server *add_server(const char * const server_name, const char * const server_hub) {
   Server *server;
   Server *parent = 0;
   if(!server_list) {
      server_list = malloc(sizeof(Server));
      server = server_list;
   }
   else
   {
      server = get_server(server_hub, server_list);
      if(!server) {
         return 0;
      }
      parent = server;
      if(server->leaf) {
         server = server->leaf;
         while(server->next) {
            server = server->next;
         }
         server->next = malloc(sizeof(Server));
         server = server->next;
      }
      else
      {
         server->leaf = malloc(sizeof(Server));
         server = server->leaf;
      }
   }
   if(server) {
      memset(server, '\0', sizeof(Server));
      strncpy_safe(server->name, server_name, 64);
      server->parent = parent;
   }
   return server;
}