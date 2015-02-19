typedef struct Server {
   char name[HOST_MAX];
   char description[IRCLINE_MAX];
   int numeric;
   time_t connected;
   struct Server *leaf;
   struct Server *next;
   struct Server *parent;
} Server;

typedef struct Ezri {
   char name[HOST_MAX];
   char link[HOST_MAX];
   char description[256];
   char password[64];
   char protocol[32];
   int numeric;
   int port;
} Ezri;

typedef struct Server_Connect {
   struct Ezri_Socket *Socket;
   unsigned int numeric;
   char name[65];
   char description[256];
   char password[32];
   unsigned int synching;
   int (*main)(const char * const message);
   int (*read)(char *bufferline, struct Ezri_Socket *sock, const unsigned int length);
   int (*send)(struct Ezri_Socket *sock, const char *message);
   void (*init)(const char * const name, const char * const pass, const char * const description);
   void (*change_nick)(const char * const source, const char * const target, const char * const new_nickname);
   void (*new_nick)(const char * const nickname, const char * const username, const char * const hostname, const char * const server, const char * const realname);
   void (*privmsg)(const char * const sender, const char * const target, const char * const message);
   void (*notice)(const char * const sender, const char * const target, const char * const message);
   void (*join)(const char * const nickname, const char * const target, const char * const channel, const int service);
   void (*part)(const char * const nickname, const char * const target, const char * const reason);
   void (*invite)(const char * const sender, const char * const nickname, const char * const target);
   void (*new_host)(const char * const sender, const char * const nickname, const char * const hostname);
   void (*modes)(const char * const sender, const char * const target,const char * const modes, const char * const params);
   void (*kill)(const char * const source, const char * const target, const char * const reason, const int svskill);
   void (*kick)(const char * const source, const char * const channel, const char * const target, const char * const reason);
} Server_Connect;

EXPORT Server *get_server(const char * const server_name, Server *list);
EXPORT Server *get_server_by_numeric(const int numeric, Server *list);
EXPORT void drop_server(const char * const server_name);
EXPORT Server *add_server(const char * const server_name, const char * const server_hub);

EXPORT Server_Connect *remote_server;
EXPORT Ezri *ezri;
#ifndef COMPILE_EXTENSION
   Server_Connect *remote_server;
   Ezri *ezri;
#endif

