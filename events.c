#include "headers/main.h"
#include "headers/users.h"
#include "headers/events.h"

/* TODO: Delete event functions */

int fire_event(const char * const event_name, const unsigned int count, ...) {
   va_list args;
   svs_event *p = event_list;
   void **passed_args = 0;
   void *p2;
   unsigned int i;
   int result = 0;
   while(p != 0) {
      if(case_compare(p->name, event_name)) {
         if(count > 0 && passed_args == 0) {
            va_start(args, count);
            passed_args = (void **)malloc(count * sizeof(void *));
            for(i = 0; i < count; i++) {
               p2 = va_arg(args, void *);
               passed_args[i] = p2;
            }
         }
         result = p->function(passed_args);
         if(result == EVENT_HALT) {
            break;
         }
      }
      p = p->next;
   }
   if(count > 0) {
      va_end(args);
      if(passed_args) {
         free(passed_args);
      }
   }
   return result;
}

int add_event(const char * const event_name, int (*function)(void **input)) {
   svs_event *p = event_list;
   if(event_list == 0) {
      event_list = malloc(sizeof(svs_event));
      p = event_list;
   }
   else
   {
      while(p->next != 0) {
         p = p->next;
      }
      p->next = malloc(sizeof(svs_event));
      p = p->next;
   }
   if(p) {
      memset(p, 0, sizeof(svs_event));
      strncpy_safe(p->name, event_name, 32);
      p->function = function;
      return 1;
   }
   return 0;
}

int add_timer(const char * const name, int (*function)(void **input, const long timer_id), const unsigned int seconds, const unsigned int count, ...) {
   svs_timer *p = timer_list;
   void **passed_args = 0;
   void *p2;
   time_t time_event;
   va_list args;
   unsigned int i = 0;
   long rand_id;
   if(!name || *name == '\0' || seconds == 0) {
      return 0;
   }
   /*
      Fix: Adding to time_t == bad idea
   */
   time_event = time(NULL) + seconds;
   rand_id = rand_range(10000, 32767) * rand_range(10000, 32767);
   if(p) {
      while(p->next) {
         if(p->timer_id == rand_id) {
            rand_id = rand_range(10000, 32767);
            p = timer_list;
         }
         else
         {
            p = p->next;
         }
      }
      p->next = malloc(sizeof(svs_timer));
      p = p->next;
   }
   else
   {
      timer_list = malloc(sizeof(svs_timer));
      p = timer_list;
   }
   if(p) {
      memset(p, 0, sizeof(svs_timer));
      strncpy_safe(p->name, name, TIMER_MAX);
      p->next_date = time_event;
      p->seconds = seconds;
      p->function = function;
      p->num_args = count;
      p->timer_id = rand_id;
      if(count > 0) {
         passed_args = (void **)malloc(count * sizeof(void *));
         if(passed_args) {
            va_start(args, count);
            for(i = 0; i < count; i++) {
               p2 = va_arg(args, void *);
               passed_args[i] = get_token(1, p2, "");
            }
            va_end(args);
         }
         else
         {
            p->num_args = 0;
            end_timer(p->name, p->timer_id);
         }
         p->data = passed_args;
      }
      return rand_id;
   }
   return 0;
}

static void delete_timer(svs_timer *timer) {
   int i;
   if(timer->data != 0) {
      for(i = 0; i < timer->num_args; i++) {
         if(timer->data[i]) {
            free(timer->data[i]);
         }
      }
      free(timer->data);
   }
   free(timer);
}

static void delete_event(svs_event *event_delete) {
   free(event_delete);
}

void drop_event(const char * const event_name, int (*function)(void **input)) {
   svs_event *prev;
   svs_event *p;
   p = event_list;
   prev = p;
   while(p != 0) {
      if(p->name != 0 && case_compare(p->name, event_name) && function == p->function) {
         if(p == event_list) {
            p = p->next;
            delete_event(event_list);
            event_list = p;
         }
         else
         {
            prev->next = p->next;
            delete_event(p);
         }
         break;
      }
      prev = p;
      p = p->next;
   }
}

void end_timer(const char * const timer_name, const long timer_id) {
   svs_timer *p = timer_list;
   svs_timer *prev = timer_list;
   while(p != 0) {
      if(p->timer_id == timer_id && case_compare(p->name, timer_name)) {
         if(p == timer_list) {
            p = p->next;
            delete_timer(timer_list);
            timer_list = p;
         }
         else
         {
            prev->next = p->next;
            delete_timer(p);
         }
         break;
      }
      prev = p;
      p = p->next;
   }
}

void end_all_timers(const char * const timer_group) {
   svs_timer *p = timer_list;
   svs_timer *prev = timer_list;
   while(p != 0) {
      if(!timer_group || case_compare(timer_group, p->name)) {
         if(p == timer_list) {
            p = p->next;
            delete_timer(timer_list);
            timer_list = p;
            prev = p;
         }
         else
         {
            prev->next = p->next;
            delete_timer(p);
            p = prev->next;
         }
      }
      else
      {
         prev = p;
         p = p->next;
      }
   }
}

int check_timers(time_t now) {
   svs_timer *p = timer_list;
   int result = 0;
   unsigned int timers = 0;
   while(p != 0) {
      if(p->next_date <= now) {
         result = p->function(p->data, p->timer_id);
         if(result == TIMER_REDO) {
            p->next_date = p->next_date + p->seconds;
         }
         else if(result == TIMER_DELETE) {
            end_timer(p->name, p->timer_id);
            if(timer_list != 0) {
               /* Restart the timer list */
               now = time(NULL);
               p = timer_list;
            }
            else
            {
               break;
            }
         }
         timers++;
      }
      p = p->next;
   }
   return timers;
}
