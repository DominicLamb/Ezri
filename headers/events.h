#define EVENT_CONTINUE 1
#define EVENT_HALT 2
#define ALL_EVENTS_RUN 3

#define TIMER_NOOP 0
#define TIMER_DELETE 1
#define TIMER_REDO 2
#define TIMER_MAX 64
typedef struct svs_event {
   struct svs_event *next;
   char name[32];
   time_t last_date;
   int (*function)(void **event_data);
} svs_event;

typedef struct svs_timer {
   struct svs_timer *next;
   void **data;
   char name[TIMER_MAX];
   time_t next_date;
   time_t last_date;
   unsigned int seconds;
   int num_args;
   long timer_id;
   int (*function)(void **timer_data, const long count);
} svs_timer;

EXPORT int fire_event(const char * const event_name, const unsigned int count, ...);
EXPORT int add_event(const char * const event_name, int (*function)(void **input));
EXPORT int add_timer(const char * const name, int (*function)(void **input, const long timer_id), const unsigned int seconds, const unsigned int count, ...);
EXPORT void drop_event(const char * const event_name, int (*function)(void **input));
EXPORT void end_timer(const char * const timer_name, const long timer_id);
EXPORT void end_all_timers(const char * const timer_group);
EXPORT int check_timers(time_t now);
svs_event *event_list;
svs_timer *timer_list;