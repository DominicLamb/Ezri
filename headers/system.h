EXPORT void create_path(char *buffer, const char * const directory, const char * const filename, const int library);
EXPORT const char *translate_error(const int error);
#ifndef COMPILE_EXTENSION
   FILE *logfile;
#endif