#define AUTHOR OFFICIAL_EXTENSION
#define EXTENSION_NAME "Memo"
#define DESCRIPTION "Gives MemoServ-style capabilities."
#include "../headers/extensions.h"

typedef struct Memo {
   char sender[NICK_MAX];
   char recipient[NICK_MAX];
   char text[450];
   time_t send_date;
   int read;
   struct Memo *next;
} Memo;

int command_ns_sendmemo(Services_User *client, User *sender, cmd_message *message);
int command_ns_readmemo(Services_User *client, User *sender, cmd_message *message);

void add_memo(Memo *memo, Memo **list);
static Memo *create_memo(const char * const from, const char * const to, const char * const text);

Memo *memo_list;

int extension_load(void) {
   if(get_svsclient_by_type("memoserv")) {
      if(wrapper_add_command("MemoServ", "SEND", command_ns_sendmemo, get_user)) {
         return 0;
      }
      if(wrapper_add_command("MemoServ", "READ", command_ns_readmemo, get_user)) {
         return 0;
      }
   }
   else
   {
      if(wrapper_add_command("NickServ", "MEMO SEND", command_ns_sendmemo, get_user)) {
         return 0;
      }
      if(wrapper_add_command("NickServ", "MEMO READ", command_ns_readmemo, get_user)) {
         return 0;
      }
   }
   return 1;
}

int extension_unload(void) {
   drop_command_by_type("nickserv", "MEMO SEND");
   drop_command_by_type("nickserv", "MEMO READ");
   drop_command_by_type("memoserv", "SEND");
   drop_command_by_type("memoserv", "READ");
   return 1;
}

int command_ns_sendmemo(Services_User *client, User *sender, cmd_message *message) {
   char *target;
   char *memo;
   Memo *newmemo;
   Reg_User *user;
   if(!message) {
      return 1;
   }
   if(!user_is_identified(sender)) {
      message_user(client, sender, "COMMAND_NOT_IDENTIFIED", 0);
      return 1;
   }
   target = get_token(1, message_get(message), " ");
   memo = get_token_remainder(2, message_get(message), " ");
   if(target && memo) {
      user = get_reg_user(target);
      if(user) {
         newmemo = create_memo(sender->nick, user->user, memo);
         add_memo(newmemo, &memo_list);
         if(is_user(target)) {
            message_user(client, sender, "COMMAND_MEMO_RECEIVED", 2, memo, "");
         }
      }
   }
   if(target) {
      free(target);
   }
   if(memo) {
      free(memo);
   }
   return 1;
}

int command_ns_readmemo(Services_User *client, User *sender, cmd_message *message) {
   Memo *memos = memo_list;
   char *memo;
   char memo_date[80];
   unsigned long memo_id = 0;
   unsigned long i = 0;
   struct tm *tm;
   memo = get_token(1, message_get(message), " ");

   if(memo) {
      memo_id = strtoul(memo, NULL, 0);
   }
   if(!memo_id) {
      if(user_is_identified(sender)) {
         while(memos) {
            if(case_compare(memos->recipient, sender->nick)) {
               plain_message_user(client, sender, memos->text);
            }
            memos = memos->next;
         }
      }
   }
   else
   {
      while(memos) {
         if(case_compare(memos->recipient, sender->nick)) {
            i++;
            if(i == memo_id) {
               break;
            }
         }
         memos = memos->next;
      }
      if(memo_id == i && memos) {
         tm = gmtime(&memos->send_date);
         strftime(memo_date, 80, "%a %B %d %Y (%H:%M) %Z", tm);
         message_user(client, sender, "COMMAND_MEMO_RECEIVED", 2, memos->sender, memo_date);
         memos->read = 1;
      }
      else
      {
         message_user(client, sender, "COMMAND_NO_MEMO", 0);
      }
   }
   if(memos) {
      free(memos);
   }
   return 1;
}

void add_memo(Memo *memo, Memo **list) {
   Memo *ptr = *list;
   if(memo) {
      if(ptr) {
         while(ptr->next) {
            ptr = ptr->next;
         }
         ptr->next = memo;
      }
      else
      {
         *list = memo;
      }
      //database->append(
   }
}

static Memo *create_memo(const char * const from, const char * const to, const char * const text) {
   Memo *memo;
   memo = malloc(sizeof(Memo));
   if(memo) {
      memset(memo, '\0', sizeof(Memo));
      strncpy_safe(memo->sender, from, NICK_MAX);
      strncpy_safe(memo->recipient, to, NICK_MAX);
      strncpy_safe(memo->text, text, 450);
      memo->send_date = time(NULL);
   }
   return memo;
}