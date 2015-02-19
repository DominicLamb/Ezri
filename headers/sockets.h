#ifdef _WIN32
   #include <winsock2.h>
   #include <Ws2tcpip.h>
#else
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netdb.h>
   #define SOCKET int
   #define SOCKET_ERROR -1
   #define closesocket close
#endif

#define TYPE_IPV4 AF_INET

typedef struct Ezri_Socket {
   SOCKET Socket;
   char hostname[HOST_MAX];
   char ip_address[40];
   unsigned int port;
   int error;
   time_t connected;
   unsigned int data_sent;
   unsigned int data_recv;
} Ezri_Socket;
EXPORT int socket_init();
EXPORT void socket_nonblock(SOCKET sock);
EXPORT Ezri_Socket *make_connection(int type, char *destination, unsigned short port);
EXPORT int sendline(SOCKET sock, const char *message);
EXPORT int socket_sendline(Ezri_Socket *sock, const char *message);
EXPORT int readline(char *bufferline, SOCKET sock, unsigned int length);
EXPORT int socket_readline(char *bufferline, Ezri_Socket *sock, unsigned int length);
