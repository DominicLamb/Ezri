#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Say"
#define DESCRIPTION "Send messages through the service bots."
#include "../headers/extensions.h"
#define MAX_DICE 10
#define MAX_MODIFIER 50

int command_roll(Services_User *client, User *sender, cmd_message *message);
int help_say(Services_User *client, User *sender, cmd_message *message);

int extension_load(void) {
   if(wrapper_add_command("chanserv", "ROLL", command_roll, is_ircop) != 1) {
      return 0;
   }
   wrapper_add_help(NULL, "ROLL", "main", "Rolls a series of dice.", help_say);
   return 1;
}

int extension_unload(void) {
   drop_command_by_type(NULL, "ROLL");
   drop_help_by_type(NULL, "ROLL");
   return 1;
}

int command_roll(Services_User *client, User *sender, cmd_message *message) {
   char result[11];
   char store[11];
   int rolls[MAX_DICE + 1];
   int i = 0;
   int modifier;
   int number = 1;
   int sides = 6;
   int sum_dice = 0;
   unsigned char *pos;
   char response[IRCLINE_MAX] = "Rolled: ";
   number = strtoul(message_get(message), &(char *)pos, 10);
   if(!number) {
      number = 1;
   }
   if(pos && (*pos == 'd' || *pos == 'D')) {
      pos++;
      if(number <= MAX_DICE) {
         if(isdigit(*pos)) {
            sides = strtoul((char *)pos, &(char *)pos, 10);
            if(sides > 10000) {
               snprintf(result, 11, "%d", sides);
               message_user(client, sender, "COMMAND_EXTRA_TOO_MANY_SIDES", 1, result);
               sides = 10000;
            }
            if(sides < 1) {
               sides = 6;
            }
         }
      }
   }
   if(number > MAX_DICE) {
      snprintf(result, 3, "%d", MAX_DICE);
      snprintf(store, 3, "%d", number);
      message_user(client, sender, "COMMAND_EXTRA_TOO_MANY_DICE", 2, result, store);
   }
   else if(number == 1) {
      message_user(client, sender, "COMMAND_EXTRA_BASIC_RESULT", 1, result);
   }
   else
   {
      snprintf(result, 4, "%d", number);
      snprintf(store, 6, "%d", sides);
      message_user(client, sender, "COMMAND_EXTRA_ROLL_COUNT", 2, result, store);
      rolls[0] = rand_range(1, sides);
      sum_dice = rolls[0];
      snprintf(response, IRCLINE_MAX, "%s%d", response, rolls[0]);
      for(i = 1; i < number; i++) {
         rolls[i] = rand_range(1, sides);
         sum_dice += rolls[i];
         snprintf(response, IRCLINE_MAX, "%s, %d", response, rolls[i]);
      }
      if(sum_dice) {
         snprintf(result, 11, "%d", sum_dice);
         strncat(response, " Total = ", IRCLINE_MAX - strlen(response));
         strncat(response, result, IRCLINE_MAX - strlen(response));
      }
      response[IRCLINE_MAX - 1] = '\0';
      if(pos && *pos) {
         modifier = atoi((char *)(pos + 1));
         if(modifier > MAX_MODIFIER) {
            snprintf(result, 11, "%d", modifier);
            snprintf(store, 11, "%d", MAX_MODIFIER);
            message_user(client, sender, "COMMAND_EXTRA_MODIFIER_TOO_BIG", 2, modifier, MAX_MODIFIER);
            modifier = MAX_MODIFIER;
         }
         if(modifier) {
            snprintf(result, 11, "%d", sum_dice);
            snprintf(store, 11, "%d", modifier);
            strncat(response, " | ", IRCLINE_MAX - strlen(response));
            strncat(response, result, IRCLINE_MAX - strlen(response));
            switch(*pos) {
               case '+':
                  strncat(response, " + ", IRCLINE_MAX - strlen(response));
                  snprintf(result, 11, "%d", modifier);
                  strncat(response, result, IRCLINE_MAX - strlen(response));
                  sum_dice += modifier;
               break;
               case '-':
                  strncat(response, " - ", IRCLINE_MAX - strlen(response));
                  snprintf(result, 11, "%d", modifier);
                  strncat(response, result, IRCLINE_MAX - strlen(response));
                  sum_dice -= modifier;
               break;
               case '*':
                  strncat(response, " * ", IRCLINE_MAX - strlen(response));
                  snprintf(result, 11, "%d", modifier);
                  strncat(response, result, IRCLINE_MAX - strlen(response));
                  sum_dice *= modifier;
               break;
               default:
                  *result = *pos;
                  *(result + 1) = '\0';
                  message_user(client, sender, "COMMAND_EXTRA_ODD_OPERATION", 1, result);
               break;
            }
            snprintf(result, 11, "%d", sum_dice);
            strncat(response, " = ", IRCLINE_MAX - strlen(response));
            strncat(response, result, IRCLINE_MAX - strlen(response));
         }
      }
      plain_message_user(client, sender, response);
   }
   return 1;
}

int help_say(Services_User *client, User *sender, cmd_message *message) {
   if(message) {
      message_user(client, sender, "HELP_SAY_SYNTAX", 0);
      message_user(client, sender, "HELP_SAY_DESCRIBE", 0);
   }
   return 1;
}