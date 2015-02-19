#include "headers/main.h"
#include "headers/system.h"
#include "headers/users.h"
#include "headers/channels.h"
#include "headers/commands.h"
static void get_executable_path() {
   #ifdef _WIN32
      char *str_pos;
      if(*ezri_path == '\0') {
         GetModuleFileNameA(NULL, ezri_path, PATH_MAX); 
         str_pos = strrchr(ezri_path, '\\');
         if(str_pos) {
            ezri_path[str_pos - ezri_path] = '\0';
         }
      }
   #endif
}

void create_path(char *buffer, const char * const directory, const char * const filename, const int library) {
   if(buffer) {
      get_executable_path();
      memset(buffer, 0, PATH_MAX);
      if(!directory || !*directory) {
         snprintf(buffer, PATH_MAX, "%s" DIRECTORY_SEPARATOR "%s", ezri_path, filename);
      }
      else
      {
         snprintf(buffer, PATH_MAX, "%s" DIRECTORY_SEPARATOR "%s" DIRECTORY_SEPARATOR "%s", ezri_path, directory, filename);
      }
      if(library) {
         strncat(buffer, LIBRARY_PREFIX, PATH_MAX - strlen(buffer));
      }
   }
}

const char *translate_error(const int error) {
   switch(error) {
      case FILE_INVALID_FILENAME:
         return "The specified filename contained invalid characters.";
      break;
      case FILE_NOT_FOUND:
         return "The file could not be found.";
      break;
      case FILE_ACCESS_DENIED:
         return "Ezri does not have permission to open the file.";
      break;
      case FILE_LOADED_NOT_VALID:
         return "The file loaded was not valid for this version of Ezri Services.";
      break;
      case FILE_SAYS_NO:
         return "The file loaded requested an unload.";
      break;
      case FILE_LOADED:
         return "The file was opened successfully.";
      break;
      default:
         return "An unknown error occurred while loading this file.";
      break;
   }
}