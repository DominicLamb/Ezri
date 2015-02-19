#define _CRT_SECURE_NO_WARNINGS
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define EZRI_VERSION "0.77"
#define EZRI_RELEASE "20100517"
#define OFFICIAL_EXTENSION "irc.arloria.net"

#ifdef _WIN32
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
   #define snprintf(dest, len, string, ...) _snprintf(dest, len, string, __VA_ARGS__); dest[len-1] = '\0';
   #define LIBRARY_PREFIX ".dll"
   #define DIRECTORY_SEPARATOR "\\"
   #ifdef COMPILE_EXTENSION
      #define EXPORT extern __declspec(dllimport)
      #define IMPORT extern __declspec(dllexport)
   #else
      #define EXPORT extern __declspec(dllexport)
      #define IMPORT extern __declspec(dllimport)
   #endif
   #define gmmktime _mkgmtime
   #define sleep Sleep
   #define getcwd(buffer, size) GetCurrentDirectory(size, (LPTSTR)buffer)
   #define extension_openfile LoadLibraryA
   #define extension_closefile FreeLibrary
   #define extension_error GetLastError
   #define extension_getfunction GetProcAddress
   #define PATH_MAX MAX_PATH
   typedef HMODULE ExtensionFile;
   typedef FARPROC ExtensionFunction;
#else
   #define EXPORT extern
   #define IMPORT extern
   #ifdef __GNUC__
      #include <dlfcn.h>
      #include <unistd.h>
      #define LIBRARY_PREFIX ".so"
      #define DIRECTORY_SEPARATOR "/"
      #define extension_openfile(file) dlopen(file, RTLD_NOW)
      #define extension_closefile dlclose
      #define extension_error dlerror
      #define extension_getfunction dlsym
      #define sleep(milliseconds) usleep(milliseconds * 1000)
      typedef void* ExtensionFile;
      typedef int (*ExtensionFunction)();
      typedef char *(*ExtensionFunctionString)();
   #endif
#endif
#include "strings.h"
#ifndef COMPILE_EXTENSION
   #include "alloc.h"
#endif

typedef struct Extension_Info {
  char name[64];
  char author[64];
  char extension_version[12];
  char description[150];
  int critical;
  ExtensionFile handle;
} Extension_Info;

EXPORT char ezri_path[PATH_MAX];
EXPORT int debug;
EXPORT void create_path(char *buffer, const char * const directory, const char * const filename, const int library);
#define IRCLINE_MAX 512
#ifndef LINE_MAX
   #define LINE_MAX 2048
#endif

#ifndef COMPILE_EXTENSION
   char ezri_path[PATH_MAX];
   /* Change to a config option later */
   int debug;
#endif