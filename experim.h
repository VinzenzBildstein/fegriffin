#ifndef EXCL_TRIGGER

#define TRIGGER_COMMON_DEFINED

typedef struct {
  INT       log_history;
  double    event_limit;
  char      format[80];
} TRIGGER_COMMON;

#define TRIGGER_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Log history = INT : 0",\
"Event limit = DOUBLE : 0",\
"Format = STRING : [80] ",\
"",\
NULL }

#define TRIGGER_SETTINGS_DEFINED

typedef struct {
} TRIGGER_SETTINGS;

#define TRIGGER_SETTINGS_STR(_name) const char *_name[] = {\
"",\
NULL }

#endif

