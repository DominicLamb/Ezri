#include "headers/main.h"
#include "headers/users.h"
#include "headers/channels.h"
#include "headers/commands.h"
#include "headers/sockets.h"
#ifdef _WIN32

void socket_nonblock(SOCKET sock) {
   u_long mode = 1;
   ioctlsocket(sock, FIONBIO, &mode);
}

int socket_init() {
   WSADATA WsaData;
   WORD WsaVersion;
   WsaVersion = MAKEWORD(2,0);
   return WSAStartup(WsaVersion, &WsaData);
}
#else

int socket_init() {
   return 1;
}
#endif

Ezri_Socket *make_connection(int type, char *destination, unsigned short port) {
   struct addrinfo hints;
   struct addrinfo *address;
   Ezri_Socket *new_socket = 0;
   char dest_port[6];
   int result = 0;
   new_socket = malloc(sizeof(Ezri_Socket));
   memset(&hints, '\0', sizeof(hints));
   hints.ai_family = type;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   if(new_socket) {
      memset(new_socket, 0, sizeof(Ezri_Socket));
      snprintf(dest_port, 6, "%d", port);
      result = getaddrinfo(destination, dest_port, &hints, &address);
      if(address) {
         new_socket->Socket = socket(type, SOCK_STREAM, address->ai_protocol);
         if(new_socket->Socket != -1) {
            if(connect(new_socket->Socket, address->ai_addr, address->ai_addrlen) != -1) {
               freeaddrinfo(address);
               return new_socket;
            }
         }
         else
         {
            closesocket(new_socket->Socket);
         }
      }
      freeaddrinfo(address);
      free(new_socket);
   }
   return 0;
}

/*
   The aim is to remove this buffer entirely
   at some point in the future
*/
#define BUFFER_SIZE 2048
int readline(char *bufferline, SOCKET sock, unsigned int length) {
   int read_data = 0;
   static char localbuffer[2048];
   size_t len;
   /*int intlength = sizeof(int); */
   if(*localbuffer == '\0') {
      read_data = recv(sock, localbuffer, BUFFER_SIZE - 1, 0);
      if(read_data == SOCKET_ERROR) {
         /* Win32 only, assuming it even works there. Remove for now.
         printf("Error %d\n", getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)0, &intlength));
         */
      }
   }
   if(read_data > 0 || *localbuffer) {
      len = copy_to(bufferline, skip_chars(localbuffer, "\n"), '\n', length);
      bufferline[len-2] = '\0';
      memmove(localbuffer, localbuffer + len, BUFFER_SIZE - len);
   }
   else
   {
      *bufferline = '\0';
   }
   return read_data;
}

int socket_readline(char *bufferline, Ezri_Socket *sock, unsigned int length) {
   int result;
   result = readline(bufferline, sock->Socket, length);
   if(result > 0) {
      sock->data_recv += result;
   }
   return result;
}

int sendline(SOCKET sock, const char *message) {
   size_t length = strlen(message);
   int result = 0;
   result = send(sock, message, (int)length, 0);
   send(sock, "\n", 1, 0);
   if(debug) {
      log_message("OUT [%d]: %s", LOG_DEBUG_CORE | LOG_NO_RELAY, result, message);
   }
   return result;
}

int socket_sendline(Ezri_Socket *sock, const char *message) {
   int result;
   result = sendline(sock->Socket, message);
   if(result > 0) {
      sock->data_sent += result;
   }
   return result;
}
