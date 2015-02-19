#include "headers/main.h"
#include "headers/config.h"

static configblock *new_config_block(const char * const name) {
   configblock *config = 0;
   if(!name) {
      return 0;
   }
   config = malloc(sizeof(configblock));
   if(config) {
      memset(config, '\0', sizeof(configblock));
      strncpy_safe(config->name, name, CONFIG_NAME_MAX);
      strtolower(config->name);
   }
   return config;
}
static void add_key_value_pair(configblock *p, const char * const key, const char * const value) {
   struct key_value_pair *keys = 0;
   if(p && key && value) {
      keys = p->keys;
      if(keys) {
         while(keys->next != 0) {
            keys = keys->next;
         }
         keys->next = malloc(sizeof(struct key_value_pair));
         keys = keys->next;
      }
      else
      {
         p->keys = malloc(sizeof(struct key_value_pair));
         keys = p->keys;
      }
      if(keys) {
         memset(keys, '\0', sizeof(struct key_value_pair));
         strncpy_safe(keys->key, key, CONFIG_NAME_MAX);
         strncpy_safe(keys->value, value, CONFIG_LINE_MAX);
         strtolower(keys->key);
      }
   }
}
static int config_load_block(char *buffer, FILE *fp, configblock **block) {
   configblock *p = *block;
   configblock *q = 0;
   char *b = 0;
   char token[CONFIG_NAME_MAX];
   char *value = 0;
   static int depth = 0;
   depth++;
   while (fgets(buffer, CONFIG_LINE_MAX + 1, fp)) {
      if(*buffer != '#' && strncmp(buffer, "//", 2) != 0) {
         b = chr_delete(buffer, '\r');
         b = chr_delete(b, '\n');
         b = skip_chars(b, "\t ");	 
         b += copy_to(token, b, ' ', CONFIG_NAME_MAX);
         if(*token) {
            if(*token == '}') {
               depth--;
               q = p->sub;
               if(q) {
                  while(q->next && q->next != p) {
                     q = q->next;
                  }
               }
               return 1;
            }
            else if(strchr(b, '{') == 0) {
               value = skip_chars(b, "\t ");
               add_key_value_pair(p, token, value);
            }
            else if(depth < 5) {
               if(p == 0) {
                  *block = new_config_block(token);
                  config_load_block(b, fp, block);
               }
               else if(p->sub) {
                  q = p->sub;
                  while(q->next && q->next != p) {
                     q = q->next;
                  }
                  q->next = new_config_block(token);
                  config_load_block(b, fp, &(q->next));
               }
               else
               {
                  p->sub = new_config_block(token);
                  config_load_block(b, fp, &(p->sub));
               }
            }
            *token = '\0';
         }
      }
   }
   depth--;
   return 1;
}

int config_load() {
   char buffer[CONFIG_LINE_MAX + 1];
   char path[PATH_MAX];
   FILE *fp;
   config_list = malloc(sizeof(configblock));
   if(config_list) {
      create_path(path, "", "ezri.conf", 0);
      fp = fopen(path, "r+");
      if(fp) {
         *buffer = '\0';
         memset(config_list, 0, sizeof(configblock));
         config_load_block(buffer, fp, &config_list);
         fclose(fp);
         return 1;
      }
   }
   return 0;
}

static void key_pair_free(struct key_value_pair *pair) {
   struct key_value_pair *p;
   struct key_value_pair *next;
   p = pair;
   while(p) {
      next = p->next;
      free(p);
      p = next;
   }
}

static void configblock_free(configblock *config) {
   configblock *p = config;
   configblock *q = config;
   while(p) {
      if(p->keys) {
         key_pair_free(p->keys);
         p->keys = 0;
      }
      if(p->sub) {
         configblock_free(p->sub);
      }
      q = p;
      p = p->next;
      free(q);
   }
}

void config_drop(configblock *config) {
   if(config) {
      configblock_free(config);
   }
}

struct key_value_pair *get_key_pair(configblock *config, const char * const name) {
   struct key_value_pair *key = 0;
   if(config) {
      key = config->keys;
   }
   if(key && config) {
      while(key->next) {
         if(case_compare(key->key, name)) {
            break;
         }
         key = key->next;
       }
       if(case_compare(key->key, name)) {
          return key;
       }
   }
   return 0;
}

char *get_key_value(configblock *config, const char * const name) {
   struct key_value_pair *key;
   if(config && name) {
      key = get_key_pair(config, name);
      if(key && case_compare(key->key, name)) {
         return key->value;
      }
   }
   return 0;
}

struct key_value_pair *get_key_pair_next(struct key_value_pair *keys, const char * const name) {
   struct key_value_pair *key = keys;
   if(key) {
      key = key->next;
      while(key) {
         if(case_compare(key->key, name)) {
            break;
         }
         key = key->next;
      }
   }
   return key;
}

char *get_key_value_next(struct key_value_pair *keys, const char * const name) {
   struct key_value_pair *key = keys;
   while(key) {
      if(case_compare(key->key, name)) {
         return key->value;
      }
      key = key->next;
   }
   return 0;
}

configblock *configblock_get(configblock *config, const char * const name) {
   configblock *block = config;
   if(block) {
      while(block) {
         if(case_compare(block->name, name)) {
            break;
         }
         block = block->next;
      }
   }
   return block;
}

configblock *configblock_get_next_name(configblock *config) {
   configblock *block = config;
   while(block->next) {
      block = block->next;
      if(case_compare(block->name, config->name)) {
         return block;
      }
   }
   return 0;
}

void get_config(const char *get, void **dest, const int type) {
   void *value = 0;
   char *token = 0;
   configblock *config = config_list;
   int i = 0;
   /* This is hackish. Fix it. */
   static int j;
   int count;
   count = str_count(get, "::");
   j = 0;
   if(count == 0) {
      if(type != RETURN_BLOCK) {
         value = get_key_value(config, get);
      }
      else
      {
         value = configblock_get(config->sub, get);
      }
   }
   else
   {

      token = get_token(1, get, "::");
      while(token && i <= count) {
         if(i != count || type == RETURN_BLOCK) {
            config = config->sub;
            config = configblock_get(config, token);
            if(!config) {
               break;
            }
            free(token);
            token = get_token(i + 2, get, "::");
         }
         i++;
      }
      value = config;
      if(token && config) {
         if(type != RETURN_BLOCK) {
            value = get_key_value(config, token);
         }
      }
   }
   if(token) {
      free(token);
   }
   /* Use bitmasks later */
   if(type == RETURN_SHORT || type == RETURN_USHORT || type == RETURN_INT || type == RETURN_UINT || type == RETURN_LONG || type == RETURN_ULONG) {
      if(!value || !*(char *)value || case_compare(value, "off")) {
         value = "0";
      }
      else if(case_compare(value, "on")) {
         value = "1";
      }
      j = atoi(value);
      value = &j;
   }
   *dest = value;
}

